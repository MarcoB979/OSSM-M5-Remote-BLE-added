#include "styles.h"
#include "colors.h"

#ifdef __cplusplus
extern "C" {
#endif

lv_style_t style_title_bar;
lv_style_t style_button_l;
lv_style_t style_button_m;
lv_style_t style_button_r;
lv_style_t style_slider_track[4];
lv_style_t style_slider_indicator[4];
lv_style_t style_battery_main;
lv_style_t style_battery_indicator;
lv_style_t style_roller;
lv_style_t style_background;
lv_style_t style_text_primary;
lv_style_t style_text_secondary;

static lv_color_t mix_with_white(uint32_t hex, uint8_t mix_amount) {
    return lv_color_mix(lv_color_hex(hex), lv_color_hex(0xFFFFFF), mix_amount);
}

void styles_init() {
    // Initialize all styles with neutral defaults — they'll be updated by
    // styles_apply_scheme() which is called from applyColorScheme().
    lv_style_init(&style_title_bar);
    lv_style_init(&style_button_l);
    lv_style_init(&style_button_m);
    lv_style_init(&style_button_r);
    for (int i = 0; i < 4; ++i) {
        lv_style_init(&style_slider_track[i]);
        lv_style_init(&style_slider_indicator[i]);
    }
    lv_style_init(&style_battery_main);
    lv_style_init(&style_battery_indicator);
    lv_style_init(&style_roller);
    lv_style_init(&style_background);
    lv_style_init(&style_text_primary);
    lv_style_init(&style_text_secondary);

    // Apply current active scheme to populate styles now.
    styles_apply_scheme(g_active_color_scheme);
}

void styles_apply_scheme(int index) {
    if (index < 0 || index >= COLOR_SCHEME_COUNT) index = 0;

    // Title / header outline
    lv_style_set_outline_color(&style_title_bar, lv_color_hex(COLOR_SCHEMES[index].title_bar));
    lv_style_set_outline_width(&style_title_bar, 2);
    lv_style_set_outline_opa(&style_title_bar, 255);
    lv_style_set_outline_pad(&style_title_bar, 5);

    // Buttons (primary background)
    lv_style_set_bg_color(&style_button_l, lv_color_hex(COLOR_SCHEMES[index].button_l));
    lv_style_set_bg_opa(&style_button_l, 255);

    lv_style_set_bg_color(&style_button_m, lv_color_hex(COLOR_SCHEMES[index].button_m));
    lv_style_set_bg_opa(&style_button_m, 255);

    lv_style_set_bg_color(&style_button_r, lv_color_hex(COLOR_SCHEMES[index].button_r));
    lv_style_set_bg_opa(&style_button_r, 255);

    // Sliders: define a track (lighter) and indicator (saturated) style per-slot
    uint32_t slider_vals[4] = {
        COLOR_SCHEMES[index].slider1,
        COLOR_SCHEMES[index].slider2,
        COLOR_SCHEMES[index].slider3,
        COLOR_SCHEMES[index].slider4,
    };
    for (int i = 0; i < 4; ++i) {
        lv_style_set_bg_color(&style_slider_track[i], mix_with_white(slider_vals[i], 64));
        lv_style_set_bg_opa(&style_slider_track[i], 255);

        lv_style_set_bg_color(&style_slider_indicator[i], lv_color_hex(slider_vals[i]));
        lv_style_set_bg_grad_color(&style_slider_indicator[i], lv_color_hex(slider_vals[i]));
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

    // Background and text
    lv_style_set_bg_color(&style_background, lv_color_hex(COLOR_SCHEMES[index].background));
    lv_style_set_bg_opa(&style_background, 255);

    lv_style_set_text_color(&style_text_primary, lv_color_hex(COLOR_SCHEMES[index].text_primary));
    lv_style_set_text_opa(&style_text_primary, 255);

    lv_style_set_text_color(&style_text_secondary, lv_color_hex(COLOR_SCHEMES[index].text_secondary));
    lv_style_set_text_opa(&style_text_secondary, 255);
}

#ifdef __cplusplus
}
#endif
