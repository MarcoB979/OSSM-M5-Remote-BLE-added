#include "CommManager.h"

#include "../addons/Eject.h"
#include "../addons/FistIT.h"
#include "BleComm.h"
#include "../config/config_ids.h"
#include "../config/debug.h"
#include "../main.h"
#include "../screens/ScreenHandler.h"
#include "../ui/ui.h"
#include "language.h"

bool Ossm_paired = false;
volatile bool OSSM_On = false;

namespace {

// -------------------------------------------------------
// Internal Transport State
// -------------------------------------------------------
static CommTransportMode g_mode = COMM_MODE_NONE;

static void setMode(CommTransportMode mode) {
  g_mode = mode;
  bleCommSetEnabled(mode == COMM_MODE_BLE);
  if (mode == COMM_MODE_BLE) {
    speedlimit = 100.0f;
    maxdepthinmm = 100.0f;
  }
}

static int g_failedAttempts = 0;

static bool isAddonTarget(int target) {
  return target == CUM || target == EJECT_ID || target == FIST_ID;
}

static void tryConnectBleAddonsFromStart()
{
  // Explicitly allow addon connect attempts only from Start->Connect flow.
  EjectSetAddonEnabled(true);
  if (EjectTryConnectNow()) {
    LogDebug("Eject addon connected over BLE from Start flow");
  }
  FistITSetAddonEnabled(true);
  if (FistITTryConnectNow()) {
    LogDebug("Fist-IT addon connected over BLE from Start flow");
  }
}

static void tryPreloadBlePatternCatalogOnce()
{
  // Best-effort preload so Pattern screen does not need to perform a
  // synchronous BLE read during navigation.
  bleCommResetPatternReadState();
  if (readPatternsFromOSSM()) {
    LogDebug("OSSM pattern catalog cached from BLE connect flow");
  } else {
    LogDebug("OSSM pattern catalog preload skipped/failed");
  }
}

}  // namespace

// -------------------------------------------------------
// Public Transport Lifecycle
// -------------------------------------------------------
void commInit() {
  setMode(COMM_MODE_NONE);
  // BLE is lazy-initialised: only start the BLE stack when ESP-NOW pairing
  // fails.  Initialising NimBLE here would start the BLE controller
  // immediately, which competes with ESP-NOW broadcast packets (no MAC-layer
  // retry) via the coexistence scheduler and silently drops them.
}

CommTransportMode commGetMode() {
  if (g_mode == COMM_MODE_BLE && !bleCommIsConnected()) {
    setMode(COMM_MODE_NONE);
  }
  return g_mode;
}

bool commIsBleMode() {
  return commGetMode() == COMM_MODE_BLE;
}

// -------------------------------------------------------
// Public UI Connect Flow
// -------------------------------------------------------
void connectbutton(lv_event_t* e) {
  (void)e;
  //LogDebug("Connect button clicked");
  if (commGetMode() == COMM_MODE_BLE) {
    tryConnectBleAddonsFromStart();
    return;
  }
  delay(2000);
  LogDebug("Attempting to connect...");
//  if (ui_connect) lv_label_set_text(ui_connect, T_AUTOCONNECTING);
  if (ui_Welcome) lv_label_set_text(ui_Welcome, T_AUTOCONNECTING);
  lv_refr_now(NULL);  // force immediate render — lv_task_handler() is re-entrant-blocked inside an event callback
//  if (ui_connect) lv_label_set_text(ui_connect, T_SEARCHING_BLE);
  if (ui_Welcome) lv_label_set_text(ui_Welcome, T_SEARCHING_BLE);
  lv_refr_now(NULL);
  bleCommInit();
  if (bleCommTryConnect()) {
    g_failedAttempts = 0;
    LogDebug("BLE device found, connecting...");
    setMode(COMM_MODE_BLE);
    tryPreloadBlePatternCatalogOnce();
    LogDebug("BLE connection established");
//    if (ui_connect) lv_label_set_text(ui_connect, T_BLECONNECTED);
    if (ui_Welcome) lv_label_set_text(ui_Welcome, T_BLECONNECTED);
    lv_refr_now(NULL);
    LogDebug("Loading Menu screen...");
    lv_scr_load_anim(ui_Menu, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0, false);
    tryConnectBleAddonsFromStart();

    return;
  }
   else {
    tryConnectBleAddonsFromStart();
    lv_obj_clear_flag(ui_StartButtonM, LV_OBJ_FLAG_HIDDEN); // Show the middle button
    lv_obj_clear_flag(ui_StartButtonR, LV_OBJ_FLAG_HIDDEN); // Show the right button
    // First failure: prompt the user to try once more before the sweep.
//    if (ui_connect) lv_label_set_text(ui_connect, T_FAILED);
    lv_obj_set_align(ui_Welcome, LV_ALIGN_CENTER);
    if (ui_Welcome) lv_label_set_text(ui_Welcome, T_FAILED);
    lv_refr_now(NULL);
  }
}

bool SendCommand(int Command, float Value, int Target) {
  // Addon traffic uses each addon's own transport layer.
  if (Target == CUM || Target == EJECT_ID) {
    return EjectSendCommand(Command, Value);
  }
  if (Target == FIST_ID) {
    return FistITSendCommand(Command, Value);
  }

  // OSSM traffic is BLE-only.
  if (bleCommIsConnected()) {
    if (commGetMode() != COMM_MODE_BLE) {
      setMode(COMM_MODE_BLE);
    }
    //LogDebugFormatted("Sending command via BLE: Command=%d, Value=%.2f, Speed=%.2f, Depth=%.2f, Stroke=%.2f, MaxDepth=%.2f, MaxSpeed=%.2f\n",
    //                Command, Value, speed, depth, stroke, maxdepthinmm, speedlimit);
    return bleCommSendAppCommand(Command, Value, speed, depth, stroke, maxdepthinmm, speedlimit);
  }

  CommTransportMode mode = commGetMode();
  if (mode == COMM_MODE_NONE) {
    if (bleCommIsConnected()) {
      setMode(COMM_MODE_BLE);
      mode = COMM_MODE_BLE;
    } else {
      return false;
    }
  }

  if (mode == COMM_MODE_BLE) {
    return bleCommSendAppCommand(Command, Value, speed, depth, stroke, maxdepthinmm, speedlimit);
  }

  return false;
}

// -------------------------------------------------------
// Public Streaming Bridge
// -------------------------------------------------------
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
