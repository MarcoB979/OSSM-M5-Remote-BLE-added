#include "Xtoys.h"
#include "OssmBLE.h"
// Note: config.h cannot be included here — it instantiates OneButton objects that
// require OneButton.h to be pulled in first (done only in main.cpp). Instead,
// copy only the constants we need.
#include "main.h"
#include "ui/ui.h"
#include <Arduino.h>
#include "language.h"

// Constants sourced from config.h (kept in sync manually)
#ifndef OSSM_ID
  #define OSSM_ID 1
#endif

// Logging macros (mirrors main.cpp — defined here since they live in main.cpp, not a shared header)
#ifndef LogDebug
  #ifdef DEBUG
    #define LogDebug(...) Serial.println(__VA_ARGS__)
    #define LogDebugFormatted(...) Serial.printf(__VA_ARGS__)
  #else
    #define LogDebug(...) ((void)0)
    #define LogDebugFormatted(...) ((void)0)
  #endif
#endif
#ifndef LogDebugPrio
  #ifdef DEBUGPRIO
    #define LogDebugPrio(...) Serial.println(__VA_ARGS__)
    #define LogDebugPrioFormatted(...) Serial.printf(__VA_ARGS__)
  #else
    #define LogDebugPrio(...) ((void)0)
    #define LogDebugPrioFormatted(...) ((void)0)
  #endif
#endif

// SendCommand command IDs (mirrors main.cpp)
#ifndef SPEED
  #define SPEED 1
#endif

// ============================================================================
// State Management
// ============================================================================

static bool xtoysMode = false;                    // Is X-Toys mode active?
static unsigned long xtoysModeStartTime = 0;     // When was it activated?
static const unsigned long XTOYS_TIMEOUT_MS = 600000;  // 10 minutes safety timeout
static bool xtoysPrimePending = false;            // One-time prime/start arm after activation
static bool xtoysPrimeDone = false;               // One-time prime/start done
static bool xtoysBufferConfigured = false;        // Streaming buffer override sent
static unsigned long xtoysLastPrimePollMs = 0;    // Throttle OSSM state polling
static const unsigned long XTOYS_CONTROL_POLL_MS = 40;
static unsigned long xtoysLastStatusPollMs = 0;   // Throttle OSSM status display
static const unsigned long XTOYS_STATUS_POLL_MS = 6000;
static bool xtoysRecoveryActive = false;          // Force menu->streaming recovery path active
static bool xtoysRecoveryMenuSent = false;        // go:menu sent during recovery
static bool xtoysRecoveryStreamingSent = false;   // retry go:streaming sent after menu/homing
static unsigned long xtoysLastRecoveryCmdMs = 0;  // Cooldown for recovery command retries
static const unsigned long XTOYS_RECOVERY_RETRY_MS = 180;
static const float XTOYS_STREAM_BUFFER_PERCENT = 0.0f;

// Forward declarations (from main.cpp)
extern bool OssmBleIsMode();
extern float speed;
extern float depth;
extern float stroke;
extern bool OSSM_State;
extern bool SendCommand(int Command, float Value, int Target);
// Note: OSSM_ID is a #define in config.h — no extern needed
extern int st_screens;
extern void screenmachine(lv_event_t *e);
extern char patternstr[];

// ============================================================================
// Helpers
// ============================================================================

static const char* xtoysStateName(OssmBleMachineMode mode) {
    switch (mode) {
        case OssmBleMachineMode::Streaming:          return "Streaming";
        case OssmBleMachineMode::Menu:               return "In Menu";
        case OssmBleMachineMode::StrokeEngineIdle:   return "SE Idle";
        case OssmBleMachineMode::StrokeEngineActive: return "SE Active";
        case OssmBleMachineMode::HomingForward:
        case OssmBleMachineMode::HomingBackward:     return "Homing...";
        case OssmBleMachineMode::SimplePenetration:  return "Simple";
        default:                                     return "Unknown";
    }
}

  static bool xtoysRefreshStateQuiet(OssmBleMachineState* outState) {
    if (outState == nullptr) {
      return false;
    }

    String ignored;
    OssmBleReadState(&ignored, false);
    return OssmBleGetCurrentState(outState, false);
  }

  static const char* xtoysRecoveryLabel(const OssmBleMachineState& state) {
    switch (state.mode) {
      case OssmBleMachineMode::StrokeEngineIdle:
      case OssmBleMachineMode::StrokeEngineActive:
      case OssmBleMachineMode::SimplePenetration:
        return "Recovering...";
      case OssmBleMachineMode::Menu:
        return "Entering stream";
      case OssmBleMachineMode::HomingForward:
      case OssmBleMachineMode::HomingBackward:
        return "Homing...";
      default:
        return nullptr;
    }
  }

  static void xtoysResetRecovery() {
    xtoysRecoveryActive = false;
    xtoysRecoveryMenuSent = false;
    xtoysRecoveryStreamingSent = false;
    xtoysLastRecoveryCmdMs = 0;
  }

  static void xtoysStartRecovery() {
    xtoysRecoveryActive = true;
    xtoysRecoveryMenuSent = false;
    xtoysRecoveryStreamingSent = false;
    xtoysLastRecoveryCmdMs = 0;
  }

