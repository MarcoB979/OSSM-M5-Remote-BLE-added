#include <Arduino.h>
#include <M5Unified.h>
#include <Preferences.h>

#include "main.h"
#include "OssmBLE.h"
#include "language.h"
#include "ui/ui.h"

static constexpr int OSSM_TARGET_ID = 1;
static constexpr int FILTER_MX = 1;

#ifdef ARDUINO_M5STACK_CORES3
OneButton Button1(10, false); // MX Button
OneButton Button2(8, false);  // Encoder Left
OneButton Button3(14, false, true); // Encoder Right
#else
OneButton Button1(35, false); // MX Button (idle LOW, pressed HIGH)
OneButton Button2(36, false); // Encoder Left
OneButton Button3(34, false, true); // Encoder Right
#endif

ESP32Encoder encoder1;
ESP32Encoder encoder2;
ESP32Encoder encoder3;
ESP32Encoder encoder4;

bool mxclick_short_waspressed  = false;
bool mxclick_long_waspressed   = false;
bool mxclick_double_waspressed = false;

bool clickLeft_short_waspressed   = false;
bool clickLeft_double_waspressed  = false;
bool clickRight_short_waspressed  = false;
bool clickRight_long_waspressed   = false;
bool clickRight_double_waspressed = false;

static void applyHomeStartStopUi()
{
  if (ui_HomeButtonM != nullptr && ui_HomeButtonMText != nullptr) {
    if (isRunningUiState(OSSM_State)) {
      lv_obj_set_style_bg_color(ui_HomeButtonM, lv_color_hex(0xB3261E), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_label_set_text(ui_HomeButtonMText, OssmBleIsMode() ? T_PAUSE : T_STOP);
    } else {
      lv_obj_set_style_bg_color(ui_HomeButtonM, lv_color_hex(0x228B22), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_label_set_text(ui_HomeButtonMText, T_START);
    }
  }
}

static void applyStreamingStartStopUi()
{
  if (ui_StreamingButtonM != nullptr && ui_StreamingButtonMText != nullptr) {
    if (isRunningUiState(OSSM_State)) {
      lv_obj_set_style_bg_color(ui_StreamingButtonM, lv_color_hex(0xB3261E), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_label_set_text(ui_StreamingButtonMText, OssmBleIsMode() ? T_PAUSE : T_STOP);
    } else {
      lv_obj_set_style_bg_color(ui_StreamingButtonM, lv_color_hex(0x228B22), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_label_set_text(ui_StreamingButtonMText, T_START);
    }
  }
}

void refreshHomeAndStreamingStartStopUi()
{
  applyHomeStartStopUi();
  applyStreamingStartStopUi();
}

static void syncStrokeEngineParametersBeforeStart()
{
  if (ui_PatternS != nullptr) {
    // Home UI is the source of truth for first start after reconnect/reboot.
    pattern = lv_roller_get_selected(ui_PatternS);
  }

  // Keep OSSM state in sync with UI before issuing ON/Resume.
  // This avoids resuming with stale parameters after reconnect/reboot.
  SendCommand(SPEED, speed, OSSM_TARGET_ID);
  SendCommand(DEPTH, depth, OSSM_TARGET_ID);
  SendCommand(STROKE, stroke, OSSM_TARGET_ID);
  SendCommand(SENSATION, sensation, OSSM_TARGET_ID);
  SendCommand(PATTERN, pattern, OSSM_TARGET_ID);
}

void homebuttonm_action(bool fromPhysicalMx)
{
  (void)fromPhysicalMx;
  const bool isRunning = isRunningUiState(OSSM_State);

  if (OssmBleIsMode() && st_screens == ST_UI_HOME) {
    if (!isRunning) {
      syncStrokeEngineParametersBeforeStart();
    }
    ossm_state_monitor_hold_until_ms = millis() + 1U;
    switch (OssmBleHandleHomeToggle(isRunning, speed)) {
      case OssmBleHomeToggleResult::Paused:
        OSSM_State = state_FALSE;
        applyHomeStartStopUi();
        LogDebugFormatted("Sent BLE pause command with paused speed: %f\n", speed);
        break;
      case OssmBleHomeToggleResult::Resumed:
        OSSM_State = state_TRUE;
        applyHomeStartStopUi();
        LogDebugFormatted("Sent BLE resume command (speed:%f)\n", OssmBleGetUnpauseSpeed());
        break;
      case OssmBleHomeToggleResult::Started:
        OSSM_State = state_TRUE;
        applyHomeStartStopUi();
        LogDebugFormatted("Sent BLE start command (speed:%f)\n", OssmBleGetUnpauseSpeed());
        break;
      case OssmBleHomeToggleResult::BlockedNotReady:
        LogDebugPrio("BLE start blocked: OSSM is not ready for stroke engine");
        break;
      case OssmBleHomeToggleResult::Failed:
      default:
        LogDebugPrio("BLE home toggle failed");
        break;
    }
    return;
  }

  if (OSSM_State == state_FALSE) {
    if (OssmBleIsMode()) {
      OssmBleGoToStrokeEngine();
    }
    syncStrokeEngineParametersBeforeStart();
    SendCommand(ON, 0, OSSM_TARGET_ID);
    if (OssmBleIsMode()) {
      OSSM_State = state_TRUE;
      applyHomeStartStopUi();
    }
  } else if (isRunning) {
    SendCommand(OFF, 0, OSSM_TARGET_ID);
    if (OssmBleIsMode()) {
      OSSM_State = state_FALSE;
      applyHomeStartStopUi();
    }
  }
}

void streamingbuttonm_action(bool fromPhysicalMx)
{
  (void)fromPhysicalMx;
  const bool isRunning = isRunningUiState(OSSM_State);

  if (OssmBleIsMode() && st_screens == ST_UI_STREAMING) {
    ossm_state_monitor_hold_until_ms = millis() + 1U;
    switch (OssmBleHandleStreamingToggle(isRunning, speed)) {
      case OssmBleHomeToggleResult::Paused:
        OSSM_State = state_FALSE;
        applyStreamingStartStopUi();
        LogDebugFormatted("Sent BLE streaming pause command with paused speed: %f\n", speed);
        break;
      case OssmBleHomeToggleResult::Resumed:
        OSSM_State = state_TRUE;
        applyStreamingStartStopUi();
        LogDebugFormatted("Sent BLE streaming resume command (speed:%f)\n", OssmBleGetUnpauseSpeed());
        LogDebugPrio("Streaming armed: waiting for external stream frames");
        break;
      case OssmBleHomeToggleResult::Started:
        OSSM_State = state_TRUE;
        applyStreamingStartStopUi();
        LogDebugFormatted("Sent BLE streaming start command (speed:%f)\n", OssmBleGetUnpauseSpeed());
        LogDebugPrio("Streaming armed: waiting for external stream frames");
        break;
      case OssmBleHomeToggleResult::BlockedNotReady:
        LogDebugPrio("BLE streaming start blocked: OSSM is not ready for streaming mode");
        break;
      case OssmBleHomeToggleResult::Failed:
      default:
        LogDebugPrio("BLE streaming toggle failed");
        break;
    }
    return;
  }

  if (OSSM_State == state_FALSE) {
    SendCommand(ON, 0, OSSM_TARGET_ID);
    if (OssmBleIsMode()) {
      OSSM_State = state_TRUE;
      applyStreamingStartStopUi();
    }
  } else if (isRunning) {
    SendCommand(OFF, 0, OSSM_TARGET_ID);
    if (OssmBleIsMode()) {
      OSSM_State = state_FALSE;
      applyStreamingStartStopUi();
    }
  }
}

void streamingStartOnlyAction()
{
  const bool isRunning = isRunningUiState(OSSM_State);

  if (OssmBleIsMode() && st_screens == ST_UI_STREAMING) {
    if (isRunning) {
      LogDebugPrio("BLE streaming start ignored: already running");
      return;
    }

    ossm_state_monitor_hold_until_ms = millis() + 1U;
    switch (OssmBleHandleStreamingToggle(false, speed)) {
      case OssmBleHomeToggleResult::Resumed:
        OSSM_State = state_TRUE;
        applyStreamingStartStopUi();
        LogDebugFormatted("Sent BLE streaming resume command (speed:%f)\n", OssmBleGetUnpauseSpeed());
        LogDebugPrio("Streaming armed: waiting for external stream frames");
        break;
      case OssmBleHomeToggleResult::Started:
        OSSM_State = state_TRUE;
        applyStreamingStartStopUi();
        LogDebugFormatted("Sent BLE streaming start command (speed:%f)\n", OssmBleGetUnpauseSpeed());
        LogDebugPrio("Streaming armed: waiting for external stream frames");
        break;
      case OssmBleHomeToggleResult::Paused:
        LogDebugPrio("BLE streaming start-only action unexpectedly returned pause");
        break;
      case OssmBleHomeToggleResult::BlockedNotReady:
        LogDebugPrio("BLE streaming start blocked: OSSM is not ready for streaming mode");
        break;
      case OssmBleHomeToggleResult::Failed:
      default:
        LogDebugPrio("BLE streaming start-only action failed");
        break;
    }
    return;
  }

  if (!isRunning) {
    SendCommand(ON, 0, OSSM_TARGET_ID);
    if (OssmBleIsMode()) {
      OSSM_State = state_TRUE;
      applyStreamingStartStopUi();
    }
  }
}

void homebuttonmevent(lv_event_t * e)
{
  (void)e;
  LogDebugPrio("HomeButton (touch/LVGL)");
  homebuttonm_action(false);
}

void streamingbuttonmevent(lv_event_t * e)
{
  (void)e;
  LogDebugPrio("StreamingButton (touch/LVGL)");
  streamingbuttonm_action(false);
}

void setupDepthInter(lv_event_t * e)
{
  (void)e;
  SendCommand(SETUP_D_I, 0, OSSM_TARGET_ID);
}

void setupdepthF(lv_event_t * e)
{
  (void)e;
  SendCommand(SETUP_D_I_F, 0, OSSM_TARGET_ID);
}

// ---------------------------------------------------------------------------
// Vibration helper
// ---------------------------------------------------------------------------

void vibrate(int vbr_Intensity, int vbr_Length)
{
  if (lv_obj_has_state(ui_vibrate, LV_STATE_CHECKED) == 1) {
    M5.Power.setVibration(vbr_Intensity);
    vTaskDelay(pdMS_TO_TICKS(vbr_Length));
    M5.Power.setVibration(0);
  }
}

// ---------------------------------------------------------------------------
// Settings screen event callbacks
// ---------------------------------------------------------------------------

void brightness_slider_event_cb(lv_event_t *e)
{
  if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
    int val = lv_slider_get_value(ui_brightness_slider);
    M5.Display.setBrightness(val);
    Preferences pref;
    pref.begin("m5-ctnr", false);
    pref.putInt("Brightness", val);
    pref.end();
  }
}

void savesettings(lv_event_t * e)
{
  (void)e;
  bool theme_Change_Previous = false;
  bool theme_Change_New = false;

  Preferences pref;
  pref.begin("m5-ctnr", false);
  theme_Change_Previous = pref.getBool("Darkmode", true);

  if (lv_obj_has_state(ui_vibrate, LV_STATE_CHECKED) == 1) {
    pref.putBool("Vibrate", true);
  } else if (lv_obj_has_state(ui_vibrate, LV_STATE_CHECKED) == 0) {
    pref.putBool("Vibrate", false);
  }

  if (lv_obj_has_state(ui_lefty, LV_STATE_CHECKED) == 1) {
    pref.putBool("Lefty", true);
  } else if (lv_obj_has_state(ui_lefty, LV_STATE_CHECKED) == 0) {
    pref.putBool("Lefty", false);
  }

  LogDebug("Saving StrokeInvert setting...");
  if (lv_obj_has_state(ui_strokeinvert, LV_STATE_CHECKED) == 1) {
    pref.putBool("StrokeInvert", true);
    strokeinvert_mode = true;
  } else if (lv_obj_has_state(ui_strokeinvert, LV_STATE_CHECKED) == 0) {
    pref.putBool("StrokeInvert", false);
    strokeinvert_mode = false;
  }

  if (lv_obj_has_state(ui_darkmode, LV_STATE_CHECKED) == 1) {
    theme_Change_New = true;
    pref.putBool("Darkmode", true);
  } else if (lv_obj_has_state(ui_darkmode, LV_STATE_CHECKED) == 0) {
    theme_Change_New = false;
    pref.putBool("Darkmode", false);
  }

  pref.putInt("Brightness", lv_slider_get_value(ui_brightness_slider));
  pref.end();

  if (theme_Change_Previous != theme_Change_New) {
    vibrate(225, 75);
    ESP.restart();
  } else {
    vibrate(225, 75);
  }

  pref.begin("m5-ctnr", true);
  LogDebug("Settings are saved, these are the values:");
  LogDebugFormatted("Vibrate: %s",      pref.getBool("Vibrate", true) ? "true" : "false");
  LogDebugFormatted("Lefty/Touch: %s",  pref.getBool("Lefty", true) ? "true" : "false");
  LogDebugFormatted("StrokeInvert: %s", pref.getBool("StrokeInvert", false) ? "true" : "false");
  LogDebugFormatted("Darkmode: %s",     pref.getBool("Darkmode", false) ? "true" : "false");
  LogDebugFormatted("Brightness: %d",   pref.getInt("Brightness", 180));
  pref.end();
}

// ---------------------------------------------------------------------------
// Home button left event and pattern save
// ---------------------------------------------------------------------------

void homebuttonLevent(lv_event_t * e)
{
  (void)e;
  lv_obj_clear_state(ui_HomeButtonL, LV_STATE_CHECKED);

  depth  = 0;
  stroke = 0;
  if (ui_homedepthslider  != nullptr) lv_slider_set_value(ui_homedepthslider,  0, LV_ANIM_OFF);
  if (ui_homestrokeslider != nullptr) lv_slider_set_value(ui_homestrokeslider, 0, LV_ANIM_OFF);

  if (OssmBleIsMode()) {
    OssmBleExecutePulloutStop(speed, maxdepthinmm, speedlimit);
    OssmBleGoToMenu();
  } else {
    SendCommand(SETUP_D_I, 0.0, OSSM_ID);
    SendCommand(DEPTH,     0.0, OSSM_ID);
    SendCommand(STROKE,    0.0, OSSM_ID);
    vTaskDelay(pdMS_TO_TICKS(300));
    SendCommand(SPEED,     0.0, OSSM_ID);
  }

  OSSM_State = state_FALSE;
  refreshHomeAndStreamingStartStopUi();
  lv_scr_load_anim(ui_Menu, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0, false);
}

void savepattern(lv_event_t * e)
{
  (void)e;
  extern char patternstr[20];
  pattern = lv_roller_get_selected(ui_PatternS);
  lv_roller_get_selected_str(ui_PatternS, patternstr, 0);
  lv_label_set_text(ui_HomePatternLabel, patternstr);
  LogDebug(pattern);
  float patterns = pattern;
  SendCommand(PATTERN, patterns, OSSM_ID);
}

// ---------------------------------------------------------------------------
// MX encoder anti-ghost filter
// ---------------------------------------------------------------------------

static unsigned long mx_last_home_action_ms = 0;
static constexpr unsigned long MX_HOME_MIN_GAP_MS = 220;
static unsigned long mx_suppress_until_ms = 0;
static constexpr unsigned long MX_SUPPRESS_AFTER_ENCODER_MS = 300;
static unsigned long mx_release_since_ms = 0;

void updateMxReleaseStability()
{
  // Core2 mapping: idle LOW, pressed HIGH for MX.
  bool mxReleased = (digitalRead(Button1.pin()) == LOW);
  if (mxReleased) {
    if (mx_release_since_ms == 0) {
      mx_release_since_ms = millis();
    }
  } else {
    mx_release_since_ms = 0;
  }
}

void markEncoderActivityForMxFilter()
{
  if (FILTER_MX == 1) {
    mx_suppress_until_ms = millis() + MX_SUPPRESS_AFTER_ENCODER_MS;
  }
}

static bool filterMxPhysicalHomeClick()
{
  if (FILTER_MX != 1) {
    return true;
  }

  unsigned long now = millis();
  unsigned long dt  = now - mx_last_home_action_ms;

  if ((long)(now - mx_suppress_until_ms) < 0) {
    LogDebugPrioFormatted("mx: ST_UI_HOME -> filtered after encoder move (%ld ms left)\n",
                          (long)(mx_suppress_until_ms - now));
    return false;
  }

  if (dt < MX_HOME_MIN_GAP_MS) {
    LogDebugPrioFormatted("mx: ST_UI_HOME -> filtered phantom click (gap=%lu ms)\n", dt);
    return false;
  }

  mx_last_home_action_ms = now;
  return true;
}

// ---------------------------------------------------------------------------
// Physical button click callbacks (registered in setup via OneButton)
// ---------------------------------------------------------------------------

void mxclick()
{
  vibrate();
  mxclick_short_waspressed = true;
  screensaver_check_activity();
}

void mxdouble()
{
  vibrate();
  mxclick_double_waspressed = true;
  screensaver_check_activity();
}

void mxlong()
{
  vibrate();
  mxclick_long_waspressed = true;
  screensaver_check_activity();
}

void clickLeft()
{
  vibrate();
  clickLeft_short_waspressed = true;
  screensaver_check_activity();
}

void clickLeftDouble()
{
  vibrate();
  clickLeft_double_waspressed = true;
  screensaver_check_activity();
}

void clickRight()
{
  vibrate();
  clickRight_short_waspressed = true;
  screensaver_check_activity();
  LogDebug("clickRight_short_waspressed set to true");
}

void clickRightLong()
{
  vibrate();
  clickRight_long_waspressed = true;
  screensaver_check_activity();
}

void clickRightDouble()
{
  vibrate();
  clickRight_double_waspressed = true;
  screensaver_check_activity();
}

// ---------------------------------------------------------------------------
// Clear all button press flags (called from handleCurrentScreen after dispatch)
// ---------------------------------------------------------------------------

void clearButtonFlags()
{
  mxclick_short_waspressed     = false;
  clickLeft_short_waspressed   = false;
  clickLeft_double_waspressed  = false;
  clickRight_short_waspressed  = false;
  clickRight_long_waspressed   = false;
  clickRight_double_waspressed = false;
}

// ---------------------------------------------------------------------------
// Register debug event callbacks on all screen buttons
// ---------------------------------------------------------------------------
void register_event_debug_callbacks()
{
  lv_obj_t *debugButtons[] = {
    ui_StartButtonL,   ui_StartButtonM,   ui_StartButtonR,
    ui_HomeButtonL,    ui_HomeButtonM,    ui_HomeButtonR,
    ui_PatternButtonL, ui_PatternButtonM, ui_PatternButtonR,
    ui_TorqeButtonL,   ui_TorqeButtonM,   ui_TorqeButtonR,
    ui_EJECTButtonL,   ui_EJECTButtonM,   ui_EJECTButtonR,
    ui_SettingsButtonL,ui_SettingsButtonM,ui_SettingsButtonR,
  };

  for (lv_obj_t *obj : debugButtons) {
    if (obj != nullptr) {
      lv_obj_add_event_cb(obj, event_cb, LV_EVENT_ALL, nullptr);
    }
  }
}