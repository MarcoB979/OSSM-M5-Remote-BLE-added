#include "x-toys.h"

#include "lvgl.h"
#include "language.h"
#include "ui/ui_helpers.h"
#include "ui/ui.h"
#include "styles.h"
#include "colors.h"
#include "OssmBLE.h"
#include "main.h"
#include "screen.h"

static lv_obj_t * s_screen = nullptr;
static lv_obj_t * s_btn_tl = nullptr;
static lv_obj_t * s_btn_tr = nullptr;
static lv_obj_t * s_btn_left = nullptr;
static lv_obj_t * s_btn_mid = nullptr;
static lv_obj_t * s_btn_right = nullptr;
static lv_obj_t * s_slider1 = nullptr;
static lv_obj_t * s_slider2 = nullptr;
static lv_obj_t * s_slider1_label = nullptr;
static lv_obj_t * s_slider2_label = nullptr;

static void on_btn_tl(lv_event_t * e)
{
  (void)e;
  OssmBleGoToStrokeEngine();
}

static void on_btn_tr(lv_event_t * e)
{
  (void)e;
  OssmBleGoToStreaming();
}

static void do_start_action()
{
  if (s_btn_tl && lv_obj_has_state(s_btn_tl, LV_STATE_FOCUSED)) {
    OssmBleGoToStrokeEngine();
    return;
  }
  if (s_btn_tr && lv_obj_has_state(s_btn_tr, LV_STATE_FOCUSED)) {
    OssmBleGoToStreaming();
    return;
  }
  // default
  OssmBleGoToStreaming();
}

static void on_btn_right(lv_event_t * e)
{
  (void)e;
  do_start_action();
}

static void on_btn_left(lv_event_t * e)
{
  (void)e;
  _ui_screen_change(ui_Menu, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
}

static void toggle_sliders_visibility()
{
  if (!s_slider1 || !s_slider2) return;
  bool vis = (lv_obj_has_flag(s_slider1, LV_OBJ_FLAG_HIDDEN) == false);
  if (vis) {
    lv_obj_add_flag(s_slider1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_slider2, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_slider1_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_slider2_label, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_clear_flag(s_slider1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s_slider2, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s_slider1_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s_slider2_label, LV_OBJ_FLAG_HIDDEN);
  }
}

static void on_btn_mid(lv_event_t * e)
{
  (void)e;
  toggle_sliders_visibility();
}

static void on_slider1(lv_event_t * e)
{
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_VALUE_CHANGED) {
    int v = lv_slider_get_value(s_slider1);
    SendCommand(SPEED, (float)v, OSSM_ID);
    if (s_slider1_label) {
      char buf[16];
      snprintf(buf, sizeof(buf), "%d", v);
      lv_label_set_text(s_slider1_label, buf);
    }
  }
}

static void on_slider2(lv_event_t * e)
{
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_VALUE_CHANGED) {
    int v = lv_slider_get_value(s_slider2);
    SendCommand(STROKE, (float)v, OSSM_ID);
    if (s_slider2_label) {
      char buf[16];
      snprintf(buf, sizeof(buf), "%d", v);
      lv_label_set_text(s_slider2_label, buf);
    }
  }
}

