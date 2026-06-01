/*
To have BLE and ESP_NOW work together and to make sure ESP_NOW communication works
we need to make a few minor changes to the original OSSM Stroke code for M5 remote 
which can be found here: https://github.com/ortlof/OSSM-Stroke/tree/main

In the code checked on 25th of june 2026 the following files were edited:
- src/main.cpp

The following code was edited:
- static sensorlessHomeProperties
  * current limit was set to 1.5f which resulted in belt scraping in several test setups.
  * to fix this, you can set the current limit to a lower level. 1.25f worked well in the 
  * test setup. The code has been changed to:
  * 
  * static sensorlessHomeProperties sensorless = {
  *   .currentPin = 36,
  *   .currentLimit = 1.25f
  * };

- void espNowRemoteTask(void *pvParameters)
  * The original code did not set the WiFi channel for ESP-NOW, which caused communication issues.
  * By default, the ESP32's WiFi starts on channel 1, but if it was changed or not set properly, 
  * ESP-NOW packets might not be received by devices on the expected channel.
  * To fix this, the code was updated to explicitly set the WiFi channel to 1 before initializing ESP-NOW. 
  * Change the void espNowRemoteTask function to include the following lines at the beginning:
  * Necessary added lines are marked with --> 
  * 
  * **************************************************

  void espNowRemoteTask(void *pvParameters)
{
    WiFi.mode(WIFI_STA);

  // Explicitly set the WiFi channel to 1 before ESP-NOW init.
  // Without this, the radio stays on channel 0 (unset) and broadcast
  // packets are never received by devices on channel 1 (OSSM, Eject, FistIT).
-->  esp_wifi_set_promiscuous(true);
-->  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
--> esp_wifi_set_promiscuous(false);

  LogDebug(WiFi.macAddress());
  LogDebugFormatted("WiFi channel after init: %d\n", WiFi.channel());
  // Debug: show WiFi channel at ESP-NOW startup
    if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
    }
    // Once ESPNow is successfully Init, we will register for Send CB to
    // get the status of Trasnmitted packet
    esp_now_register_send_cb(OnDataSent);

    
    // Register peer
    memcpy(peerInfo.peer_addr, m5RemoteAddress, 6);
--> peerInfo.channel = 1;  
    peerInfo.encrypt = false;
    
    * **************************************************

    * the code can remain the same after this point
    * The key change is the addition of the WiFi channel setting at the start of the task.
*/









//tryout to copy/paste code from working backup firmware to new codebase. This file is the main ESP-NOW communication handler for the M5 remote, which manages pairing, sending, and receiving of ESP-NOW messages, including those from addons like Eject and FistIT.



#include "EspNowComm.h"
#include "../addons/Eject.h"
#ifdef FIST_ID
#undef FIST_ID
#endif
#include "../addons/FistIT.h"
#include "../main.h"
#include "../config/debug.h"
#include "../config/config_ids.h"
#include "../ui/ui.h"
#include "communication/CommManager.h"
#include <Arduino.h>
#include <esp_wifi.h>
#include <cstring>
#include <cstddef>
#include "language.h"


