#include "EspNowComm.h"
#include "../addons/Eject.h"
#ifdef FIST_ID
#undef FIST_ID
#endif
#include "../addons/FistIT.h"
#include "../main.h"
#include "../screens/ScreenHandler.h"
#include "../debug.h"
#include "../config_ids.h"
#include <cstring>
#include <cstddef>

// ---- Variable definitions ----
struct_message      outgoingcontrol;
struct_message      incomingcontrol;
esp_now_peer_info_t peerInfo;
uint8_t OSSM_Address[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
bool Ossm_paired = false;
bool OSSM_On     = false;
TaskHandle_t eRemote_t = nullptr;
static bool Eject_paired = false;
static bool Fist_paired  = false;
static uint32_t Eject_lastSeenMs = 0;
static uint32_t Fist_lastSeenMs = 0;

static constexpr int ESP_NOW_MSG_LEGACY_SIZE = (int)offsetof(struct_message, esp_sender);
static constexpr int ESP_NOW_MSG_FULL_SIZE = (int)sizeof(struct_message);

static void formatMac(const uint8_t *mac, char *out, size_t outLen) {
  if (!out || outLen == 0) return;
  if (!mac) {
    snprintf(out, outLen, "<null>");
    return;
  }
  snprintf(out,
           outLen,
           "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0],
           mac[1],
           mac[2],
           mac[3],
           mac[4],
           mac[5]);
}

static void logEspNowFrame(const char *tag, const uint8_t *mac, const struct_message &msg, int len) {
  char macBuf[24];
  formatMac(mac, macBuf, sizeof(macBuf));
  LogDebugFormatted("[ESP-NOW][%s] mac=%s len=%d cmd=%d val=%.2f target=%d sender=%d hb=%d connected=%d\n",
                    tag ? tag : "?",
                    macBuf,
                    len,
                    msg.esp_command,
                    msg.esp_value,
                    msg.esp_target,
                    msg.esp_sender,
                    msg.esp_heartbeat ? 1 : 0,
                    msg.esp_connected ? 1 : 0);
}

static void refreshAddonLinkState() {
  const uint32_t now = millis();
  if (Eject_paired && (now - Eject_lastSeenMs) > 6000UL) Eject_paired = false;
  if (Fist_paired && (now - Fist_lastSeenMs) > 6000UL) Fist_paired = false;
}

// ---- Callbacks ----

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  char macBuf[24];
  formatMac(mac_addr, macBuf, sizeof(macBuf));
  LogDebugFormatted("[ESP-NOW][TX-DONE] mac=%s status=%s\n",
                    macBuf,
                    (status == ESP_NOW_SEND_SUCCESS) ? "OK" : "FAIL");
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  if (incomingData == nullptr || len <= 0) {
    return;
  }

  if (len < ESP_NOW_MSG_LEGACY_SIZE) {
    return;
  }

  // Old OSSM may send legacy packets with no esp_sender field.
  memset(&incomingcontrol, 0, sizeof(incomingcontrol));
  const int copyLen = (len < ESP_NOW_MSG_FULL_SIZE) ? len : ESP_NOW_MSG_FULL_SIZE;
  memcpy(&incomingcontrol, incomingData, copyLen);
  if (len < ESP_NOW_MSG_FULL_SIZE) {
    incomingcontrol.esp_sender = 0;
  }
  logEspNowFrame("RX", mac, incomingcontrol, len);

  if (incomingcontrol.esp_sender == EJECT_ID) {
    logEspNowFrame("RX from EJECT - ", mac, incomingcontrol, len);
    if (EjectHandleIncomingEspNowFrame(mac,
                                       incomingcontrol.esp_target,
                                       incomingcontrol.esp_sender,
                                       incomingcontrol.esp_command,
                                       incomingcontrol.esp_value,
                                       incomingcontrol.esp_heartbeat)) {
      Eject_paired = EjectIsPaired();
      Eject_lastSeenMs = millis();
      return;
    }
  } else if (incomingcontrol.esp_sender == FIST_ID) {
    logEspNowFrame("RX from FIST - ", mac, incomingcontrol, len);
    if (FistITHandleIncomingEspNowFrame(mac,
                                        incomingcontrol.esp_target,
                                        incomingcontrol.esp_sender,
                                        incomingcontrol.esp_command,
                                        incomingcontrol.esp_value,
                                        incomingcontrol.esp_heartbeat)) {
      Fist_paired = FistITIsPaired();
      Fist_lastSeenMs = millis();
      return;
    }
  }

  const int announcedId = (int)(incomingcontrol.esp_value + (incomingcontrol.esp_value >= 0 ? 0.5f : -0.5f));
  const bool sourceIsBroadcast = (mac[0] == 0xFF && mac[1] == 0xFF && mac[2] == 0xFF &&
                                  mac[3] == 0xFF && mac[4] == 0xFF && mac[5] == 0xFF);
  const bool sourceIsZero = (mac[0] == 0x00 && mac[1] == 0x00 && mac[2] == 0x00 &&
                             mac[3] == 0x00 && mac[4] == 0x00 && mac[5] == 0x00);
  const bool addonAnnounced = (announcedId == CUM_ID || announcedId == FIST_ID);
  const bool ossmAnnounced = (announcedId == OSSM_ID);

  const bool isEjectFrame =
      (incomingcontrol.esp_target == CUM ||
       incomingcontrol.esp_target == CUM_ID ||
       incomingcontrol.esp_command == CUMSPEED ||
       incomingcontrol.esp_command == CUMTIME ||
       incomingcontrol.esp_command == CUMSIZE ||
       incomingcontrol.esp_command == CUMACCEL ||
       ((incomingcontrol.esp_command == CONNECT || incomingcontrol.esp_command == CONN || incomingcontrol.esp_command == HEARTBEAT) &&
        announcedId == CUM_ID));
  const bool isFistFrame =
      (incomingcontrol.esp_target == FIST_ID ||
       incomingcontrol.esp_command == FIST_SPEED ||
       incomingcontrol.esp_command == FIST_ROTATION ||
       incomingcontrol.esp_command == FIST_PAUSE ||
       incomingcontrol.esp_command == FIST_ACCEL ||
       ((incomingcontrol.esp_command == CONNECT || incomingcontrol.esp_command == CONN || incomingcontrol.esp_command == HEARTBEAT) &&
        announcedId == FIST_ID));
  const bool looksLikeOssmPairFrame =
      !addonAnnounced &&
      (incomingcontrol.esp_command == CONNECT || incomingcontrol.esp_command == CONN) &&
      (ossmAnnounced || announcedId == 0);

  if (isEjectFrame) {
    Eject_paired = true;
    Eject_lastSeenMs = millis();
  }
  if (isFistFrame) {
    Fist_paired = true;
    Fist_lastSeenMs = millis();
  }

    if(incomingcontrol.esp_target == M5_ID && Ossm_paired == false &&
      !isEjectFrame && !isFistFrame && looksLikeOssmPairFrame){

    if (sourceIsBroadcast || sourceIsZero) {
      return;
    }

    // Remove the existing peer (0xFF:0xFF:0xFF:0xFF:0xFF:0xFF)
    const bool currentPeerIsBroadcast =
        peerInfo.peer_addr[0] == 0xFF && peerInfo.peer_addr[1] == 0xFF && peerInfo.peer_addr[2] == 0xFF &&
        peerInfo.peer_addr[3] == 0xFF && peerInfo.peer_addr[4] == 0xFF && peerInfo.peer_addr[5] == 0xFF;
    const bool currentPeerIsZero =
        peerInfo.peer_addr[0] == 0x00 && peerInfo.peer_addr[1] == 0x00 && peerInfo.peer_addr[2] == 0x00 &&
        peerInfo.peer_addr[3] == 0x00 && peerInfo.peer_addr[4] == 0x00 && peerInfo.peer_addr[5] == 0x00;

    esp_err_t result = ESP_OK;
    if (!currentPeerIsBroadcast && !currentPeerIsZero && esp_now_is_peer_exist(peerInfo.peer_addr)) {
      result = esp_now_del_peer(peerInfo.peer_addr);
    }

    if (result == ESP_OK) {

      memcpy(OSSM_Address, mac, 6); // get the mac address of the sender

      // Add the new peer
      memcpy(peerInfo.peer_addr, OSSM_Address, 6);
      if (esp_now_add_peer(&peerInfo) == ESP_OK) {
        LogDebugFormatted("New peer added successfully, OSSM address: %02X:%02X:%02X:%02X:%02X:%02X\n",
          OSSM_Address[0], OSSM_Address[1], OSSM_Address[2],
          OSSM_Address[3], OSSM_Address[4], OSSM_Address[5]);
        Ossm_paired = true;
      } else {
        LogDebug("Failed to add new peer");
      }
    } else {
      LogDebug("Failed to remove peer");
    }

    if(incomingcontrol.esp_speed > speedlimit){
      speedlimit = 300;
    } else (
    speedlimit = incomingcontrol.esp_speed);
    LogDebug(speedlimit);
    maxdepthinmm = incomingcontrol.esp_depth;
    LogDebug(maxdepthinmm);
    pattern = incomingcontrol.esp_pattern;
    LogDebug(pattern);
    outgoingcontrol.esp_target = OSSM_ID;

    result = esp_now_send(OSSM_Address, (uint8_t *) &outgoingcontrol, sizeof(outgoingcontrol));
    LogDebug(result);

    if (result == ESP_OK) {
      Ossm_paired = true;
    }
  }
  switch(incomingcontrol.esp_command)
  {
    case OFF:
      OSSM_On = false;
      break;
    case ON:
      OSSM_On = true;
      break;
  }
}