// ============================================================================
// Core X-Toys Functions
// ============================================================================

void XToysInit() {
    xtoysMode = false;
    xtoysModeStartTime = 0;
    xtoysPrimePending = false;
    xtoysPrimeDone = false;
    xtoysBufferConfigured = false;
    xtoysLastPrimePollMs = 0;
    xtoysLastStatusPollMs = 0;
  xtoysResetRecovery();
    LogDebug("X-Toys mode initialized");
}

static bool xtoysShouldActivelyPollState() {
  // Only poll aggressively while trying to reach/confirm streaming.
  return xtoysPrimePending || xtoysRecoveryActive;
}

void XToysActivate() {
    // Only works in BLE mode
    if (!OssmBleIsMode()) {
        LogDebug("X-Toys: BLE mode not active, cannot activate");
        return;
    }

    // Prevent accidental reactivation
    if (xtoysMode) {
        LogDebug("X-Toys: Already in X-Toys mode, ignoring");
        return;
    }

    LogDebugPrio("X-Toys: Activating X-Toys mode");

    // Step 1: Enter Streaming mode for real-time external position control.
    if (!OssmBleGoToStreaming()) {
      LogDebug("X-Toys: Failed to enter Streaming mode");
      lv_label_set_text(ui_connect, "X-Toys Fail");
      return;
    }

    // Wait a moment for mode transition
    vTaskDelay(pdMS_TO_TICKS(200));

    // Step 2: Mark mode as active and arm one-time prime/start.
    // We prime on first confirmed Streaming state sample after activation.
    xtoysMode = true;
    xtoysModeStartTime = millis();
    xtoysPrimePending = true;
    xtoysPrimeDone = false;
    xtoysBufferConfigured = false;
    xtoysStartRecovery();

    // Step 3: Update UI to show X-Toys waiting state
    LogDebugPrio("X-Toys: Mode activated successfully");
    XToysDisplayStatus();

    // Show initial state on Home screen immediately
    if (lv_scr_act() == ui_Home) {
        lv_label_set_text(ui_HomePatternLabel1, "OSSM:");
        lv_label_set_text(ui_HomePatternLabel, "Waiting...");
    }
}

void XToysDeactivate() {
    if (!xtoysMode) {
        return;  // Already deactivated
    }

    LogDebugPrio("X-Toys: Deactivating X-Toys mode");

    xtoysMode = false;
    xtoysModeStartTime = 0;
    xtoysPrimePending = false;
    xtoysPrimeDone = false;
    xtoysBufferConfigured = false;
    xtoysLastPrimePollMs = 0;
    xtoysLastStatusPollMs = 0;
    xtoysResetRecovery();

    // Restore pattern label row
    lv_label_set_text(ui_HomePatternLabel1, T_Patterns);
    lv_label_set_text(ui_HomePatternLabel, patternstr);

    // Return to normal UI
    lv_label_set_text(ui_connect, "BLE");

    LogDebugPrio("X-Toys: Mode deactivated");
}

bool XToysIsActive() {
    return xtoysMode;
}

