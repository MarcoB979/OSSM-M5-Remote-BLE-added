#include <M5Unified.h>
#include <Arduino.h>
#include <esp_sleep.h>
#include <esp_timer.h>

#include "main.h"
#include "config.h"
#include "OssmBLE.h"
#include "language.h"
#include "ui/ui.h"
#include "colors.h"

int st_screens = ST_UI_START;

bool dark_mode = false;
bool strokeinvert_mode = false;
bool touch_disabled = false;

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

bool Ossm_paired = false;
bool waiting_for_limits = false; // true after pairing until OSSM reports max depth/speed

int OSSM_State = state_FALSE;

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
          case OssmBleMachineMode::HomingBackward:
            nextState = state_HOMING;
            break;
          case OssmBleMachineMode::Menu:
            nextState = state_MENU;
            break;
          case OssmBleMachineMode::Streaming:
            nextState = state_STREAMING;
            break;
          case OssmBleMachineMode::SimplePenetration:
            nextState = state_SIMPLE_PENETRATION;
            break;
          case OssmBleMachineMode::StrokeEngineActive:
            nextState = (bleState.speed > 0) ? state_ON : state_PAUSE;
            break;
          case OssmBleMachineMode::StrokeEngineIdle:
            nextState = (bleState.speed > 0) ? state_ON : state_PAUSE;
            break;
          default:
            break;
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

  // Get active color scheme colors (defined in colors.cpp)
  extern uint32_t getActivePrimaryColor(void);
  extern uint32_t getActiveSecondaryColor(void);
  uint32_t schemePrimary = getActivePrimaryColor();
  uint32_t schemeSecondary = getActiveSecondaryColor();
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
  mxclick_short_waspressed = false;
  mxclick_long_waspressed = false;
  mxclick_double_waspressed = false;
  clickLeft_short_waspressed = false;
  clickRight_short_waspressed = false;
  clickRight_long_waspressed = false;
  clickRight_double_waspressed = false;

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
  lv_obj_set_style_text_color(titleLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(titleLabel, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_center(titleLabel);

  lv_obj_t *bodyLabel = lv_label_create(panel);
  lv_obj_set_width(bodyLabel, lv_pct(90));
  lv_label_set_long_mode(bodyLabel, LV_LABEL_LONG_WRAP);
  lv_label_set_text(bodyLabel, (text != nullptr) ? text : "");
  lv_obj_set_style_text_color(bodyLabel, lv_color_hex(0x2E0A2B), LV_PART_MAIN | LV_STATE_DEFAULT);
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
      lv_obj_set_style_text_color(leftLbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_center(leftLbl);
      lv_obj_add_event_cb(leftBtn, notification_left_button_cb, LV_EVENT_CLICKED, nullptr);
    }

    if (showRightButton) {
      lv_obj_t *rightBtn = lv_btn_create(buttonRow);
      lv_obj_set_size(rightBtn, 120, 36);
      lv_obj_set_style_bg_color(rightBtn, lv_color_hex(schemePrimary), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_bg_color(rightBtn, lv_color_hex(schemeDarker), LV_PART_MAIN | LV_STATE_PRESSED);
      lv_obj_set_style_border_width(rightBtn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

      lv_obj_t *rightLbl = lv_label_create(rightBtn);
      lv_label_set_text(rightLbl, (rightButtonText != nullptr && rightButtonText[0] != '\0') ? rightButtonText : "Right");
      lv_obj_set_style_text_color(rightLbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_center(rightLbl);
      lv_obj_add_event_cb(rightBtn, notification_right_button_cb, LV_EVENT_CLICKED, nullptr);
    }
  }

  static uint32_t notif_last_state_log_ms = 0;

  while (true) {
    M5.update();
    lv_task_handler();
    updateMxReleaseStability();
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

    if (hasButtons) {
      if (g_notification_touch_result != NOTIFICATION_RESULT_NONE) {
        result = g_notification_touch_result;
        break;
      }
      if (showLeftButton && clickLeft_short_waspressed) {
        result = NOTIFICATION_RESULT_LEFT;
        break;
      }
      if (showRightButton && clickRight_short_waspressed) {
        result = NOTIFICATION_RESULT_RIGHT;
        break;
      }
    }

    // Consume all activity so the current screen never receives latent button events.
    mxclick_short_waspressed = false;
    mxclick_long_waspressed = false;
    mxclick_double_waspressed = false;
    clickLeft_short_waspressed = false;
    clickRight_short_waspressed = false;
    clickRight_long_waspressed = false;
    clickRight_double_waspressed = false;

    vTaskDelay(pdMS_TO_TICKS(5));
  }

  lv_obj_del(overlay);

  mxclick_short_waspressed = false;
  mxclick_long_waspressed = false;
  mxclick_double_waspressed = false;
  clickLeft_short_waspressed = false;
  clickRight_short_waspressed = false;
  clickRight_long_waspressed = false;
  clickRight_double_waspressed = false;

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

void enterDeepSleep()
{
  gpio_num_t mxPin   = static_cast<gpio_num_t>(Button1.pin());
  gpio_num_t leftPin = static_cast<gpio_num_t>(Button2.pin());
  gpio_num_t rightPin = static_cast<gpio_num_t>(Button3.pin());
  uint64_t wakeMask = (1ULL << mxPin) | (1ULL << leftPin) | (1ULL << rightPin);

  LogDebug("Entering deep sleep (wake on MX/left/right)");
  M5.Display.setBrightness(0);
  M5.Power.setVibration(0);

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
    String response;
    bool ok = OssmBleSendAppCommand(
      Command, Value,
      speed, depth, stroke,
      isRunningUiState(OSSM_State),
      maxdepthinmm, speedlimit,
      &response);
    if (response.length() > 0) {
      LogDebug("BLE response:");
      LogDebug(response);
    }
    return ok;
  }

  if (Ossm_paired == true) {
    return EspNowSendControlCommand(Command, Value, Target);
  }

  return false;
}

// ---------------------------------------------------------------------------
// connectbutton initiates BLE / ESP-NOW pairing from the start screen
// IMPROVED FLOW: ESP-NOW is tried first (faster), then BLE immediately in parallel
// ---------------------------------------------------------------------------
extern "C" void connectbutton(lv_event_t *e)
{
  static constexpr uint32_t ESP_NOW_FIRST_WINDOW_MS = 2000UL;   // ESP_NOW is fast, give it short window
  static constexpr uint32_t BLEAND_ESPNOW_ATTEMPTS  = 8;        // Parallel attempts
  static constexpr uint32_t LONG_WAIT_WINDOW_MS     = 60000UL;
  static constexpr uint32_t HEARTBEAT_INTERVAL_MS   = 500UL;

  lv_label_set_text(ui_Welcome, T_CONNECTING);

  if (!Ossm_paired) {
    OssmBleSetMode(false);

    // Phase 1: Try ESP_NOW first (it connects much faster than BLE)
    lv_label_set_text(ui_Welcome, "Searching...");
    EspNowWaitForPairingOrTimeout(ESP_NOW_FIRST_WINDOW_MS, HEARTBEAT_INTERVAL_MS);
    
    if (!Ossm_paired) {
      // Phase 2: ESP_NOW didn't connect, now try BLE + ESP_NOW in parallel
      for (uint32_t attempt = 0; attempt < BLEAND_ESPNOW_ATTEMPTS && !Ossm_paired; ++attempt) {
        // Send ESP_NOW pairing heartbeat
        EspNowSendPairingHeartbeat();

        // Try BLE connection
        bool bleConnected = OssmBleTryConnect();
        if (bleConnected) {
          lv_label_set_text(ui_Welcome, T_BLECONNECTED);
          OssmBleSetMode(true);
          Ossm_paired = true;
          syncBleConnectUi(true);
          break;
        }

        // Brief wait to allow ESP_NOW callback to set Ossm_paired if it receives pairing message
        if (!Ossm_paired) {
          vTaskDelay(pdMS_TO_TICKS(200));
        }
      }
    } else {
      // ESP_NOW connected in Phase 1
      lv_label_set_text(ui_Welcome, T_ESPCONNECTED);
    }

    // Phase 3: If still not paired, wait longer
    if (!Ossm_paired) {
      lv_label_set_text(ui_Welcome, T_LONG_WAIT);
      EspNowWaitForPairingOrTimeout(LONG_WAIT_WINDOW_MS, HEARTBEAT_INTERVAL_MS);
      if (!Ossm_paired) {
        lv_label_set_text(ui_Welcome, T_FAILED);
      }
    }
    return;
  }

  if (OssmBleIsMode()) {
    syncBleConnectUi(true);
  }
}

extern "C" void requestMenuEntryAction(void)
{
  if (OssmBleIsMode() && OssmBleConnected()) {
    OssmBleGoToMenu();
  }
}

extern "C" void menuSleepAction(void)
{
  enterDeepSleep();
}

extern "C" void menuRestartAction(void)
{
  esp_restart();
}