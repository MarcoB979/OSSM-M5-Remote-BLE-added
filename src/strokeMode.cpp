#include "ui/ui.h"
#include "lvgl.h"
#include "language.h"
#include "strokeMode.h"
#include "main.h"
#include "styles.h"
#include "OssmBLE.h"
#include "encoder_ramp.h"
#include "ui/ui_helpers.h"

// Independent Stroke screen implementation (no re-parenting of Home widgets).
// This file creates its own widgets and provides a per-loop handler so
// encoders/knobs work the same as on Home.

lv_obj_t * ui_StrokePatternLabel1 = nullptr;
lv_obj_t * ui_StrokePatternLabel = nullptr;
static lv_obj_t * s_Logo = nullptr;
static lv_obj_t * s_SpeedL = nullptr;
static lv_obj_t * s_strokespeedslider = nullptr;
static lv_obj_t * s_strokespeedvalue = nullptr;
static lv_obj_t * s_StrokeL = nullptr;
static lv_obj_t * s_strokestrokeslider = nullptr;
static lv_obj_t * s_strokestrokevalue = nullptr;
static lv_obj_t * s_SensationL = nullptr;
static lv_obj_t * s_strokesensationslider = nullptr;
static lv_obj_t * s_strokesensationvalue = nullptr;
static lv_obj_t * s_StrokeButtonL = nullptr;
static lv_obj_t * s_StrokeButtonLText = nullptr;
static lv_obj_t * s_StrokeButtonM = nullptr;
static lv_obj_t * s_StrokeButtonMText = nullptr;
static lv_obj_t * s_StrokeButtonR = nullptr;
static lv_obj_t * s_StrokeButtonRText = nullptr;

static void ui_event_Stroke(lv_event_t * e)
{
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_SCREEN_LOADED) {
    if (s_Logo) lv_label_set_text_fmt(s_Logo, "%s %s", T_STROKE_SCREEN, T_STROKE_SUB);;
    // Ensure screen state machine updates when Stroke screen loads
    screenmachine(e);
    // Ensure OSSM is in Stroke Engine mode when this screen loads
    // (OssmBleGoToStrokeEngine is idempotent and will no-op if already in mode).
    if (OssmBleIsMode()) {
      OssmBleGoToStrokeEngine();
    }
    // Update Stroke start/stop button appearance to reflect current OSSM state
    refreshStrokeStartStopUi();
  }
}

