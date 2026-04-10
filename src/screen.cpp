#include <Arduino.h>
#include <M5Unified.h>
#include "PatternMath.h"
#include <lvgl.h>

#include "main.h"
#include "config.h"
#include "screen.h"
#include "OssmBLE.h"
#include "ui/ui.h"
#include "ui/ui_helpers.h"
#include "language.h"

// Global UI/session state shared across modules.
int g_brightness_value = 180;
bool eject_status = false;
bool vibrate_mode = true;
bool touch_home = false;
char patternstr[20];

int screensaver_timeout_ms = SCREENSAVER_TIMEOUT_MS_DEFAULT;
int screensaver_dim_brightness = SCREENSAVER_DIM_BRIGHTNESS_DEFAULT;
unsigned long deep_sleep_timeout_ms = DEEP_SLEEP_TIMEOUT_MS_DEFAULT;

long speedenc = 0;
long torqe_f_enc = 0;
long torqe_r_enc = 0;
long encoder3_enc = 0;
long encoder4_enc = 0;

static bool rampEnabled = true;
static int  rampValue   = 2;
static int  rampTime    = 75;
static int  maxRamp     = 6;
static int  activeEncId = 0;
static unsigned long rampMs = 0;

static uint32_t ble_home_go_strokeengine_ms = 0;

static const char* battery_symbol_for_level(int level)
{
  if (level < 10) return LV_SYMBOL_BATTERY_EMPTY;
  if (level < 25) return LV_SYMBOL_BATTERY_1;
  if (level < 50) return LV_SYMBOL_BATTERY_2;
  if (level < 75) return LV_SYMBOL_BATTERY_3;
  if (level > 97) return LV_SYMBOL_BATTERY_FULL;
  return LV_SYMBOL_BATTERY_3;
}

void update_battery_icons_all_screens(int level)
{
  static bool batteryUiInitialized = false;

  lv_obj_t *batteryTitleLabels[] = {
    ui_Batt, ui_Batt1, ui_Batt2, ui_Batt3, ui_Batt4, ui_Batt5, ui_Batt6, ui_Batt7, ui_Batt8
  };

  lv_obj_t *batteryValueLabels[] = {
    ui_BattValue, ui_BattValue1, ui_BattValue2, ui_BattValue3, ui_BattValue4,
    ui_BattValue5, ui_BattValue6, ui_BattValue7, ui_BattValue8
  };

  lv_obj_t *batteryBars[] = {
    ui_Battery, ui_Battery1, ui_Battery2, ui_Battery3, ui_Battery4,
    ui_Battery5, ui_Battery6, ui_Battery7, ui_Battery8
  };

  if (!batteryUiInitialized) {
    for (lv_obj_t *label : batteryValueLabels) {
      if (label != nullptr) {
        lv_obj_clear_flag(label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_align(label, LV_ALIGN_RIGHT_MID);
        lv_obj_set_y(label, 0);
        lv_obj_set_x(label, -34);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
      }
    }
    for (lv_obj_t *bar : batteryBars) {
      if (bar != nullptr) {
        lv_obj_add_flag(bar, LV_OBJ_FLAG_HIDDEN);
      }
    }
    for (lv_obj_t *label : batteryTitleLabels) {
      if (label != nullptr) {
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
      }
    }
    batteryUiInitialized = true;
  }

  const char *symbol = battery_symbol_for_level(level);
  char percentText[8];
  snprintf(percentText, sizeof(percentText), "%d%%", level);

  for (lv_obj_t *label : batteryTitleLabels) {
    if (label != nullptr) {
      lv_label_set_text(label, symbol);
    }
  }
  for (lv_obj_t *label : batteryValueLabels) {
    if (label != nullptr) {
      lv_label_set_text(label, percentText);
    }
  }
}

void screen_power_tick()
{
  if (encoder1.getCount() + encoder2.getCount() + encoder3.getCount() + encoder4.getCount() != 0) {
    screensaver_check_activity();
  }

  if (!screensaver_active && (millis() - last_activity_ms > (unsigned long)screensaver_timeout_ms)) {
    screensaver_prev_brightness = g_brightness_value;
    M5.Lcd.setBrightness(screensaver_dim_brightness);
    screensaver_active = true;
  }

  if (millis() - last_activity_ms > deep_sleep_timeout_ms) {
    if (canEnterDeepSleep()) {
      enterDeepSleep();
    }
  }
}


void updateHomeTopLeftStateLabel()
{
  if (ui_connect == nullptr) {
    return;
  }

  char labelText[32];
  if (OssmBleIsMode()) {
    if (OssmBleConnected()) {
      switch (OSSM_State) {
        case state_OFF: snprintf(labelText, sizeof(labelText), "OFF"); break;
        case state_ON: snprintf(labelText, sizeof(labelText), "ON"); break;
        case state_PAUSE: snprintf(labelText, sizeof(labelText), "PAUSE"); break;
        case state_STOP: snprintf(labelText, sizeof(labelText), "STOP"); break;
        case state_MENU: snprintf(labelText, sizeof(labelText), "MENU"); break;
        case state_HOMING: snprintf(labelText, sizeof(labelText), "HOMING"); break;
        case state_STREAMING: snprintf(labelText, sizeof(labelText), "STREAM"); break;
        case state_SIMPLE_PENETRATION: snprintf(labelText, sizeof(labelText), "SIMPLE"); break;
        case state_STROKE_ENGINE: snprintf(labelText, sizeof(labelText), "STROKE"); break;
        case state_ERROR: snprintf(labelText, sizeof(labelText), "ERROR"); break;
        default: snprintf(labelText, sizeof(labelText), "UNKNOWN"); break;
      }
    } else {
      snprintf(labelText, sizeof(labelText), "%s", T_CONNECTING);
    }
  } else {
    switch (OSSM_State) {
      case state_OFF: snprintf(labelText, sizeof(labelText), "OFF"); break;
      case state_ON: snprintf(labelText, sizeof(labelText), "ON"); break;
      case state_PAUSE: snprintf(labelText, sizeof(labelText), "PAUSE"); break;
      case state_STOP: snprintf(labelText, sizeof(labelText), "STOP"); break;
      case state_MENU: snprintf(labelText, sizeof(labelText), "MENU"); break;
      case state_HOMING: snprintf(labelText, sizeof(labelText), "HOMING"); break;
      case state_STREAMING: snprintf(labelText, sizeof(labelText), "STREAM"); break;
      case state_SIMPLE_PENETRATION: snprintf(labelText, sizeof(labelText), "SIMPLE"); break;
      case state_STROKE_ENGINE: snprintf(labelText, sizeof(labelText), "STROKE"); break;
      case state_ERROR: snprintf(labelText, sizeof(labelText), "ERROR"); break;
      default: snprintf(labelText, sizeof(labelText), "UNKNOWN"); break;
    }
  }

  lv_label_set_text(ui_connect, labelText);
}

void my_display_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
  (void)disp;
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  lv_draw_sw_rgb565_swap(px_map, w * h);
  M5.Display.pushImageDMA<uint16_t>(area->x1, area->y1, w, h, (uint16_t *)px_map);
  lv_disp_flush_ready(disp);
}

