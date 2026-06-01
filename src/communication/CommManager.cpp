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


/*
void connectbuttonNEW(lv_event_t *e)
{
  static constexpr uint32_t ESP_NOW_FIRST_WINDOW_MS_AUTO = 800UL;
  static constexpr uint32_t ESP_NOW_FIRST_WINDOW_MS_MANUAL = 2000UL;
  static constexpr uint32_t BLEAND_ESPNOW_ATTEMPTS_AUTO  = 2;
  static constexpr uint32_t BLEAND_ESPNOW_ATTEMPTS_MANUAL  = 8;
  static constexpr uint32_t LONG_WAIT_WINDOW_MS_AUTO     = 0UL;
  static constexpr uint32_t LONG_WAIT_WINDOW_MS_MANUAL   = 60000UL;
  static constexpr uint32_t HEARTBEAT_INTERVAL_MS   = 500UL;

  const bool manualAttempt = (e != nullptr);
  const uint32_t espNowFirstWindowMs = manualAttempt ? ESP_NOW_FIRST_WINDOW_MS_MANUAL : ESP_NOW_FIRST_WINDOW_MS_AUTO;
  const uint32_t bleAndEspNowAttempts = manualAttempt ? BLEAND_ESPNOW_ATTEMPTS_MANUAL : BLEAND_ESPNOW_ATTEMPTS_AUTO;
  const uint32_t longWaitWindowMs = manualAttempt ? LONG_WAIT_WINDOW_MS_MANUAL : LONG_WAIT_WINDOW_MS_AUTO;

  // Only treat OSSM as connected when transport is actually up.
  bool Ossm_paired = (OssmBleConnected() || Ossm_paired);
  if (Ossm_paired) {
    if (OssmBleIsMode()) {
      syncBleConnectUi(true);
    } else {
      lv_label_set_text(ui_Welcome, T_ESPCONNECTED);
    }
    return;
  }

  lv_label_set_text(ui_Welcome, T_CONNECTING);

  // Reset stale state so addon traffic can never block OSSM connection attempts.
  OssmBleSetMode(false);
  Ossm_paired = false;
  waiting_for_limits = false;

  // Phase 1: Try ESP_NOW first (it connects much faster than BLE)
  lv_label_set_text(ui_Welcome, "Searching...");
  EspNowWaitForPairingOrTimeout(longWaitWindowMs, HEARTBEAT_INTERVAL_MS);

  if (!Ossm_paired) {
    // Phase 2: ESP_NOW didn't connect, now try BLE + ESP_NOW in parallel
    for (uint32_t attempt = 0; attempt < bleAndEspNowAttempts && !Ossm_paired; ++attempt) {
      // Send ESP_NOW pairing heartbeat
      EspNowSendPairingHeartbeat();
      // Try BLE connection (skip if user forced ESP-only)
      bool bleConnected = false;
      if (!g_force_esp_only) {
        bleConnected = OssmBleTryConnect();
      }
      if (bleConnected) {
        lv_label_set_text(ui_Welcome, T_BLECONNECTED);
        OssmBleSetMode(true);
        Ossm_paired = true;
        syncBleConnectUi(true);
        return;
      }

      // Brief wait to allow ESP_NOW callback to set OSSM connected state.
      if (!Ossm_paired) {
        vTaskDelay(pdMS_TO_TICKS(200));
      }
    }
  }

  if (Ossm_paired) {
    lv_label_set_text(ui_Welcome, T_ESPCONNECTED);
    return;
  }

  // Phase 3: If still not connected, wait longer (manual attempts only)
  if (longWaitWindowMs > 0) {
    lv_label_set_text(ui_Welcome, T_LONG_WAIT);
    EspNowWaitForPairingOrTimeout(longWaitWindowMs, HEARTBEAT_INTERVAL_MS);
    if (Ossm_paired) {
      lv_label_set_text(ui_Welcome, T_ESPCONNECTED);
    } else {
      lv_label_set_text(ui_Welcome, T_FAILED);
    }
  } else {
    lv_label_set_text(ui_Welcome, T_CONNECT);
  }
}
*/


void connectbutton(lv_event_t* e) {
  (void)e;
  LogDebug("Connect button clicked");
  if (commGetMode() == COMM_MODE_ESPNOW || commGetMode() == COMM_MODE_BLE) {
    return;
  }
  delay(2000);
  LogDebug("Attempting to connect...");
  if (ui_connect) lv_label_set_text(ui_connect, T_AUTOCONNECTING);
  if (ui_Welcome) lv_label_set_text(ui_Welcome, T_AUTOCONNECTING);
  lv_refr_now(NULL);  // force immediate render — lv_task_handler() is re-entrant-blocked inside an event callback

  // ── Tier 1: ESP-NOW (channel 1, standard path) ─────────────────────────
  if (tryEspNowFastConnect()) {

    g_failedAttempts = 0;
    setMode(COMM_MODE_ESPNOW);
    if (ui_connect) lv_label_set_text(ui_connect, T_ESPCONNECTED);
    if (ui_Welcome) lv_label_set_text(ui_Welcome, T_ESPCONNECTED);
    lv_refr_now(NULL);
    lv_scr_load_anim(ui_Menu, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0, false);
    LogDebug("Connected via ESP-NOW");
    return;
  }

  // ── Tier 2: BLE fallback ────────────────────────────────────────────────
  LogDebug("ESP-NOW connect failed, trying BLE...");
  if (ui_connect) lv_label_set_text(ui_connect, T_SEARCHING_BLE);
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
    if (ui_connect) lv_label_set_text(ui_connect, T_BLECONNECTED);
    if (ui_Welcome) lv_label_set_text(ui_Welcome, T_BLECONNECTED);
    lv_refr_now(NULL);
    LogDebug("Loading ui_Menu...");
    lv_scr_load_anim(ui_Menu, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0, false);
    LogDebug("Connected via BLE");
    return;
  }
/*
  // ── Both methods failed ─────────────────────────────────────────────────
  LogDebug("BLE connect failed");
  g_failedAttempts++;

  if (g_failedAttempts >= 1) {
    // ── Tier 3 (last resort): multi-channel ESP-NOW sweep ────────────────
    // Shown when ESP-NOW and BLE have both failed twice in a row, which
    // typically means OSSM firmware has no explicit channel lock and its
    // radio drifted to a non-default channel via NVS (e.g. after OTA use).
    LogDebug("Starting multi-channel ESP-NOW sweep (last resort)");
    if (ui_connect) lv_label_set_text(ui_connect, "Trying multi-scan...\nPlease upgrade OSSM");
    if (ui_Welcome) lv_label_set_text(ui_Welcome, "Please upgrade\nOSSM firmware");

    if (espNowMultiChannelSweep()) {
      g_failedAttempts = 0;
      setMode(COMM_MODE_ESPNOW);
      if (ui_connect) lv_label_set_text(ui_connect, "Connected (ESP-NOW)");
      if (ui_Welcome) lv_label_set_text(ui_Welcome, "Connected");
      lv_scr_load_anim(ui_Menu, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0, false);
      LogDebug("Connected via ESP-NOW multi-channel sweep");
      return;
    }

    // Sweep exhausted all channels — reset so next press starts fresh.
    LogDebug("Multi-channel sweep failed");
    g_failedAttempts = 0;
    if (ui_connect) lv_label_set_text(ui_connect, "Connection failed");
    if (ui_Welcome) lv_label_set_text(ui_Welcome, "Connection failed");

  }
*/  
   else {
    // First failure: prompt the user to try once more before the sweep.
    if (ui_connect) lv_label_set_text(ui_connect, T_FAILED);
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