static void createScreenIfNeeded()
{
  if (s_screen) return;

  s_screen = lv_obj_create(NULL);
  lv_obj_clear_flag(s_screen, LV_OBJ_FLAG_SCROLLABLE);

  // Top-left button
  s_btn_tl = lv_btn_create(s_screen);
  lv_obj_set_width(s_btn_tl, 150);
  lv_obj_set_height(s_btn_tl, 94);
  lv_obj_set_y(s_btn_tl, -25);
  lv_obj_set_x(s_btn_tl, -81);
  lv_obj_set_align(s_btn_tl, LV_ALIGN_CENTER);
  lv_obj_add_flag(s_btn_tl, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
  lv_obj_clear_flag(s_btn_tl, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(s_btn_tl, on_btn_tl, LV_EVENT_SHORT_CLICKED, NULL);
  lv_obj_add_style(s_btn_tl, &style_slider_indicator[0], LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_style(s_btn_tl, &style_button_m, LV_PART_MAIN | LV_STATE_FOCUSED);
  lv_obj_add_style(s_btn_tl, &style_button_m_disabled, LV_PART_MAIN | LV_STATE_DISABLED);
  lv_obj_set_style_radius(s_btn_tl, 8, LV_PART_MAIN | LV_STATE_DEFAULT);

  lv_obj_t * tl_text = lv_label_create(s_btn_tl);
  lv_obj_set_align(tl_text, LV_ALIGN_CENTER);
  lv_label_set_text(tl_text, "Speed\nMode");
  lv_obj_add_style(tl_text, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(tl_text, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(tl_text, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

  // Top-right button
  s_btn_tr = lv_btn_create(s_screen);
  lv_obj_set_width(s_btn_tr, 150);
  lv_obj_set_height(s_btn_tr, 94);
  lv_obj_set_y(s_btn_tr, -25);
  lv_obj_set_x(s_btn_tr, 81);
  lv_obj_set_align(s_btn_tr, LV_ALIGN_CENTER);
  lv_obj_add_flag(s_btn_tr, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
  lv_obj_clear_flag(s_btn_tr, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(s_btn_tr, on_btn_tr, LV_EVENT_SHORT_CLICKED, NULL);
  lv_obj_add_style(s_btn_tr, &style_slider_indicator[1], LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_style(s_btn_tr, &style_button_m, LV_PART_MAIN | LV_STATE_FOCUSED);
  lv_obj_add_style(s_btn_tr, &style_button_m_disabled, LV_PART_MAIN | LV_STATE_DISABLED);
  lv_obj_set_style_radius(s_btn_tr, 8, LV_PART_MAIN | LV_STATE_DEFAULT);

  lv_obj_t * tr_text = lv_label_create(s_btn_tr);
  lv_obj_set_align(tr_text, LV_ALIGN_CENTER);
  lv_label_set_text(tr_text, T_STREAMING "\n" T_STREAMING_SUB);
  lv_obj_add_style(tr_text, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(tr_text, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(tr_text, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

  // Left/back button
  s_btn_left = lv_btn_create(s_screen);
  lv_obj_set_width(s_btn_left, 150);
  lv_obj_set_height(s_btn_left, 44);
  lv_obj_set_y(s_btn_left, 50);
  lv_obj_set_x(s_btn_left, -81);
  lv_obj_set_align(s_btn_left, LV_ALIGN_CENTER);
  lv_obj_add_flag(s_btn_left, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
  lv_obj_clear_flag(s_btn_left, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(s_btn_left, on_btn_left, LV_EVENT_SHORT_CLICKED, NULL);
  lv_obj_add_style(s_btn_left, &style_slider_indicator[2], LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_style(s_btn_left, &style_button_l, LV_PART_MAIN | LV_STATE_FOCUSED);
  lv_obj_add_style(s_btn_left, &style_button_l_disabled, LV_PART_MAIN | LV_STATE_DISABLED);

  lv_obj_t * left_text = lv_label_create(s_btn_left);
  lv_label_set_text(left_text, T_BACK);
  lv_obj_add_style(left_text, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

  // Middle button toggles sliders
  s_btn_mid = lv_btn_create(s_screen);
  lv_obj_set_width(s_btn_mid, 150);
  lv_obj_set_height(s_btn_mid, 44);
  lv_obj_set_y(s_btn_mid, 50);
  lv_obj_set_x(s_btn_mid, 0);
  lv_obj_set_align(s_btn_mid, LV_ALIGN_CENTER);
  lv_obj_add_event_cb(s_btn_mid, on_btn_mid, LV_EVENT_SHORT_CLICKED, NULL);
  lv_obj_add_style(s_btn_mid, &style_slider_indicator[3], LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_style(s_btn_mid, &style_button_m, LV_PART_MAIN | LV_STATE_FOCUSED);
  lv_obj_add_style(s_btn_mid, &style_button_m_disabled, LV_PART_MAIN | LV_STATE_DISABLED);

  lv_obj_t * mid_text = lv_label_create(s_btn_mid);
  lv_label_set_text(mid_text, "TUNES");
  lv_obj_add_style(mid_text, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

  // Right / START button
  s_btn_right = lv_btn_create(s_screen);
  lv_obj_set_width(s_btn_right, 150);
  lv_obj_set_height(s_btn_right, 44);
  lv_obj_set_y(s_btn_right, 50);
  lv_obj_set_x(s_btn_right, 81);
  lv_obj_set_align(s_btn_right, LV_ALIGN_CENTER);
  lv_obj_add_event_cb(s_btn_right, on_btn_right, LV_EVENT_SHORT_CLICKED, NULL);
  lv_obj_add_style(s_btn_right, &style_slider_indicator[0], LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_style(s_btn_right, &style_button_r, LV_PART_MAIN | LV_STATE_FOCUSED);
  lv_obj_add_style(s_btn_right, &style_button_r_disabled, LV_PART_MAIN | LV_STATE_DISABLED);

  lv_obj_t * right_text = lv_label_create(s_btn_right);
  lv_label_set_text(right_text, "START");
  lv_obj_add_style(right_text, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

  // Sliders (hidden by default)
  s_slider1 = lv_slider_create(s_screen);
  lv_obj_set_width(s_slider1, 240);
  lv_obj_set_height(s_slider1, 20);
  lv_obj_set_align(s_slider1, LV_ALIGN_CENTER);
  lv_obj_set_y(s_slider1, 100);
  lv_slider_set_range(s_slider1, 0, 100);
  lv_slider_set_value(s_slider1, 50, LV_ANIM_OFF);
  lv_obj_add_event_cb(s_slider1, on_slider1, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_style(s_slider1, &style_slider_indicator[2], LV_PART_INDICATOR | LV_STATE_DEFAULT);
  lv_obj_add_style(s_slider1, &style_slider_indicator[2], LV_PART_KNOB | LV_STATE_DEFAULT);
  lv_obj_add_flag(s_slider1, LV_OBJ_FLAG_HIDDEN);

  s_slider1_label = lv_label_create(s_screen);
  lv_obj_set_align(s_slider1_label, LV_ALIGN_CENTER);
  lv_obj_set_y(s_slider1_label, 120);
  lv_label_set_text_fmt(s_slider1_label, "%d", 50);
  lv_obj_add_style(s_slider1_label, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_flag(s_slider1_label, LV_OBJ_FLAG_HIDDEN);

  s_slider2 = lv_slider_create(s_screen);
  lv_obj_set_width(s_slider2, 240);
  lv_obj_set_height(s_slider2, 20);
  lv_obj_set_align(s_slider2, LV_ALIGN_CENTER);
  lv_obj_set_y(s_slider2, 148);
  lv_slider_set_range(s_slider2, 0, 100);
  lv_slider_set_value(s_slider2, 50, LV_ANIM_OFF);
  lv_obj_add_event_cb(s_slider2, on_slider2, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_style(s_slider2, &style_slider_indicator[3], LV_PART_INDICATOR | LV_STATE_DEFAULT);
  lv_obj_add_style(s_slider2, &style_slider_indicator[3], LV_PART_KNOB | LV_STATE_DEFAULT);
  lv_obj_add_flag(s_slider2, LV_OBJ_FLAG_HIDDEN);

  s_slider2_label = lv_label_create(s_screen);
  lv_obj_set_align(s_slider2_label, LV_ALIGN_CENTER);
  lv_obj_set_y(s_slider2_label, 168);
  lv_label_set_text_fmt(s_slider2_label, "%d", 50);
  lv_obj_add_style(s_slider2_label, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_flag(s_slider2_label, LV_OBJ_FLAG_HIDDEN);
}

static void refreshTheme()
{
  if (s_screen == nullptr) return;

  lv_obj_add_style(s_screen, &style_background, LV_PART_MAIN | LV_STATE_DEFAULT);

  if (s_btn_tl) {
    lv_obj_add_style(s_btn_tl, &style_slider_indicator[0], LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(s_btn_tl, &style_button_m, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_add_style(s_btn_tl, &style_button_m_disabled, LV_PART_MAIN | LV_STATE_DISABLED);
  }

  if (s_btn_tr) {
    lv_obj_add_style(s_btn_tr, &style_slider_indicator[1], LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(s_btn_tr, &style_button_m, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_add_style(s_btn_tr, &style_button_m_disabled, LV_PART_MAIN | LV_STATE_DISABLED);
  }

  if (s_btn_left) {
    lv_obj_add_style(s_btn_left, &style_slider_indicator[2], LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(s_btn_left, &style_button_l, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_add_style(s_btn_left, &style_button_l_disabled, LV_PART_MAIN | LV_STATE_DISABLED);
  }

  if (s_btn_mid) {
    lv_obj_add_style(s_btn_mid, &style_slider_indicator[3], LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(s_btn_mid, &style_button_m, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_add_style(s_btn_mid, &style_button_m_disabled, LV_PART_MAIN | LV_STATE_DISABLED);
  }

  if (s_btn_right) {
    lv_obj_add_style(s_btn_right, &style_slider_indicator[0], LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(s_btn_right, &style_button_r, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_add_style(s_btn_right, &style_button_r_disabled, LV_PART_MAIN | LV_STATE_DISABLED);
  }

  if (s_slider1) {
    lv_obj_add_style(s_slider1, &style_slider_indicator[2], LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_add_style(s_slider1, &style_slider_indicator[2], LV_PART_KNOB | LV_STATE_DEFAULT);
  }
  if (s_slider2) {
    lv_obj_add_style(s_slider2, &style_slider_indicator[3], LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_add_style(s_slider2, &style_slider_indicator[3], LV_PART_KNOB | LV_STATE_DEFAULT);
  }
}

void XtoysPrepareScreen()
{
  createScreenIfNeeded();
  refreshTheme();
}

lv_obj_t * XtoysGetScreen()
{
  createScreenIfNeeded();
  return s_screen;
}