// Refresh Stroke start/stop UI to reflect OSSM state
void refreshStrokeStartStopUi()
{
  if (s_StrokeButtonM != nullptr && s_StrokeButtonMText != nullptr) {
    if (isRunningUiState(OSSM_State)) {
      lv_obj_set_style_bg_color(s_StrokeButtonM, lv_color_hex(0xB3261E), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_label_set_text(s_StrokeButtonMText, OssmBleIsMode() ? T_PAUSE : T_STOP);
    } else {
      lv_obj_set_style_bg_color(s_StrokeButtonM, lv_color_hex(0x228B22), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_label_set_text(s_StrokeButtonMText, T_START);
    }
  }
}

static void s_stroke_btn_r_short_cb(lv_event_t * e) { (void)e; lv_scr_load_anim(ui_Pattern, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0, false); }

void ui_Stroke_screen_init(void)
{
  ui_Stroke = lv_obj_create(NULL);
  lv_obj_clear_flag(ui_Stroke, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(ui_Stroke, ui_event_Stroke, LV_EVENT_ALL, NULL);

  // Header
  s_Logo = lv_label_create(ui_Stroke);
  lv_obj_set_width(s_Logo, LV_SIZE_CONTENT);
  lv_obj_set_height(s_Logo, LV_SIZE_CONTENT);
  lv_obj_set_y(s_Logo, -103);
  lv_obj_set_x(s_Logo, lv_pct(0));
  lv_obj_set_align(s_Logo, LV_ALIGN_CENTER);
  lv_label_set_text(s_Logo, T_STROKE_SCREEN);
  lv_obj_set_style_text_font(s_Logo, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_style(s_Logo, &style_title_bar, LV_PART_MAIN | LV_STATE_DEFAULT);

  // Speed label + slider + value
  s_SpeedL = lv_label_create(ui_Stroke);
  lv_obj_set_width(s_SpeedL, lv_pct(95));
  lv_obj_set_height(s_SpeedL, LV_SIZE_CONTENT);
  lv_obj_set_x(s_SpeedL, 0);
  lv_obj_set_y(s_SpeedL, -40);
  lv_obj_set_align(s_SpeedL, LV_ALIGN_CENTER);
  lv_label_set_text(s_SpeedL, T_SPEED);
  lv_obj_set_style_text_font(s_SpeedL, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

  s_strokespeedslider = lv_slider_create(s_SpeedL);
  lv_slider_set_range(s_strokespeedslider, 0, speedlimit);
  lv_obj_set_width(s_strokespeedslider, 130);
  lv_obj_set_height(s_strokespeedslider, 25);
  lv_obj_set_x(s_strokespeedslider, -15);
  lv_obj_set_align(s_strokespeedslider, LV_ALIGN_RIGHT_MID);
  lv_obj_add_style(s_strokespeedslider, &style_slider_track[0], LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_style(s_strokespeedslider, &style_slider_indicator[0], LV_PART_INDICATOR | LV_STATE_DEFAULT);
  lv_obj_add_style(s_strokespeedslider, &style_slider_indicator[0], LV_PART_KNOB | LV_STATE_DEFAULT);
  lv_obj_set_style_width(s_strokespeedslider, 12, LV_PART_KNOB | LV_STATE_DEFAULT);
  lv_obj_set_style_height(s_strokespeedslider, 12, LV_PART_KNOB | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(s_strokespeedslider, LV_RADIUS_CIRCLE, LV_PART_KNOB | LV_STATE_DEFAULT);

  s_strokespeedvalue = lv_label_create(s_SpeedL);
  lv_obj_set_width(s_strokespeedvalue, LV_SIZE_CONTENT);
  lv_obj_set_height(s_strokespeedvalue, LV_SIZE_CONTENT);
  lv_obj_set_x(s_strokespeedvalue, 80);
  lv_obj_set_y(s_strokespeedvalue, 0);
  lv_obj_set_align(s_strokespeedvalue, LV_ALIGN_LEFT_MID);
  lv_label_set_text(s_strokespeedvalue, T_BLANK);

  // Stroke label + slider + value
  s_StrokeL = lv_label_create(ui_Stroke);
  lv_obj_set_width(s_StrokeL, lv_pct(95));
  lv_obj_set_height(s_StrokeL, LV_SIZE_CONTENT);
  lv_obj_set_x(s_StrokeL, 0);
  lv_obj_set_y(s_StrokeL, -4);
  lv_obj_set_align(s_StrokeL, LV_ALIGN_CENTER);
  lv_label_set_text(s_StrokeL, T_STROKE);
  lv_obj_set_style_text_font(s_StrokeL, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

  s_strokestrokeslider = lv_slider_create(s_StrokeL);
  lv_slider_set_range(s_strokestrokeslider, 0, maxdepthinmm);
  lv_slider_set_mode(s_strokestrokeslider, LV_SLIDER_MODE_NORMAL);
  lv_obj_set_width(s_strokestrokeslider, 130);
  lv_obj_set_height(s_strokestrokeslider, 25);
  lv_obj_set_x(s_strokestrokeslider, -15);
  lv_obj_set_align(s_strokestrokeslider, LV_ALIGN_RIGHT_MID);
  lv_obj_add_style(s_strokestrokeslider, &style_slider_track[2], LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_style(s_strokestrokeslider, &style_slider_indicator[2], LV_PART_INDICATOR | LV_STATE_DEFAULT);
  lv_obj_add_style(s_strokestrokeslider, &style_slider_indicator[2], LV_PART_KNOB | LV_STATE_DEFAULT);
  lv_obj_set_style_width(s_strokestrokeslider, 12, LV_PART_KNOB | LV_STATE_DEFAULT);
  lv_obj_set_style_height(s_strokestrokeslider, 12, LV_PART_KNOB | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(s_strokestrokeslider, LV_RADIUS_CIRCLE, LV_PART_KNOB | LV_STATE_DEFAULT);

  s_strokestrokevalue = lv_label_create(s_StrokeL);
  lv_obj_set_width(s_strokestrokevalue, LV_SIZE_CONTENT);
  lv_obj_set_height(s_strokestrokevalue, LV_SIZE_CONTENT);
  lv_obj_set_x(s_strokestrokevalue, 80);
  lv_obj_set_y(s_strokestrokevalue, 0);
  lv_obj_set_align(s_strokestrokevalue, LV_ALIGN_LEFT_MID);
  lv_label_set_text(s_strokestrokevalue, T_BLANK);

  // Sensation label + slider + value
  s_SensationL = lv_label_create(ui_Stroke);
  lv_obj_set_width(s_SensationL, lv_pct(95));
  lv_obj_set_height(s_SensationL, LV_SIZE_CONTENT);
  lv_obj_set_x(s_SensationL, 0);
  lv_obj_set_y(s_SensationL, 35);
  lv_obj_set_align(s_SensationL, LV_ALIGN_CENTER);
  lv_label_set_text(s_SensationL, T_SENSATION);
  lv_obj_set_style_text_font(s_SensationL, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

  s_strokesensationslider = lv_slider_create(s_SensationL);
  lv_slider_set_range(s_strokesensationslider, -100, 100);
  lv_slider_set_mode(s_strokesensationslider, LV_SLIDER_MODE_SYMMETRICAL);
  lv_obj_set_width(s_strokesensationslider, 170);
  lv_obj_set_height(s_strokesensationslider, 25);
  lv_obj_set_x(s_strokesensationslider, -15);
  lv_obj_set_align(s_strokesensationslider, LV_ALIGN_RIGHT_MID);
  lv_obj_add_style(s_strokesensationslider, &style_slider_track[3], LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_style(s_strokesensationslider, &style_slider_indicator[3], LV_PART_INDICATOR | LV_STATE_DEFAULT);
  lv_obj_add_style(s_strokesensationslider, &style_slider_indicator[3], LV_PART_KNOB | LV_STATE_DEFAULT);
  lv_obj_set_style_width(s_strokesensationslider, 12, LV_PART_KNOB | LV_STATE_DEFAULT);
  lv_obj_set_style_height(s_strokesensationslider, 12, LV_PART_KNOB | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(s_strokesensationslider, LV_RADIUS_CIRCLE, LV_PART_KNOB | LV_STATE_DEFAULT);

  s_strokesensationvalue = lv_label_create(s_SensationL);
  lv_obj_set_width(s_strokesensationvalue, LV_SIZE_CONTENT);
  lv_obj_set_height(s_strokesensationvalue, LV_SIZE_CONTENT);
  lv_obj_set_x(s_strokesensationvalue, 80);
  lv_obj_set_align(s_strokesensationvalue, LV_ALIGN_LEFT_MID);
  lv_label_set_text(s_strokesensationvalue, T_BLANK);

  // Battery display at top-right (use ui_Batt3 slot so shared updater updates it)
  ui_Batt3 = lv_label_create(ui_Stroke);
  lv_obj_set_width(ui_Batt3, 85);
  lv_obj_set_height(ui_Batt3, 30);
  lv_obj_set_x(ui_Batt3, 115);
  lv_obj_set_y(ui_Batt3, -103);
  lv_obj_set_align(ui_Batt3, LV_ALIGN_CENTER);
  lv_label_set_text(ui_Batt3, T_BATT);
  lv_obj_add_style(ui_Batt3, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_BattValue3 = lv_label_create(ui_Batt3);
  lv_obj_set_width(ui_BattValue3, LV_SIZE_CONTENT);
  lv_obj_set_height(ui_BattValue3, LV_SIZE_CONTENT);
  lv_obj_set_x(ui_BattValue3, 0);
  lv_obj_set_y(ui_BattValue3, -7);
  lv_obj_set_align(ui_BattValue3, LV_ALIGN_RIGHT_MID);
  lv_label_set_text(ui_BattValue3, T_BLANK);
  lv_obj_add_style(ui_BattValue3, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_Battery3 = lv_bar_create(ui_Batt3);
  lv_bar_set_range(ui_Battery3, 0, 100);
  lv_obj_set_width(ui_Battery3, 80);
  lv_obj_set_height(ui_Battery3, 10);
  lv_obj_set_x(ui_Battery3, 0);
  lv_obj_set_y(ui_Battery3, 10);
  lv_obj_set_align(ui_Battery3, LV_ALIGN_CENTER);
  lv_obj_add_style(ui_Battery3, &style_battery_main, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_style(ui_Battery3, &style_battery_indicator, LV_PART_INDICATOR | LV_STATE_DEFAULT);

  // Pattern label (mirror Home): show currently selected pattern on Stroke screen
  ui_StrokePatternLabel1 = lv_label_create(ui_Stroke);
  lv_obj_set_width(ui_StrokePatternLabel1, lv_pct(95));
  lv_obj_set_height(ui_StrokePatternLabel1, LV_SIZE_CONTENT);
  lv_obj_set_x(ui_StrokePatternLabel1, 10);
  lv_obj_set_y(ui_StrokePatternLabel1, 70);
  lv_obj_set_align(ui_StrokePatternLabel1, LV_ALIGN_LEFT_MID);
  lv_label_set_text(ui_StrokePatternLabel1, T_Patterns);
  lv_obj_set_style_text_font(ui_StrokePatternLabel1, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_StrokePatternLabel = lv_label_create(ui_StrokePatternLabel1);
  lv_obj_set_width(ui_StrokePatternLabel, LV_SIZE_CONTENT);
  lv_obj_set_height(ui_StrokePatternLabel, LV_SIZE_CONTENT);
  lv_obj_set_x(ui_StrokePatternLabel, 110);
  lv_obj_set_y(ui_StrokePatternLabel, 0);
  lv_obj_set_align(ui_StrokePatternLabel, LV_ALIGN_LEFT_MID);
  lv_label_set_text(ui_StrokePatternLabel, patternstr);

  // Apply requested temporary adjustments: speed a bit lower, stroke +10, sensation -10
  {
    // speed: reduce by 5 units, clamp to [0, max]
    const float speedMin = 0.0f;
    const float speedMax = OssmBleIsMode() ? 100.0f : speedlimit;
    speed = speed - 5.0f;
    if (speed < speedMin) speed = speedMin;
    if (speed > speedMax) speed = speedMax;

    // stroke: increase by 10 (points), clamp to [0, strokeMax]
    const float strokeMax = OssmBleIsMode() ? 100.0f : maxdepthinmm;
    stroke = stroke + 10.0f;
    if (stroke < 0.0f) stroke = 0.0f;
    if (stroke > strokeMax) stroke = strokeMax;

    // When opening Stroke screen, set depth so the rail is centered under the
    // requested stroke: depth = (max/2) - (stroke/2). Send DEPTH to OSSM.
    {
      float depthVal = (strokeMax / 2.0f) + (stroke / 2.0f);
      if (depthVal < 0.0f) depthVal = 0.0f;
      if (depthVal > strokeMax) depthVal = strokeMax;
      depth = depthVal;
      SendCommand(DEPTH, depth, OSSM_ID);
    }
    LogDebugFormatted("Initial Depth set to: %f\n", depth);

    // sensation: reduce by 0, clamp to [-100,100]
    sensation = sensation - 0.0f;
    if (sensation < -100.0f) sensation = -100.0f;
    if (sensation > 100.0f) sensation = 100.0f;

    // Update sliders to reflect new values
    lv_slider_set_value(s_strokespeedslider, (int)(speed + 0.5f), LV_ANIM_OFF);
    lv_slider_set_value(s_strokestrokeslider, (int)(stroke + 0.5f), LV_ANIM_OFF);
    // Sensation slider is symmetrical and may be negative
    lv_slider_set_value(s_strokesensationslider, (int)(sensation + 0.5f), LV_ANIM_OFF);
  }

  // Bottom control buttons (left, middle, right) - mirror Home layout
  s_StrokeButtonL = lv_btn_create(ui_Stroke);
  lv_obj_set_width(s_StrokeButtonL, 100);
  lv_obj_set_height(s_StrokeButtonL, 30);
  lv_obj_set_y(s_StrokeButtonL, 100);
  lv_obj_set_x(s_StrokeButtonL, lv_pct(-33));
  lv_obj_set_align(s_StrokeButtonL, LV_ALIGN_CENTER);
  lv_obj_add_flag(s_StrokeButtonL, LV_OBJ_FLAG_CHECKABLE);
  lv_obj_add_flag(s_StrokeButtonL, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
  lv_obj_clear_flag(s_StrokeButtonL, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(s_StrokeButtonL, homebuttonLevent, LV_EVENT_SHORT_CLICKED, NULL);
  lv_obj_add_event_cb(s_StrokeButtonL, homebuttonLlongEvent, LV_EVENT_LONG_PRESSED, NULL);
  lv_obj_add_style(s_StrokeButtonL, &style_button_l, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_style(s_StrokeButtonL, &style_button_l, LV_PART_MAIN | LV_STATE_FOCUSED);

  s_StrokeButtonLText = lv_label_create(s_StrokeButtonL);
  lv_obj_set_width(s_StrokeButtonLText, LV_SIZE_CONTENT);
  lv_obj_set_height(s_StrokeButtonLText, LV_SIZE_CONTENT);
  lv_obj_set_align(s_StrokeButtonLText, LV_ALIGN_CENTER);
  lv_label_set_text(s_StrokeButtonLText, T_MENU);
  lv_obj_add_style(s_StrokeButtonLText, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

  s_StrokeButtonM = lv_btn_create(ui_Stroke);
  lv_obj_set_width(s_StrokeButtonM, 100);
  lv_obj_set_height(s_StrokeButtonM, 30);
  lv_obj_set_y(s_StrokeButtonM, 100);
  lv_obj_set_x(s_StrokeButtonM, lv_pct(0));
  lv_obj_set_align(s_StrokeButtonM, LV_ALIGN_CENTER);
  lv_obj_add_flag(s_StrokeButtonM, LV_OBJ_FLAG_CHECKABLE);
  lv_obj_add_flag(s_StrokeButtonM, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
  lv_obj_clear_flag(s_StrokeButtonM, LV_OBJ_FLAG_SCROLLABLE);
  // Use stroke-specific UI button event that calls the shared action but
  // ensures Stroke button visuals are updated — hook the generic event
  // handler (homebuttonmevent) so behavior matches Home.
  lv_obj_add_event_cb(s_StrokeButtonM, homebuttonmevent, LV_EVENT_SHORT_CLICKED, NULL);
  lv_obj_add_event_cb(s_StrokeButtonM, homebuttonMlongEvent, LV_EVENT_LONG_PRESSED, NULL);
  lv_obj_add_style(s_StrokeButtonM, &style_button_m, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_style(s_StrokeButtonM, &style_button_m, LV_PART_MAIN | LV_STATE_FOCUSED);

  s_StrokeButtonMText = lv_label_create(s_StrokeButtonM);
  lv_obj_set_width(s_StrokeButtonMText, LV_SIZE_CONTENT);
  lv_obj_set_height(s_StrokeButtonMText, LV_SIZE_CONTENT);
  lv_obj_set_align(s_StrokeButtonMText, LV_ALIGN_CENTER);
  lv_label_set_text(s_StrokeButtonMText, T_START);
  lv_obj_add_style(s_StrokeButtonMText, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);


  s_StrokeButtonR = lv_btn_create(ui_Stroke);
  lv_obj_set_width(s_StrokeButtonR, 100);
  lv_obj_set_height(s_StrokeButtonR, 30);
  lv_obj_set_y(s_StrokeButtonR, 100);
  lv_obj_set_x(s_StrokeButtonR, lv_pct(33));
  lv_obj_set_align(s_StrokeButtonR, LV_ALIGN_CENTER);
  lv_obj_add_flag(s_StrokeButtonR, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
  lv_obj_clear_flag(s_StrokeButtonR, LV_OBJ_FLAG_SCROLLABLE);
  // Short click: go to Pattern screen; Long press: invoke existing long handler
  lv_obj_add_event_cb(s_StrokeButtonR, s_stroke_btn_r_short_cb, LV_EVENT_SHORT_CLICKED, NULL);
  lv_obj_add_event_cb(s_StrokeButtonR, homebuttonRlongEvent, LV_EVENT_LONG_PRESSED, NULL);
  lv_obj_add_style(s_StrokeButtonR, &style_button_r, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_style(s_StrokeButtonR, &style_button_r, LV_PART_MAIN | LV_STATE_FOCUSED);

  s_StrokeButtonRText = lv_label_create(s_StrokeButtonR);
  lv_obj_set_width(s_StrokeButtonRText, LV_SIZE_CONTENT);
  lv_obj_set_height(s_StrokeButtonRText, LV_SIZE_CONTENT);
  lv_obj_set_align(s_StrokeButtonRText, LV_ALIGN_CENTER);
  lv_label_set_text(s_StrokeButtonRText, T_SCREEN_PATTERN);
  lv_obj_add_style(s_StrokeButtonRText, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

  // Add stroke screen buttons to the global menu group so encoder focus works
  if (ui_g_menu != nullptr) {
    lv_group_add_obj(ui_g_menu, s_StrokeButtonL);
    lv_group_add_obj(ui_g_menu, s_StrokeButtonM);
    lv_group_add_obj(ui_g_menu, s_StrokeButtonR);
  }
}

// Per-loop handler — mirror Home behavior but without Depth handling.
void handleStrokeScreen(const ButtonEvents &events)
{
  static float last_speed_for_toggle = 0.0f;
  static float last_stroke_for_toggle = 0.0f;
  static int lastHomeTransportMode = -1;
  int currentTransportMode = OssmBleIsMode() ? 1 : 0;
  if (lastHomeTransportMode != currentTransportMode) {
    if (currentTransportMode == 1) {
      lv_slider_set_range(s_strokespeedslider, 0, 100);
      lv_slider_set_range(s_strokestrokeslider, 0, 100);
    } else {
      lv_slider_set_range(s_strokespeedslider, 0, speedlimit);
      lv_slider_set_range(s_strokestrokeslider, 0, maxdepthinmm);
    }
    if (speed  > lv_slider_get_max_value(s_strokespeedslider))
      speed  = lv_slider_get_max_value(s_strokespeedslider);
    if (stroke > lv_slider_get_max_value(s_strokestrokeslider))
      stroke = lv_slider_get_max_value(s_strokestrokeslider);
    lastHomeTransportMode = currentTransportMode;
  }

  if (lv_obj_has_state(ui_lefty, LV_STATE_CHECKED) == 1) {
    touch_disabled = true;
  }

  bool changed = false;
  // Encoder 1 – Speed
  if (lv_slider_is_dragged(s_strokespeedslider) == false) {
    changed = false;
    lv_slider_set_value(s_strokespeedslider, speed, LV_ANIM_OFF);

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
  } else if (lv_slider_get_value(s_strokespeedslider) != speed) {
    speed = lv_slider_get_value(s_strokespeedslider);
    SendCommand(SPEED, speed, OSSM_ID);
  }
  char speed_v[12];
  if (OssmBleIsMode()) {
    snprintf(speed_v, sizeof(speed_v), "%d", (int)(speed + 0.5f));
  } else {
    dtostrf(speed, 6, 0, speed_v);
  }
  lv_label_set_text(s_strokespeedvalue, speed_v);

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

   // Encoder 2 – stroke
  if (lv_slider_is_dragged(s_strokestrokeslider) == false) {
    changed = false;
    lv_slider_set_value(s_strokestrokeslider, stroke, LV_ANIM_OFF);

    long strokeCount   = encoder2.getCount();
    int  strokeDetents = (int)(strokeCount / 4);
    int  strokeRem = (int)(strokeCount - strokeDetents * 4);
    if (strokeRem < 0) { strokeRem += 4; strokeDetents -= 1; }
    if (strokeDetents != 0) {
      changed = true;
      LogDebugFormatted("ENC2 raw=%ld det=%d stroke_before=%f\n", strokeCount, strokeDetents, stroke);
      stroke += getRampedDetentDelta(2, strokeDetents);
      encoder2.setCount(strokeRem);
      screensaver_check_activity();
    }
    if (stroke < 0) { changed = true; stroke = 0; }
    const float strokeMax = OssmBleIsMode() ? 100.0f : maxdepthinmm;
    if (stroke > strokeMax) { changed = true; stroke = strokeMax; }
    if (changed) {
      SendCommand(STROKE, stroke, OSSM_ID);
      float depthVal = (strokeMax/2.0f) + (stroke / 2.0f);
      if (depthVal < 0.0f) depthVal = 0.0f;
      if (depthVal > strokeMax) depthVal = strokeMax;
      SendCommand(DEPTH, depthVal, OSSM_ID);
    }
  } else if (lv_slider_get_value(s_strokestrokeslider) != stroke) {
    stroke = lv_slider_get_value(s_strokestrokeslider);
      SendCommand(STROKE, stroke, OSSM_ID);
      const float strokeMax = OssmBleIsMode() ? 100.0f : maxdepthinmm;
      float depthVal = (strokeMax/2.0f) + (stroke / 2.0f);
      if (depthVal < 0.0f) depthVal = 0.0f;
      if (depthVal > strokeMax) depthVal = strokeMax;
      SendCommand(DEPTH, depthVal, OSSM_ID);
  }
  char stroke_v[12];
  if (OssmBleIsMode()) {
    snprintf(stroke_v, sizeof(stroke_v), "%d", (int)(stroke + 0.5f));
  } else {
    dtostrf(stroke, 6, 0, stroke_v);
  }
  lv_label_set_text(s_strokestrokevalue, stroke_v);

  // Detect 0 -> non-zero and non-zero -> 0 transitions to auto start/pause OSSM.
  {
    float prev = last_stroke_for_toggle;
    float cur = stroke;
    bool crossed_to_nonzero = (prev <= 0.0f && cur > 0.0f);
    bool crossed_to_zero = (prev > 0.0f && cur <= 0.0f);

    if (crossed_to_nonzero) {
      homebuttonm_action(false);
    } else if (crossed_to_zero) {
      homebuttonm_action(false);
    }
    last_stroke_for_toggle = cur;
  }


  // Encoder 4 – Sensation
  if (lv_slider_is_dragged(s_strokesensationslider) == false) {
    changed = false;
    lv_slider_set_value(s_strokesensationslider, sensation, LV_ANIM_OFF);
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
  } else if (lv_slider_get_value(s_strokesensationslider) != sensation) {
    sensation = lv_slider_get_value(s_strokesensationslider);
    SendCommand(SENSATION, sensation, OSSM_ID);
  }
  // Button dispatch
  if (events.leftShort) {
    homebuttonLevent(nullptr);
    clearButtonFlags();
  } else if (events.leftLong) {
    bool launched = triggerAddonForSlot(ADDON_SLOT_LEFT);
    LogDebugFormatted("STROKE leftLong -> triggerAddonForSlot(LEFT) result=%s\n", launched ? "OK" : "NONE");
    clearButtonFlags();
  } else if (events.mxShort) {
    LogDebug("mx: ST_UI_STROKE -> direct StrokeButtonM action");
    strokebuttonm_action(true);
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

// Ramping helper is provided by screen.cpp via encoder_ramp.h