// ---- Variable definitions ----
struct_message outgoingcontrol;
struct_message incomingcontrol;
esp_now_peer_info_t peerInfo;
uint8_t OSSM_Address[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
bool Ossm_paired = false;
bool OSSM_On = false;
TaskHandle_t eRemote_t = nullptr;
int ESP_NOW_CHANNEL = 1;

static constexpr int ESP_NOW_MSG_LEGACY_SIZE = (int)offsetof(struct_message, esp_sender);
static constexpr int ESP_NOW_MSG_FULL_SIZE = (int)sizeof(struct_message);

static const uint8_t BROADCAST_ADDR[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static constexpr TickType_t HEARTBEAT_INTERVAL = pdMS_TO_TICKS(5000);

bool OssmBleIsMode() {
  return (commGetMode() == COMM_MODE_ESPNOW);
}


void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  (void)mac_addr;
  (void)status;
}

// SECURITY: Sender verification for addon isolation
// OSSM does not have esp_sender field in its struct, so incoming messages will have it = 0 (uninitialized)
// New addons MUST set esp_sender to their device ID (e.g., FIXIT_ID=10, EJECT_ID=11, etc)
// This prevents addon messages from being mistaken for OSSM commands
// 
// Verification logic:
// - If esp_sender == 0: Message is from OSSM (uninitialized/not set by old firmware) ✓
// - If esp_sender == OSSM_ID: Message is from OSSM (for future compatibility) ✓
// - If esp_sender == known_addon_id: Message is from that specific addon (reject - not OSSM)
// - Otherwise: Unknown sender, reject as security violation
static bool isOssmMessage(const uint8_t *mac) {
  // Check if sender is OSSM (sender ID 0 = uninitialized from old OSSM, or explicit OSSM_ID)
  bool senderIsOssm = (incomingcontrol.esp_sender == 0 || incomingcontrol.esp_sender == OSSM_ID);
  
  // During pairing phase, verify it's meant for M5 //and sender is claiming to be OSSM
  if (!Ossm_paired) {
    return (incomingcontrol.esp_target == M5_ID);// && senderIsOssm);
  }
  
  // After pairing, verify:
  // 1. MAC matches stored OSSM_Address (MAC-level verification)
  // 2. Sender identifies as OSSM (payload-level verification)
  return (memcmp(mac, OSSM_Address, 6) == 0); //&& senderIsOssm);
}

void OnDataRecvOLD(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&incomingcontrol, incomingData, sizeof(incomingcontrol));

//first see if incoming messages are from addons, if not then they must be from OSSM
  if (incomingcontrol.esp_sender == EJECT_ID) {
    LogDebug("Received message from Eject addon");
    EjectHandleIncomingEspNowFrame(mac,
                                   incomingcontrol.esp_target,
                                   incomingcontrol.esp_sender,
                                   incomingcontrol.esp_command,
                                   incomingcontrol.esp_value,
                                   incomingcontrol.esp_heartbeat);
    return;
  }

  if (incomingcontrol.esp_sender == FIST_ID) {
    LogDebug("Received message from Fist-IT addon");
    FistITHandleIncomingEspNowFrame(mac,
                                    incomingcontrol.esp_target,
                                    incomingcontrol.esp_sender,
                                    incomingcontrol.esp_command,
                                    incomingcontrol.esp_value,
                                    incomingcontrol.esp_heartbeat);
    return;
  }

//If not from addons, then handle as OSSM message. If not paired, try to pair. If paired, update state and send ack back to OSSM
LogDebug("Received message from OSSM");
  if(incomingcontrol.esp_target == M5_ID && Ossm_paired == false){

    // Remove the existing peer (0xFF:0xFF:0xFF:0xFF:0xFF:0xFF)
    esp_err_t result = esp_now_del_peer(peerInfo.peer_addr);

    if (result == ESP_OK) {

      memcpy(OSSM_Address, mac, 6); //get the mac address of the sender
      
      // Add the new peer
      memcpy(peerInfo.peer_addr, OSSM_Address, 6);
      if (esp_now_add_peer(&peerInfo) == ESP_OK) {
        LogDebugFormatted("New peer added successfully, OSSM addresss : %02X:%02X:%02X:%02X:%02X:%02X\n", OSSM_Address[0], OSSM_Address[1], OSSM_Address[2], OSSM_Address[3], OSSM_Address[4], OSSM_Address[5]);
        Ossm_paired = true;
      }
      else {
        LogDebug("Failed to add new OSSM peer");
      }
    }
    else {
      LogDebug("Failed to remove OSSM peer");
    }

    
    if(incomingcontrol.esp_speed > speedlimit){
      speedlimit = 300;
    } else {
      speedlimit = incomingcontrol.esp_speed;
      LogDebugFormatted("Speed limit: %f\n", speedlimit);
    }
    maxdepthinmm = incomingcontrol.esp_depth;
    LogDebugFormatted("Max depth: %f\n", maxdepthinmm);
    pattern = incomingcontrol.esp_pattern;
    LogDebugFormatted("Pattern: %d\n", pattern);
    outgoingcontrol.esp_target = OSSM_ID;
    
    result = esp_now_send(OSSM_Address, (uint8_t *) &outgoingcontrol, sizeof(outgoingcontrol));
    LogDebug(result);
    
    if (result == ESP_OK) {
      Ossm_paired = true;
      lv_label_set_text(ui_connect, "Connected");
      lv_label_set_text(ui_Welcome, "Connected");
      lv_scr_load_anim(ui_Menu, LV_SCR_LOAD_ANIM_FADE_ON,20,0,false);
    }
  }
  switch(incomingcontrol.esp_command)
    {
    case OFF: 
    {
    OSSM_On = false;
    }
    break;
    case ON:
    {
    OSSM_On = true;
    }
    break;
    }
}


