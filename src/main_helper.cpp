#include "main.h"
#include <Arduino.h>
#include <esp_sleep.h>
#include <esp_timer.h>
#include <NimBLEDevice.h>

#include "config.h"
#include "OssmBLE.h"
#include "language.h"
#include "ui/ui.h"
#include "colors.h"
#include "esp_nowCommunication.h"

int st_screens = ST_UI_START;

bool strokeinvert_mode = false;
bool touch_disabled = false;
// When true, entering the UI Menu will send a go:menu BLE command and
// enable the Menu->Home re-entry homing path. Default false to avoid
// forcing homing on simple menu navigation.
bool bleForceHomeing = false;

int pattern = 2;

float maxdepthinmm = 400.0;
float speedlimit = 600;

float speed = 0.0;
float depth = 0.0;
float stroke = 0.0;
float sensation = 0.0;
float torqe_f = 100.0;
float torqe_r = -180.0;

unsigned long last_activity_ms = 0;
int screensaver_prev_brightness = 180;
bool screensaver_active = false;
uint32_t ossm_state_monitor_hold_until_ms = 0;

bool dynamicStroke = false;
bool g_ble_menu_requires_stroke_reentry = false;

bool Ossm_paired = false;
bool waiting_for_limits = false; // true after pairing until OSSM reports max depth/speed
bool g_force_esp_only = false;

int OSSM_State = state_FALSE;
int g_homing_direction = 0;

static uint32_t g_start_screen_loaded_ms = 0;

void markStartScreenLoaded()
{
  g_start_screen_loaded_ms = millis();
}

bool isStartScreenMinTimeElapsed()
{
  if (g_start_screen_loaded_ms == 0) {
    return false;
  }
  return (millis() - g_start_screen_loaded_ms) >= START_SCREEN_MIN_DISPLAY_MS;
}

static const char* ossmStateToText(int state)
{
  switch (state) {
    case state_OFF: return "OFF";
    case state_ON: return "ON";
    case state_PAUSE: return "PAUSE";
    case state_STOP: return "STOP";
    case state_MENU: return "MENU";
    case state_HOMING: return "HOMING";
    case state_STREAMING: return "STREAM";
    case state_SIMPLE_PENETRATION: return "SIMPLE";
    case state_STROKE_ENGINE: return "STROKE";
    case state_ERROR: return "ERROR";
    default: return "UNKNOWN";
  }
}

void monitorOssmState(bool forceBlePoll)
{
  static uint32_t lastBlePollMs = 0;
  const uint32_t now = millis();
  int nextState = OSSM_State;

  if (OssmBleIsMode()) {
    if (!forceBlePoll && now < ossm_state_monitor_hold_until_ms) {
      refreshHomeAndStreamingStartStopUi();
      updateHomeTopLeftStateLabel();
      return;
    }

    const uint32_t pollInterval = (OSSM_State == state_STREAMING) ? 3000U : 300U;
    bool shouldPoll = forceBlePoll || ((now - lastBlePollMs) >= pollInterval);
    if (shouldPoll) {
      lastBlePollMs = now;
      OssmBleMachineState bleState;
      if (OssmBleGetCurrentState(&bleState, true)) {
        switch (bleState.mode) {
          case OssmBleMachineMode::HomingForward:
            g_homing_direction = 1;
            nextState = state_HOMING;
            break;
          case OssmBleMachineMode::HomingBackward:
            g_homing_direction = -1;
            nextState = state_HOMING;
            break;
          case OssmBleMachineMode::Menu:
            g_homing_direction = 0;
            nextState = state_MENU;
            break;
          case OssmBleMachineMode::Streaming:
            g_homing_direction = 0;
            nextState = state_STREAMING;
            break;
          case OssmBleMachineMode::SimplePenetration:
            g_homing_direction = 0;
            nextState = state_SIMPLE_PENETRATION;
            break;
          case OssmBleMachineMode::StrokeEngineActive:
            g_homing_direction = 0;
            nextState = (bleState.speed > 0) ? state_ON : state_PAUSE;
            break;
          case OssmBleMachineMode::StrokeEngineIdle:
            g_homing_direction = 0;
            nextState = (bleState.speed > 0) ? state_ON : state_PAUSE;
            break;
          default:
            g_homing_direction = 0;
            break;
        }
        // If OSSM BLE code intentionally paused the device as a workaround
        // for setting stroke->0, suppress transient UI pause transitions
        // until the machine actually reports stroke==0.
        if (nextState == state_PAUSE) {
          if (OssmBleShouldSuppressUiPause(bleState.stroke)) {
            nextState = OSSM_State; // keep previous UI state
          }
        }
      }
    }
  }

  static int pre_menu_ossm_state = state_FALSE;

  if (st_screens == ST_UI_MENU && nextState != state_HOMING && nextState != state_ERROR) {
    if (!OssmBleIsMode() && OSSM_State != state_MENU) {
      pre_menu_ossm_state = OSSM_State;  // save last known state before menu overrides it
    }
    nextState = state_MENU;
  } else if (!OssmBleIsMode() && OSSM_State == state_MENU) {
    // Returned from Menu in ESP-NOW mode – restore the pre-menu state so
    // homebuttonm_action can correctly toggle the OSSM on/off.
    nextState = pre_menu_ossm_state;
  }

  if (nextState != OSSM_State) {
    OSSM_State = nextState;
    LogDebugFormatted("OSSM_State -> %d\n", OSSM_State);
  }

  refreshHomeAndStreamingStartStopUi();
  updateHomeTopLeftStateLabel();
}

