#include "CommManager.h"

#include "../addons/Eject.h"
#include "../addons/FistIT.h"
#include "EspNowComm.h"
#include "BleComm.h"
#include "../config/config_ids.h"
#include "../config/debug.h"
#include "../main.h"
#include "../screens/ScreenHandler.h"
#include "../ui/ui.h"
#include "language.h"

namespace {

static CommTransportMode g_mode = COMM_MODE_NONE;

static void setMode(CommTransportMode mode) {
  g_mode = mode;
  bleCommSetEnabled(mode == COMM_MODE_BLE);
  if (mode == COMM_MODE_BLE) {
    speedlimit = 100.0f;
    maxdepthinmm = 100.0f;
    // Suspend the ESP-NOW heartbeat task while BLE is active so it doesn't
    // broadcast stale pairing packets or interfere with BLE coexistence.
    if (eRemote_t) vTaskSuspend(eRemote_t);
  } else {
    // Resume the ESP-NOW task whenever BLE is not active so auto-reconnect
    // (or re-pairing after BLE drop) can proceed normally.
    if (eRemote_t) vTaskResume(eRemote_t);
  }
}

// Tracks how many consecutive times both ESP-NOW and BLE have failed.
// First failure  (g_failedAttempts == 1): show "try again" hint.
// Second failure (g_failedAttempts == 2): escalate to multi-channel sweep.
// Reset to 0 on any successful connection or after the sweep completes.
static int g_failedAttempts = 0;

static bool tryEspNowFastConnect() {

  for (int i = 0; i < 5; ++i) {
    espNowKickPairing();
    delay(500);
    if (espNowIsPaired()) return true;
  }
  return false;
}

static bool isAddonTarget(int target) {
  return target == CUM || target == CUM_ID || target == FIST_ID;
}

}  // namespace

void commInit() {
  setMode(COMM_MODE_NONE);
  // BLE is lazy-initialised: only start the BLE stack when ESP-NOW pairing
  // fails.  Initialising NimBLE here would start the BLE controller
  // immediately, which competes with ESP-NOW broadcast packets (no MAC-layer
  // retry) via the coexistence scheduler and silently drops them.
}

CommTransportMode commGetMode() {
  if (g_mode == COMM_MODE_ESPNOW && !espNowIsPaired()) {
    setMode(COMM_MODE_NONE);
  }
  if (g_mode == COMM_MODE_BLE && !bleCommIsConnected()) {
    setMode(COMM_MODE_NONE);
  }
  return g_mode;
}

bool commIsBleMode() {
  return commGetMode() == COMM_MODE_BLE;
}

bool commIsEspNowMode() {
  return commGetMode() == COMM_MODE_ESPNOW;
}

