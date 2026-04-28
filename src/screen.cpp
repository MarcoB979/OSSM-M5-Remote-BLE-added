#include "main.h"
#include <Arduino.h>
#include "PatternMath.h"

#include "main.h"
#include "config.h"
#include "screen.h"
#include "colors.h"
#include "styles.h"
#include "OssmBLE.h"
#include "esp_nowCommunication.h"
#include "Eject.h"
#include "FistIT.h"
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
// Homing request dedupe: avoid issuing two homing requests within short window
static unsigned long last_homing_request_ms = 0;
static const unsigned long HOMING_REQUEST_MIN_GAP_MS = 400;

// Per-loop one-shot snapshot consumed by all screen handlers in this tick.
static ButtonEvents g_button_events = {};

static const char* battery_symbol_for_level(int level, bool isCharging)
{
  if (isCharging) return LV_SYMBOL_CHARGE;
  if (level < 10) return LV_SYMBOL_BATTERY_EMPTY;
  if (level < 25) return LV_SYMBOL_BATTERY_1;
  if (level < 50) return LV_SYMBOL_BATTERY_2;
  if (level < 75) return LV_SYMBOL_BATTERY_3;
  if (level > 97) return LV_SYMBOL_BATTERY_FULL;
  return LV_SYMBOL_BATTERY_3;
}

