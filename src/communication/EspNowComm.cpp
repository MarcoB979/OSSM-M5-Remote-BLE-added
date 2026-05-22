#include "EspNowComm.h"
#include "../ui/ui.h"
#include "../main.h"
#include "../screens/ScreenHandler.h"
#include "../debug.h"
#include "../config_ids.h"

// ---- Variable definitions ----
struct_message      outgoingcontrol;
struct_message      incomingcontrol;
esp_now_peer_info_t peerInfo;
uint8_t OSSM_Address[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
bool Ossm_paired = false;
bool OSSM_On     = false;
TaskHandle_t eRemote_t = nullptr;

// ---- Callbacks ----

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&incomingcontrol, incomingData, sizeof(incomingcontrol));

  if(incomingcontrol.esp_target == M5_ID && Ossm_paired == false){

    // Remove the existing peer (0xFF:0xFF:0xFF:0xFF:0xFF:0xFF)
    esp_err_t result = esp_now_del_peer(peerInfo.peer_addr);

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
      lv_label_set_text(ui_connect, "Connected");
      lv_scr_load_anim(ui_Home, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0, false);
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
  if(Ossm_paired == true){
    outgoingcontrol.esp_connected = true;
    outgoingcontrol.esp_command = Command;
    outgoingcontrol.esp_value = Value;
    outgoingcontrol.esp_target = Target;
    esp_err_t result = esp_now_send(OSSM_Address, (uint8_t *) &outgoingcontrol, sizeof(outgoingcontrol));
    if (result == ESP_OK) {
      return true;
    } else {
      delay(20);
      esp_now_send(OSSM_Address, (uint8_t *) &outgoingcontrol, sizeof(outgoingcontrol));
      return false;
    }
  }
  return false;
}

void espNowKickPairing() {
  if(!Ossm_paired){
    outgoingcontrol.esp_command = HEARTBEAT;
    outgoingcontrol.esp_heartbeat = true;
    outgoingcontrol.esp_target = OSSM_ID;
    esp_now_send(OSSM_Address, (uint8_t *) &outgoingcontrol, sizeof(outgoingcontrol));
  }
}

bool espNowIsPaired() {
  return Ossm_paired;
}

void espNowRemoteTask(void *pvParameters)
{
  for(;;){
    if(Ossm_paired){
      outgoingcontrol.esp_command   = HEARTBEAT;
      outgoingcontrol.esp_heartbeat = true;
      outgoingcontrol.esp_target    = OSSM_ID;
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
