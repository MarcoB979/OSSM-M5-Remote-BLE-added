#include <Arduino.h>
#include <M5Unified.h>
#include <esp_now.h>
#include <WiFi.h>
#include <cstring>
#include <cstddef>

#include "main.h"
#include "config.h"
#include "OssmBLE.h"
#include "language.h"
#include "ui/ui.h"

bool ossm_espnow_connected = false;  // True when confirmed OSSM is connected via ESP-NOW

namespace {

TaskHandle_t espNowRemoteTaskHandle = nullptr;

typedef struct struct_message {
  float esp_speed;
  float esp_depth;
  float esp_stroke;
  float esp_sensation;
  float esp_pattern;
  bool esp_rstate;
  bool esp_connected;
  bool esp_heartbeat;
  int esp_command;
  float esp_value;
  int esp_target;
  int esp_sender;  // Identifies the sender: OSSM_ID=1, or addon device ID
} struct_message;

struct_message outgoingcontrol;
struct_message incomingcontrol;
esp_now_peer_info_t peerInfo;
uint8_t OSSM_Address[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static constexpr int ESP_NOW_MSG_LEGACY_SIZE = (int)offsetof(struct_message, esp_sender);
static constexpr int ESP_NOW_MSG_FULL_SIZE = (int)sizeof(struct_message);

// Flags for UI updates (set from OnDataRecv wifi task, processed by main task)
static volatile bool esp_now_ui_pairing_complete = false;
static volatile bool esp_now_ui_limits_ready = false;
static volatile bool esp_now_ui_homing = false;

static constexpr TickType_t HEARTBEAT_INTERVAL = pdMS_TO_TICKS(5000);

void espNowRemoteTask(void *pvParameters)
{
  (void)pvParameters;
  for (;;) {
    if (Ossm_paired && !OssmBleIsMode()) {
      outgoingcontrol.esp_command = HEARTBEAT;
      outgoingcontrol.esp_heartbeat = true;
      outgoingcontrol.esp_target = OSSM_ID;
      outgoingcontrol.esp_sender = M5_ID;  // Identify M5 as sender (OSSM safely ignores unknown fields)
      esp_now_send(OSSM_Address, (uint8_t*)&outgoingcontrol, sizeof(outgoingcontrol));
    }
    vTaskDelay(HEARTBEAT_INTERVAL);
  }
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
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
  
  // During pairing phase, verify it's meant for M5 and sender is claiming to be OSSM
  if (!Ossm_paired) {
    return (incomingcontrol.esp_target == M5_ID && senderIsOssm);
  }
  
  // After pairing, verify:
  // 1. MAC matches stored OSSM_Address (MAC-level verification)
  // 2. Sender identifies as OSSM (payload-level verification)
  return (memcmp(mac, OSSM_Address, 6) == 0 && senderIsOssm);
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len)
{
  if (len < ESP_NOW_MSG_LEGACY_SIZE) {
    LogDebugPrioFormatted("ESP-NOW: Reject short packet len=%d (min=%d)\n", len, ESP_NOW_MSG_LEGACY_SIZE);
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
  LogDebugPrioFormatted(
    "ESP-NOW rx: from=%02X:%02X:%02X:%02X:%02X:%02X len=%d target=%d cmd=%d sender=%d hb=%d paired=%d bleMode=%d rawSpeed=%.1f rawDepth=%.1f\n",
    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
    len,
    incomingcontrol.esp_target,
    incomingcontrol.esp_command,
    incomingcontrol.esp_sender,
    incomingcontrol.esp_heartbeat ? 1 : 0,
    Ossm_paired ? 1 : 0,
    OssmBleIsMode() ? 1 : 0,
    incomingcontrol.esp_speed,
    incomingcontrol.esp_depth);

  if (incomingcontrol.esp_target == M5_ID && Ossm_paired == false) {
    esp_err_t result = esp_now_del_peer(peerInfo.peer_addr);

    if (result == ESP_OK) {
      memcpy(OSSM_Address, mac, 6);
      memcpy(peerInfo.peer_addr, OSSM_Address, 6);
      if (esp_now_add_peer(&peerInfo) == ESP_OK) {
        LogDebugFormatted("New peer added successfully, OSSM addresss : %02X:%02X:%02X:%02X:%02X:%02X\n",
          OSSM_Address[0], OSSM_Address[1], OSSM_Address[2], OSSM_Address[3], OSSM_Address[4], OSSM_Address[5]);
        Ossm_paired = true;
        esp_now_ui_pairing_complete = true;  // Signal UI update from main task
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
      if (incomingcontrol.esp_speed > 0 && incomingcontrol.esp_depth > 0) {
        // Received valid OSSM limits data - confirm OSSM is connected via ESP_NOW
        ossm_espnow_connected = true;
        LogDebugPrio("ESP_NOW: OSSM confirmed connected");
        esp_now_ui_limits_ready = true;  // Signal UI to show Home screen
      } else {
        esp_now_ui_homing = true;  // Signal UI to show homing message
      }
    }
  }

  // SECURITY: Verify incoming message is from paired OSSM, not an addon
  // This prevents interference from other ESP_NOW devices like Fist-IT or Eject
  if (!isOssmMessage(mac)) {
    LogDebugPrioFormatted(
      "ESP-NOW: Rejecting frame (from=%02X:%02X:%02X:%02X:%02X:%02X sender=%d target=%d paired=%d expected=%02X:%02X:%02X:%02X:%02X:%02X)\n",
      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
      incomingcontrol.esp_sender,
      incomingcontrol.esp_target,
      Ossm_paired ? 1 : 0,
      OSSM_Address[0], OSSM_Address[1], OSSM_Address[2], OSSM_Address[3], OSSM_Address[4], OSSM_Address[5]);
    return;  // Ignore messages from addon devices
  }

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

    if (waiting_for_limits && gotSpeed && gotDepth) {
      waiting_for_limits = false;
      esp_now_ui_limits_ready = true;  // Signal main task to update UI
    }
  }

  // NOTE: Do not mirror OSSM_State from ESP-NOW telemetry here.
  // Some OSSM firmware variants provide inconsistent run-state metadata,
  // which can cause incorrect UI flips. Local ON/OFF commands remain the
  // source of truth for UI state in ESP-NOW mode.
}

} // namespace

void EspNowInitCommunication()
{
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
  } else {
    Serial.println("esp_now_init ok");
  }

  esp_now_register_send_cb(OnDataSent);

  peerInfo.channel = 0;
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
    &espNowRemoteTaskHandle,
    0);
}