static void updateScreenTitleLabels()
{
  if (ui_Logo != nullptr) lv_label_set_text(ui_Logo, T_SCREEN_START);
  if (ui_Logo2 != nullptr) lv_label_set_text(ui_Logo2, T_SCREEN_STROKE_ENGINE);
  if (ui_Logo1 != nullptr) lv_label_set_text(ui_Logo1, T_SCREEN_SETTINGS);
  if (ui_Logo4 != nullptr) lv_label_set_text(ui_Logo4, T_SCREEN_TORQUE);
  if (ui_Logo5 != nullptr) lv_label_set_text(ui_Logo5, T_SCREEN_PATTERN);
  if (ui_Logo6 != nullptr) lv_label_set_text(ui_Logo6, T_SCREEN_EJECT);
  if (ui_LogoMenu != nullptr) lv_label_set_text(ui_LogoMenu, T_SCREEN_MENU);
  if (ui_LogoStreaming != nullptr) lv_label_set_text(ui_LogoStreaming, T_SCREEN_STREAMING);
  if (ui_LogoAddons != nullptr) lv_label_set_text(ui_LogoAddons, T_SCREEN_ADDONS);
}

static void applyHomeDefaultsForModeChange()
{
  speed = 0.0f;
  depth = 0.0f;
  stroke = 0.0f;
  sensation = 50.0f;

  if (ui_homespeedslider != nullptr) lv_slider_set_value(ui_homespeedslider, 0, LV_ANIM_OFF);
  if (ui_homedepthslider != nullptr) lv_slider_set_value(ui_homedepthslider, 0, LV_ANIM_OFF);
  if (ui_homestrokeslider != nullptr) lv_slider_set_value(ui_homestrokeslider, 0, LV_ANIM_OFF);
  if (ui_homesensationslider != nullptr) lv_slider_set_value(ui_homesensationslider, 50, LV_ANIM_OFF);
  if (ui_homespeedvalue != nullptr) lv_label_set_text(ui_homespeedvalue, "0");
  if (ui_homedepthvalue != nullptr) lv_label_set_text(ui_homedepthvalue, "0");
  if (ui_homestrokevalue != nullptr) lv_label_set_text(ui_homestrokevalue, "0");

  SendCommand(SPEED, 0, OSSM_ID);
  SendCommand(DEPTH, 0, OSSM_ID);
  SendCommand(STROKE, 0, OSSM_ID);
  SendCommand(SENSATION, 50, OSSM_ID);
}