uint32_t my_tick_function()
{
  return (esp_timer_get_time() / 1000LL);
}

void screensaver_check_activity()
{
  last_activity_ms = millis();
  if (screensaver_active) {
    M5.Lcd.setBrightness(screensaver_prev_brightness);
    screensaver_active = false;
  }
}

// Keep this configurable for future non-blocking interaction modes.
static bool g_notification_blocks_inputs = true;
static volatile int g_notification_touch_result = NOTIFICATION_RESULT_NONE;

static void notification_left_button_cb(lv_event_t *e)
{
  (void)e;
  g_notification_touch_result = NOTIFICATION_RESULT_LEFT;
}

static void notification_right_button_cb(lv_event_t *e)
{
  (void)e;
  g_notification_touch_result = NOTIFICATION_RESULT_RIGHT;
}

int showNotification(const char *title,
                     const char *text,
                     uint32_t duration,
                     bool showLeftButton,
                     const char *leftButtonText,
                     bool showRightButton,
                     const char *rightButtonText,
                     bool showFullScreen)
{
  const bool hasButtons = showLeftButton || showRightButton;
  const bool prevTouchDisabled = touch_disabled;
  const bool shouldBlockTouch = g_notification_blocks_inputs && !hasButtons;
  const uint32_t startMs = millis();
  int result = NOTIFICATION_RESULT_NONE;
  g_notification_touch_result = NOTIFICATION_RESULT_NONE;

  // Get active color scheme colors
  uint32_t schemePrimary = getActivePrimaryColor();
  uint32_t schemeSecondary = getActiveSecondaryColor();
  uint32_t schemeTextPrimary = getActiveTextPrimaryColor();
  uint32_t schemeTextSecondary = getActiveTextSecondaryColor();
  // Derive darker color for overlay (approximately 60% of primary's brightness reduced)
  uint8_t pr = (schemePrimary >> 16) & 0xFF;
  uint8_t pg = (schemePrimary >> 8) & 0xFF;
  uint8_t pb = schemePrimary & 0xFF;
  uint32_t schemeDarker = 0x000000;  // Default to black if we can't compute
  // Dark shade: reduce each component by 50%
  schemeDarker = (((pr >> 1) & 0xFF) << 16) | (((pg >> 1) & 0xFF) << 8) | ((pb >> 1) & 0xFF);

  if (shouldBlockTouch) {
    touch_disabled = true;
  }

  // Drain stale button states before opening the modal.
  clearButtonFlags();

  lv_obj_t *overlay = lv_obj_create(lv_layer_top());
  lv_obj_remove_style_all(overlay);
  lv_obj_set_size(overlay, HOR_RES, VER_RES);
  lv_obj_center(overlay);
  lv_obj_set_style_bg_opa(overlay, LV_OPA_50, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(overlay, lv_color_hex(schemeDarker), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_flag(overlay, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *panel = lv_obj_create(overlay);
  if (showFullScreen) {
    const int topOffset = 32;
    const int bottomPadding = 5;
    lv_obj_set_size(panel, 310, VER_RES - topOffset - bottomPadding);
    lv_obj_set_pos(panel, 5, topOffset);
  } else {
    lv_obj_set_size(panel, (HOR_RES * 90) / 100, (VER_RES * 75) / 100);
    lv_obj_align(panel, LV_ALIGN_CENTER, 0, 5);
  }
  lv_obj_set_style_radius(panel, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(panel, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(panel, lv_color_hex(schemePrimary), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(panel, lv_color_hex(schemeSecondary), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

  lv_obj_t *titleBar = lv_obj_create(panel);
  lv_obj_remove_style_all(titleBar);
  lv_obj_set_size(titleBar, lv_pct(100), 32);
  lv_obj_align(titleBar, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_set_style_bg_opa(titleBar, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(titleBar, lv_color_hex(schemeDarker), LV_PART_MAIN | LV_STATE_DEFAULT);

  lv_obj_t *titleLabel = lv_label_create(titleBar);
  lv_label_set_text(titleLabel, (title != nullptr && title[0] != '\0') ? title : "Notification");
  lv_obj_set_style_text_color(titleLabel, lv_color_hex(schemeTextPrimary), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(titleLabel, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_center(titleLabel);

  lv_obj_t *bodyLabel = lv_label_create(panel);
  lv_obj_set_width(bodyLabel, lv_pct(90));
  lv_label_set_long_mode(bodyLabel, LV_LABEL_LONG_WRAP);
  lv_label_set_text(bodyLabel, (text != nullptr) ? text : "");
  lv_obj_set_style_text_color(bodyLabel, lv_color_hex(schemeTextSecondary), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(bodyLabel, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_align(bodyLabel, LV_ALIGN_TOP_MID, 0, 48);

  if (hasButtons) {
    lv_obj_t *buttonRow = lv_obj_create(panel);
    lv_obj_remove_style_all(buttonRow);
    lv_obj_set_size(buttonRow, lv_pct(94), 44);
    lv_obj_align(buttonRow, LV_ALIGN_BOTTOM_MID, 0, -12);
    lv_obj_set_flex_flow(buttonRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(buttonRow, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    if (showLeftButton) {
      lv_obj_t *leftBtn = lv_btn_create(buttonRow);
      lv_obj_set_size(leftBtn, 120, 36);
      lv_obj_set_style_bg_color(leftBtn, lv_color_hex(schemePrimary), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_bg_color(leftBtn, lv_color_hex(schemeDarker), LV_PART_MAIN | LV_STATE_PRESSED);
      lv_obj_set_style_border_width(leftBtn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

      lv_obj_t *leftLbl = lv_label_create(leftBtn);
      lv_label_set_text(leftLbl, (leftButtonText != nullptr && leftButtonText[0] != '\0') ? leftButtonText : "Left");
      lv_obj_set_style_text_color(leftLbl, lv_color_hex(schemeTextPrimary), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_center(leftLbl);
      lv_obj_add_event_cb(leftBtn, notification_left_button_cb, LV_EVENT_SHORT_CLICKED, nullptr);
    }

    if (showRightButton) {
      lv_obj_t *rightBtn = lv_btn_create(buttonRow);
      lv_obj_set_size(rightBtn, 120, 36);
      lv_obj_set_style_bg_color(rightBtn, lv_color_hex(schemePrimary), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_bg_color(rightBtn, lv_color_hex(schemeDarker), LV_PART_MAIN | LV_STATE_PRESSED);
      lv_obj_set_style_border_width(rightBtn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

      lv_obj_t *rightLbl = lv_label_create(rightBtn);
      lv_label_set_text(rightLbl, (rightButtonText != nullptr && rightButtonText[0] != '\0') ? rightButtonText : "Right");
      lv_obj_set_style_text_color(rightLbl, lv_color_hex(schemeTextPrimary), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_center(rightLbl);
      lv_obj_add_event_cb(rightBtn, notification_right_button_cb, LV_EVENT_SHORT_CLICKED, nullptr);
    }
  }

  static uint32_t notif_last_state_log_ms = 0;
  ButtonEvents modalButtons = {};

  while (true) {
    M5.update();
    lv_task_handler();
    Button1.tick();
    Button2.tick();
    Button3.tick();

    // Periodic BLE state log inside modal (every 2 s) so streaming activity
    // is visible on serial even while notification blocks the main loop.
    if (OssmBleIsMode() && (millis() - notif_last_state_log_ms) >= 2000U) {
      notif_last_state_log_ms = millis();
      String stateText;
      OssmBleReadState(&stateText, true);
    }

    if (duration > 0 && (millis() - startMs) >= duration) {
      result = NOTIFICATION_RESULT_NONE;
      break;
    }

    pollButtonEvents(modalButtons);

    if (hasButtons) {
      if (g_notification_touch_result != NOTIFICATION_RESULT_NONE) {
        result = g_notification_touch_result;
        break;
      }
      if (showLeftButton && modalButtons.leftShort) {
        result = NOTIFICATION_RESULT_LEFT;
        clearButtonFlags();
        break;
      }
      if (showRightButton && modalButtons.rightShort) {
        result = NOTIFICATION_RESULT_RIGHT;
        clearButtonFlags();
        break;
      }
    }

    // Consume all activity so the current screen never receives latent button events.
    clearButtonFlags();

    vTaskDelay(pdMS_TO_TICKS(5));
  }

  lv_obj_del(overlay);

  clearButtonFlags();

  if (shouldBlockTouch) {
    touch_disabled = prevTouchDisabled;
  }

  return result;
}

// ---------------------------------------------------------------------------
// Deep sleep helpers
// ---------------------------------------------------------------------------
bool canEnterDeepSleep()
{
  switch (OSSM_State) {
    case state_OFF:
    case state_PAUSE:
    case state_STOP:
    case state_MENU:
      return true;
    default:
      return false;
  }
}

static bool areWakeButtonsReleased()
{
  // Core2 mapping in this project: all three hardware buttons are active-high.
  // Released state is LOW.
  return (digitalRead(Button1.pin()) == LOW) &&
         (digitalRead(Button2.pin()) == LOW) &&
         (digitalRead(Button3.pin()) == LOW);
}

static bool waitWakeButtonsReleasedStable(uint32_t stableMs, uint32_t timeoutMs)
{
  const uint32_t startMs = millis();
  uint32_t releasedSinceMs = 0;

  while ((millis() - startMs) < timeoutMs) {
    const bool released = areWakeButtonsReleased();
    if (released) {
      if (releasedSinceMs == 0) {
        releasedSinceMs = millis();
      }
      if ((millis() - releasedSinceMs) >= stableMs) {
        return true;
      }
    } else {
      releasedSinceMs = 0;
    }
    delay(5);
  }

  return false;
}

void enterDeepSleep()
{
  gpio_num_t mxPin   = static_cast<gpio_num_t>(Button1.pin());
  gpio_num_t leftPin = static_cast<gpio_num_t>(Button2.pin());
  gpio_num_t rightPin = static_cast<gpio_num_t>(Button3.pin());
  uint64_t wakeMask = (1ULL << mxPin) | (1ULL << leftPin) | (1ULL << rightPin);

  LogDebug("Entering deep sleep (wake on MX/left/right)");
  M5.Display.setBrightness(0);
  M5.Power.setVibration(0);

  // Guard against instant wake / fake sleep when any wake button is still held.
  // If buttons do not settle to released state quickly, skip this sleep attempt.
  if (!waitWakeButtonsReleasedStable(120, 1200)) {
    LogDebugFormatted("Deep sleep canceled: wake button(s) still active (mx=%d left=%d right=%d)",
                      digitalRead(Button1.pin()),
                      digitalRead(Button2.pin()),
                      digitalRead(Button3.pin()));
    screensaver_check_activity();
    return;
  }

  esp_sleep_enable_ext1_wakeup(wakeMask, ESP_EXT1_WAKEUP_ANY_HIGH);
  delay(50);
  esp_deep_sleep_start();
}

// ---------------------------------------------------------------------------
// SendCommand � sends speed/depth/stroke commands to OSSM via BLE or ESP-NOW
// ---------------------------------------------------------------------------
bool SendCommand(int Command, float Value, int Target)
{
  if (Command == OFF) {
    OSSM_State = state_FALSE;
    refreshHomeAndStreamingStartStopUi();
    LogDebug("Local OSSM_State set to false (sent OFF)");
  } else if (Command == ON) {
    OSSM_State = state_TRUE;
    refreshHomeAndStreamingStartStopUi();
    LogDebug("Local OSSM_State set to true (sent ON)");
  }

  if (Target == OSSM_ID && OssmBleIsMode()) {
    LogDebugFormatted("TX BLE cmd=%d val=%.2f target=%d\n", Command, Value, Target);
    String response;
    bool ok = OssmBleSendAppCommand(
      Command, Value,
      speed, depth, stroke,
      isRunningUiState(OSSM_State),
      maxdepthinmm, speedlimit,
      &response);
    LogDebugFormatted("TX BLE result=%s cmd=%d target=%d\n", ok ? "OK" : "FAIL", Command, Target);
    if (response.length() > 0) {
      LogDebug("BLE response:");
      LogDebug(response);
    }
    return ok;
  } else  if (Target == OSSM_ID && !OssmBleIsMode()) {

    if (Ossm_paired == true) {
      LogDebugFormatted("TEMP TX ESP cmd=%d val=%.2f target=%d (paired=%d)\n", Command, Value, Target, Ossm_paired ? 1 : 0);
      bool ok = EspNowSendControlCommand(Command, Value, Target);
      LogDebugFormatted("TEMP TX ESP result=%s cmd=%d target=%d\n", ok ? "OK" : "FAIL", Command, Target);
      return ok;
    } else {
      LogDebugFormatted("ESP-NOW command not sent because not paired (cmd=%d val=%.2f target=%d)\n", Command, Value, Target);
      EspNowSendControlCommand(Command, Value, Target);  // Still update outgoing command for logging/monitoring even if not paired yet.
    }
  }

  return false;
}

// ---------------------------------------------------------------------------
// connectbutton initiates BLE / ESP-NOW pairing from the start screen
// IMPROVED FLOW: ESP-NOW is tried first (faster), then BLE immediately in parallel
// ---------------------------------------------------------------------------
extern "C" void connectbutton(lv_event_t *e)
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
  EspNowWaitForPairingOrTimeout(espNowFirstWindowMs, HEARTBEAT_INTERVAL_MS);

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

extern "C" void requestMenuEntryAction(void)
{
  if (!(OssmBleIsMode() && OssmBleConnected())) {
    g_ble_menu_requires_stroke_reentry = false;
    return;
  }

  // If we just came from Home (stroke engine), keep current mode so Home->Menu->Home
  // does not force an unnecessary re-home sequence.
  if (st_screens == ST_UI_HOME || st_screens == ST_UI_STROKE) {
    g_ble_menu_requires_stroke_reentry = false;
    return;
  }

  // Only force OSSM into Menu (which can trigger homing) if the
  // `bleForceHomeing` setting is enabled. When disabled we skip the
  // explicit `go:menu` on UI Menu load to avoid unnecessary homing.
  if (bleForceHomeing) {
    OssmBleGoToMenu();
    g_ble_menu_requires_stroke_reentry = true;
  } else {
    // Don't require stroke re-entry since we didn't request mode change.
    g_ble_menu_requires_stroke_reentry = false;
  }
}

extern "C" void menuPrepareNonHomeAction(void)
{
  if (!(OssmBleIsMode() && OssmBleConnected())) {
    return;
  }

  // Any non-home path from Menu should arm a full Menu/Homing path before
  // returning to Home again. Only force OSSM into Menu (which can trigger
  // homing) if the `bleForceHomeing` setting is enabled.
  if (bleForceHomeing) {
    OssmBleGoToMenu();
    g_ble_menu_requires_stroke_reentry = true;
  } else {
    g_ble_menu_requires_stroke_reentry = false;
  }
}

extern "C" void menuSleepAction(void)
{
  const int result = showNotification(
    "Enter Deep-Sleep",
    "Are you sure you want to enter deep-sleep mode? This will stop all connections.",
    0,
    true,
    "Yes",
    true,
    "No",
    false);

  if (result == NOTIFICATION_RESULT_LEFT) {
    enterDeepSleep();
  }
}

extern "C" void menuRestartAction(void)
{
  const int result = showNotification(
    "Restart",
    "Are you sure you want to perform a restart?",
    0,
    true,
    "Yes",
    true,
    "No",
    false);

  if (result == NOTIFICATION_RESULT_LEFT) {
    esp_restart();
  }
}