void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len)
{
  if (len < ESP_NOW_MSG_LEGACY_SIZE) {
    LogDebugFormatted("ESP-NOW: Reject short packet len=%d (min=%d)\n", len, ESP_NOW_MSG_LEGACY_SIZE);
    return;
  }

  // OSSM legacy firmware sends packets without esp_sender (36 bytes).
  // Zero-init first so missing tail fields default safely.
  memset(&incomingcontrol, 0, sizeof(incomingcontrol));
  const int copyLen = (len < ESP_NOW_MSG_FULL_SIZE) ? len : ESP_NOW_MSG_FULL_SIZE;
  memcpy(&incomingcontrol, incomingData, copyLen);
  if (len < ESP_NOW_MSG_FULL_SIZE) {
    incomingcontrol.esp_sender = 0;
  }

  LogDebug("Received ESP-NOW data");

  if(incomingcontrol.esp_sender == EJECT_ID) {
    if (EjectHandleIncomingEspNowFrame(mac,
                                     incomingcontrol.esp_target,
                                     incomingcontrol.esp_sender,
                                     incomingcontrol.esp_command,
                                     incomingcontrol.esp_value,
                                     incomingcontrol.esp_heartbeat)) {
      return;
    }
  } else if(incomingcontrol.esp_sender == FIST_ID) {
    if (FistITHandleIncomingEspNowFrame(mac,
                                      incomingcontrol.esp_target,
                                      incomingcontrol.esp_sender,
                                      incomingcontrol.esp_command,
                                      incomingcontrol.esp_value,
                                      incomingcontrol.esp_heartbeat)) {
      return;
    }
  }

  if (incomingcontrol.esp_target == M5_ID && Ossm_paired == false && isOssmMessage(mac)) {
    // Guard against false pairing: legacy addon firmware can omit esp_sender,
    // which makes sender appear as 0 (same as legacy OSSM). For sender==0,
    // only accept frames that carry valid OSSM limits payload.
    const bool explicitOssmSender = (incomingcontrol.esp_sender == OSSM_ID);
    const bool legacyUnspecifiedSender = (incomingcontrol.esp_sender == 0);
    const bool hasValidOssmLimits = (incomingcontrol.esp_speed > 0.0f && incomingcontrol.esp_depth > 0.0f);
    if (!(explicitOssmSender || (legacyUnspecifiedSender && hasValidOssmLimits))) {
      LogDebugFormatted(
        "ESP-NOW: Ignore pre-pair frame (sender=%d target=%d speed=%.1f depth=%.1f cmd=%d)\n",
        incomingcontrol.esp_sender,
        incomingcontrol.esp_target,
        incomingcontrol.esp_speed,
        incomingcontrol.esp_depth,
        incomingcontrol.esp_command);
      return;
    }

    // Only try to delete if a peer entry exists; treat 'no peer' as success.
    bool peer_exists = esp_now_is_peer_exist(peerInfo.peer_addr);
    esp_err_t result = ESP_OK;
    if (peer_exists) {
      result = esp_now_del_peer(peerInfo.peer_addr);
      if (result != ESP_OK) {
        LogDebugFormatted("esp_now_del_peer failed: %d", (int)result);
      }
    }
    
    if (result == ESP_OK) {
      memcpy(OSSM_Address, mac, 6);
      memcpy(peerInfo.peer_addr, OSSM_Address, 6);
      if (esp_now_add_peer(&peerInfo) == ESP_OK) {
        LogDebugFormatted("New peer added successfully, OSSM addresss : %02X:%02X:%02X:%02X:%02X:%02X\n",
          OSSM_Address[0], OSSM_Address[1], OSSM_Address[2], OSSM_Address[3], OSSM_Address[4], OSSM_Address[5]);
        Ossm_paired = true;
//        esp_now_ui_pairing_complete = true;  // Signal UI update from main task
      } else {
        LogDebug("Failed to add new peer");
      }
    } else {
      LogDebug("Failed to remove peer");
    }

    if (incomingcontrol.esp_speed > 600) {
      speedlimit = 600;
    } else {
      speedlimit = incomingcontrol.esp_speed;
    }
    LogDebug(speedlimit);
    maxdepthinmm = incomingcontrol.esp_depth;
    LogDebug(maxdepthinmm);
    outgoingcontrol.esp_target = OSSM_ID;
    outgoingcontrol.esp_sender = M5_ID;  // Identify M5 as sender

    result = esp_now_send(OSSM_Address, (uint8_t *)&outgoingcontrol, sizeof(outgoingcontrol));
    LogDebug(result);

    if (result == ESP_OK) {
      Ossm_paired = true;
//      if (incomingcontrol.esp_speed > 0 && incomingcontrol.esp_depth > 0) {
        // Received valid OSSM limits data - confirm OSSM is connected via ESP_NOW
//        ossm_espnow_connected = true;
//        LogDebugPrio("ESP_NOW: OSSM confirmed connected");
//        esp_now_ui_limits_ready = true;  // Signal UI to show Home screen
//      } else {
//        esp_now_ui_homing = true;  // Signal UI to show homing message
//      }
    }
  }

  // SECURITY: Verify incoming message is from paired OSSM, not an addon
  // This prevents interference from other ESP_NOW devices like Fist-IT or Eject
  if (!isOssmMessage(mac)) {
    LogDebugFormatted(
      "ESP-NOW: Rejecting frame (from=%02X:%02X:%02X:%02X:%02X:%02X sender=%d target=%d paired=%d expected=%02X:%02X:%02X:%02X:%02X:%02X)\n",
      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
      incomingcontrol.esp_sender,
      incomingcontrol.esp_target,
      Ossm_paired ? 1 : 0,
      OSSM_Address[0], OSSM_Address[1], OSSM_Address[2], OSSM_Address[3], OSSM_Address[4], OSSM_Address[5]);
    return;  // Ignore messages from addon devices
  }
//(commGetMode() == COMM_MODE_ESPNOW || !commGetMode() == COMM_MODE_BLE)
  if (incomingcontrol.esp_target == M5_ID && Ossm_paired == true && !OssmBleIsMode()) {
    bool gotSpeed = false;
    bool gotDepth = false;
    if (incomingcontrol.esp_speed > 0) {
      // Cap at 600 (same as initial pairing) to protect slider range and SendCommand values.
      // Slider range updates happen in the main task via screenmachine/handleHomeScreen.
      speedlimit = (incomingcontrol.esp_speed > 600.0f) ? 600.0f : incomingcontrol.esp_speed;
      gotSpeed = true;
    }
    if (incomingcontrol.esp_depth > 0) {
      maxdepthinmm = incomingcontrol.esp_depth;
      gotDepth = true;
    }

  }

  // NOTE: Do not mirror OSSM_State from ESP-NOW telemetry here.
  // Some OSSM firmware variants provide inconsistent run-state metadata,
  // which can cause incorrect UI flips. Local ON/OFF commands remain the
  // source of truth for UI state in ESP-NOW mode.
}