void screenmachine(lv_event_t * e)
{
  (void)e;
  const int previousScreen = st_screens;
  updateScreenTitleLabels();

  if (lv_scr_act() == ui_Start) {
    st_screens = ST_UI_START;
  } else if (lv_scr_act() == ui_Home) {
    st_screens = ST_UI_HOME;

    if (previousScreen == ST_UI_STREAMING) {
      applyHomeDefaultsForModeChange();
    }

    speed = lv_slider_get_value(ui_homespeedslider);
    LogDebug(speedenc);
    LogDebug(speed);
    encoder1.setCount(0);
    encoder2.setCount(0);
    encoder3.setCount(0);
    encoder4.setCount(0);
    if (!strokeinvert_mode) {
      lv_slider_set_mode(ui_homestrokeslider, LV_SLIDER_MODE_NORMAL);
    } else {
      lv_slider_set_mode(ui_homestrokeslider, LV_SLIDER_MODE_RANGE);
    }

    if (OssmBleIsMode()) {
      lv_slider_set_range(ui_homespeedslider, 0, 100);
      lv_slider_set_range(ui_homedepthslider, 0, 100);
      lv_slider_set_range(ui_homestrokeslider, 0, 100);

      uint32_t nowMs = millis();
      if ((nowMs - ble_home_go_strokeengine_ms) > 1200U) {
        OssmBleGoToStrokeEngine();
        ble_home_go_strokeengine_ms = nowMs;
      }
    } else {
      lv_slider_set_range(ui_homespeedslider, 0, speedlimit);
      lv_slider_set_range(ui_homedepthslider, 0, maxdepthinmm);
      lv_slider_set_range(ui_homestrokeslider, 0, maxdepthinmm);
    }

    refreshHomeAndStreamingStartStopUi();

  } else if (lv_scr_act() == ui_Pattern) {
    st_screens = ST_UI_PATTERN;
    encoder1.setCount(0);
    encoder2.setCount(0);
    encoder3.setCount(0);
    encoder4_enc = encoder4.getCount();
  } else if (lv_scr_act() == ui_Torqe) {
    st_screens = ST_UI_Torqe;
    torqe_f = lv_slider_get_value(ui_outtroqeslider);
    torqe_f_enc = fscale(50, 200, 0, Encoder_MAP, torqe_f, 0);
    encoder1.setCount(torqe_f_enc);

    torqe_r = lv_slider_get_value(ui_introqeslider);
    torqe_r_enc = fscale(20, 200, 0, Encoder_MAP, torqe_r, 0);
    encoder4.setCount(torqe_r_enc);

  } else if (lv_scr_act() == ui_EJECTSettings) {
    st_screens = ST_UI_EJECTSETTINGS;
  } else if (lv_scr_act() == ui_Settings) {
    st_screens = ST_UI_SETTINGS;
  } else if (lv_scr_act() == ui_Menu) {
    st_screens = ST_UI_MENU;
    if (previousScreen == ST_UI_STREAMING) {
      applyHomeDefaultsForModeChange();
    }
    if (previousScreen != ST_UI_MENU) {
      // Clear carry-over button flags so entering the menu does not trigger an immediate action.
      mxclick_short_waspressed = false;
      mxclick_long_waspressed = false;
      mxclick_double_waspressed = false;
      clickRight_short_waspressed = false;
      clickRight_long_waspressed = false;
      clickRight_double_waspressed = false;
      clickLeft_short_waspressed = false;
      clickLeft_double_waspressed = false;
    }
    encoder4_enc = encoder4.getCount();
    if (ui_g_menu != nullptr && ui_MenuButtonTL != nullptr) {
      lv_group_focus_obj(ui_MenuButtonTL);
    }

    // Disable Streaming button if using ESP_NOW OSSM (streaming requires BLE)
    if (ui_MenuButtonTR != nullptr) {
      if (ossm_espnow_connected) {
        // ESP_NOW OSSM mode: disable streaming button
        lv_obj_add_state(ui_MenuButtonTR, LV_STATE_DISABLED);
      } else {
        // BLE mode or not paired: enable streaming button
        lv_obj_clear_state(ui_MenuButtonTR, LV_STATE_DISABLED);
      }
    }
  } else if (lv_scr_act() == ui_Streaming) {
    st_screens = ST_UI_STREAMING;
    encoder1.setCount(0);
    encoder2.setCount(0);
    encoder3.setCount(0);
  } else if (lv_scr_act() == ui_Addons) {
    st_screens = ST_UI_ADDONS;
    encoder4_enc = encoder4.getCount();
    if (ui_g_addons != nullptr && ui_AddonsItem0 != nullptr) {
      lv_group_focus_obj(ui_AddonsItem0);
    }
  }

  monitorOssmState(true);
}

// ---------------------------------------------------------------------------
// BLE start-screen sync helper (also used by connectbutton in main.cpp)
// ---------------------------------------------------------------------------

void syncBleConnectUi(bool forceRefresh)
{
  (void)forceRefresh;
  if (OssmBleConnected() && isStartScreenMinTimeElapsed()) {
    lv_scr_load_anim(ui_Home, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0, false);
  }
}

// ---------------------------------------------------------------------------
// Encoder ramp (used by screen handlers below)
// ---------------------------------------------------------------------------

static int getRampedDetentDelta(int encoderId, int detents)
{
  if (detents == 0) return 0;
  if (!rampEnabled) {
    rampValue   = 1;
    activeEncId = encoderId;
    rampMs      = millis();
    return detents;
  }
  unsigned long now          = millis();
  bool sameEncoder           = (encoderId == activeEncId);
  bool withinRampWindow      = ((now - rampMs) <= (unsigned long)rampTime);
  if (!sameEncoder || !withinRampWindow) {
    rampValue = 1;
  }
  int sign  = (detents > 0) ? 1 : -1;
  int steps = abs(detents);
  int delta = 0;
  for (int i = 0; i < steps; ++i) {
    delta += sign * rampValue;
    if (rampValue < maxRamp) ++rampValue;
  }
  activeEncId = encoderId;
  rampMs      = now;
  return delta;
}

// ---------------------------------------------------------------------------
// Per-screen handler functions
// ---------------------------------------------------------------------------

static void handleStartScreen()
{
  if (OssmBleIsMode()) {
    static uint32_t lastBleHomingPollMs = 0;
    uint32_t nowPollMs = millis();
    if ((nowPollMs - lastBleHomingPollMs) >= 500UL) {
      lastBleHomingPollMs = nowPollMs;
      syncBleConnectUi(true);
    }
  }

  if (lv_obj_has_state(ui_lefty, LV_STATE_CHECKED) == 1) {
    touch_disabled = true;
  }

  if (clickLeft_short_waspressed == true) {
    lv_obj_send_event(ui_StartButtonL, LV_EVENT_CLICKED, NULL);
  } else if (mxclick_short_waspressed == true) {
    LogDebug("mx: ST_UI_START -> sending StartButtonM click");
    lv_obj_send_event(ui_StartButtonM, LV_EVENT_CLICKED, NULL);
  } else if (clickRight_short_waspressed == true) {
    lv_obj_send_event(ui_StartButtonR, LV_EVENT_CLICKED, NULL);
  }
}

