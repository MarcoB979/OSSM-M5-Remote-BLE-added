// strokeMode.cpp — LVGL Stroke ("Bator mode") screen for M5_Remote
//
// This file creates the Stroke screen widgets and owns refreshStrokeStartStopUi().
// All encoder and button input logic for this screen is handled in
// ScreenHandler.cpp (case ST_UI_STROKE) to keep the architecture consistent
// with the rest of M5_Remote.

#include "ui/ui.h"
#include "ui/ui_helpers.h"
#include "language.h"
#include "strokeMode.h"
#include "main.h"
#include "display/styles.h"
#include "communication/EspNowComm.h"
#include "screens/ScreenHandler.h"

// ---------------------------------------------------------------------------
// Exported slider and value-label objects (used by ScreenHandler.cpp)
// ---------------------------------------------------------------------------
lv_obj_t *ui_StrokeSpeedSlider      = nullptr;
lv_obj_t *ui_StrokeStrokeSlider     = nullptr;
lv_obj_t *ui_StrokeSensationSlider  = nullptr;
lv_obj_t *ui_StrokeSpeedValue       = nullptr;
lv_obj_t *ui_StrokeStrokeValue      = nullptr;
lv_obj_t *ui_StrokeSensationValue   = nullptr;

// ---------------------------------------------------------------------------
// Exported screen objects declared extern in ui.h
// ---------------------------------------------------------------------------
// ui_Stroke is declared in ui.h and defined in ui.c — only SET here
// ui_StrokePatternLabel1 / ui_StrokePatternLabel: defined here, extern in ui.h
// ui_Batt7 / ui_BattValue7 / ui_Battery7: defined here, extern in ui.h
lv_obj_t *ui_StrokePatternLabel1 = nullptr;
lv_obj_t *ui_StrokePatternLabel  = nullptr;
lv_obj_t *ui_Batt7               = nullptr;
lv_obj_t *ui_BattValue7          = nullptr;
lv_obj_t *ui_Battery7            = nullptr;

// ---------------------------------------------------------------------------
// Private widget pointers (screen-internal only)
// ---------------------------------------------------------------------------
static lv_obj_t *s_Logo           = nullptr;
static lv_obj_t *s_SpeedL         = nullptr;
static lv_obj_t *s_StrokeL        = nullptr;
static lv_obj_t *s_SensationL     = nullptr;
static lv_obj_t *s_ButtonL        = nullptr;
static lv_obj_t *s_ButtonLText    = nullptr;
static lv_obj_t *s_ButtonM        = nullptr;
static lv_obj_t *s_ButtonMText    = nullptr;
static lv_obj_t *s_ButtonR        = nullptr;
static lv_obj_t *s_ButtonRText    = nullptr;

// ---------------------------------------------------------------------------
// Event helpers
// ---------------------------------------------------------------------------
static void ui_event_Stroke(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_SCREEN_LOADED) {
        if (s_Logo) lv_label_set_text(s_Logo, T_STROKE_SCREEN);
        refreshStrokeStartStopUi();
        screenmachine(e);
    }
}