void connectbutton(lv_event_t* e) {
  (void)e;
  //LogDebug("Connect button clicked");
  if (commGetMode() == COMM_MODE_ESPNOW || commGetMode() == COMM_MODE_BLE) {
    return;
  }
  delay(2000);
  LogDebug("Attempting to connect...");
//  if (ui_connect) lv_label_set_text(ui_connect, T_AUTOCONNECTING);
  if (ui_Welcome) lv_label_set_text(ui_Welcome, T_AUTOCONNECTING);
  lv_refr_now(NULL);  // force immediate render — lv_task_handler() is re-entrant-blocked inside an event callback

  // ── Tier 1: ESP-NOW (channel 1, standard path) ─────────────────────────
  if (tryEspNowFastConnect()) {

    g_failedAttempts = 0;
    setMode(COMM_MODE_ESPNOW);
//    if (ui_connect) lv_label_set_text(ui_connect, T_ESPCONNECTED);
    if (ui_Welcome) lv_label_set_text(ui_Welcome, T_ESPCONNECTED);
    lv_refr_now(NULL);
    lv_scr_load_anim(ui_Menu, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0, false);
    LogDebug("Connected via ESP-NOW");
    return;
  }

  // ── Tier 2: BLE fallback ────────────────────────────────────────────────
  LogDebug("ESP-NOW connect failed, trying BLE...");
//  if (ui_connect) lv_label_set_text(ui_connect, T_SEARCHING_BLE);
  if (ui_Welcome) lv_label_set_text(ui_Welcome, T_SEARCHING_BLE);
  lv_refr_now(NULL);
  // BLE is initialised lazily so the BLE controller is offline during the
  // ESP-NOW window above (prevents coexistence contention on broadcasts).
  bleCommInit();
  if (bleCommTryConnect()) {
    g_failedAttempts = 0;
    LogDebug("BLE device found, connecting...");
    setMode(COMM_MODE_BLE);
    LogDebug("BLE connection established");
//    if (ui_connect) lv_label_set_text(ui_connect, T_BLECONNECTED);
    if (ui_Welcome) lv_label_set_text(ui_Welcome, T_BLECONNECTED);
    lv_refr_now(NULL);
    LogDebug("Loading Menu screen...");
    lv_scr_load_anim(ui_Menu, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0, false);
    LogDebug("Connected via BLE");

    return;
  }
   else {
    lv_obj_clear_flag(ui_StartButtonM, LV_OBJ_FLAG_HIDDEN); // Show the middle button
    lv_obj_clear_flag(ui_StartButtonR, LV_OBJ_FLAG_HIDDEN); // Show the right button
    // First failure: prompt the user to try once more before the sweep.
//    if (ui_connect) lv_label_set_text(ui_connect, T_FAILED);
    if (ui_Welcome) lv_label_set_text(ui_Welcome, T_FAILED);
    lv_refr_now(NULL);
  }
}

bool SendCommand(int Command, float Value, int Target) {
  // Addon traffic always uses ESP-NOW so BLE OSSM control can coexist with addon modules.
  if (Target == CUM || Target == CUM_ID) {
    return EjectSendCommand(Command, Value);
  }
  if (Target == FIST_ID) {
    return FistITSendCommand(Command, Value);
  }

  // OSSM traffic prefers BLE when available, with ESP-NOW as fallback.
  if (bleCommIsConnected()) {
    if (commGetMode() != COMM_MODE_BLE) {
      setMode(COMM_MODE_BLE);
    }
    //LogDebugFormatted("Sending command via BLE: Command=%d, Value=%.2f, Speed=%.2f, Depth=%.2f, Stroke=%.2f, MaxDepth=%.2f, MaxSpeed=%.2f\n",
    //                Command, Value, speed, depth, stroke, maxdepthinmm, speedlimit);
    return bleCommSendAppCommand(Command, Value, speed, depth, stroke, maxdepthinmm, speedlimit);
  }

  if (espNowIsPaired()) {
    if (commGetMode() != COMM_MODE_ESPNOW) {
      setMode(COMM_MODE_ESPNOW);
    }
    return espNowSendCommand(Command, Value, Target);
  }

  CommTransportMode mode = commGetMode();
  if (mode == COMM_MODE_NONE) {
    if (espNowIsPaired()) {
      setMode(COMM_MODE_ESPNOW);
      mode = COMM_MODE_ESPNOW;
    } else if (bleCommIsConnected()) {
      setMode(COMM_MODE_BLE);
      mode = COMM_MODE_BLE;
    } else {
      return false;
    }
  }

  if (mode == COMM_MODE_ESPNOW) {
    return espNowSendCommand(Command, Value, Target);
  }

  if (mode == COMM_MODE_BLE) {
    return bleCommSendAppCommand(Command, Value, speed, depth, stroke, maxdepthinmm, speedlimit);
  }

  return false;
}

bool SendStreamCommand(int position, int durationMs) {
  // Stream commands are BLE-only by design.
  if (!bleCommTryConnect()) {
    return false;
  }

  if (commGetMode() != COMM_MODE_BLE) {
    setMode(COMM_MODE_BLE);
  }

  return bleCommSendStreamCommand(position, durationMs);
}