static void handleHomeScreen()
{
  static int lastHomeTransportMode = -1;
  int currentTransportMode = OssmBleIsMode() ? 1 : 0;
  if (lastHomeTransportMode != currentTransportMode) {
    if (currentTransportMode == 1) {
      lv_slider_set_range(ui_homespeedslider, 0, 100);
      lv_slider_set_range(ui_homedepthslider, 0, 100);
      lv_slider_set_range(ui_homestrokeslider, 0, 100);
    } else {
      lv_slider_set_range(ui_homespeedslider, 0, speedlimit);
      lv_slider_set_range(ui_homedepthslider, 0, maxdepthinmm);
      lv_slider_set_range(ui_homestrokeslider, 0, maxdepthinmm);
    }
    if (speed  > lv_slider_get_max_value(ui_homespeedslider))
      speed  = lv_slider_get_max_value(ui_homespeedslider);
    if (depth  > lv_slider_get_max_value(ui_homedepthslider))
      depth  = lv_slider_get_max_value(ui_homedepthslider);
    if (stroke > lv_slider_get_max_value(ui_homestrokeslider))
      stroke = lv_slider_get_max_value(ui_homestrokeslider);
    lastHomeTransportMode = currentTransportMode;
  }

  if (lv_obj_has_state(ui_lefty, LV_STATE_CHECKED) == 1) {
    touch_disabled = true;
  }

  bool changed = false;

  // Encoder 1 – Speed
  if (lv_slider_is_dragged(ui_homespeedslider) == false) {
    changed = false;
    lv_slider_set_value(ui_homespeedslider, speed, LV_ANIM_OFF);

    long speedCount   = encoder1.getCount();
    int  speedDetents = (int)(speedCount / 2);
    if (speedDetents != 0) {
      changed = true;
      speed += getRampedDetentDelta(1, speedDetents);
      encoder1.setCount(speedCount % 2);
      markEncoderActivityForMxFilter();
    }
    if (speed < 0) { changed = true; speed = 0; }
    const float speedMax = OssmBleIsMode() ? 100.0f : speedlimit;
    if (speed > speedMax) { changed = true; speed = speedMax; }
    if (changed) SendCommand(SPEED, speed, OSSM_ID);
  } else if (lv_slider_get_value(ui_homespeedslider) != speed) {
    speed = lv_slider_get_value(ui_homespeedslider);
    SendCommand(SPEED, speed, OSSM_ID);
  }
  char speed_v[12];
  if (OssmBleIsMode()) {
    snprintf(speed_v, sizeof(speed_v), "%d", (int)(speed + 0.5f));
  } else {
    dtostrf(speed, 6, 0, speed_v);
  }
  lv_label_set_text(ui_homespeedvalue, speed_v);

  // Encoder 2 – Depth
  if (lv_slider_is_dragged(ui_homedepthslider) == false) {
    changed = false;
    lv_slider_set_value(ui_homedepthslider, depth, LV_ANIM_OFF);

    long depthCount   = encoder2.getCount();
    int  depthDetents = (int)(depthCount / 2);
    if (depthDetents != 0) {
      changed = true;
      int depthDelta = getRampedDetentDelta(2, depthDetents);
      depth += depthDelta;
      if (dynamicStroke == true) {
        stroke += depthDelta;
        if (stroke >= depth) stroke = depth;
      }
      encoder2.setCount(depthCount % 2);
      markEncoderActivityForMxFilter();
    }
    if (depth < 0) { changed = true; depth = 0; }
    const float depthMax = OssmBleIsMode() ? 100.0f : maxdepthinmm;
    if (depth > depthMax) { changed = true; depth = depthMax; }
    if (changed) SendCommand(DEPTH, depth, OSSM_ID);
  } else if (lv_slider_get_value(ui_homedepthslider) != depth) {
    depth = lv_slider_get_value(ui_homedepthslider);
    SendCommand(DEPTH, depth, OSSM_ID);
  }
  char depth_v[12];
  if (OssmBleIsMode()) {
    snprintf(depth_v, sizeof(depth_v), "%d", (int)(depth + 0.5f));
  } else {
    dtostrf(depth, 6, 0, depth_v);
  }
  lv_label_set_text(ui_homedepthvalue, depth_v);

  // Encoder 3 – Stroke
  if (lv_slider_is_dragged(ui_homestrokeslider) == false) {
    changed = false;
    if (!strokeinvert_mode) {
      lv_slider_set_mode(ui_homestrokeslider, LV_SLIDER_MODE_NORMAL);
      lv_slider_set_value(ui_homestrokeslider, stroke, LV_ANIM_OFF);
    } else {
      lv_slider_set_mode(ui_homestrokeslider, LV_SLIDER_MODE_RANGE);
      lv_slider_set_start_value(ui_homestrokeslider, depth - stroke, LV_ANIM_OFF);
      lv_slider_set_value(ui_homestrokeslider, depth, LV_ANIM_OFF);
    }

    long strokeCount   = encoder3.getCount();
    int  strokeDetents = (int)(strokeCount / 2);
    if (strokeDetents != 0) {
      changed = true;
      if (!strokeinvert_mode) {
        stroke += getRampedDetentDelta(3, strokeDetents);
      } else {
        stroke -= getRampedDetentDelta(3, strokeDetents);
      }
      encoder3.setCount(strokeCount % 2);
      markEncoderActivityForMxFilter();
    }
    if (stroke < 0) { changed = true; stroke = 0; }
    const float strokeMax = OssmBleIsMode() ? 100.0f : maxdepthinmm;
    if (stroke > strokeMax) { changed = true; stroke = strokeMax; }
    if (stroke > depth)     { changed = true; stroke = depth; }
    if (changed) SendCommand(STROKE, stroke, OSSM_ID);
  } else {
    if (!strokeinvert_mode) {
      if (lv_slider_get_left_value(ui_homestrokeslider) != depth - stroke) {
        stroke = depth - lv_slider_get_left_value(ui_homestrokeslider);
        if (stroke > depth) stroke = depth;
        SendCommand(STROKE, stroke, OSSM_ID);
      } else if (lv_slider_get_value(ui_homestrokeslider) != depth) {
        depth = lv_slider_get_value(ui_homestrokeslider);
        SendCommand(DEPTH, depth, OSSM_ID);
      }
    } else {
      if (lv_slider_get_value(ui_homestrokeslider) != depth) {
        depth = lv_slider_get_value(ui_homestrokeslider);
        SendCommand(DEPTH, depth, OSSM_ID);
      } else if (lv_slider_get_left_value(ui_homestrokeslider) != depth - stroke) {
        stroke = depth - lv_slider_get_left_value(ui_homestrokeslider);
        if (stroke > depth) stroke = depth;
        SendCommand(STROKE, stroke, OSSM_ID);
      }
    }
  }
  char stroke_v[12];
  if (OssmBleIsMode()) {
    snprintf(stroke_v, sizeof(stroke_v), "%d", (int)(stroke + 0.5f));
  } else {
    dtostrf(stroke, 6, 0, stroke_v);
  }
  lv_label_set_text(ui_homestrokevalue, stroke_v);

  // Encoder 4 – Sensation
  if (lv_slider_is_dragged(ui_homesensationslider) == false) {
    changed = false;
    lv_slider_set_value(ui_homesensationslider, sensation, LV_ANIM_OFF);

    long sensationCount   = encoder4.getCount();
    int  sensationDetents = (int)(sensationCount / 2);
    if (sensationDetents != 0) {
      changed = true;
      sensation += 2.0f * getRampedDetentDelta(4, sensationDetents);
      encoder4.setCount(sensationCount % 2);
      markEncoderActivityForMxFilter();
    }
    if (sensation < -100) { changed = true; sensation = -100; }
    if (sensation >  100) { changed = true; sensation =  100; }
    if (changed) SendCommand(SENSATION, sensation, OSSM_ID);
  } else if (lv_slider_get_value(ui_homesensationslider) != sensation) {
    sensation = lv_slider_get_value(ui_homesensationslider);
    SendCommand(SENSATION, sensation, OSSM_ID);
  }

  // Button dispatch
  if (clickLeft_short_waspressed == true) {
    lv_obj_send_event(ui_HomeButtonL, LV_EVENT_CLICKED, NULL);
    clickLeft_short_waspressed = false;
  } else if (clickLeft_double_waspressed == true) {
    triggerAddonForSlot(ADDON_SLOT_LEFT);
    clickLeft_double_waspressed = false;
  } else if (mxclick_short_waspressed == true) {
    LogDebug("mx: ST_UI_HOME -> direct HomeButtonM action");
    homebuttonm_action(true);
    mxclick_short_waspressed = false;
  } else if (clickRight_short_waspressed == true) {
    lv_obj_send_event(ui_HomeButtonR, LV_EVENT_CLICKED, NULL);
    clickRight_short_waspressed = false;
  } else if (clickRight_long_waspressed == true) {
    _ui_screen_change(ui_Menu, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    clickRight_long_waspressed = false;
  } else if (clickRight_double_waspressed == true) {
    triggerAddonForSlot(ADDON_SLOT_RIGHT);
    clickRight_double_waspressed = false;
  }
}

static void handlePatternScreen()
{
  if (lv_obj_has_state(ui_lefty, LV_STATE_CHECKED) == 1) {
    touch_disabled = true;
  }

  if (encoder4.getCount() > encoder4_enc + 2) {
    LogDebug("next");
    uint32_t t = LV_KEY_DOWN;
    lv_obj_send_event(ui_PatternS, LV_EVENT_KEY, &t);
    encoder4_enc = encoder4.getCount();
  } else if (encoder4.getCount() < encoder4_enc - 2) {
    uint32_t t = LV_KEY_UP;
    lv_obj_send_event(ui_PatternS, LV_EVENT_KEY, &t);
    LogDebug("Preview");
    encoder4_enc = encoder4.getCount();
  }

  if (clickLeft_short_waspressed == true) {
    lv_obj_send_event(ui_PatternButtonL, LV_EVENT_CLICKED, NULL);
  } else if (mxclick_short_waspressed == true) {
    LogDebug("mx: ST_UI_PATTERN -> sending PatternButtonM click");
    lv_obj_send_event(ui_PatternButtonM, LV_EVENT_CLICKED, NULL);
  } else if (clickRight_short_waspressed == true) {
    lv_obj_send_event(ui_PatternButtonR, LV_EVENT_CLICKED, NULL);
  }
}

static void handleTorqueScreen()
{
  if (lv_obj_has_state(ui_lefty, LV_STATE_CHECKED) == 1) {
    touch_disabled = true;
  }

  // Encoder 1 – Torque Out
  if (lv_slider_is_dragged(ui_outtroqeslider) == false) {
    if (encoder1.getCount() != torqe_f_enc) {
      lv_slider_set_value(ui_outtroqeslider, torqe_f, LV_ANIM_OFF);
      if (encoder1.getCount() <= 0) {
        encoder1.setCount(0);
      } else if (encoder1.getCount() >= Encoder_MAP) {
        encoder1.setCount(Encoder_MAP);
      }
      torqe_f_enc = encoder1.getCount();
      torqe_f     = fscale(0, Encoder_MAP, 50, 200, torqe_f_enc, 0);
      SendCommand(TORQE_F, torqe_f, OSSM_ID);
    }
  } else if (lv_slider_get_value(ui_outtroqeslider) != torqe_f) {
    torqe_f_enc = fscale(50, 200, 0, Encoder_MAP, torqe_f, 0);
    encoder1.setCount(torqe_f_enc);
    torqe_f = lv_slider_get_value(ui_outtroqeslider);
    SendCommand(TORQE_F, torqe_f, OSSM_ID);
  }
  char torqe_f_v[7];
  dtostrf((torqe_f * -1), 6, 0, torqe_f_v);
  lv_label_set_text(ui_outtroqevalue, torqe_f_v);

  // Encoder 4 – Torque In
  if (lv_slider_is_dragged(ui_introqeslider) == false) {
    if (encoder4.getCount() != torqe_r_enc) {
      lv_slider_set_value(ui_introqeslider, torqe_r, LV_ANIM_OFF);
      if (encoder4.getCount() <= 0) {
        encoder4.setCount(0);
      } else if (encoder4.getCount() >= Encoder_MAP) {
        encoder4.setCount(Encoder_MAP);
      }
      torqe_r_enc = encoder4.getCount();
      torqe_r     = fscale(0, Encoder_MAP, 20, 200, torqe_r_enc, 0);
      SendCommand(TORQE_R, torqe_r, OSSM_ID);
    }
  } else if (lv_slider_get_value(ui_introqeslider) != torqe_r) {
    torqe_r_enc = fscale(20, 200, 0, Encoder_MAP, torqe_r, 0);
    encoder4.setCount(torqe_r_enc);
    torqe_r = lv_slider_get_value(ui_introqeslider);
    SendCommand(TORQE_R, torqe_r, OSSM_ID);
  }
  char torqe_r_v[7];
  dtostrf(torqe_r, 6, 0, torqe_r_v);
  lv_label_set_text(ui_introqevalue, torqe_r_v);

  if (clickLeft_short_waspressed == true) {
    lv_obj_send_event(ui_TorqeButtonL, LV_EVENT_CLICKED, NULL);
  } else if (mxclick_short_waspressed == true) {
    LogDebug("mx: ST_UI_Torqe -> sending TorqeButtonM click");
    lv_obj_send_event(ui_TorqeButtonM, LV_EVENT_CLICKED, NULL);
  } else if (clickRight_short_waspressed == true) {
    lv_obj_send_event(ui_TorqeButtonR, LV_EVENT_CLICKED, NULL);
  }
}

static void handleEjectSettingsScreen()
{
  if (lv_obj_has_state(ui_lefty, LV_STATE_CHECKED) == 1) {
    touch_disabled = true;
  }

  if (clickLeft_short_waspressed == true) {
    lv_obj_send_event(ui_EJECTButtonL, LV_EVENT_CLICKED, NULL);
  } else if (mxclick_short_waspressed == true) {
    LogDebug("mx: ST_UI_EJECTSETTINGS -> sending EJECTButtonM click");
    lv_obj_send_event(ui_EJECTButtonM, LV_EVENT_CLICKED, NULL);
  } else if (clickRight_short_waspressed == true) {
    lv_obj_send_event(ui_EJECTButtonR, LV_EVENT_CLICKED, NULL);
  }
}

static void handleSettingsScreen()
{
  touch_disabled = false;

  if (encoder3.getCount() > encoder3_enc + 2) {
    int val = lv_slider_get_value(ui_brightness_slider);
    int mx  = lv_slider_get_max_value(ui_brightness_slider);
    if (val < mx) {
      int newval = (val + 5 <= mx) ? val + 5 : mx;
      lv_slider_set_value(ui_brightness_slider, newval, LV_ANIM_OFF);
      M5.Display.setBrightness(newval);
    }
    encoder3_enc = encoder3.getCount();
  } else if (encoder3.getCount() < encoder3_enc - 2) {
    int val = lv_slider_get_value(ui_brightness_slider);
    int mn  = lv_slider_get_min_value(ui_brightness_slider);
    if (val > mn) {
      int newval = (val - 5 >= mn) ? val - 5 : mn;
      lv_slider_set_value(ui_brightness_slider, newval, LV_ANIM_OFF);
      M5.Display.setBrightness(newval);
    }
    encoder3_enc = encoder3.getCount();
  }

  if (encoder4.getCount() > encoder4_enc + 2) {
    LogDebug("next setting");
    lv_group_focus_next(ui_g_settings);
    encoder4_enc = encoder4.getCount();
  } else if (encoder4.getCount() < encoder4_enc - 2) {
    lv_group_focus_prev(ui_g_settings);
    LogDebug("previous setting");
    encoder4_enc = encoder4.getCount();
  }

  if (mxclick_long_waspressed || mxclick_double_waspressed) {
    mxclick_short_waspressed  = false;
    mxclick_long_waspressed   = false;
    mxclick_double_waspressed = false;
  } else if (clickRight_short_waspressed == true) {
    lv_obj_t *focused = lv_group_get_focused(ui_g_settings);
    if (focused) {
      if (focused == ui_vibrate || focused == ui_lefty ||
          focused == ui_strokeinvert || focused == ui_darkmode) {
        bool checked = lv_obj_has_state(focused, LV_STATE_CHECKED);
        if (checked) {
          lv_obj_clear_state(focused, LV_STATE_CHECKED);
        } else {
          lv_obj_add_state(focused, LV_STATE_CHECKED);
        }
        lv_obj_send_event(focused, LV_EVENT_VALUE_CHANGED, NULL);
      } else {
        lv_obj_send_event(focused, LV_EVENT_CLICKED, NULL);
      }
    }
    clickRight_short_waspressed  = false;
    clickRight_long_waspressed   = false;
    clickRight_double_waspressed = false;
  } else if (mxclick_short_waspressed) {
    LogDebug("mx: ST_UI_SETTINGS -> go to menu");
    lv_obj_send_event(ui_SettingsButtonM, LV_EVENT_CLICKED, NULL);
    mxclick_short_waspressed  = false;
    mxclick_long_waspressed   = false;
    mxclick_double_waspressed = false;
  } else if (clickLeft_short_waspressed == true) {
    lv_obj_send_event(ui_SettingsButtonL, LV_EVENT_CLICKED, NULL);
    clickLeft_short_waspressed = false;
  }
}

static void handleMenuScreen()
{
  if (lv_obj_has_state(ui_lefty, LV_STATE_CHECKED) == 1) {
    touch_disabled = true;
  }

  if (encoder4.getCount() > encoder4_enc + 2) {
    lv_group_focus_next(ui_g_menu);
    encoder4_enc = encoder4.getCount();
  } else if (encoder4.getCount() < encoder4_enc - 2) {
    lv_group_focus_prev(ui_g_menu);
    encoder4_enc = encoder4.getCount();
  }

  if (clickLeft_short_waspressed == true) {
    lv_obj_send_event(ui_MenuButtonL, LV_EVENT_CLICKED, NULL);
    clickLeft_short_waspressed = false;
  }

  if (mxclick_short_waspressed == true) {
    lv_obj_send_event(ui_MenuButtonM, LV_EVENT_CLICKED, NULL);
    mxclick_short_waspressed = false;
  }

  if (clickRight_short_waspressed == true) {
    lv_obj_t *focused = (ui_g_menu != nullptr) ? lv_group_get_focused(ui_g_menu) : nullptr;
    if (focused != nullptr) {
      lv_obj_send_event(focused, LV_EVENT_CLICKED, NULL);
    }
    clickRight_short_waspressed = false;
  }
}

static void handleAddonsScreen()
{
  if (lv_obj_has_state(ui_lefty, LV_STATE_CHECKED) == 1) {
    touch_disabled = true;
  }

  if (encoder4.getCount() > encoder4_enc + 2) {
    lv_group_focus_next(ui_g_addons);
    encoder4_enc = encoder4.getCount();
  } else if (encoder4.getCount() < encoder4_enc - 2) {
    lv_group_focus_prev(ui_g_addons);
    encoder4_enc = encoder4.getCount();
  }

  if (clickLeft_short_waspressed == true) {
    _ui_screen_change(ui_Menu, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    clickLeft_short_waspressed = false;
  } else if (clickRight_short_waspressed == true) {
    lv_obj_send_event(lv_group_get_focused(ui_g_addons), LV_EVENT_CLICKED, NULL);
    clickRight_short_waspressed = false;
  }
}

static void handleStreamingScreen()
{
  if (g_streaming_entry_flow_pending) {
    handleStreamingEntryFlow();
    if (lv_scr_act() != ui_Streaming) {
      return;
    }
  }

  if (g_streaming_controls_locked) {
    encoder1.setCount(0);
    encoder2.setCount(0);
    encoder3.setCount(0);
    return;
  }

  static int lastStreamingTransportMode = -1;
  int currentTransportMode = OssmBleIsMode() ? 1 : 0;
  if (lastStreamingTransportMode != currentTransportMode) {
    lv_slider_set_max_value(ui_streamingspeedslider, 100);
    lv_slider_set_max_value(ui_streamingdepthslider, 100);
    lv_slider_set_max_value(ui_streamingstrokeslider, 100);
    if (speed  > lv_slider_get_max_value(ui_streamingspeedslider))
      speed  = lv_slider_get_max_value(ui_streamingspeedslider);
    if (depth  > lv_slider_get_max_value(ui_streamingdepthslider))
      depth  = lv_slider_get_max_value(ui_streamingdepthslider);
    if (stroke > lv_slider_get_max_value(ui_streamingstrokeslider))
      stroke = lv_slider_get_max_value(ui_streamingstrokeslider);
    lastStreamingTransportMode = currentTransportMode;
  }

  if (lv_obj_has_state(ui_lefty, LV_STATE_CHECKED) == 1) {
    touch_disabled = true;
  }

  bool changed = false;

  // Encoder 1 – Speed
  if (lv_slider_is_dragged(ui_streamingspeedslider) == false) {
    changed = false;
    lv_slider_set_value(ui_streamingspeedslider, speed, LV_ANIM_OFF);
    long enc = encoder1.getCount();
    int  det = (int)(enc / 2);
    if (det != 0) {
      changed = true;
      speed += getRampedDetentDelta(1, det);
      encoder1.setCount(enc % 2);
      markEncoderActivityForMxFilter();
    }
    if (speed < 0)      { changed = true; speed = 0; }
    if (speed > 100.0f) { changed = true; speed = 100.0f; }
    if (changed) SendCommand(SPEED, speed, OSSM_ID);
  } else if (lv_slider_get_value(ui_streamingspeedslider) != speed) {
    speed = lv_slider_get_value(ui_streamingspeedslider);
    SendCommand(SPEED, speed, OSSM_ID);
  }
  char speed_v[12];
  if (OssmBleIsMode()) {
    snprintf(speed_v, sizeof(speed_v), "%d", (int)(speed + 0.5f));
  } else {
    dtostrf(speed, 6, 0, speed_v);
  }
  lv_label_set_text(ui_streamingspeedvalue, speed_v);

  // Encoder 2 – Depth
  if (lv_slider_is_dragged(ui_streamingdepthslider) == false) {
    changed = false;
    lv_slider_set_value(ui_streamingdepthslider, depth, LV_ANIM_OFF);
    long enc = encoder2.getCount();
    int  det = (int)(enc / 2);
    if (det != 0) {
      changed = true;
      depth += getRampedDetentDelta(2, det);
      encoder2.setCount(enc % 2);
      markEncoderActivityForMxFilter();
    }
    if (depth < 0)      { changed = true; depth = 0; }
    if (depth > 100.0f) { changed = true; depth = 100.0f; }
    if (changed) SendCommand(DEPTH, depth, OSSM_ID);
  } else if (lv_slider_get_value(ui_streamingdepthslider) != depth) {
    depth = lv_slider_get_value(ui_streamingdepthslider);
    SendCommand(DEPTH, depth, OSSM_ID);
  }
  char depth_v[12];
  if (OssmBleIsMode()) {
    snprintf(depth_v, sizeof(depth_v), "%d", (int)(depth + 0.5f));
  } else {
    dtostrf(depth, 6, 0, depth_v);
  }
  lv_label_set_text(ui_streamingdepthvalue, depth_v);

  // Encoder 3 – Stroke
  if (lv_slider_is_dragged(ui_streamingstrokeslider) == false) {
    changed = false;
    lv_slider_set_value(ui_streamingstrokeslider, stroke, LV_ANIM_OFF);
    long enc = encoder3.getCount();
    int  det = (int)(enc / 2);
    if (det != 0) {
      changed = true;
      stroke += getRampedDetentDelta(3, det);
      encoder3.setCount(enc % 2);
      markEncoderActivityForMxFilter();
    }
    if (stroke < 0)      { changed = true; stroke = 0; }
    if (stroke > 100.0f) { changed = true; stroke = 100.0f; }
    if (stroke > depth)  { changed = true; stroke = depth; }
    if (changed) SendCommand(STROKE, stroke, OSSM_ID);
  } else if (lv_slider_get_value(ui_streamingstrokeslider) != stroke) {
    stroke = lv_slider_get_value(ui_streamingstrokeslider);
    SendCommand(STROKE, stroke, OSSM_ID);
  }
  char stroke_v[12];
  if (OssmBleIsMode()) {
    snprintf(stroke_v, sizeof(stroke_v), "%d", (int)(stroke + 0.5f));
  } else {
    dtostrf(stroke, 6, 0, stroke_v);
  }
  lv_label_set_text(ui_streamingstrokevalue, stroke_v);

  if (mxclick_short_waspressed == true) {
    LogDebug("mx: ST_UI_STREAMING -> direct StreamingButtonM action");
    streamingbuttonm_action(true);
    mxclick_short_waspressed = false;
  } else if (clickRight_short_waspressed == true) {
    lv_obj_send_event(ui_StreamingButtonR, LV_EVENT_CLICKED, NULL);
    clickRight_short_waspressed = false;
  }
}

// ---------------------------------------------------------------------------
// Main dispatcher — called from loop()
// ---------------------------------------------------------------------------

void handleCurrentScreen(){
  update_battery_icons_all_screens(M5.Power.getBatteryLevel());

  switch (st_screens) {
    case ST_UI_START:         handleStartScreen();         break;
    case ST_UI_HOME:          handleHomeScreen();          break;
    case ST_UI_PATTERN:       handlePatternScreen();       break;
    case ST_UI_Torqe:         handleTorqueScreen();        break;
    case ST_UI_EJECTSETTINGS: handleEjectSettingsScreen(); break;
    case ST_UI_SETTINGS:      handleSettingsScreen();      break;
    case ST_UI_MENU:          handleMenuScreen();          break;
    case ST_UI_ADDONS:        handleAddonsScreen();        break;
    case ST_UI_STREAMING:     handleStreamingScreen();     break;
    default: break;
  }

  // Clear all deferred button press flags.
  clearButtonFlags();
}

// ---------------------------------------------------------------------------
// Touch input driver for LVGL
// ---------------------------------------------------------------------------
void my_touchpad_read(lv_indev_t *drv, lv_indev_data_t *data)
{
  M5.update();
  auto count = M5.Touch.getCount();

  if (touch_disabled != true) {
    if (count == 0) {
      data->state = LV_INDEV_STATE_RELEASED;
    } else {
      screensaver_check_activity();
      auto touch = M5.Touch.getDetail(0);
      data->state     = LV_INDEV_STATE_PRESSED;
      data->point.x   = touch.x;
      data->point.y   = touch.y;
    }
  }
}

// ---------------------------------------------------------------------------
// Debug event callback (non-static so register_event_debug_callbacks can use it)
// ---------------------------------------------------------------------------
void event_cb(lv_event_t *e)
{
  lv_event_code_t code   = lv_event_get_code(e);
  lv_obj_t *target       = reinterpret_cast<lv_obj_t *>(lv_event_get_target(e));
  lv_obj_t *label        = reinterpret_cast<lv_obj_t *>(lv_event_get_user_data(e));
  const char *eventName  = nullptr;
  const char *buttonName = "unknown";

  if (target == ui_StartButtonL || target == ui_HomeButtonL || target == ui_MenuButtonL ||
      target == ui_PatternButtonL || target == ui_TorqeButtonL || target == ui_EJECTButtonL ||
      target == ui_SettingsButtonL) {
    buttonName = "left";
  } else if (target == ui_StartButtonM || target == ui_HomeButtonM || target == ui_MenuButtonM ||
             target == ui_PatternButtonM || target == ui_TorqeButtonM || target == ui_EJECTButtonM ||
             target == ui_SettingsButtonM) {
    buttonName = "mx";
  } else if (target == ui_StartButtonR || target == ui_HomeButtonR || target == ui_MenuButtonR ||
             target == ui_PatternButtonR || target == ui_TorqeButtonR || target == ui_EJECTButtonR ||
             target == ui_SettingsButtonR) {
    buttonName = "right";
  }

  switch (code) {
    case LV_EVENT_PRESSED:             eventName = "LV_EVENT_PRESSED";             break;
    case LV_EVENT_RELEASED:            eventName = "LV_EVENT_RELEASED";            break;
    case LV_EVENT_SHORT_CLICKED:       eventName = "LV_EVENT_SHORT_CLICKED";       break;
    case LV_EVENT_CLICKED:             eventName = "LV_EVENT_CLICKED";             break;
    case LV_EVENT_LONG_PRESSED:        eventName = "LV_EVENT_LONG_PRESSED";        break;
    case LV_EVENT_LONG_PRESSED_REPEAT: eventName = "LV_EVENT_LONG_PRESSED_REPEAT"; break;
    default: break;
  }

  if (eventName != nullptr) {
    if (label != nullptr) {
      lv_label_set_text_fmt(label, "The last button event:\n%s %s", eventName, buttonName);
    }
    LogDebugPrioFormatted("The last button event: %s %s\n", eventName, buttonName);
  }
}