void EspNowSendPairingHeartbeat()
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
      EspNowSendPairingHeartbeat();
      lastHeartbeatMs = millis();
    }

    lv_task_handler();
    M5.update();
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

bool EspNowSendControlCommand(int command, float value, int target)
{
  if (!Ossm_paired) {
    return false;
  }

  outgoingcontrol.esp_connected = true;
  outgoingcontrol.esp_command = command;
  outgoingcontrol.esp_value = value;
  outgoingcontrol.esp_target = target;
  outgoingcontrol.esp_sender = M5_ID;  // Identify M5 as sender

  esp_err_t result = esp_now_send(OSSM_Address, (uint8_t*)&outgoingcontrol, sizeof(outgoingcontrol));
  if (result == ESP_OK) {
    return true;
  }

  vTaskDelay(pdMS_TO_TICKS(20));
  esp_now_send(OSSM_Address, (uint8_t*)&outgoingcontrol, sizeof(outgoingcontrol));
  return false;
}

// Process pending UI updates from ESP-NOW pairing/data in a thread-safe way
// Call this from the main task loop to safely update LVGL UI
void EspNowProcessPendingUiUpdates()
{
  // Check if pairing just completed
  if (esp_now_ui_pairing_complete) {
    esp_now_ui_pairing_complete = false;
    lv_label_set_text(ui_Welcome, T_ESPCONNECTED);
  }

  // Check if limits are ready and we should show home screen
  if (esp_now_ui_limits_ready) {
    if (isStartScreenMinTimeElapsed()) {
      esp_now_ui_limits_ready = false;
      lv_label_set_text(ui_connect, "WIFI");
      lv_scr_load_anim(ui_Home, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0, false);
    }
  }

  // Check if homing is in progress
  if (esp_now_ui_homing) {
    esp_now_ui_homing = false;
    waiting_for_limits = true;
    lv_label_set_text(ui_Welcome, T_HOMING);
  }
}
