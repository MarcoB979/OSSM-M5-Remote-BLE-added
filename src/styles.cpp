#include "styles.h"
#include "colors.h"
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

lv_style_t style_title_bar;
lv_style_t style_button_l;
lv_style_t style_button_m;
lv_style_t style_button_r;
lv_style_t style_button_running;
lv_style_t style_button_stopped;
lv_style_t style_button_l_pressed;
lv_style_t style_button_m_pressed;
lv_style_t style_button_r_pressed;
lv_style_t style_button_running_pressed;
lv_style_t style_button_stopped_pressed;
lv_style_t style_slider_track[4];
lv_style_t style_slider_indicator[4];
lv_style_t style_battery_main;
lv_style_t style_battery_indicator;
lv_style_t style_roller;
lv_style_t style_roller_main;
lv_style_t style_background;
lv_style_t style_text_primary;
lv_style_t style_text_secondary;
lv_style_t style_button_l_focused;
lv_style_t style_button_m_focused;
lv_style_t style_button_r_focused;
lv_style_t style_button_l_disabled;
lv_style_t style_button_m_disabled;
lv_style_t style_button_r_disabled;
lv_style_t style_checkbox_indicator;
lv_style_t style_option_bg;

static lv_color_t mix_with_white(uint32_t hex, uint8_t mix_amount) {
    return lv_color_mix(lv_color_hex(hex), lv_color_hex(0xFFFFFF), mix_amount);
}
static lv_color_t mix_with_black(uint32_t hex, uint8_t mix_amount) {
    return lv_color_mix(lv_color_hex(hex), lv_color_hex(0x000000), mix_amount);
}

void styles_init(void) {
    lv_style_init(&style_title_bar);
    lv_style_init(&style_button_l);
    lv_style_init(&style_button_m);
    lv_style_init(&style_button_r);
    lv_style_init(&style_button_running);
    lv_style_init(&style_button_stopped);
    lv_style_init(&style_button_l_pressed);
    lv_style_init(&style_button_m_pressed);
    lv_style_init(&style_button_r_pressed);
    lv_style_init(&style_button_running_pressed);
    lv_style_init(&style_button_stopped_pressed);
    lv_style_init(&style_button_l_focused);
    lv_style_init(&style_button_m_focused);
    lv_style_init(&style_button_r_focused);
    lv_style_init(&style_button_l_disabled);
    lv_style_init(&style_button_m_disabled);
    lv_style_init(&style_button_r_disabled);
    lv_style_init(&style_option_bg);
    for (int i = 0; i < 4; ++i) {
        lv_style_init(&style_slider_track[i]);
        lv_style_init(&style_slider_indicator[i]);
    }
    lv_style_init(&style_battery_main);
    lv_style_init(&style_battery_indicator);
    lv_style_init(&style_roller);
    lv_style_init(&style_roller_main);
    lv_style_init(&style_background);
    lv_style_init(&style_text_primary);
    lv_style_init(&style_text_secondary);
    lv_style_init(&style_checkbox_indicator);

    // Populate styles with the active scheme
    styles_apply_scheme(g_active_color_scheme);
}

