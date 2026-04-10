// colors.cpp — UI Color Scheme system
//
// Owns:
//   • The COLOR_SCHEMES catalog (6 named schemes)
//   • The LVGL "Color Schemes" selector screen (ui_Colors)
//   • NVS persistence for the active scheme index
//   • applyColorScheme() — re-styles every themed widget at runtime
//   • handleColorsScreen() — per-loop input/encoder handler
//
// Only the absolute minimum (extern "C" declarations, one ui.c call, one
// screen.cpp dispatch line) lives outside this file.

#include <Arduino.h>
#include <Preferences.h>
#include <lvgl.h>

#include "config.h"
#include "language.h"
#include "colors.h"
#include "main.h"
#include "screen.h"
#include "ui/ui.h"
#include "ui/ui_helpers.h"

// ---------------------------------------------------------------------------
// Color scheme catalog
// ---------------------------------------------------------------------------

const UiColorScheme COLOR_SCHEMES[COLOR_SCHEME_COUNT] = {
    // name                primary      secondary
    { "Deep Purple",       0x83277B,    0xD591D5 },  // 0 — default (original OSSM)
    { "Midnight Navy",     0x1E3A6E,    0x7A9FCC },  // 1 — deep navy + periwinkle
    { "Forest Army",       0x3D5C3D,    0x8BA88B },  // 2 — army green dark + light, gray text
    { "Steel Blue",        0x2C5F7A,    0x6AABCC },  // 3 — steel + sky blue
    { "Amber Sunset",      0xB8860B,    0xFFD700 },  // 4 — dark amber + golden sunset
    { "Rainbow",           0x83277B,    0xD591D5 },  // 5 — rainbow sliders (uses primary as base)
};

int g_active_color_scheme = 0;

// ---------------------------------------------------------------------------
// Colors screen LVGL objects  (defined here, extern'd in ui.h)
// ---------------------------------------------------------------------------

lv_obj_t   *ui_Colors   = nullptr;
lv_group_t *ui_g_colors = nullptr;

static lv_obj_t *s_schemeBtn[COLOR_SCHEME_COUNT]   = {};
static lv_obj_t *s_schemeLbl[COLOR_SCHEME_COUNT]   = {};
static lv_obj_t *s_backBtn                         = nullptr;
static lv_obj_t *s_colorsLogo                      = nullptr;

// ---------------------------------------------------------------------------
// Internal restyle helpers
// ---------------------------------------------------------------------------