void XToysUpdate() {
    // If X-Toys mode is not active, nothing to do
    if (!xtoysMode) {
        return;
    }

    // Safety timeout: if in X-Toys mode for too long, auto-deactivate
    unsigned long elapsedMs = millis() - xtoysModeStartTime;
    if (elapsedMs > XTOYS_TIMEOUT_MS) {
        LogDebugPrio("X-Toys: Timeout reached, auto-deactivating");
        XToysDeactivate();
        lv_label_set_text(ui_connect, "Timeout");
        return;
    }

    // One-time trial behavior: once Streaming is confirmed, prime limits and
    // issue start semantics exactly once. This approximates "first stream
    // activity" from M5 side without requiring raw third-party frame visibility.
    if (xtoysPrimePending && !xtoysPrimeDone) {
      unsigned long nowMs = millis();
      if ((nowMs - xtoysLastPrimePollMs) >= XTOYS_CONTROL_POLL_MS) {
        xtoysLastPrimePollMs = nowMs;
        OssmBleMachineState bleState;
        if (xtoysRefreshStateQuiet(&bleState)) {
          const char* recoveryLabel = xtoysRecoveryLabel(bleState);
          if (recoveryLabel != nullptr && lv_scr_act() == ui_Home) {
            lv_label_set_text(ui_HomePatternLabel1, "OSSM:");
            lv_label_set_text(ui_HomePatternLabel, recoveryLabel);
          }

          if (bleState.mode == OssmBleMachineMode::Streaming) {
            if (!xtoysBufferConfigured) {
              if (OssmBleSetBuffer(XTOYS_STREAM_BUFFER_PERCENT)) {
                xtoysBufferConfigured = true;
                LogDebugPrio("X-Toys: Set streaming buffer to minimal latency");
              } else {
                LogDebug("X-Toys: Failed to set streaming buffer");
              }
            }

            OssmBleSendText("set:depth:100", nullptr);
            OssmBleSendText("set:stroke:100", nullptr);
            OssmBleSendText("set:speed:100", nullptr);

            depth = 100.0f;
            stroke = 100.0f;
            speed = 100.0f;
            OSSM_State = true;

            // Reflect primed values on-screen immediately when Home is visible.
            if (lv_scr_act() == ui_Home) {
              lv_slider_set_value(ui_homedepthslider, 100, LV_ANIM_OFF);
              lv_slider_set_value(ui_homestrokeslider, 100, LV_ANIM_OFF);
              lv_slider_set_value(ui_homespeedslider, 100, LV_ANIM_OFF);
              lv_label_set_text(ui_homedepthvalue, "100");
              lv_label_set_text(ui_homestrokevalue, "100");
              lv_label_set_text(ui_homespeedvalue, "100");
            }

            xtoysPrimeDone = true;
            xtoysPrimePending = false;
            xtoysResetRecovery();
            LogDebugPrio("X-Toys: One-time streaming prime/start applied (100/100/100)");
          } else if (xtoysRecoveryActive) {
            bool canSendRecoveryCmd = (millis() - xtoysLastRecoveryCmdMs) >= XTOYS_RECOVERY_RETRY_MS;
            if (!xtoysRecoveryMenuSent &&
                (bleState.mode == OssmBleMachineMode::StrokeEngineIdle ||
                 bleState.mode == OssmBleMachineMode::StrokeEngineActive ||
                 bleState.mode == OssmBleMachineMode::SimplePenetration) &&
                canSendRecoveryCmd) {
              if (OssmBleGoToMenu()) {
                xtoysRecoveryMenuSent = true;
                xtoysLastRecoveryCmdMs = millis();
                LogDebugPrio("X-Toys: OSSM not in streaming, sent go:menu recovery");
              }
            } else if (xtoysRecoveryMenuSent && !xtoysRecoveryStreamingSent &&
                       (bleState.mode == OssmBleMachineMode::Menu ||
                        bleState.mode == OssmBleMachineMode::HomingForward ||
                        bleState.mode == OssmBleMachineMode::HomingBackward) &&
                       canSendRecoveryCmd) {
              if (OssmBleGoToStreaming()) {
                xtoysRecoveryStreamingSent = true;
                xtoysLastRecoveryCmdMs = millis();
                LogDebugPrio("X-Toys: Recovery retry sent go:streaming");
              }
            } else if (xtoysRecoveryStreamingSent && bleState.mode == OssmBleMachineMode::Menu && canSendRecoveryCmd) {
              if (OssmBleGoToStreaming()) {
                xtoysLastRecoveryCmdMs = millis();
                LogDebugPrio("X-Toys: Recovery re-sent go:streaming from menu");
              }
            }
          }
        }
      }
    }

    // Periodic OSSM state display on Home screen
    if (lv_scr_act() == ui_Home) {
        unsigned long statusNowMs = millis();
        if ((statusNowMs - xtoysLastStatusPollMs) >= XTOYS_STATUS_POLL_MS) {
            xtoysLastStatusPollMs = statusNowMs;
            OssmBleMachineState statusState;
            lv_label_set_text(ui_HomePatternLabel1, "OSSM:");
        bool gotState = false;
        if (xtoysShouldActivelyPollState()) {
          gotState = xtoysRefreshStateQuiet(&statusState);
        } else {
          gotState = OssmBleGetCurrentState(&statusState, false);
        }

        if (gotState) {
                lv_label_set_text(ui_HomePatternLabel, xtoysStateName(statusState.mode));
            } else {
                lv_label_set_text(ui_HomePatternLabel, "No signal");
            }
        }
    }

    // Monitor BLE connection: if all clients disconnected, we should exit
    // Note: M5 remote stays connected, so this shouldn't happen while X-Toys is using it
    // But if something goes wrong, fallback to safety
    if (!OssmBleIsMode()) {
        LogDebugPrio("X-Toys: BLE connection lost, deactivating");
        XToysDeactivate();
        lv_label_set_text(ui_connect, "No BLE");
    }
}

void XToysDisplayStatus() {
    if (!xtoysMode) {
        return;
    }

    // Display shows "Waiting for X-Toys..."
    lv_label_set_text(ui_connect, T_XTOYS);

    // Optional: show elapsed time or just the status
    LogDebugFormatted("X-Toys: Active for %lu ms\n", millis() - xtoysModeStartTime);
}