bool espNowSendCommand(int Command, float Value, int Target) {

  if (!Ossm_paired) {
    LogDebug("Cannot send ESP-NOW command, OSSM not paired");
    return false;
  }

  if (Target == CUM || Target == CUM_ID) {
    return EjectSendCommand(Command, Value);
  }
  if (Target == FIST_ID) {
    return FistITSendCommand(Command, Value);
  } 
  else {
    LogDebugFormatted("Sending ESP-NOW command to OSSM: cmd=%d val=%.2f target=%d\n", Command, Value, Target);
    outgoingcontrol.esp_connected = true;
    outgoingcontrol.esp_command = Command;
    outgoingcontrol.esp_value = Value;
    outgoingcontrol.esp_target = Target;
    esp_err_t result = esp_now_send(OSSM_Address, (uint8_t *) &outgoingcontrol, sizeof(outgoingcontrol));
  
    if (result == ESP_OK) {
      return true;
    } 
    else {
      delay(20);
      esp_err_t result = esp_now_send(OSSM_Address, (uint8_t *) &outgoingcontrol, sizeof(outgoingcontrol));
      LogDebugFormatted("Retry ESP-NOW command result: %s\n", esp_err_to_name(result));
      return false;
    }
  }
  

}

void espNowKickPairingOLD() {
//    LogDebugFormatted("WIFI Channel: %d\n", WiFi.channel());
//    LogDebugFormatted("MAC Address: %s\n", WiFi.macAddress().c_str());
//    LogDebugFormatted("ESP_NOW Peerinfo Channel: %d\n", peerInfo.channel);
//    LogDebugFormatted("Peer exists: %d\n", esp_now_is_peer_exist(OSSM_Address));

    //re-add the peer if it doesn't exist because an addon falsely triggered a peer deletion by sending a message with the broadcast address as sender
    if (!esp_now_is_peer_exist(OSSM_Address)) {
      memcpy(peerInfo.peer_addr, OSSM_Address, 6);
      peerInfo.channel = 1;
      peerInfo.encrypt = false;
      esp_now_add_peer(&peerInfo);
      LogDebug("Re-added broadcast peer");
    }

    if(!Ossm_paired){
      outgoingcontrol.esp_command = HEARTBEAT;
      outgoingcontrol.esp_heartbeat = true;
      outgoingcontrol.esp_target = OSSM_ID;
      //esp_err_t result = esp_now_send(OSSM_Address, (uint8_t *) &outgoingcontrol, sizeof(outgoingcontrol));
      esp_err_t result = esp_now_send(BROADCAST_ADDR, (uint8_t *) &outgoingcontrol, sizeof(outgoingcontrol));
      if (result == ESP_OK) {
        LogDebug("Pairing heartbeat to OSSM sent successfully");
      } else {
        LogDebug("Failed to send pairing heartbeat for OSSM");
//        LogDebugFormatted("Send error: %s\n", esp_err_to_name(result));
      }
    }

}