static void rs_btnPrimary(lv_obj_t *btn, uint32_t primary) {
    if (!btn) return;
    lv_obj_set_style_bg_color(btn, lv_color_hex(primary), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa  (btn, 255,                   LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void rs_btnFocused(lv_obj_t *btn, uint32_t secondary) {
    if (!btn) return;
    lv_obj_set_style_bg_color(btn, lv_color_hex(secondary), LV_PART_MAIN | LV_STATE_FOCUSED);
}

static void rs_outline(lv_obj_t *obj, uint32_t primary) {
    if (!obj) return;
    lv_obj_set_style_outline_color(obj, lv_color_hex(primary), LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void rs_slider(lv_obj_t *sl, uint32_t primary, uint32_t secondary) {
    if (!sl) return;
    lv_obj_set_style_bg_color     (sl, lv_color_hex(secondary), LV_PART_MAIN      | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa       (sl, 255,                      LV_PART_MAIN      | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color     (sl, lv_color_hex(primary),   LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa       (sl, 255,                      LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(sl, lv_color_hex(primary),   LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color     (sl, lv_color_hex(primary),   LV_PART_KNOB      | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa       (sl, 255,                      LV_PART_KNOB      | LV_STATE_DEFAULT);
}

// ---------------------------------------------------------------------------
// Apply a color scheme to the entire UI
// ---------------------------------------------------------------------------

void applyColorScheme(int index) {
    if (index < 0 || index >= COLOR_SCHEME_COUNT) index = 0;
    g_active_color_scheme = index;
    const uint32_t P = COLOR_SCHEMES[index].primary;
    const uint32_t S = COLOR_SCHEMES[index].secondary;

    // --- 1. Re-init the LVGL default theme ---
    lv_disp_t *dispp = lv_disp_get_default();
    if (dispp) {
        lv_theme_t *theme = lv_theme_default_init(
            dispp, lv_color_hex(P), lv_color_hex(S), dark_mode, LV_FONT_DEFAULT);
        lv_disp_set_theme(dispp, theme);
    }

    // --- 2. Buttons: primary bg (default state) ---
    // NOTE: HomeButtonM (the green start/stop) is intentionally excluded.
    lv_obj_t *accentBtns[] = {
        // Start screen
        ui_StartButtonL, ui_StartButtonM, ui_StartButtonR,
        // Home screen (L + R only, M is start/stop green)
        ui_HomeButtonL, ui_HomeButtonR,
        // Pattern screen
        ui_PatternButtonL, ui_PatternButtonM, ui_PatternButtonR,
        // Torque screen
        ui_TorqeButtonL, ui_TorqeButtonM, ui_TorqeButtonR,
        // Eject screen
        ui_EJECTButtonL, ui_EJECTButtonM, ui_EJECTButtonR,
        // Settings screen
        ui_SettingsButtonL, ui_SettingsButtonM, ui_SettingsButtonR,
        // Menu screen — bottom controls
        ui_MenuButtonL, ui_MenuButtonM, ui_MenuButtonR,
        // Streaming screen
        ui_StreamingButtonL, ui_StreamingButtonM, ui_StreamingButtonR,
        // Addons screen — nav buttons + items
        ui_AddonsButtonL, ui_AddonsButtonR,
        ui_AddonsItem0, ui_AddonsItem1, ui_AddonsItem2,
        // Colors screen — back button
        s_backBtn,
    };
    for (lv_obj_t *btn : accentBtns) {
        rs_btnPrimary(btn, P);
    }

    // --- 3. Menu big tiles: primary bg + secondary on focus ---
    lv_obj_t *menuTiles[] = {
        ui_MenuButtonTL, ui_MenuButtonTR, ui_MenuButtonML, ui_MenuButtonBL,
    };
    for (lv_obj_t *btn : menuTiles) {
        rs_btnPrimary(btn, P);
        rs_btnFocused(btn, S);
    }

    // --- 4. Addons items: focused state ---
    rs_btnFocused(ui_AddonsItem0, S);
    rs_btnFocused(ui_AddonsItem1, S);
    rs_btnFocused(ui_AddonsItem2, S);

    // --- 5. Sliders ---
    // Special handling for rainbow theme
    if (index == 5) {  // Rainbow theme
        // Rainbow slider colors: red, orange, yellow, green, cyan, blue, purple
        static const uint32_t rainbowColors[7] = {
            0xFF0000,  // red
            0xFF7F00,  // orange
            0xFFFF00,  // yellow
            0x00FF00,  // green
            0x00FFFF,  // cyan
            0x0000FF,  // blue
            0xFF00FF,  // purple
        };
        lv_obj_t *sliders[] = {
            ui_homespeedslider, ui_homedepthslider,
            ui_homestrokeslider, ui_homesensationslider,
            ui_outtroqeslider, ui_introqeslider,
            ui_streamingspeedslider, ui_streamingdepthslider,
            ui_streamingstrokeslider, ui_brightness_slider,
        };
        for (size_t i = 0; i < sizeof(sliders)/sizeof(sliders[0]) && i < 7; i++) {
            if (!sliders[i]) continue;
            uint32_t rainbowColor = rainbowColors[i];
            uint32_t rainbowLight = 0xFFFFFF;  // light version (will be slightly modified)
            // Blend the rainbow color with white for the light accent
            uint8_t r = ((rainbowColor >> 16) & 0xFF);
            uint8_t g = ((rainbowColor >> 8) & 0xFF);
            uint8_t b = (rainbowColor & 0xFF);
            uint32_t rainbowLighter = ((((r+0xFF)/2) & 0xFF) << 16) | ((((g+0xFF)/2) & 0xFF) << 8) | (((b+0xFF)/2) & 0xFF);
            rs_slider(sliders[i], rainbowColor, rainbowLighter);
        }
    } else {
        // Normal sliders with scheme colors
        lv_obj_t *sliders[] = {
            ui_homespeedslider, ui_homedepthslider,
            ui_homestrokeslider, ui_homesensationslider,
            ui_outtroqeslider, ui_introqeslider,
            ui_streamingspeedslider, ui_streamingdepthslider,
            ui_streamingstrokeslider, ui_brightness_slider,
        };
        for (lv_obj_t *sl : sliders) {
            rs_slider(sl, P, S);
        }
    }

    // --- 6. Pattern roller menu ---
    if (ui_PatternS) {
        lv_obj_set_style_bg_color(ui_PatternS, lv_color_hex(S), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa  (ui_PatternS, 255,            LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(ui_PatternS, lv_color_hex(P), LV_PART_SELECTED | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa  (ui_PatternS, 255,             LV_PART_SELECTED | LV_STATE_DEFAULT);
    }

    // --- 7. Logo / header outlines ---
    lv_obj_t *logos[] = {
        ui_Logo, ui_Logo2, ui_Logo1, ui_Logo4, ui_Logo5, ui_Logo6,
        ui_LogoMenu, ui_LogoStreaming, ui_LogoAddons,
        s_colorsLogo,
    };
    for (lv_obj_t *logo : logos) {
        rs_outline(logo, P);
    }

    // --- 8. Colors screen scheme buttons: each keeps its own scheme color ---
    for (int i = 0; i < COLOR_SCHEME_COUNT; i++) {
        if (!s_schemeBtn[i]) continue;
        lv_obj_set_style_bg_color(s_schemeBtn[i],
            lv_color_hex(COLOR_SCHEMES[i].primary), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(s_schemeBtn[i],
            lv_color_hex(COLOR_SCHEMES[i].secondary), LV_PART_MAIN | LV_STATE_FOCUSED);
    }

    // --- 9. Refresh Colors screen selection indicators ---
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
// Color getters (for external use, e.g., from main_helper.cpp)
// ---------------------------------------------------------------------------

extern "C" uint32_t getActivePrimaryColor() {
    if (g_active_color_scheme < 0 || g_active_color_scheme >= COLOR_SCHEME_COUNT) {
        return COLOR_SCHEMES[0].primary;
    }
    return COLOR_SCHEMES[g_active_color_scheme].primary;
}

extern "C" uint32_t getActiveSecondaryColor() {
    if (g_active_color_scheme < 0 || g_active_color_scheme >= COLOR_SCHEME_COUNT) {
        return COLOR_SCHEMES[0].secondary;
    }
    return COLOR_SCHEMES[g_active_color_scheme].secondary;
}

// ---------------------------------------------------------------------------
// Public API (extern "C" for ui.c compatibility)
// ---------------------------------------------------------------------------

extern "C" void colorSchemeScreenLoaded() {
    for (int i = 0; i < COLOR_SCHEME_COUNT; i++) {
        if (!s_schemeLbl[i]) continue;
        char buf[48];
        if (i == g_active_color_scheme) {
            snprintf(buf, sizeof(buf), "%s  " LV_SYMBOL_OK, COLOR_SCHEMES[i].name);
        } else {
            snprintf(buf, sizeof(buf), "%s", COLOR_SCHEMES[i].name);
        }
        lv_label_set_text(s_schemeLbl[i], buf);

        // Active scheme: white border highlight; others: no border
        if (s_schemeBtn[i]) {
            if (i == g_active_color_scheme) {
                lv_obj_set_style_border_color(s_schemeBtn[i],
                    lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_border_width(s_schemeBtn[i],
                    2, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_border_opa(s_schemeBtn[i],
                    255, LV_PART_MAIN | LV_STATE_DEFAULT);
            } else {
                lv_obj_set_style_border_width(s_schemeBtn[i],
                    0, LV_PART_MAIN | LV_STATE_DEFAULT);
            }
        }
    }
}

extern "C" void colorSchemeSelectIndex(int index) {
    if (index < 0 || index >= COLOR_SCHEME_COUNT) return;
    saveColorScheme(index);
    applyColorScheme(index);
    // Stay on the Colors screen so user can see the result; checkmarks refresh automatically.
}

// ---------------------------------------------------------------------------
// Per-loop input handler (called from screen.cpp handleCurrentScreen)
// ---------------------------------------------------------------------------

void handleColorsScreen() {
    // Encoder 4: scroll focus through scheme list
    if (encoder4.getCount() > encoder4_enc + 2) {
        if (ui_g_colors) lv_group_focus_next(ui_g_colors);
        encoder4_enc = encoder4.getCount();
    } else if (encoder4.getCount() < encoder4_enc - 2) {
        if (ui_g_colors) lv_group_focus_prev(ui_g_colors);
        encoder4_enc = encoder4.getCount();
    }

    // Left button: back to Addons
    if (clickLeft_short_waspressed) {
        _ui_screen_change(ui_Addons, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
        clickLeft_short_waspressed = false;
        return;
    }

    // MX / Right button: apply focused scheme
    if (mxclick_short_waspressed || clickRight_short_waspressed) {
        lv_obj_t *focused = (ui_g_colors != nullptr)
            ? lv_group_get_focused(ui_g_colors) : nullptr;
        if (focused) {
            for (int i = 0; i < COLOR_SCHEME_COUNT; i++) {
                if (focused == s_schemeBtn[i]) {
                    colorSchemeSelectIndex(i);
                    break;
                }
            }
        }
        mxclick_short_waspressed   = false;
        clickRight_short_waspressed = false;
    }
}

// ---------------------------------------------------------------------------
// Colors screen: LVGL event handlers
// ---------------------------------------------------------------------------

static void ev_schemeBtn(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    lv_obj_t *target = reinterpret_cast<lv_obj_t *>(lv_event_get_target(e));
    for (int i = 0; i < COLOR_SCHEME_COUNT; i++) {
        if (target == s_schemeBtn[i]) {
            colorSchemeSelectIndex(i);
            return;
        }
    }
}

static void ev_backBtn(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        _ui_screen_change(ui_Addons, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
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
    // ── Screen container ────────────────────────────────────────────────────
    ui_Colors = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Colors, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(ui_Colors, ev_colorsScreen, LV_EVENT_ALL, NULL);

    // ── Header logo ─────────────────────────────────────────────────────────
    s_colorsLogo = lv_label_create(ui_Colors);
    lv_obj_set_width(s_colorsLogo,  LV_SIZE_CONTENT);
    lv_obj_set_height(s_colorsLogo, LV_SIZE_CONTENT);
    lv_obj_set_y(s_colorsLogo, -103);
    lv_obj_set_align(s_colorsLogo, LV_ALIGN_CENTER);
    lv_label_set_text(s_colorsLogo, T_SCREEN_COLORS);
    lv_obj_set_style_text_font   (s_colorsLogo, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_color(s_colorsLogo, lv_color_hex(COLOR_SCHEMES[0].primary), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_opa  (s_colorsLogo, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(s_colorsLogo, 2,   LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_pad  (s_colorsLogo, 5,   LV_PART_MAIN | LV_STATE_DEFAULT);

    // ── Scheme selection buttons ─────────────────────────────────────────────
    // Layout: 6 buttons of 300×24 px, with 4 px gap → total 164 px.
    // Centered in the content area between header (-93) and nav strip (+85).
    // Content center ≈ y=-4; first button center at y=-66, pitch=28.
    //   Button centers: -66, -38, -10, 18, 46, 74
    // Last button bottom: 74+12 = 86 < 85 nav strip top — fits with 1px margin.
    const int btnW   = 300;
    const int btnH   = 24;
    const int pitch  = 28;   // btnH (24) + gap (4)
    const int startY = -66;  // y of first button's center from screen center

    ui_g_colors = lv_group_create();

    for (int i = 0; i < COLOR_SCHEME_COUNT; i++) {
        const int yPos = startY + i * pitch;

        lv_obj_t *btn = lv_btn_create(ui_Colors);
        lv_obj_set_width (btn, btnW);
        lv_obj_set_height(btn, btnH);
        lv_obj_set_y(btn, yPos);
        lv_obj_set_align(btn, LV_ALIGN_CENTER);
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_pad_all(btn, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius (btn, 6, LV_PART_MAIN | LV_STATE_DEFAULT);

        // This scheme's own primary as button background — visually distinctive
        lv_obj_set_style_bg_color(btn,
            lv_color_hex(COLOR_SCHEMES[i].primary), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(btn, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

        // Secondary color when encoder-focused
        lv_obj_set_style_bg_color(btn,
            lv_color_hex(COLOR_SCHEMES[i].secondary), LV_PART_MAIN | LV_STATE_FOCUSED);

        lv_obj_add_event_cb(btn, ev_schemeBtn, LV_EVENT_CLICKED, NULL);

        // Label: scheme name, left-aligned in button
        lv_obj_t *lbl = lv_label_create(btn);
        lv_obj_set_align(lbl, LV_ALIGN_LEFT_MID);
        lv_obj_set_x(lbl, 8);
        lv_label_set_text(lbl, COLOR_SCHEMES[i].name);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font (lbl, &lv_font_montserrat_14,  LV_PART_MAIN | LV_STATE_DEFAULT);

        s_schemeBtn[i] = btn;
        s_schemeLbl[i] = lbl;

        lv_group_add_obj(ui_g_colors, btn);
    }

    // ── Back button (bottom-left) ────────────────────────────────────────────
    s_backBtn = lv_btn_create(ui_Colors);
    lv_obj_set_width (s_backBtn, 100);
    lv_obj_set_height(s_backBtn, 30);
    lv_obj_set_y(s_backBtn, 100);
    lv_obj_set_x(s_backBtn, lv_pct(-33));
    lv_obj_set_align(s_backBtn, LV_ALIGN_CENTER);
    lv_obj_clear_flag(s_backBtn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_backBtn,
        lv_color_hex(COLOR_SCHEMES[0].primary), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(s_backBtn, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(s_backBtn, ev_backBtn, LV_EVENT_CLICKED, NULL);

    lv_obj_t *backLbl = lv_label_create(s_backBtn);
    lv_obj_center(backLbl);
    lv_label_set_text(backLbl, T_BACK);
    lv_obj_set_style_text_color(backLbl,
        lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);

    // ── Select button (bottom-right) ─────────────────────────────────────────
    lv_obj_t *selectBtn = lv_btn_create(ui_Colors);
    lv_obj_set_width (selectBtn, 100);
    lv_obj_set_height(selectBtn, 30);
    lv_obj_set_y(selectBtn, 100);
    lv_obj_set_x(selectBtn, lv_pct(33));
    lv_obj_set_align(selectBtn, LV_ALIGN_CENTER);
    lv_obj_clear_flag(selectBtn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(selectBtn,
        lv_color_hex(COLOR_SCHEMES[0].primary), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(selectBtn, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Same event callback as MX/right button — applies the focused scheme
    lv_obj_add_event_cb(selectBtn, ev_schemeBtn, LV_EVENT_CLICKED, NULL);
    
    // Add to group so encoder can navigate to it and select via button
    lv_group_add_obj(ui_g_colors, selectBtn);

    lv_obj_t *selectLbl = lv_label_create(selectBtn);
    lv_obj_center(selectLbl);
    lv_label_set_text(selectLbl, T_SELECT);
    lv_obj_set_style_text_color(selectLbl,
        lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
}