void styles_apply_scheme(int index) {
    if (index < 0 || index >= COLOR_SCHEME_COUNT) index = 0;

    // Title / header outline
    lv_style_set_outline_color(&style_title_bar, lv_color_hex(COLOR_SCHEMES[index].title_bar));
    lv_style_set_outline_width(&style_title_bar, 2);
    lv_style_set_outline_opa(&style_title_bar, 255);
    lv_style_set_outline_pad(&style_title_bar, 5);

    // Buttons — primary background
    lv_style_set_bg_color(&style_button_l, lv_color_hex(COLOR_SCHEMES[index].button_l));
    lv_style_set_bg_opa(&style_button_l, 255);

    lv_style_set_bg_color(&style_button_m, lv_color_hex(COLOR_SCHEMES[index].button_m));
    lv_style_set_bg_opa(&style_button_m, 255);

    lv_style_set_bg_color(&style_button_r, lv_color_hex(COLOR_SCHEMES[index].button_r));
    lv_style_set_bg_opa(&style_button_r, 255);

    // Running / stopped button states (fixed semantic colors)
    lv_style_set_bg_color(&style_button_running, lv_color_hex(0xB3261E));
    lv_style_set_bg_opa(&style_button_running, 255);

    lv_style_set_bg_color(&style_button_stopped, lv_color_hex(0x228B22));
    lv_style_set_bg_opa(&style_button_stopped, 255);

    // Pressed states: slightly darker
    lv_style_set_bg_color(&style_button_l_pressed, mix_with_black(COLOR_SCHEMES[index].button_l, 128));
    lv_style_set_bg_opa(&style_button_l_pressed, 255);

    lv_style_set_bg_color(&style_button_m_pressed, mix_with_black(COLOR_SCHEMES[index].button_m, 128));
    lv_style_set_bg_opa(&style_button_m_pressed, 255);

    lv_style_set_bg_color(&style_button_r_pressed, mix_with_black(COLOR_SCHEMES[index].button_r, 128));
    lv_style_set_bg_opa(&style_button_r_pressed, 255);

    lv_style_set_bg_color(&style_button_running_pressed, mix_with_black(0xB3261E, 128));
    lv_style_set_bg_opa(&style_button_running_pressed, 255);

    lv_style_set_bg_color(&style_button_stopped_pressed, mix_with_black(0x228B22, 128));
    lv_style_set_bg_opa(&style_button_stopped_pressed, 255);

    // Focused states: use the scheme's focussed_element accent
    uint32_t focus_col = COLOR_SCHEMES[index].focussed_element;
    lv_style_set_bg_color(&style_button_l_focused, lv_color_hex(focus_col));
    lv_style_set_bg_opa(&style_button_l_focused, 255);

    lv_style_set_bg_color(&style_button_m_focused, lv_color_hex(focus_col));
    lv_style_set_bg_opa(&style_button_m_focused, 255);

    lv_style_set_bg_color(&style_button_r_focused, lv_color_hex(focus_col));
    lv_style_set_bg_opa(&style_button_r_focused, 255);

    // Disabled states: much darker than primary
    lv_style_set_bg_color(&style_button_l_disabled, mix_with_black(COLOR_SCHEMES[index].button_l, 192));
    lv_style_set_bg_opa(&style_button_l_disabled, 255);

    lv_style_set_bg_color(&style_button_m_disabled, mix_with_black(COLOR_SCHEMES[index].button_m, 192));
    lv_style_set_bg_opa(&style_button_m_disabled, 255);

    lv_style_set_bg_color(&style_button_r_disabled, mix_with_black(COLOR_SCHEMES[index].button_r, 192));
    lv_style_set_bg_opa(&style_button_r_disabled, 255);

    // Sliders: track (lighter), indicator (saturated)
    uint32_t slider_vals[4] = {
        COLOR_SCHEMES[index].slider1,
        COLOR_SCHEMES[index].slider2,
        COLOR_SCHEMES[index].slider3,
        COLOR_SCHEMES[index].slider4,
    };
    for (int i = 0; i < 4; ++i) {
        // track: very light tint of the slider color
        lv_style_set_bg_color(&style_slider_track[i], mix_with_white(slider_vals[i], 95));
        lv_style_set_bg_opa(&style_slider_track[i], 255);

        // indicator: full saturated color
        lv_style_set_bg_color(&style_slider_indicator[i], mix_with_white(slider_vals[i], 255));
        lv_style_set_bg_grad_color(&style_slider_indicator[i], mix_with_white(slider_vals[i], 255));
        lv_style_set_bg_opa(&style_slider_indicator[i], 255);
    }

    // Battery
    lv_style_set_bg_color(&style_battery_main, lv_color_hex(COLOR_SCHEMES[index].battery_main));
    lv_style_set_bg_opa(&style_battery_main, 255);

    lv_style_set_bg_color(&style_battery_indicator, lv_color_hex(COLOR_SCHEMES[index].battery_indicator));
    lv_style_set_bg_opa(&style_battery_indicator, 255);

    // Roller
    lv_style_set_bg_color(&style_roller, lv_color_hex(COLOR_SCHEMES[index].roller));
    lv_style_set_bg_opa(&style_roller, 255);

    lv_style_set_bg_color(&style_roller_main, mix_with_white(COLOR_SCHEMES[index].roller, 160));
    lv_style_set_bg_opa(&style_roller_main, 255);

    // Background
    lv_style_set_bg_color(&style_background, lv_color_hex(COLOR_SCHEMES[index].background));
    lv_style_set_bg_opa(&style_background, 255);

    // Text color: auto-computed from background luminance for readable contrast
    uint32_t bg_hex = COLOR_SCHEMES[index].background;
    uint8_t r = (bg_hex >> 16) & 0xFF;
    uint8_t g = (bg_hex >> 8)  & 0xFF;
    uint8_t b =  bg_hex        & 0xFF;
    float lum = 0.299f * r + 0.587f * g + 0.114f * b;
    lv_color_t primary_text = (lum < 128.0f) ? lv_color_white() : lv_color_black();

    lv_style_set_text_color(&style_text_primary, primary_text);
    lv_style_set_text_opa(&style_text_primary, 255);

    lv_style_set_text_color(&style_text_secondary, primary_text);
    lv_style_set_text_opa(&style_text_secondary, 255);

    lv_style_set_border_color(&style_checkbox_indicator, lv_color_hex(COLOR_SCHEMES[index].battery_indicator));
    lv_style_set_border_opa(&style_checkbox_indicator, 255);

    // Option background: follows the scheme background for dark/light consistency
    lv_style_set_bg_color(&style_option_bg, lv_color_hex(COLOR_SCHEMES[index].background));
    lv_style_set_bg_opa(&style_option_bg, 255);
}

#ifdef __cplusplus
}
#endif
