#pragma once

#include <lvgl.h>
#include <stdint.h>
// Initialize LVGL styles used across the app. Call after lv_init() and
// before creating UI screens so screens may add these styles to widgets.
#ifdef __cplusplus
extern "C" {
#endif

void styles_init();

// Update existing styles to match the given scheme index (0..COLOR_SCHEME_COUNT-1)
void styles_apply_scheme(int index);

// Expose style objects for explicit use in UI code (e.g., lv_obj_add_style)
extern lv_style_t style_title_bar;
extern lv_style_t style_button_l;
extern lv_style_t style_button_m;
extern lv_style_t style_button_r;
extern lv_style_t style_button_running;
extern lv_style_t style_button_stopped;
extern lv_style_t style_button_l_pressed;
extern lv_style_t style_button_m_pressed;
extern lv_style_t style_button_r_pressed;
extern lv_style_t style_button_running_pressed;
extern lv_style_t style_button_stopped_pressed;
extern lv_style_t style_slider_track[4];
extern lv_style_t style_slider_indicator[4];
extern lv_style_t style_battery_main;
extern lv_style_t style_battery_indicator;
extern lv_style_t style_roller;
extern lv_style_t style_background;
extern lv_style_t style_text_primary;
extern lv_style_t style_text_secondary;
extern lv_style_t style_button_l_focused;
extern lv_style_t style_button_m_focused;
extern lv_style_t style_button_r_focused;
extern lv_style_t style_button_l_disabled;
extern lv_style_t style_button_m_disabled;
extern lv_style_t style_button_r_disabled;
extern lv_style_t style_option_bg;

#ifdef __cplusplus
}
#endif