// Sends command and value to remote device; returns true if sent successfully
bool espNowSendCommand(int Command, float Value, int Target){
  const bool targetIsEject = (Target == CUM || Target == CUM_ID);
  const bool targetIsFist = (Target == FIST_ID);
  const bool targetIsAddon = (targetIsEject || targetIsFist);
  if (!targetIsAddon && !Ossm_paired) {
    return false;
  }

  outgoingcontrol.esp_connected = true;
  outgoingcontrol.esp_command = Command;
  outgoingcontrol.esp_value = Value;
  outgoingcontrol.esp_target = Target;
  outgoingcontrol.esp_sender = M5_ID;

  const uint8_t broadcastAddr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  const uint8_t *dest = targetIsAddon ? broadcastAddr : OSSM_Address;

  logEspNowFrame("TX", dest, outgoingcontrol, (int)sizeof(outgoingcontrol));
  esp_err_t result = esp_now_send(dest, (uint8_t *) &outgoingcontrol, sizeof(outgoingcontrol));
  if (result == ESP_OK) {
    return true;
  }

  LogDebugFormatted("[ESP-NOW][TX] first send failed err=%d, retrying\n", (int)result);
  delay(20);
  result = esp_now_send(dest, (uint8_t *) &outgoingcontrol, sizeof(outgoingcontrol));
  LogDebugFormatted("[ESP-NOW][TX] retry result=%d\n", (int)result);
  return (result == ESP_OK);
}

