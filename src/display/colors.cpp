// colors.cpp — UI Color Scheme system for M5_Remote
//
// Owns:
//   • The LVGL "Color Schemes" selector screen (ui_Colors)
//   • NVS persistence for the active scheme index
//   • applyColorScheme() — re-styles every themed widget at runtime
//   • Carousel focus helpers for ScreenHandler.cpp encoder/button handling
//
// Input handling (encoder + buttons) for the Colors screen lives in
// ScreenHandler.cpp (case ST_UI_COLORS), keeping it consistent with
// the rest of M5_Remote's screen architecture.

#include <Arduino.h>
#include <Preferences.h>

#include "display/colors.h"
#include "display/styles.h"
#include "language.h"
#include "ui/ui.h"
#include "ui/ui_helpers.h"

// `COLOR_SCHEMES` is defined in src/ColorSchemes.cpp
int g_active_color_scheme = 0;

// ---------------------------------------------------------------------------
// Colors screen LVGL objects (defined here, extern'd in ui.h)
// ---------------------------------------------------------------------------
lv_obj_t   *ui_Colors   = nullptr;
lv_group_t *ui_g_colors = nullptr;

// Battery widget for colors screen (slot 9 — referenced by ScreenHandler battery arrays)
lv_obj_t *ui_Batt9      = nullptr;
lv_obj_t *ui_BattValue9 = nullptr;
lv_obj_t *ui_Battery9   = nullptr;

static constexpr int VISIBLE_SCHEME_COUNT = 5;
static lv_obj_t *s_schemeBtn[VISIBLE_SCHEME_COUNT] = {};
static lv_obj_t *s_schemeLbl[VISIBLE_SCHEME_COUNT] = {};
static int s_visibleToScheme[VISIBLE_SCHEME_COUNT]  = {0, 1, 2, 3, 4};
static int s_focus_scheme_index                     = 0;
static lv_obj_t *s_backBtn                          = nullptr;
static lv_obj_t *s_colorsLogo                       = nullptr;

static int wrapSchemeIndex(int idx) {
    while (idx < 0)                 idx += COLOR_SCHEME_COUNT;
    while (idx >= COLOR_SCHEME_COUNT) idx -= COLOR_SCHEME_COUNT;
    return idx;
}

static int schemeForVisibleSlot(int slot) {
    if (COLOR_SCHEME_COUNT <= VISIBLE_SCHEME_COUNT) {
        if (slot < COLOR_SCHEME_COUNT) return slot;
        return -1;
    }
    const int start = s_focus_scheme_index - (VISIBLE_SCHEME_COUNT / 2);
    return wrapSchemeIndex(start + slot);
}