void update_battery_icons_all_screens(int level, bool isCharging)
{
  static bool batteryUiInitialized = false;
  lv_obj_t *fistBattTitle = FistITGetBatteryTitleLabel();
  lv_obj_t *fistBattValue = FistITGetBatteryValueLabel();

  lv_obj_t *batteryTitleLabels[] = {
    ui_Batt, ui_Batt1, ui_Batt2, ui_Batt3, ui_Batt4, ui_Batt5, ui_Batt6, ui_Batt7, ui_Batt8, fistBattTitle
  };

  lv_obj_t *batteryValueLabels[] = {
    ui_BattValue, ui_BattValue1, ui_BattValue2, ui_BattValue3, ui_BattValue4,
    ui_BattValue5, ui_BattValue6, ui_BattValue7, ui_BattValue8, fistBattValue
  };

  lv_obj_t *batteryBars[] = {
    ui_Battery, ui_Battery1, ui_Battery2, ui_Battery3, ui_Battery4,
    ui_Battery5, ui_Battery6, ui_Battery7, ui_Battery8, nullptr
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

  const char *symbol = battery_symbol_for_level(level, isCharging);
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

static bool detectChargingNow()
{
  auto chargingState = M5.Power.isCharging();
  if (chargingState == m5::Power_Class::is_charging) {
    return true;
  }
  if (chargingState == m5::Power_Class::is_discharging) {
    return false;
  }

  // Fallback for unsupported/unknown PMIC charging state.
  return M5.Power.getBatteryCurrent() > 15;
}

static bool getStableChargingState()
{
  static bool initialized = false;
  static bool rawState = false;
  static bool stableState = false;
  static uint32_t rawSinceMs = 0;

  const bool nowRaw = detectChargingNow();
  const uint32_t nowMs = millis();

  if (!initialized) {
    initialized = true;
    rawState = nowRaw;
    stableState = nowRaw;
    rawSinceMs = nowMs;
    return stableState;
  }

  if (nowRaw != rawState) {
    rawState = nowRaw;
    rawSinceMs = nowMs;
  }

  if (stableState != rawState && (nowMs - rawSinceMs) >= 800U) {
    stableState = rawState;
  }

  return stableState;
}

static void maybeShowChargingWarning(bool isCharging)
{
  static bool shownForCurrentChargeSession = false;

  if (!isCharging) {
    shownForCurrentChargeSession = false;
    return;
  }

  if (shownForCurrentChargeSession) {
    return;
  }

  shownForCurrentChargeSession = true;
  showNotification(
      T_CHARGING_WARNING_TITLE,
      T_CHARGING_WARNING_TEXT,
      8000,
      false,
      nullptr,
      false,
      nullptr,
      false);
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

#if AUTO_IDLE_DEEP_SLEEP_ENABLED
  if (millis() - last_activity_ms > deep_sleep_timeout_ms) {
    if (canEnterDeepSleep()) {
      enterDeepSleep();
    }
  }
#endif
}

// (no MX suppression here — keep MX responsive)

static lv_obj_t* createStatusIconBase(lv_obj_t* parent, int width, int height)
{
  if (parent == nullptr) {
    return nullptr;
  }

  lv_obj_t* icon = lv_canvas_create(parent);
  lv_obj_remove_style_all(icon);
  lv_obj_set_size(icon, width, height);
  lv_obj_clear_flag(icon, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(icon, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_bg_opa(icon, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_opa(icon, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
  return icon;
}

// Icon rendering delegated to src/icons.cpp
#include "icons.h"

static lv_obj_t* createStatusEjectIcon(lv_obj_t* parent)
{
  static uint8_t iconBuffer[LV_CANVAS_BUF_SIZE(32, 32, 32, LV_DRAW_BUF_STRIDE_ALIGN)];
  static bool iconReady = false;
  const int iconW = addonsGetEjectIconWidth();
  const int iconH = addonsGetEjectIconHeight();
  const char* const* iconMask = addonsGetEjectIconMask();

  lv_obj_t* icon = createStatusIconBase(parent, iconW, iconH);
  if (icon == nullptr) {
    return nullptr;
  }

  icons_render_mask_canvas(icon, iconBuffer, iconReady, iconMask, iconW, iconH,
                           getActiveBackgroundColor(), getActiveTextPrimaryColor());
  return icon;
}

static lv_obj_t* createStatusFistIcon(lv_obj_t* parent)
{
  static uint8_t iconBuffer[LV_CANVAS_BUF_SIZE(32, 32, 32, LV_DRAW_BUF_STRIDE_ALIGN)];
  static bool iconReady = false;
  const int iconW = addonsGetFistIconWidth();
  const int iconH = addonsGetFistIconHeight();
  const char* const* iconMask = addonsGetFistIconMask();

  lv_obj_t* icon = createStatusIconBase(parent, iconW, iconH);
  if (icon == nullptr) {
    return nullptr;
  }
  icons_render_mask_canvas(icon, iconBuffer, iconReady, iconMask, iconW, iconH,
                           getActiveBackgroundColor(), getActiveTextPrimaryColor());
  return icon;
}


void updateHomeTopLeftStateLabel()
{
  static lv_obj_t *statusLabels[12] = { nullptr };
  static lv_obj_t *statusEjectIcons[12] = { nullptr };
  static lv_obj_t *statusFistIcons[12] = { nullptr };
  lv_obj_t *statusScreens[12] = {
    ui_Start,
    ui_Home,
    ui_Pattern,
    ui_EJECTSettings,
    ui_Settings,
    ui_Menu,
    ui_Streaming,
    ui_Addons,
    ui_Colors,
    FistITGetScreen(),
    nullptr,
  };

  // Ensure every screen has a top-left status label.
  for (size_t i = 0; i < 12; ++i) {
    if (statusLabels[i] != nullptr) {
      continue;
    }
    if (statusScreens[i] == nullptr) {
      continue;
    }

    if (statusScreens[i] == ui_Home && ui_connect != nullptr) {
      statusLabels[i] = ui_connect;
    } else {
      statusLabels[i] = lv_label_create(statusScreens[i]);
    }

    lv_obj_set_width(statusLabels[i], LV_SIZE_CONTENT);
    lv_obj_set_height(statusLabels[i], LV_SIZE_CONTENT);
    lv_obj_set_align(statusLabels[i], LV_ALIGN_LEFT_MID);
    lv_obj_set_x(statusLabels[i], 10);
    lv_obj_set_y(statusLabels[i], -102);
    // Use semantic primary text style so status labels follow the active
    // color scheme (dark/light) instead of forcing white which becomes
    // invisible on light backgrounds.
    lv_obj_add_style(statusLabels[i], &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(statusLabels[i], &lv_font_montserrat_22, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(statusLabels[i], LV_OBJ_FLAG_HIDDEN);

    if (statusEjectIcons[i] == nullptr) {
      statusEjectIcons[i] = createStatusEjectIcon(statusScreens[i]);
    }
    if (statusFistIcons[i] == nullptr) {
      statusFistIcons[i] = createStatusFistIcon(statusScreens[i]);
    }
  }

  char labelText[48];
  size_t pos = 0;
  labelText[0] = '\0';

  auto appendToken = [&](const char *token) {
    if (token == nullptr || token[0] == '\0' || pos >= sizeof(labelText) - 1) {
      return;
    }
    if (pos > 0 && pos < sizeof(labelText) - 1) {
      labelText[pos++] = ' ';
      labelText[pos] = '\0';
    }
    int written = snprintf(labelText + pos, sizeof(labelText) - pos, "%s", token);
    if (written > 0) {
      pos += (size_t)written;
      if (pos >= sizeof(labelText)) {
        pos = sizeof(labelText) - 1;
      }
    }
  };

  if (OssmBleConnected()) {
    appendToken(LV_SYMBOL_BLUETOOTH);
  }
  if (Ossm_paired) {
    appendToken(LV_SYMBOL_WIFI);
  }
  if (OSSM_State == state_HOMING) {
    if (g_homing_direction > 0) {
      appendToken(LV_SYMBOL_UP);
    } else if (g_homing_direction < 0) {
      appendToken(LV_SYMBOL_DOWN);
    }
  }

  if (labelText[0] == '\0') {
    snprintf(labelText, sizeof(labelText), " ");
  }

  const bool ejectPaired = EjectIsPaired();
  const bool fistPaired = FistITIsPaired();

  for (size_t i = 0; i < 12; ++i) {
    lv_obj_t *label = statusLabels[i];
    if (label == nullptr) {
      continue;
    }

    // Respect the semantic primary text style so runtime updates follow
    // the active color scheme instead of forcing white.
    lv_obj_add_style(label, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_22, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(label, labelText);
    lv_obj_update_layout(label);

    int iconX = 10 + lv_obj_get_width(label) + 4;

    if (statusEjectIcons[i] != nullptr) {
      lv_obj_set_align(statusEjectIcons[i], LV_ALIGN_LEFT_MID);
      lv_obj_set_x(statusEjectIcons[i], iconX);
      lv_obj_set_y(statusEjectIcons[i], -102);
      if (ejectPaired) {
        lv_obj_clear_flag(statusEjectIcons[i], LV_OBJ_FLAG_HIDDEN);
        iconX += lv_obj_get_width(statusEjectIcons[i]) + 2;
      } else {
        lv_obj_add_flag(statusEjectIcons[i], LV_OBJ_FLAG_HIDDEN);
      }
    }

    if (statusFistIcons[i] != nullptr) {
      lv_obj_set_align(statusFistIcons[i], LV_ALIGN_LEFT_MID);
      lv_obj_set_x(statusFistIcons[i], iconX);
      lv_obj_set_y(statusFistIcons[i], -102);
      if (fistPaired) {
        lv_obj_clear_flag(statusFistIcons[i], LV_OBJ_FLAG_HIDDEN);
      } else {
        lv_obj_add_flag(statusFistIcons[i], LV_OBJ_FLAG_HIDDEN);
      }
    }
  }
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
  if (ui_Logo5 != nullptr) lv_label_set_text(ui_Logo5, T_SCREEN_PATTERN);
  if (ui_Logo6 != nullptr) lv_label_set_text(ui_Logo6, T_SCREEN_EJECT);
  if (ui_LogoMenu != nullptr) lv_label_set_text(ui_LogoMenu, T_SCREEN_MENU);
  if (ui_LogoStreaming != nullptr) lv_label_set_text(ui_LogoStreaming, T_SCREEN_STREAMING);
  if (ui_LogoAddons != nullptr) lv_label_set_text(ui_LogoAddons, T_SCREEN_ADDONS);
}

static void applyHomeDefaultsForModeChange()
{
  speed = 0.0f;
  depth = 20.0f;
  stroke = 15.0f;
  sensation = 0.0f;

  if (ui_homespeedslider != nullptr) lv_slider_set_value(ui_homespeedslider, 0, LV_ANIM_OFF);
  if (ui_homedepthslider != nullptr) lv_slider_set_value(ui_homedepthslider, 20, LV_ANIM_OFF);
  if (ui_homestrokeslider != nullptr) lv_slider_set_value(ui_homestrokeslider, 15, LV_ANIM_OFF);
  if (ui_homesensationslider != nullptr) lv_slider_set_value(ui_homesensationslider, 0, LV_ANIM_OFF);
  if (ui_homespeedvalue != nullptr) lv_label_set_text(ui_homespeedvalue, "0");
  if (ui_homedepthvalue != nullptr) lv_label_set_text(ui_homedepthvalue, "20");
  if (ui_homestrokevalue != nullptr) lv_label_set_text(ui_homestrokevalue, "15");

  // Set pattern to RoboStroke (index 2) and update UI label if available.
  pattern = 2;
  if (ui_PatternS != nullptr) {
    lv_roller_set_selected(ui_PatternS, 2, LV_ANIM_OFF);
    lv_roller_get_selected_str(ui_PatternS, patternstr, 0);
  }
  if (ui_HomePatternLabel != nullptr) {
    lv_label_set_text(ui_HomePatternLabel, patternstr);
  }

  SendCommand(SPEED, 0, OSSM_ID);
  SendCommand(DEPTH, 20, OSSM_ID);
  SendCommand(STROKE, 15, OSSM_ID);
  SendCommand(SENSATION, 0, OSSM_ID);
  SendCommand(PATTERN, 2, OSSM_ID);
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

      // Only re-enter stroke engine from Menu when Menu flow explicitly armed it
      // (e.g. after selecting non-home actions that should force homing later).
      if (previousScreen == ST_UI_MENU && g_ble_menu_requires_stroke_reentry) {
        if ((millis() - last_homing_request_ms) >= HOMING_REQUEST_MIN_GAP_MS) {
          OssmBleGoToStrokeEngine();
          // Apply home defaults when returning from Menu where a re-entry was armed.
          applyHomeDefaultsForModeChange();
          last_homing_request_ms = millis();
        } else {
          LogDebugPrioFormatted("Skipped duplicate homing request (gap=%lu ms)\n", (unsigned long)(millis() - last_homing_request_ms));
        }
        g_ble_menu_requires_stroke_reentry = false;
      }
    } else {
      lv_slider_set_range(ui_homespeedslider, 0, speedlimit);
      lv_slider_set_range(ui_homedepthslider, 0, maxdepthinmm);
      lv_slider_set_range(ui_homestrokeslider, 0, maxdepthinmm);
    }

    refreshHomeAndStreamingStartStopUi();
    refreshHomeAddonButtonLabels();

  } else if (lv_scr_act() == ui_Pattern) {
    st_screens = ST_UI_PATTERN;
    encoder1.setCount(0);
    encoder2.setCount(0);
    encoder3.setCount(0);
    encoder4_enc = encoder4.getCount();
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
      clearButtonFlags();
    }
    encoder4_enc = encoder4.getCount();
    if (ui_g_menu != nullptr && ui_MenuButtonTL != nullptr) {
      lv_group_focus_obj(ui_MenuButtonTL);
    }

    // Disable Streaming button if using ESP_NOW OSSM (streaming requires BLE)
    if (ui_MenuButtonTR != nullptr) {
      if (Ossm_paired) {
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
  } else if (lv_scr_act() == ui_Colors) {
    st_screens = ST_UI_COLORS;
    encoder4_enc = encoder4.getCount();
    if (ui_g_colors != nullptr) {
      lv_group_focus_next(ui_g_colors);
    }
  } else if (lv_scr_act() == FistITGetScreen()) {
    st_screens = ST_UI_FISTIT;
  }

  monitorOssmState(true);
}

// ---------------------------------------------------------------------------
// BLE start-screen sync helper (also used by connectbutton in main.cpp)
// ---------------------------------------------------------------------------

void syncBleConnectUi(bool forceRefresh)
{
  (void)forceRefresh;
  static bool startupMenuRequested = false;
  static uint32_t startupMenuRequestedMs = 0;

  if (!OssmBleConnected()) {
    startupMenuRequested = false;
    startupMenuRequestedMs = 0;
    return;
  }

  if (!isStartScreenMinTimeElapsed()) {
    return;
  }

  if (!startupMenuRequested) {
    lv_label_set_text(ui_Welcome, T_HOMING);
    // Request stroke-engine entry to force a homing sequence on connect.
    // Using GoToStrokeEngine triggers the OSSM to enter stroke mode and
    // start homing if necessary. Do not set UI homing text here elsewhere;
    // the UI will update from BLE polling.
    if ((millis() - last_homing_request_ms) >= HOMING_REQUEST_MIN_GAP_MS) {
      OssmBleGoToStrokeEngine();
      last_homing_request_ms = millis();
    } else {
      LogDebugPrioFormatted("syncBleConnectUi: skipped duplicate homing (gap=%lu ms)\n", (unsigned long)(millis() - last_homing_request_ms));
    }
    startupMenuRequested = true;
    startupMenuRequestedMs = millis();
    return;
  }

  OssmBleMachineState bleState;
  if (OssmBleGetCurrentState(&bleState, true)) {
    if (bleState.mode == OssmBleMachineMode::HomingForward ||
        bleState.mode == OssmBleMachineMode::HomingBackward) {
      lv_label_set_text(ui_Welcome, T_HOMING);
      return;
    }

    if (bleState.mode == OssmBleMachineMode::Menu ||
        bleState.mode == OssmBleMachineMode::StrokeEngineIdle ||
        bleState.mode == OssmBleMachineMode::StrokeEngineActive) {
      lv_scr_load_anim(ui_Menu, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0, false);
      return;
    }
  }

  // Fallback to avoid getting stuck if state polling is temporarily unavailable.
  if ((millis() - startupMenuRequestedMs) > 5000UL) {
    lv_scr_load_anim(ui_Menu, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0, false);
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

static void handleStartScreen(const ButtonEvents &events)
{
  static uint32_t nextAutoConnectAttemptMs = 0;

  // Keep trying auto-connect while Start is visible.
  bool Ossm_paired = (OssmBleConnected() || Ossm_paired);
  if (!Ossm_paired && millis() >= nextAutoConnectAttemptMs) {
    connectbutton(nullptr);
    nextAutoConnectAttemptMs = millis() + 4000UL;
  }

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

  if (events.leftShort) {
    lv_obj_send_event(ui_StartButtonL, LV_EVENT_SHORT_CLICKED, NULL);
    clearButtonFlags();
  } else if (events.mxShort) {
    LogDebug("mx: ST_UI_START -> sending StartButtonM click");
    lv_obj_send_event(ui_StartButtonM, LV_EVENT_SHORT_CLICKED, NULL);
    clearButtonFlags();
  } else if (events.rightShort) {
    lv_obj_send_event(ui_StartButtonR, LV_EVENT_SHORT_CLICKED, NULL);
    clearButtonFlags();
  }
}

static void handleHomeScreen(const ButtonEvents &events)
{
  static float last_speed_for_toggle = 0.0f;
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
    int  speedDetents = (int)(speedCount / 4);
    int  speedRem = (int)(speedCount - speedDetents * 4);
    if (speedRem < 0) { speedRem += 4; speedDetents -= 1; }
    if (speedDetents != 0) {
      changed = true;
      LogDebugFormatted("ENC1 raw=%ld det=%d speed_before=%f\n", speedCount, speedDetents, speed);
      speed += getRampedDetentDelta(1, speedDetents);
      encoder1.setCount(speedRem);
      screensaver_check_activity();
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

  // Detect 0 -> non-zero and non-zero -> 0 transitions to auto start/pause OSSM.
  {
    float prev = last_speed_for_toggle;
    float cur = speed;
    bool crossed_to_nonzero = (prev <= 0.0f && cur > 0.0f);
    bool crossed_to_zero = (prev > 0.0f && cur <= 0.0f);

    if (crossed_to_nonzero) {
      homebuttonm_action(false);
    } else if (crossed_to_zero) {
      homebuttonm_action(false);
    }
    last_speed_for_toggle = cur;
  }

  // Encoder 2 – Depth
  if (lv_slider_is_dragged(ui_homedepthslider) == false) {
    changed = false;
    lv_slider_set_value(ui_homedepthslider, depth, LV_ANIM_OFF);

    long depthCount   = encoder2.getCount();
    int  depthDetents = (int)(depthCount / 4);
    int  depthRem = (int)(depthCount - depthDetents * 4);
    if (depthRem < 0) { depthRem += 4; depthDetents -= 1; }
    if (depthDetents != 0) {
      changed = true;
      LogDebugFormatted("ENC2 raw=%ld det=%d depth_before=%f\n", depthCount, depthDetents, depth);
      int depthDelta = getRampedDetentDelta(2, depthDetents);
      depth += depthDelta;
      if (dynamicStroke == true) {
        stroke += depthDelta;
        if (stroke >= depth) stroke = depth;
      }
      encoder2.setCount(depthRem);
      screensaver_check_activity();
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
    int  strokeDetents = (int)(strokeCount / 4);
    int  strokeRem = (int)(strokeCount - strokeDetents * 4);
    if (strokeRem < 0) { strokeRem += 4; strokeDetents -= 1; }
    if (strokeDetents != 0) {
      LogDebugFormatted("ENC3 raw=%ld det=%d stroke_before=%f\n", strokeCount, strokeDetents, stroke);
      int delta = getRampedDetentDelta(3, strokeDetents);
      float newStroke = stroke;
      if (!strokeinvert_mode) {
        newStroke += delta;
      } else {
        newStroke -= delta;
      }
      // clamp proposed value
      if (newStroke < 0.0f) newStroke = 0.0f;
      const float strokeMax = OssmBleIsMode() ? 100.0f : maxdepthinmm;
      if (newStroke > strokeMax) newStroke = strokeMax;
      if (newStroke > depth) newStroke = depth;

      if (newStroke != stroke) {
        float oldStroke = stroke;
        changed = true;
        stroke = newStroke;
        LogDebugFormatted("ENC3 applied stroke change old=%f new=%f raw=%ld det=%d\n", oldStroke, stroke, strokeCount, strokeDetents);
      }
      encoder3.setCount(strokeRem);
      screensaver_check_activity();
    }
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
    int  sensationDetents = (int)(sensationCount / 4);
    int  sensationRem = (int)(sensationCount - sensationDetents * 4);
    if (sensationRem < 0) { sensationRem += 4; sensationDetents -= 1; }
    if (sensationDetents != 0) {
      changed = true;
      LogDebugFormatted("ENC4 raw=%ld det=%d sensation_before=%f\n", sensationCount, sensationDetents, sensation);
      sensation += 2.0f * getRampedDetentDelta(4, sensationDetents);
      encoder4.setCount(sensationRem);
      screensaver_check_activity();
    }
    if (sensation < -100) { changed = true; sensation = -100; }
    if (sensation >  100) { changed = true; sensation =  100; }
    if (changed) SendCommand(SENSATION, sensation, OSSM_ID);
  } else if (lv_slider_get_value(ui_homesensationslider) != sensation) {
    sensation = lv_slider_get_value(ui_homesensationslider);
    SendCommand(SENSATION, sensation, OSSM_ID);
  }

  // Button dispatch
  if (events.leftShort) {
    homebuttonLevent(nullptr);
    clearButtonFlags();
  } else if (events.leftLong) {
    bool launched = triggerAddonForSlot(ADDON_SLOT_LEFT);
    LogDebugFormatted("HOME leftLong -> triggerAddonForSlot(LEFT) result=%s\n", launched ? "OK" : "NONE");
    clearButtonFlags();
  } else if (events.mxShort) {
    LogDebug("mx: ST_UI_HOME -> direct HomeButtonM action");
    homebuttonm_action(true);
    clearButtonFlags();
  } else if (events.mxLong) {
    homebuttonMlongEvent(nullptr);
    clearButtonFlags();
  } else if (events.rightShort) {
    _ui_screen_change(ui_Pattern, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    clearButtonFlags();
  } else if (events.rightLong) {
    bool launched = triggerAddonForSlot(ADDON_SLOT_RIGHT);
    LogDebugFormatted("HOME rightLong -> triggerAddonForSlot(RIGHT) result=%s\n", launched ? "OK" : "NONE");
    clearButtonFlags();
  }
}

static void handlePatternScreen(const ButtonEvents &events)
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

  if (events.leftShort) {
    lv_obj_send_event(ui_PatternButtonL, LV_EVENT_SHORT_CLICKED, NULL);
    clearButtonFlags();
  } else if (events.mxShort) {
    LogDebug("mx: ST_UI_PATTERN -> sending PatternButtonM click");
    lv_obj_send_event(ui_PatternButtonM, LV_EVENT_SHORT_CLICKED, NULL);
    clearButtonFlags();
  } else if (events.rightShort) {
    lv_obj_send_event(ui_PatternButtonR, LV_EVENT_SHORT_CLICKED, NULL);
    clearButtonFlags();
  }
}


static void handleEjectSettingsScreen(const ButtonEvents &events)
{
  EjectHandleScreen(events);
}

static void handleFistITScreen(const ButtonEvents &events)
{
  FistITHandleScreen(events);
}

static void handleSettingsScreen(const ButtonEvents &events)
{
  touch_disabled = false;

  if (encoder3.getCount() > encoder3_enc + 2) {
    int val = lv_slider_get_value(ui_brightness_slider);
    int mx  = lv_slider_get_max_value(ui_brightness_slider);
    if (val < 5) val = 5;
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
    if (ui_g_settings != nullptr) {
      lv_group_focus_next(ui_g_settings);
    }
    encoder4_enc = encoder4.getCount();
  } else if (encoder4.getCount() < encoder4_enc - 2) {
    if (ui_g_settings != nullptr) {
      lv_group_focus_prev(ui_g_settings);
    }
    LogDebug("previous setting");
    encoder4_enc = encoder4.getCount();
  }

  if (events.mxLong || events.mxDouble) {
    clearButtonFlags();
  } else if (events.rightShort) {
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
        lv_obj_send_event(focused, LV_EVENT_SHORT_CLICKED, NULL);
      }
    }
    clearButtonFlags();
  } else if (events.mxShort) {
    LogDebug("mx: ST_UI_SETTINGS -> go to menu");
    lv_obj_send_event(ui_SettingsButtonM, LV_EVENT_SHORT_CLICKED, NULL);
    clearButtonFlags();
  } else if (events.leftShort) {
    lv_obj_send_event(ui_SettingsButtonL, LV_EVENT_SHORT_CLICKED, NULL);
    clearButtonFlags();
  }
}

static void handleMenuScreen(const ButtonEvents &events)
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

  if (events.leftShort) {
    lv_obj_send_event(ui_MenuButtonL, LV_EVENT_SHORT_CLICKED, NULL);
    clearButtonFlags();
  }

  if (events.mxShort) {
    lv_obj_send_event(ui_MenuButtonM, LV_EVENT_SHORT_CLICKED, NULL);
    clearButtonFlags();
  }

  if (events.rightShort) {
    lv_obj_t *focused = (ui_g_menu != nullptr) ? lv_group_get_focused(ui_g_menu) : nullptr;
    if (focused != nullptr) {
      lv_obj_send_event(focused, LV_EVENT_SHORT_CLICKED, NULL);
    }
    clearButtonFlags();
  }
}

static void handleAddonsScreen(const ButtonEvents &events)
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

  if (events.leftShort) {
    _ui_screen_change(ui_Menu, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    clearButtonFlags();
  } else if (events.mxShort) {
    // Middle button: trigger the focused addon (launch if assigned, cycle if not)
    lv_obj_t * focused = lv_group_get_focused(ui_g_addons);
    if (focused != NULL) {
      int addonIndex = -1;
      if (focused == ui_AddonsItem0) {
        addonIndex = 0;
      } else if (focused == ui_AddonsItem1) {
        addonIndex = 1;
      } else if (focused == ui_AddonsItem2) {
        addonIndex = 2;
      }
      
      if (addonIndex >= 0) {
        triggerAddonByIndex(addonIndex);
      }
    }
    clearButtonFlags();
  } else if (events.rightShort) {
    lv_obj_t *focused = (ui_g_addons != nullptr) ? lv_group_get_focused(ui_g_addons) : nullptr;
    if (focused != nullptr) {
      lv_obj_send_event(focused, LV_EVENT_SHORT_CLICKED, NULL);
    }
    clearButtonFlags();
  }
}

static void handleStreamingScreen(const ButtonEvents &events)
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
    int  det = (int)(enc / 4);
    int  rem = (int)(enc - det * 4);
    if (rem < 0) { rem += 4; det -= 1; }
    if (det != 0) {
      changed = true;
      speed += getRampedDetentDelta(1, det);
      encoder1.setCount(rem);
      screensaver_check_activity();
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
    int  det = (int)(enc / 4);
    int  rem = (int)(enc - det * 4);
    if (rem < 0) { rem += 4; det -= 1; }
    if (det != 0) {
      changed = true;
      depth += getRampedDetentDelta(2, det);
      encoder2.setCount(rem);
      screensaver_check_activity();
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
    int  det = (int)(enc / 4);
    int  rem = (int)(enc - det * 4);
    if (rem < 0) { rem += 4; det -= 1; }
    if (det != 0) {
      changed = true;
      LogDebugFormatted("ENC3(stream) raw=%ld det=%d stroke_before=%f\n", enc, det, stroke);
      stroke += getRampedDetentDelta(3, det);
      encoder3.setCount(rem);
      screensaver_check_activity();
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

  if (events.mxShort) {
    LogDebug("mx: ST_UI_STREAMING -> direct StreamingButtonM action");
    streamingbuttonm_action(true);
    clearButtonFlags();
  } else if (events.rightShort) {
    lv_obj_send_event(ui_StreamingButtonR, LV_EVENT_SHORT_CLICKED, NULL);
    clearButtonFlags();
  }
}

// ---------------------------------------------------------------------------
// Main dispatcher — called from loop()
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Battery level smoothing
//   - Samples raw level every 30 s
//   - Applies an EMA (α=0.1) to filter noise
//   - Holds the displayed value for 60 s after charger disconnect to absorb
//     the transient voltage sag that the PMIC otherwise reports as a real drop
// ---------------------------------------------------------------------------
static int getSmoothedBatteryLevel(bool isCharging)
{
  static uint32_t lastSampleMs    = 0;
  static bool     wasCharging     = false;
  static uint32_t disconnectedMs  = 0;
  static float    emaLevel        = -1.0f;   // -1 = uninitialised
  static int      displayedLevel  = -1;

  const uint32_t now = millis();

  // Detect charger removal and start the settling hold timer.
  if (wasCharging && !isCharging) {
    disconnectedMs = now;
  }
  wasCharging = isCharging;

  const bool inSettlingWindow = (!isCharging && (now - disconnectedMs) < 120000UL);

  // Initialise on very first call.
  if (emaLevel < 0.0f) {
    emaLevel       = (float)M5.Power.getBatteryLevel();
    displayedLevel = (int)(emaLevel + 0.5f);
    lastSampleMs   = now;
    // Mark settling window as just started if charger not connected at boot.
    if (!isCharging) {
      disconnectedMs = now;
    }
  }

  // While in the post-disconnect settling window, show last known good level.
  if (inSettlingWindow) {
    return displayedLevel;
  }

  // Sample every 30 seconds.
  if (now - lastSampleMs >= 30000UL || lastSampleMs == 0) {
    lastSampleMs = now;
    const float raw = (float)M5.Power.getBatteryLevel();
    // EMA: weight new sample at 10 %, carry 90 % from history.
    emaLevel = 0.1f * raw + 0.9f * emaLevel;
    displayedLevel = (int)(emaLevel + 0.5f);
    // Clamp to valid range.
    if (displayedLevel < 0)   displayedLevel = 0;
    if (displayedLevel > 100) displayedLevel = 100;

    // If the battery is essentially full, and charger or charge current
    // indicates it's still charging (or recently was), hold at 100% to
    // avoid a visible transient drop immediately after unplugging.
    const int CHG_HOLD_CURRENT_MA = 30; // treat currents >30mA as still charging
    int measuredCurrent = M5.Power.getBatteryCurrent();
    if (displayedLevel >= 99 && (isCharging || measuredCurrent > CHG_HOLD_CURRENT_MA)) {
      displayedLevel = 100;
      emaLevel = 100.0f;
    }
  }

  return displayedLevel;
}

void handleCurrentScreen(){
  const bool isCharging = getStableChargingState();
  update_battery_icons_all_screens(getSmoothedBatteryLevel(isCharging), isCharging);
  maybeShowChargingWarning(isCharging);
  pollButtonEvents(g_button_events);

  switch (st_screens) {
    case ST_UI_START:         handleStartScreen(g_button_events);         break;
    case ST_UI_HOME:          handleHomeScreen(g_button_events);          break;
    case ST_UI_PATTERN:       handlePatternScreen(g_button_events);       break;
    /* ST_UI_Torqe removed */
    case ST_UI_EJECTSETTINGS: handleEjectSettingsScreen(g_button_events); break;
    case ST_UI_SETTINGS:      handleSettingsScreen(g_button_events);      break;
    case ST_UI_MENU:          handleMenuScreen(g_button_events);          break;
    case ST_UI_ADDONS:        handleAddonsScreen(g_button_events);        break;
    case ST_UI_COLORS:        handleColorsScreen(g_button_events);        break;
    case ST_UI_STREAMING:     handleStreamingScreen(g_button_events);     break;
    case ST_UI_FISTIT:        handleFistITScreen(g_button_events);        break;
    default: break;
  }

  // Individual handlers consume and clear button flags immediately.
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
      target == ui_PatternButtonL || target == ui_EJECTButtonL ||
      target == ui_SettingsButtonL) {
    buttonName = "left";
  } else if (target == ui_StartButtonM || target == ui_HomeButtonM || target == ui_MenuButtonM ||
             target == ui_PatternButtonM || target == ui_EJECTButtonM ||
             target == ui_SettingsButtonM) {
    buttonName = "mx";
  } else if (target == ui_StartButtonR || target == ui_HomeButtonR || target == ui_MenuButtonR ||
             target == ui_PatternButtonR || target == ui_EJECTButtonR ||
             target == ui_SettingsButtonR) {
    buttonName = "right";
  }

  if (code == LV_EVENT_PRESSED) {
    eventName = "LV_EVENT_PRESSED";
  } else if (code == LV_EVENT_RELEASED) {
    eventName = "LV_EVENT_RELEASED";
  } else if (code == LV_EVENT_SHORT_CLICKED) {
    eventName = "LV_EVENT_SHORT_CLICKED";
  } else if (code == LV_EVENT_LONG_PRESSED) {
    eventName = "LV_EVENT_LONG_PRESSED";
  } else if (code == LV_EVENT_LONG_PRESSED_REPEAT) {
    eventName = "LV_EVENT_LONG_PRESSED_REPEAT";
  }

  if (eventName != nullptr) {
    if (label != nullptr) {
      lv_label_set_text_fmt(label, "The last button event:\n%s %s", eventName, buttonName);
    }
    LogDebugPrioFormatted("The last button event: %s %s\n", eventName, buttonName);
  }
}