void espNowKickPairing() {
  if(!Ossm_paired){
    outgoingcontrol.esp_command = HEARTBEAT;
    outgoingcontrol.esp_heartbeat = true;
    outgoingcontrol.esp_target = OSSM_ID;
    outgoingcontrol.esp_sender = M5_ID;
    logEspNowFrame("TX-PAIR", OSSM_Address, outgoingcontrol, (int)sizeof(outgoingcontrol));
    esp_now_send(OSSM_Address, (uint8_t *) &outgoingcontrol, sizeof(outgoingcontrol));
  }
}

bool espNowIsPaired() {
  return Ossm_paired;
}

bool espNowIsEjectConnected() {
  Eject_paired = EjectIsPaired();
  refreshAddonLinkState();
  return Eject_paired;
}

bool espNowIsFistConnected() {
  Fist_paired = FistITIsPaired();
  refreshAddonLinkState();
  return Fist_paired;
}

void espNowRemoteTask(void *pvParameters)
{
  for(;;){
    if(Ossm_paired){
      outgoingcontrol.esp_command   = HEARTBEAT;
      outgoingcontrol.esp_heartbeat = true;
      outgoingcontrol.esp_target    = OSSM_ID;
      outgoingcontrol.esp_sender    = M5_ID;
      logEspNowFrame("TX-HB", OSSM_Address, outgoingcontrol, (int)sizeof(outgoingcontrol));
      esp_now_send(OSSM_Address, (uint8_t *) &outgoingcontrol, sizeof(outgoingcontrol));
    }
    vTaskDelay(HEARTBEAT_INTERVAL);
  }
}

// ---- Initialization ----

void espNowInit(){
  WiFi.mode(WIFI_STA);
  LogDebug(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
  }
  esp_now_register_send_cb(OnDataSent);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  memcpy(peerInfo.peer_addr, OSSM_Address, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
  }
  esp_now_register_recv_cb(OnDataRecv);
  LogDebug("\n esp now initialized");

  xTaskCreatePinnedToCore(espNowRemoteTask, "espNowRemoteTask", 3096, NULL, 5, &eRemote_t, 0);
  delay(200);
  LogDebug("\n esp_task created");
}