static void s_stroke_btn_l_cb(lv_event_t * /*e*/) {
    _ui_screen_change(ui_Menu, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
}

static void s_stroke_btn_r_cb(lv_event_t * /*e*/) {
    _ui_screen_change(ui_Pattern, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
}

void refreshStrokeStartStopUi() {
    if (!s_ButtonM || !s_ButtonMText) return;
    if (OSSM_On) {
        lv_obj_add_style(s_ButtonM, &style_button_running, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(s_ButtonMText, T_STOP);
    } else {
        lv_obj_add_style(s_ButtonM, &style_button_stopped, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(s_ButtonMText, T_RESUME);
    }
}

// ---------------------------------------------------------------------------
// Slider helper: creates label + slider + value-label as a horizontal row
// ---------------------------------------------------------------------------
static lv_obj_t *createSliderRow(lv_obj_t *parent, const char *label_text, int yOffset,
                                 int rangeMin, int rangeMax, lv_slider_mode_t mode,
                                 lv_style_t *trackStyle, lv_style_t *indicatorStyle,
                                 int initialValue,
                                 lv_obj_t **out_slider, lv_obj_t **out_value) {
    lv_obj_t *row = lv_label_create(parent);
    lv_obj_set_width(row, lv_pct(95));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_x(row, 0);
    lv_obj_set_y(row, yOffset);
    lv_obj_set_align(row, LV_ALIGN_CENTER);
    lv_label_set_text(row, label_text);
    lv_obj_set_style_text_font(row, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *sl = lv_slider_create(row);
    lv_slider_set_range(sl, rangeMin, rangeMax);
    lv_slider_set_mode(sl, mode);
    lv_obj_set_width(sl, 130);
    lv_obj_set_height(sl, 25);
    lv_obj_set_x(sl, -15);
    lv_obj_set_align(sl, LV_ALIGN_RIGHT_MID);
    if (trackStyle)     lv_obj_add_style(sl, trackStyle,     LV_PART_MAIN      | LV_STATE_DEFAULT);
    if (indicatorStyle) lv_obj_add_style(sl, indicatorStyle, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    if (indicatorStyle) lv_obj_add_style(sl, indicatorStyle, LV_PART_KNOB      | LV_STATE_DEFAULT);
    lv_obj_set_style_width (sl, 12, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_height(sl, 12, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(sl, LV_RADIUS_CIRCLE, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_slider_set_value(sl, initialValue, LV_ANIM_OFF);

    lv_obj_t *val = lv_label_create(row);
    lv_obj_set_width(val, LV_SIZE_CONTENT);
    lv_obj_set_height(val, LV_SIZE_CONTENT);
    lv_obj_set_x(val, 80);
    lv_obj_set_y(val, 0);
    lv_obj_set_align(val, LV_ALIGN_LEFT_MID);
    lv_label_set_text(val, T_BLANK);

    if (out_slider) *out_slider = sl;
    if (out_value)  *out_value  = val;
    return row;
}

// ---------------------------------------------------------------------------
// Screen construction
// ---------------------------------------------------------------------------
void ui_Stroke_screen_init() {
    ui_Stroke = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Stroke, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(ui_Stroke, ui_event_Stroke, LV_EVENT_ALL, NULL);

    // ---- Header ----
    s_Logo = lv_label_create(ui_Stroke);
    lv_obj_set_width(s_Logo, LV_SIZE_CONTENT);
    lv_obj_set_height(s_Logo, LV_SIZE_CONTENT);
    lv_obj_set_y(s_Logo, -103);
    lv_obj_set_align(s_Logo, LV_ALIGN_CENTER);
    lv_label_set_text(s_Logo, T_STROKE_SCREEN);
    lv_obj_set_style_text_font(s_Logo, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(s_Logo, &style_title_bar, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ---- Sliders ----
    s_SpeedL = createSliderRow(
        ui_Stroke, T_SPEED, -40,
        0, (int)speedlimit, LV_SLIDER_MODE_NORMAL,
        &style_slider_track[0], &style_slider_indicator[0],
        (int)speed,
        &ui_StrokeSpeedSlider, &ui_StrokeSpeedValue);

    s_StrokeL = createSliderRow(
        ui_Stroke, T_STROKE, -4,
        0, (int)maxdepthinmm, LV_SLIDER_MODE_NORMAL,
        &style_slider_track[2], &style_slider_indicator[2],
        (int)stroke,
        &ui_StrokeStrokeSlider, &ui_StrokeStrokeValue);

    s_SensationL = createSliderRow(
        ui_Stroke, T_SENSATION, 35,
        -100, 100, LV_SLIDER_MODE_SYMMETRICAL,
        &style_slider_track[3], &style_slider_indicator[3],
        (int)sensation,
        &ui_StrokeSensationSlider, &ui_StrokeSensationValue);

    // Widen sensation slider a bit to show symmetrical mode properly
    if (ui_StrokeSensationSlider) lv_obj_set_width(ui_StrokeSensationSlider, 170);
    // Hide the numeric value label for sensation (not needed in bator mode)
    if (ui_StrokeSensationValue) lv_obj_add_flag(ui_StrokeSensationValue, LV_OBJ_FLAG_HIDDEN);

    // ---- Battery display (top-right, slot 7 — Stroke screen exclusive) ----
    ui_Batt7 = lv_label_create(ui_Stroke);
    lv_obj_set_width(ui_Batt7, 85);
    lv_obj_set_height(ui_Batt7, 30);
    lv_obj_set_x(ui_Batt7, 115);
    lv_obj_set_y(ui_Batt7, -103);
    lv_obj_set_align(ui_Batt7, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Batt7, T_BATT);
    lv_obj_add_style(ui_Batt7, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_BattValue7 = lv_label_create(ui_Batt7);
    lv_obj_set_width(ui_BattValue7, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_BattValue7, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_BattValue7, 0);
    lv_obj_set_y(ui_BattValue7, -7);
    lv_obj_set_align(ui_BattValue7, LV_ALIGN_RIGHT_MID);
    lv_label_set_text(ui_BattValue7, T_BLANK);
    lv_obj_add_style(ui_BattValue7, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Battery7 = lv_bar_create(ui_Batt7);
    lv_bar_set_range(ui_Battery7, 0, 100);
    lv_obj_set_width(ui_Battery7, 80);
    lv_obj_set_height(ui_Battery7, 10);
    lv_obj_set_x(ui_Battery7, 0);
    lv_obj_set_y(ui_Battery7, 10);
    lv_obj_set_align(ui_Battery7, LV_ALIGN_CENTER);
    lv_obj_add_style(ui_Battery7, &style_battery_main,      LV_PART_MAIN      | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_Battery7, &style_battery_indicator, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    // ---- Pattern label row (mirrors Home screen) ----
    ui_StrokePatternLabel1 = lv_label_create(ui_Stroke);
    lv_obj_set_width(ui_StrokePatternLabel1, lv_pct(95));
    lv_obj_set_height(ui_StrokePatternLabel1, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_StrokePatternLabel1, 10);
    lv_obj_set_y(ui_StrokePatternLabel1, 70);
    lv_obj_set_align(ui_StrokePatternLabel1, LV_ALIGN_LEFT_MID);
    lv_label_set_text(ui_StrokePatternLabel1, T_Patterns);
    lv_obj_set_style_text_font(ui_StrokePatternLabel1, &lv_font_montserrat_14,
        LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_StrokePatternLabel = lv_label_create(ui_StrokePatternLabel1);
    lv_obj_set_width(ui_StrokePatternLabel, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_StrokePatternLabel, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_StrokePatternLabel, 110);
    lv_obj_set_y(ui_StrokePatternLabel, 0);
    lv_obj_set_align(ui_StrokePatternLabel, LV_ALIGN_LEFT_MID);
    lv_label_set_text(ui_StrokePatternLabel, patternstr);

    // ---- Bottom control buttons ----
    // Left — back to Menu
    s_ButtonL = lv_btn_create(ui_Stroke);
    lv_obj_set_width(s_ButtonL, 100);
    lv_obj_set_height(s_ButtonL, 30);
    lv_obj_set_y(s_ButtonL, 100);
    lv_obj_set_x(s_ButtonL, lv_pct(-33));
    lv_obj_set_align(s_ButtonL, LV_ALIGN_CENTER);
    lv_obj_clear_flag(s_ButtonL, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(s_ButtonL, s_stroke_btn_l_cb, LV_EVENT_SHORT_CLICKED, NULL);
    lv_obj_add_style(s_ButtonL, &style_button_l, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(s_ButtonL, &style_button_l, LV_PART_MAIN | LV_STATE_FOCUSED);

    s_ButtonLText = lv_label_create(s_ButtonL);
    lv_obj_set_align(s_ButtonLText, LV_ALIGN_CENTER);
    lv_label_set_text(s_ButtonLText, T_MENU);
    lv_obj_add_style(s_ButtonLText, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Middle — Start/Stop
    s_ButtonM = lv_btn_create(ui_Stroke);
    lv_obj_set_width(s_ButtonM, 100);
    lv_obj_set_height(s_ButtonM, 30);
    lv_obj_set_y(s_ButtonM, 100);
    lv_obj_set_x(s_ButtonM, lv_pct(0));
    lv_obj_set_align(s_ButtonM, LV_ALIGN_CENTER);
    lv_obj_clear_flag(s_ButtonM, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(s_ButtonM, homebuttonmevent, LV_EVENT_SHORT_CLICKED, NULL);
    lv_obj_add_style(s_ButtonM, &style_button_m, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(s_ButtonM, &style_button_m, LV_PART_MAIN | LV_STATE_FOCUSED);

    s_ButtonMText = lv_label_create(s_ButtonM);
    lv_obj_set_align(s_ButtonMText, LV_ALIGN_CENTER);
    lv_label_set_text(s_ButtonMText, T_START);
    lv_obj_add_style(s_ButtonMText, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Right — Pattern screen
    s_ButtonR = lv_btn_create(ui_Stroke);
    lv_obj_set_width(s_ButtonR, 100);
    lv_obj_set_height(s_ButtonR, 30);
    lv_obj_set_y(s_ButtonR, 100);
    lv_obj_set_x(s_ButtonR, lv_pct(33));
    lv_obj_set_align(s_ButtonR, LV_ALIGN_CENTER);
    lv_obj_clear_flag(s_ButtonR, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(s_ButtonR, s_stroke_btn_r_cb, LV_EVENT_SHORT_CLICKED, NULL);
    lv_obj_add_style(s_ButtonR, &style_button_r, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(s_ButtonR, &style_button_r, LV_PART_MAIN | LV_STATE_FOCUSED);

    s_ButtonRText = lv_label_create(s_ButtonR);
    lv_obj_set_align(s_ButtonRText, LV_ALIGN_CENTER);
    lv_label_set_text(s_ButtonRText, T_SCREEN_PATTERN);
    lv_obj_add_style(s_ButtonRText, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);
}