//void EspNowSendPairingHeartbeat()
void espNowKickPairing()
{
  outgoingcontrol.esp_command = HEARTBEAT;
  outgoingcontrol.esp_heartbeat = true;
  outgoingcontrol.esp_target = OSSM_ID;
  outgoingcontrol.esp_sender = M5_ID;  // Identify M5 as sender
  esp_now_send(OSSM_Address, (uint8_t*)&outgoingcontrol, sizeof(outgoingcontrol));
}

void EspNowWaitForPairingOrTimeout(uint32_t timeoutMs, uint32_t heartbeatIntervalMs)
{
  uint32_t waitStartMs = millis();
  uint32_t lastHeartbeatMs = 0;

  while (!Ossm_paired && (millis() - waitStartMs) < timeoutMs) {
    if ((millis() - lastHeartbeatMs) >= heartbeatIntervalMs) {
      espNowKickPairing();
      lastHeartbeatMs = millis();
    }

    lv_task_handler();
    //M5.update();
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

bool espNowIsPaired() {
  return Ossm_paired;
}

bool espNowIsEjectConnected() {
  return EjectIsPaired();
}

bool espNowIsFistConnected() {
  return FistITIsPaired();
}

// ---------------------------------------------------------------------------
// Last-resort multi-channel sweep
// Cycles through all 13 WiFi channels broadcasting OSSM heartbeats.
// OSSM devices whose radio channel was set by a prior WiFi connection (e.g.
// OTA users) may not be on channel 1.  This sweep finds whatever channel OSSM
// happens to be on.
//
// Dwell time per channel: 60 × 500 ms = 30 s — long enough to cover a full
// sensorless homing run before OSSM sends its pairing response.
//
// Side-effect when OSSM is found on channel N ≠ 1: addon devices (Eject,
// FistIT) that operate on channel 1 will be unreachable until OSSM firmware
// is updated to explicitly lock to channel 1.
// ---------------------------------------------------------------------------
bool espNowMultiChannelSweep() {
  // Priority: channels 1, 6, 11 are the three non-overlapping 2.4 GHz channels
  // used by most routers worldwide, so they are tried first.
  static const uint8_t channels[] = {1, 6, 11, 2, 3, 4, 5, 7, 8, 9, 10, 12, 13};
  static const int CHANNEL_COUNT = (int)(sizeof(channels) / sizeof(channels[0]));

  // Suspend the background heartbeat task so it cannot reset the broadcast
  // peer's channel to 1 while we are iterating through other channels.
  if (eRemote_t) vTaskSuspend(eRemote_t);

  bool paired = false;
  char buf[32];

  for (int i = 0; i < CHANNEL_COUNT && !paired; i++) {
    snprintf(buf, sizeof(buf), "Scanning ch %d/%d", channels[i], CHANNEL_COUNT);
    if (ui_connect) lv_label_set_text(ui_connect, buf);

    // Move the radio to this channel.
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(channels[i], WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(false);

    // Re-register the broadcast peer on the new channel so esp_now_send
    // accepts packets (ESP-NOW rejects sends if peer channel != radio channel).
    if (esp_now_is_peer_exist(OSSM_Address)) {
      esp_now_del_peer(OSSM_Address);
    }
    memcpy(peerInfo.peer_addr, OSSM_Address, 6);
    peerInfo.channel = channels[i];
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);

    // Broadcast heartbeats for up to 30 s; stop early if OSSM responds.
    for (int j = 0; j < 60 && !Ossm_paired; j++) {
      outgoingcontrol.esp_command  = HEARTBEAT;
      outgoingcontrol.esp_heartbeat = true;
      outgoingcontrol.esp_target   = OSSM_ID;
      outgoingcontrol.esp_sender   = M5_ID;
      esp_now_send(BROADCAST_ADDR, (uint8_t *)&outgoingcontrol, sizeof(outgoingcontrol));
      delay(500);
    }
    paired = Ossm_paired;
  }

  if (!paired) {
    // Restore channel 0 and the broadcast peer so normal operation can
    // continue (the user can retry the standard path later).
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(0, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(false);
    if (esp_now_is_peer_exist(OSSM_Address)) {
      esp_now_del_peer(OSSM_Address);
    }
    memcpy(peerInfo.peer_addr, OSSM_Address, 6);
    peerInfo.channel = 1;
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);
  }

  // Resume the background task regardless of outcome.
  if (eRemote_t) vTaskResume(eRemote_t);
  return paired;
}

void espNowRemoteTask(void *pvParameters) {
  (void)pvParameters;

  LogDebug("ESP-NOW remote task started");
  for(;;){
    if(Ossm_paired){
      outgoingcontrol.esp_command = HEARTBEAT;
      outgoingcontrol.esp_heartbeat = true;
      outgoingcontrol.esp_target = OSSM_ID;
      outgoingcontrol.esp_sender = M5_ID;  // Identify M5 as sender (OSSM safely ignores unknown fields)
      esp_err_t result = esp_now_send(OSSM_Address, (uint8_t *) &outgoingcontrol, sizeof(outgoingcontrol));
    }
    else {
      espNowKickPairing();
    }
    vTaskDelay(HEARTBEAT_INTERVAL);
  } 
}

void espNowInit() {
/*
  WiFi.mode(WIFI_STA);

  // Explicitly set the WiFi channel to 1 before ESP-NOW init.
  // Without this, the radio stays on channel 0 (unset) and broadcast
  // packets are never received by devices on channel 1 (OSSM, Eject, FistIT).
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  LogDebug(WiFi.macAddress());
  LogDebugFormatted("WiFi channel after init: %d\n", WiFi.channel());

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);

  // register peer
  peerInfo.channel = 1;  
  peerInfo.encrypt = false;
  memcpy(peerInfo.peer_addr, OSSM_Address, 6);
  // register first peer  
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;}
  else {
    Serial.println("Peer added successfully");
  }
  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);

  xTaskCreatePinnedToCore(espNowRemoteTask,      // Task function. 
                            "espNowRemoteTask",  // name of task. 
                            3096,               // Stack size of task 
                            NULL,               // parameter of the task 
                            5,                  // priority of the task 
                            &eRemote_t,         // Task handle to keep track of created task 
                            0);                 // pin task to core 0 
  delay(200);
*/  

  // Ensure station mode and set explicit channel to match OSSM
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(ESP_NOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
  LogDebugFormatted("ESP-NOW channel set to %d\n", ESP_NOW_CHANNEL);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
  } else {
    Serial.println("esp_now_init ok");
  }

  esp_now_register_send_cb(OnDataSent);

  peerInfo.channel = ESP_NOW_CHANNEL;
  peerInfo.encrypt = false;
  memcpy(peerInfo.peer_addr, OSSM_Address, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
  }

  esp_now_register_recv_cb(OnDataRecv);

  xTaskCreatePinnedToCore(
    espNowRemoteTask,
    "espNowRemoteTask",
    3096,
    nullptr,
    5,
    &eRemote_t,
    0);
}

