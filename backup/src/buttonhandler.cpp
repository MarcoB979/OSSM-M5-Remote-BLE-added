#include <Arduino.h>
#include <M5Unified.h>
#include <Preferences.h>

#include "config.h"
#include "main.h"
#include "OssmBLE.h"
#include "Eject.h"
#include "language.h"
#include "ui/ui.h"

static constexpr int OSSM_TARGET_ID = 1;
static constexpr int HOME_LEFT_ADDON_SLOT = 1;
static constexpr int HOME_RIGHT_ADDON_SLOT = 2;

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

// Button lifecycle (single model used across the project):
// 1) OneButton callbacks set these raw flags.
// 2) pollButtonEvents() snapshots and consumes them once per loop tick.
// 3) Screen/modal handlers act on the snapshot and optionally clear leftovers.
bool mxclick_short_waspressed  = false;
bool mxclick_long_waspressed   = false;
bool mxclick_double_waspressed = false;

bool clickLeft_short_waspressed   = false;
bool clickLeft_long_waspressed    = false;
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

    // ejectAddon NVS key removed — addon slot assignments are persisted by addons.cpp

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

  // Home short-left now only navigates to Menu.
  lv_scr_load_anim(ui_Menu, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0, false);
}

static void homePulloutToMenuAction()
{
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

void homebuttonMlongEvent(lv_event_t * e)
{
  (void)e;
  homePulloutToMenuAction();
}

void homebuttonLlongEvent(lv_event_t * e)
{
  (void)e;
  triggerAddonForSlot(HOME_LEFT_ADDON_SLOT);
}

void homebuttonRlongEvent(lv_event_t * e)
{
  (void)e;
  triggerAddonForSlot(HOME_RIGHT_ADDON_SLOT);
}

void savepattern(lv_event_t * e)
{
  //(void)e;
  extern char patternstr[20];
  if (ui_PatternS != nullptr) {
    // Home UI is the source of truth for first start after reconnect/reboot.
    pattern = lv_roller_get_selected(ui_PatternS);
  }
//  pattern = lv_roller_get_selected(ui_PatternS);
  lv_roller_get_selected_str(ui_PatternS, patternstr, 0);
  lv_label_set_text(ui_HomePatternLabel, patternstr);
  LogDebug(pattern);
  float patterns = pattern;
  LogDebugFormatted("Saving pattern: %d\n", pattern);
  SendCommand(PATTERN, pattern, OSSM_TARGET_ID);
  LogDebugFormatted("Saving patternS: %d\n", patterns);
  SendCommand(PATTERN, patterns, OSSM_ID);
}

static bool consumeButtonFlag(bool &flag)
{
  if (!flag) return false;
  flag = false;
  return true;
}

// ---------------------------------------------------------------------------
// Physical button click callbacks (registered in setup via OneButton)
// ---------------------------------------------------------------------------

void mxclick()
{
  static uint32_t lastMxClickMs = 0;
  const uint32_t nowMs = millis();
  if ((nowMs - lastMxClickMs) < 350U) {
    LogDebug("mxclick ignored by cooldown");
    return;
  }
  lastMxClickMs = nowMs;

  vibrate();
  mxclick_short_waspressed = true;
  LogDebug("mxclick_short_waspressed set to true");
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

void clickLeftLong()
{
  vibrate();
  clickLeft_long_waspressed = true;
  screensaver_check_activity();
}

void clickLeftDouble()
{
  vibrate();
  clickLeft_double_waspressed = true;
  LogDebug("clickLeft_double_waspressed set to true");
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
  LogDebug("clickRight_double_waspressed set to true");
  screensaver_check_activity();
}

void pollButtonEvents(ButtonEvents &events)
{
  events.mxShort     = consumeButtonFlag(mxclick_short_waspressed);
  events.mxLong      = consumeButtonFlag(mxclick_long_waspressed);
  events.mxDouble    = consumeButtonFlag(mxclick_double_waspressed);
  events.leftShort   = consumeButtonFlag(clickLeft_short_waspressed);
  events.leftLong    = consumeButtonFlag(clickLeft_long_waspressed);
  events.leftDouble  = consumeButtonFlag(clickLeft_double_waspressed);
  events.rightShort  = consumeButtonFlag(clickRight_short_waspressed);
  events.rightLong   = consumeButtonFlag(clickRight_long_waspressed);
  events.rightDouble = consumeButtonFlag(clickRight_double_waspressed);
}

// ---------------------------------------------------------------------------
// Clear all button press flags (called from handleCurrentScreen after dispatch)
// ---------------------------------------------------------------------------

void clearButtonFlags()
{
  mxclick_short_waspressed     = false;
  mxclick_long_waspressed      = false;
  mxclick_double_waspressed    = false;
  clickLeft_short_waspressed   = false;
  clickLeft_long_waspressed    = false;
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
    ui_EJECTButtonL,   ui_EJECTButtonM,   ui_EJECTButtonR,
    ui_SettingsButtonL,ui_SettingsButtonM,ui_SettingsButtonR,
  };

  for (lv_obj_t *obj : debugButtons) {
    if (obj != nullptr) {
      lv_obj_add_event_cb(obj, event_cb, LV_EVENT_ALL, nullptr);
    }
  }
}