static void updateVisibleSchemeButtons() {
    for (int slot = 0; slot < VISIBLE_SCHEME_COUNT; ++slot) {
        if (!s_schemeBtn[slot] || !s_schemeLbl[slot]) continue;
        const int scheme = schemeForVisibleSlot(slot);
        s_visibleToScheme[slot] = scheme;

        if (scheme < 0) {
            lv_obj_add_flag(s_schemeBtn[slot], LV_OBJ_FLAG_HIDDEN);
            continue;
        } else {
            lv_obj_clear_flag(s_schemeBtn[slot], LV_OBJ_FLAG_HIDDEN);
        }

        lv_obj_set_style_bg_color(s_schemeBtn[slot],
            lv_color_hex(COLOR_SCHEMES[scheme].title_bar),
            LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(s_schemeBtn[slot],
            lv_color_hex(COLOR_SCHEMES[scheme].button_m),
            LV_PART_MAIN | LV_STATE_FOCUSED);

        char buf[48];
        if (scheme == g_active_color_scheme) {
            snprintf(buf, sizeof(buf), "%s  " LV_SYMBOL_OK, COLOR_SCHEMES[scheme].name);
        } else {
            snprintf(buf, sizeof(buf), "%s", COLOR_SCHEMES[scheme].name);
        }

        const int centerSlot = VISIBLE_SCHEME_COUNT / 2;
        if (slot == centerSlot) {
            lv_obj_set_style_border_color(s_schemeBtn[slot], lv_color_white(),
                LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(s_schemeBtn[slot], lv_color_white(),
                LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_obj_set_style_border_width(s_schemeBtn[slot], 2,
                LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(s_schemeBtn[slot], 2,
                LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_obj_set_style_border_opa(s_schemeBtn[slot], 255,
                LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_opa(s_schemeBtn[slot], 255,
                LV_PART_MAIN | LV_STATE_FOCUSED);
        } else {
            lv_obj_set_style_border_width(s_schemeBtn[slot], 0,
                LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_opa(s_schemeBtn[slot], 0,
                LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        lv_label_set_text(s_schemeLbl[slot], buf);
    }

    if (ui_g_colors && s_schemeBtn[VISIBLE_SCHEME_COUNT / 2]) {
        lv_group_focus_obj(s_schemeBtn[VISIBLE_SCHEME_COUNT / 2]);
    }
}

// ---------------------------------------------------------------------------
// Internal restyle helpers (null-safe — ok if widgets not yet created)
// ---------------------------------------------------------------------------
static void rs_btnPrimary(lv_obj_t *btn, uint32_t primary) {
    if (!btn) return;
    if (primary == COLOR_SCHEMES[g_active_color_scheme].button_l) {
        lv_obj_add_style(btn, &style_button_l, LV_PART_MAIN | LV_STATE_DEFAULT);
        return;
    }
    if (primary == COLOR_SCHEMES[g_active_color_scheme].button_m) {
        lv_obj_add_style(btn, &style_button_m, LV_PART_MAIN | LV_STATE_DEFAULT);
        return;
    }
    if (primary == COLOR_SCHEMES[g_active_color_scheme].button_r) {
        lv_obj_add_style(btn, &style_button_r, LV_PART_MAIN | LV_STATE_DEFAULT);
        return;
    }
    lv_obj_set_style_bg_color(btn, lv_color_hex(primary), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa  (btn, 255,                   LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void rs_btnFocused(lv_obj_t *btn, uint32_t secondary) {
    if (!btn) return;
    if (secondary == COLOR_SCHEMES[g_active_color_scheme].button_l) {
        lv_obj_add_style(btn, &style_button_l, LV_PART_MAIN | LV_STATE_FOCUSED); return;
    }
    if (secondary == COLOR_SCHEMES[g_active_color_scheme].button_m) {
        lv_obj_add_style(btn, &style_button_m, LV_PART_MAIN | LV_STATE_FOCUSED); return;
    }
    if (secondary == COLOR_SCHEMES[g_active_color_scheme].button_r) {
        lv_obj_add_style(btn, &style_button_r, LV_PART_MAIN | LV_STATE_FOCUSED); return;
    }
    lv_obj_set_style_bg_color(btn, lv_color_hex(secondary), LV_PART_MAIN | LV_STATE_FOCUSED);
}

static void rs_outline(lv_obj_t *obj, uint32_t /*primary*/) {
    if (!obj) return;
    lv_obj_add_style(obj, &style_title_bar, LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void rs_slider(lv_obj_t *sl, uint32_t primary) {
    if (!sl) return;
    uint32_t vals[4] = {
        COLOR_SCHEMES[g_active_color_scheme].slider1,
        COLOR_SCHEMES[g_active_color_scheme].slider2,
        COLOR_SCHEMES[g_active_color_scheme].slider3,
        COLOR_SCHEMES[g_active_color_scheme].slider4,
    };
    int found = -1;
    for (int i = 0; i < 4; ++i) {
        if (primary == vals[i]) { found = i; break; }
    }
    if (found >= 0) {
        lv_obj_add_style(sl, &style_slider_track[found],     LV_PART_MAIN      | LV_STATE_DEFAULT);
        lv_obj_add_style(sl, &style_slider_indicator[found], LV_PART_INDICATOR | LV_STATE_DEFAULT);
        lv_obj_add_style(sl, &style_slider_indicator[found], LV_PART_KNOB      | LV_STATE_DEFAULT);
        return;
    }
    // Fallback: manual color set
    lv_obj_set_style_bg_color(sl,
        lv_color_mix(lv_color_hex(primary), lv_color_hex(0xFFFFFF), 64),
        LV_PART_MAIN      | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa  (sl, 255,                   LV_PART_MAIN      | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(sl, lv_color_hex(primary), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa  (sl, 255,                   LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(sl, lv_color_hex(primary), LV_PART_KNOB      | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa  (sl, 255,                   LV_PART_KNOB      | LV_STATE_DEFAULT);
}

// ---------------------------------------------------------------------------
// Apply a color scheme to the entire UI
// ---------------------------------------------------------------------------
void applyColorScheme(int index) {
    if (index < 0 || index >= COLOR_SCHEME_COUNT) index = 0;
    g_active_color_scheme = index;

    // 1. Re-initialise the LVGL default theme
    lv_disp_t *dispp = lv_disp_get_default();
    if (dispp) {
        lv_theme_t *theme = lv_theme_default_init(
            dispp,
            lv_color_hex(COLOR_SCHEMES[index].title_bar),
            lv_color_hex(COLOR_SCHEMES[index].slider1),
            true,            // dark mode
            LV_FONT_DEFAULT);
        lv_disp_set_theme(dispp, theme);
    }

    // Update shared styles
    styles_apply_scheme(index);

    // 2. Buttons — existing screens
    // Start screen
    rs_btnPrimary(ui_StartButtonL, COLOR_SCHEMES[index].button_l);
    rs_btnPrimary(ui_StartButtonM, COLOR_SCHEMES[index].button_m);
    rs_btnPrimary(ui_StartButtonR, COLOR_SCHEMES[index].button_r);

    // Home screen (L + R only — M is start/stop, styled separately)
    rs_btnPrimary(ui_HomeButtonL, COLOR_SCHEMES[index].button_l);
    rs_btnPrimary(ui_HomeButtonR, COLOR_SCHEMES[index].button_r);

    // Pattern screen
    rs_btnPrimary(ui_PatternButtonL, COLOR_SCHEMES[index].button_l);
    rs_btnPrimary(ui_PatternButtonM, COLOR_SCHEMES[index].button_m);
    rs_btnPrimary(ui_PatternButtonR, COLOR_SCHEMES[index].button_r);

    // EJECT settings screen
    rs_btnPrimary(ui_EJECTButtonL, COLOR_SCHEMES[index].button_l);
    rs_btnPrimary(ui_EJECTButtonM, COLOR_SCHEMES[index].button_m);
    rs_btnPrimary(ui_EJECTButtonR, COLOR_SCHEMES[index].button_r);

    // Settings screen
    rs_btnPrimary(ui_SettingsButtonL, COLOR_SCHEMES[index].button_l);
    rs_btnPrimary(ui_SettingsButtonM, COLOR_SCHEMES[index].button_m);
    rs_btnPrimary(ui_SettingsButtonR, COLOR_SCHEMES[index].button_r);

    // Torque screen
    rs_btnPrimary(ui_TorqeButtonL, COLOR_SCHEMES[index].button_l);
    rs_btnPrimary(ui_TorqeButtonM, COLOR_SCHEMES[index].button_m);
    rs_btnPrimary(ui_TorqeButtonR, COLOR_SCHEMES[index].button_r);

    // 3. New screens — null-safe for screens not yet created
    // Menu screen bottom buttons
    rs_btnPrimary(ui_MenuButtonL, COLOR_SCHEMES[index].button_l);
    rs_btnPrimary(ui_MenuButtonM, COLOR_SCHEMES[index].button_m);
    rs_btnPrimary(ui_MenuButtonR, COLOR_SCHEMES[index].button_r);

    // Menu screen big tiles: assign slider accent colors
    if (ui_MenuButtonTL) {
        lv_obj_add_style(ui_MenuButtonTL, &style_slider_indicator[0], LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_style(ui_MenuButtonTL, &style_button_m, LV_PART_MAIN | LV_STATE_FOCUSED);
        lv_obj_add_style(ui_MenuButtonTL, &style_button_m_disabled, LV_PART_MAIN | LV_STATE_DISABLED);
    }
    if (ui_MenuButtonTR) {
        lv_obj_add_style(ui_MenuButtonTR, &style_slider_indicator[1], LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_style(ui_MenuButtonTR, &style_button_m, LV_PART_MAIN | LV_STATE_FOCUSED);
        lv_obj_add_style(ui_MenuButtonTR, &style_button_m_disabled, LV_PART_MAIN | LV_STATE_DISABLED);
    }
    if (ui_MenuButtonML) {
        lv_obj_add_style(ui_MenuButtonML, &style_slider_indicator[2], LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_style(ui_MenuButtonML, &style_button_m, LV_PART_MAIN | LV_STATE_FOCUSED);
        lv_obj_add_style(ui_MenuButtonML, &style_button_m_disabled, LV_PART_MAIN | LV_STATE_DISABLED);
    }
    if (ui_MenuButtonMR) {
        lv_obj_add_style(ui_MenuButtonMR, &style_slider_indicator[3], LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_style(ui_MenuButtonMR, &style_button_m, LV_PART_MAIN | LV_STATE_FOCUSED);
        lv_obj_add_style(ui_MenuButtonMR, &style_button_m_disabled, LV_PART_MAIN | LV_STATE_DISABLED);
    }

    // Tile focused state uses the scheme's focussed_element accent
    uint32_t focusAccent = COLOR_SCHEMES[index].focussed_element;
    if (ui_MenuButtonTL) rs_btnFocused(ui_MenuButtonTL, focusAccent);
    if (ui_MenuButtonTR) rs_btnFocused(ui_MenuButtonTR, focusAccent);
    if (ui_MenuButtonML) rs_btnFocused(ui_MenuButtonML, focusAccent);
    if (ui_MenuButtonMR) rs_btnFocused(ui_MenuButtonMR, focusAccent);

    // Streaming screen (placeholder widgets — null-safe)
    rs_btnPrimary(ui_StreamingButtonL, COLOR_SCHEMES[index].button_l);
    rs_btnPrimary(ui_StreamingButtonR, COLOR_SCHEMES[index].button_r);

    // Addons screen (placeholder widgets — null-safe)
    rs_btnPrimary(ui_AddonsButtonL, COLOR_SCHEMES[index].button_l);
    rs_btnPrimary(ui_AddonsButtonM, COLOR_SCHEMES[index].button_m);
    rs_btnPrimary(ui_AddonsButtonR, COLOR_SCHEMES[index].button_r);
    rs_btnPrimary(ui_AddonsItem0, COLOR_SCHEMES[index].button_l);
    rs_btnPrimary(ui_AddonsItem1, COLOR_SCHEMES[index].button_l);
    rs_btnPrimary(ui_AddonsItem2, COLOR_SCHEMES[index].button_l);

    // Colors screen back button
    rs_btnPrimary(s_backBtn, COLOR_SCHEMES[index].button_l);

    // 4. Sliders
    rs_slider(ui_homespeedslider,      COLOR_SCHEMES[index].slider1);
    rs_slider(ui_homedepthslider,      COLOR_SCHEMES[index].slider2);
    rs_slider(ui_homestrokeslider,     COLOR_SCHEMES[index].slider3);
    rs_slider(ui_homesensationslider,  COLOR_SCHEMES[index].slider4);

    // Streaming sliders (null-safe)
    rs_slider(ui_streamingspeedslider,   COLOR_SCHEMES[index].slider1);
    rs_slider(ui_streamingdepthslider,   COLOR_SCHEMES[index].slider2);
    rs_slider(ui_streamingstrokeslider,  COLOR_SCHEMES[index].slider3);

    // Torque sliders
    rs_slider(ui_outtroqeslider, COLOR_SCHEMES[index].slider1);
    rs_slider(ui_introqeslider,  COLOR_SCHEMES[index].slider2);

    // 5. Pattern roller
    if (ui_PatternS) {
        lv_obj_set_style_bg_opa(ui_PatternS, LV_OPA_TRANSP,
            LV_PART_MAIN     | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(ui_PatternS, LV_OPA_TRANSP,
            LV_PART_SELECTED | LV_STATE_DEFAULT);
    }

    // 6. Logo / header outlines
    lv_obj_t *logos[] = {
        ui_Logo,  ui_Logo2, ui_Logo1,
        ui_Logo5, ui_Logo6, ui_Logo4,
        ui_LogoMenu, ui_LogoStreaming, ui_LogoAddons,
        s_colorsLogo,
    };
    for (lv_obj_t *logo : logos) {
        rs_outline(logo, COLOR_SCHEMES[index].title_bar);
    }

    // 7. Refresh Colors screen selection indicators
    updateVisibleSchemeButtons();

    // 8. Refresh Colors screen check marks
    colorSchemeScreenLoaded();
}

// ---------------------------------------------------------------------------
// NVS persistence
// ---------------------------------------------------------------------------
void colors_init() {
    Preferences pref;
    pref.begin("m5-ctnr", true);
    int saved = pref.getInt("ColorScheme", 0);
    pref.end();
    if (saved < 0 || saved >= COLOR_SCHEME_COUNT) saved = 0;
    applyColorScheme(saved);
}

static void saveColorScheme(int index) {
    Preferences pref;
    pref.begin("m5-ctnr", false);
    pref.putInt("ColorScheme", index);
    pref.end();
}

// ---------------------------------------------------------------------------
// Color getters (extern "C" so C translation units can call them)
// ---------------------------------------------------------------------------
extern "C" uint32_t getActivePrimaryColor()          { return COLOR_SCHEMES[g_active_color_scheme].title_bar; }
extern "C" uint32_t getActiveSecondaryColor()        { return COLOR_SCHEMES[g_active_color_scheme].slider1;   }
extern "C" uint32_t getActiveTitleBarColor(void)     { return COLOR_SCHEMES[g_active_color_scheme].title_bar; }
extern "C" uint32_t getActiveButtonLColor(void)      { return COLOR_SCHEMES[g_active_color_scheme].button_l;  }
extern "C" uint32_t getActiveButtonMColor(void)      { return COLOR_SCHEMES[g_active_color_scheme].button_m;  }
extern "C" uint32_t getActiveButtonRColor(void)      { return COLOR_SCHEMES[g_active_color_scheme].button_r;  }
extern "C" uint32_t getActiveSlider1Color(void)      { return COLOR_SCHEMES[g_active_color_scheme].slider1;   }
extern "C" uint32_t getActiveSlider2Color(void)      { return COLOR_SCHEMES[g_active_color_scheme].slider2;   }
extern "C" uint32_t getActiveSlider3Color(void)      { return COLOR_SCHEMES[g_active_color_scheme].slider3;   }
extern "C" uint32_t getActiveSlider4Color(void)      { return COLOR_SCHEMES[g_active_color_scheme].slider4;   }
extern "C" uint32_t getActiveBatteryMainColor(void)  { return COLOR_SCHEMES[g_active_color_scheme].battery_main; }
extern "C" uint32_t getActiveBatteryIndicatorColor(void) { return COLOR_SCHEMES[g_active_color_scheme].battery_indicator; }
extern "C" uint32_t getActiveRollerColor(void)       { return COLOR_SCHEMES[g_active_color_scheme].roller;    }
extern "C" uint32_t getActiveBackgroundColor(void)   { return COLOR_SCHEMES[g_active_color_scheme].background;}
extern "C" uint32_t getActiveTextPrimaryColor(void)  { return COLOR_SCHEMES[g_active_color_scheme].text_primary;   }
extern "C" uint32_t getActiveTextSecondaryColor(void){ return COLOR_SCHEMES[g_active_color_scheme].text_secondary; }

// ---------------------------------------------------------------------------
// Public API (extern "C")
// ---------------------------------------------------------------------------
extern "C" void colorSchemeScreenLoaded() {
    updateVisibleSchemeButtons();
}

extern "C" void colorSchemeSelectIndex(int index) {
    if (index < 0 || index >= COLOR_SCHEME_COUNT) return;
    saveColorScheme(index);
    applyColorScheme(index);
}

// Carousel helpers for ScreenHandler.cpp
void colorsScrollFocus(int delta) {
    s_focus_scheme_index = wrapSchemeIndex(s_focus_scheme_index + delta);
    updateVisibleSchemeButtons();
}

int colorsGetFocusIndex(void) {
    return s_focus_scheme_index;
}

// ---------------------------------------------------------------------------
// Colors screen: LVGL event handlers
// ---------------------------------------------------------------------------
static void ev_schemeBtn(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_SHORT_CLICKED) return;
    lv_obj_t *target = (lv_obj_t *)lv_event_get_target(e);
    for (int slot = 0; slot < VISIBLE_SCHEME_COUNT; slot++) {
        if (target == s_schemeBtn[slot]) {
            s_focus_scheme_index = s_visibleToScheme[slot];
            colorSchemeSelectIndex(s_focus_scheme_index);
            return;
        }
    }
}

static void ev_backBtn(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_SHORT_CLICKED) {
        _ui_screen_change(ui_Menu, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    }
}

static void ev_selectBtn(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_SHORT_CLICKED) {
        colorSchemeSelectIndex(s_focus_scheme_index);
    }
}

static void ev_colorsScreen(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_SCREEN_LOADED) {
        colorSchemeScreenLoaded();
        screenmachine(e);
    }
}

// ---------------------------------------------------------------------------
// Colors screen: LVGL construction
// ---------------------------------------------------------------------------
extern "C" void colors_ui_screen_init() {
    ui_Colors = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Colors, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(ui_Colors, ev_colorsScreen, LV_EVENT_ALL, NULL);

    // Header logo
    s_colorsLogo = lv_label_create(ui_Colors);
    lv_obj_set_width(s_colorsLogo,  LV_SIZE_CONTENT);
    lv_obj_set_height(s_colorsLogo, LV_SIZE_CONTENT);
    lv_obj_set_y(s_colorsLogo, -103);
    lv_obj_set_align(s_colorsLogo, LV_ALIGN_CENTER);
    lv_label_set_text(s_colorsLogo, T_SCREEN_COLORS);
    lv_obj_set_style_text_font(s_colorsLogo, &lv_font_montserrat_16,
        LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(s_colorsLogo, &style_title_bar, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Scheme selection buttons (carousel, max VISIBLE_SCHEME_COUNT visible)
    const int btnW  = 300;
    const int btnH  = 24;
    const int pitch = 28;    // 24 + 4 gap
    const int startY = -52;  // y of first button center (screen-relative)

    ui_g_colors = lv_group_create();
    s_focus_scheme_index = g_active_color_scheme;

    for (int slot = 0; slot < VISIBLE_SCHEME_COUNT; slot++) {
        const int yPos   = startY + slot * pitch;
        const int scheme = schemeForVisibleSlot(slot);
        s_visibleToScheme[slot] = scheme;

        lv_obj_t *btn = lv_btn_create(ui_Colors);
        lv_obj_set_width (btn, btnW);
        lv_obj_set_height(btn, btnH);
        lv_obj_set_y(btn, yPos);
        lv_obj_set_align(btn, LV_ALIGN_CENTER);
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_pad_all(btn, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius (btn, 6, LV_PART_MAIN | LV_STATE_DEFAULT);

        if (scheme < 0) {
            lv_obj_add_flag(btn, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_set_style_bg_color(btn,
                lv_color_hex(COLOR_SCHEMES[scheme].title_bar),
                LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(btn, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(btn,
                lv_color_hex(COLOR_SCHEMES[scheme].button_m),
                LV_PART_MAIN | LV_STATE_FOCUSED);
        }

        lv_obj_add_event_cb(btn, ev_schemeBtn, LV_EVENT_SHORT_CLICKED, NULL);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_obj_set_align(lbl, LV_ALIGN_LEFT_MID);
        lv_obj_set_x(lbl, 8);
        lv_label_set_text(lbl, (scheme >= 0) ? COLOR_SCHEMES[scheme].name : "");
        lv_obj_add_style(lbl, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

        s_schemeBtn[slot] = btn;
        s_schemeLbl[slot] = lbl;
        lv_group_add_obj(ui_g_colors, btn);
    }

    // Back button (bottom-left)
    s_backBtn = lv_btn_create(ui_Colors);
    lv_obj_set_width (s_backBtn, 100);
    lv_obj_set_height(s_backBtn, 30);
    lv_obj_set_y(s_backBtn, 100);
    lv_obj_set_x(s_backBtn, lv_pct(-33));
    lv_obj_set_align(s_backBtn, LV_ALIGN_CENTER);
    lv_obj_clear_flag(s_backBtn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(s_backBtn, &style_button_l, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(s_backBtn, ev_backBtn, LV_EVENT_SHORT_CLICKED, NULL);

    lv_obj_t *backLbl = lv_label_create(s_backBtn);
    lv_obj_set_align(backLbl, LV_ALIGN_CENTER);
    lv_label_set_text(backLbl, T_BACK);
    lv_obj_add_style(backLbl, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ---- Select button (bottom-right, mirrors back button) ----
    lv_obj_t *s_selectBtn = lv_btn_create(ui_Colors);
    lv_obj_set_width (s_selectBtn, 100);
    lv_obj_set_height(s_selectBtn, 30);
    lv_obj_set_y(s_selectBtn, 100);
    lv_obj_set_x(s_selectBtn, lv_pct(33));
    lv_obj_set_align(s_selectBtn, LV_ALIGN_CENTER);
    lv_obj_clear_flag(s_selectBtn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(s_selectBtn, &style_button_r, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(s_selectBtn, ev_selectBtn, LV_EVENT_SHORT_CLICKED, NULL);

    lv_obj_t *selectLbl = lv_label_create(s_selectBtn);
    lv_obj_set_align(selectLbl, LV_ALIGN_CENTER);
    lv_label_set_text(selectLbl, T_SELECT);
    lv_obj_add_style(selectLbl, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ---- Battery widget (top-right, slot 9) ----
    ui_Batt9 = lv_label_create(ui_Colors);
    lv_obj_set_width(ui_Batt9, 85);
    lv_obj_set_height(ui_Batt9, 30);
    lv_obj_set_x(ui_Batt9, 115);
    lv_obj_set_y(ui_Batt9, -103);
    lv_obj_set_align(ui_Batt9, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Batt9, T_BATT);
    lv_obj_add_style(ui_Batt9, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_BattValue9 = lv_label_create(ui_Batt9);
    lv_obj_set_width(ui_BattValue9, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_BattValue9, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_BattValue9, 0);
    lv_obj_set_y(ui_BattValue9, -7);
    lv_obj_set_align(ui_BattValue9, LV_ALIGN_RIGHT_MID);
    lv_label_set_text(ui_BattValue9, T_BLANK);
    lv_obj_add_style(ui_BattValue9, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Battery9 = lv_bar_create(ui_Batt9);
    lv_bar_set_range(ui_Battery9, 0, 100);
    lv_obj_set_size(ui_Battery9, 75, 8);
    lv_obj_set_align(ui_Battery9, LV_ALIGN_BOTTOM_MID);
    lv_obj_add_style(ui_Battery9, &style_battery_main, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_Battery9, &style_battery_indicator, LV_PART_INDICATOR | LV_STATE_DEFAULT);
}
