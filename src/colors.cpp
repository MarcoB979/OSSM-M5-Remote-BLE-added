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

#include "main.h"
#include <Arduino.h>
#include <Preferences.h>

#include "config.h"
#include "language.h"
#include "colors.h"
#include "screen.h"
#include "ui/ui.h"
#include "ui/ui_helpers.h"
#include "styles.h"

// `COLOR_SCHEMES` is defined in src/ColorSchemes.cpp (single-definition).

int g_active_color_scheme = 0;

// ---------------------------------------------------------------------------
// Colors screen LVGL objects  (defined here, extern'd in ui.h)
// ---------------------------------------------------------------------------

lv_obj_t   *ui_Colors   = nullptr;
lv_group_t *ui_g_colors = nullptr;

static constexpr int VISIBLE_SCHEME_COUNT = 5;
static lv_obj_t *s_schemeBtn[VISIBLE_SCHEME_COUNT] = {};
static lv_obj_t *s_schemeLbl[VISIBLE_SCHEME_COUNT] = {};
static int s_visibleToScheme[VISIBLE_SCHEME_COUNT] = {0, 1, 2, 3, 4};
static int s_focus_scheme_index = 0;
static lv_obj_t *s_backBtn                         = nullptr;
static lv_obj_t *s_colorsLogo                      = nullptr;

static int wrapSchemeIndex(int idx) {
    while (idx < 0) idx += COLOR_SCHEME_COUNT;
    while (idx >= COLOR_SCHEME_COUNT) idx -= COLOR_SCHEME_COUNT;
    return idx;
}

static int schemeForVisibleSlot(int slot) {
    // If we have fewer schemes than visible slots, map slots 0..N-1 to schemes
    // and mark the remainder as empty by returning -1.
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
            // No scheme for this visible slot; hide the button
            lv_obj_add_flag(s_schemeBtn[slot], LV_OBJ_FLAG_HIDDEN);
            continue;
        } else {
            lv_obj_clear_flag(s_schemeBtn[slot], LV_OBJ_FLAG_HIDDEN);
        }

        lv_obj_set_style_bg_color(
            s_schemeBtn[slot], lv_color_hex(COLOR_SCHEMES[scheme].title_bar),
            LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(
            s_schemeBtn[slot], lv_color_hex(COLOR_SCHEMES[scheme].button_m),
            LV_PART_MAIN | LV_STATE_FOCUSED);

        char buf[48];
        const int centerSlot = VISIBLE_SCHEME_COUNT / 2;
        if (scheme == g_active_color_scheme) {
            snprintf(buf, sizeof(buf), "%s  " LV_SYMBOL_OK, COLOR_SCHEMES[scheme].name);
        } else {
            snprintf(buf, sizeof(buf), "%s", COLOR_SCHEMES[scheme].name);
        }

        // Show a white border only for the centered (focused) visible slot.
        if (slot == centerSlot) {
            lv_obj_set_style_border_color(s_schemeBtn[slot],
                lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(s_schemeBtn[slot],
                lv_color_white(), LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_obj_set_style_border_width(s_schemeBtn[slot],
                2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(s_schemeBtn[slot],
                2, LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_obj_set_style_border_opa(s_schemeBtn[slot],
                255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_opa(s_schemeBtn[slot],
                255, LV_PART_MAIN | LV_STATE_FOCUSED);
        } else {
            lv_obj_set_style_border_width(s_schemeBtn[slot],
                0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(s_schemeBtn[slot],
                0, LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_obj_set_style_border_opa(s_schemeBtn[slot],
                0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_opa(s_schemeBtn[slot],
                0, LV_PART_MAIN | LV_STATE_FOCUSED);
        }
        lv_label_set_text(s_schemeLbl[slot], buf);
    }

    if (ui_g_colors && s_schemeBtn[VISIBLE_SCHEME_COUNT / 2]) {
        lv_group_focus_obj(s_schemeBtn[VISIBLE_SCHEME_COUNT / 2]);
    }
}

// ---------------------------------------------------------------------------
// Internal restyle helpers
// ---------------------------------------------------------------------------

static void rs_btnPrimary(lv_obj_t *btn, uint32_t primary) {
    if (!btn) return;
    // Prefer adding the shared semantic button style when possible so
    // runtime theme updates apply via `styles_apply_scheme()`.
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
    // Fallback to direct color set for unusual cases.
    lv_obj_set_style_bg_color(btn, lv_color_hex(primary), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa  (btn, 255,                   LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void rs_btnFocused(lv_obj_t *btn, uint32_t secondary) {
    if (!btn) return;
    if (secondary == COLOR_SCHEMES[g_active_color_scheme].button_l) {
        lv_obj_add_style(btn, &style_button_l, LV_PART_MAIN | LV_STATE_FOCUSED);
        return;
    }
    if (secondary == COLOR_SCHEMES[g_active_color_scheme].button_m) {
        lv_obj_add_style(btn, &style_button_m, LV_PART_MAIN | LV_STATE_FOCUSED);
        return;
    }
    if (secondary == COLOR_SCHEMES[g_active_color_scheme].button_r) {
        lv_obj_add_style(btn, &style_button_r, LV_PART_MAIN | LV_STATE_FOCUSED);
        return;
    }
    lv_obj_set_style_bg_color(btn, lv_color_hex(secondary), LV_PART_MAIN | LV_STATE_FOCUSED);
}

static void rs_outline(lv_obj_t *obj, uint32_t primary) {
    if (!obj) return;
    lv_obj_add_style(obj, &style_title_bar, LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void rs_slider(lv_obj_t *sl, uint32_t primary, uint32_t /*secondary*/) {
    if (!sl) return;
    // Map known per-scheme slider values to the shared per-slot styles so
    // runtime theme updates update sliders automatically.
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
        lv_obj_add_style(sl, &style_slider_track[found], LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_style(sl, &style_slider_indicator[found], LV_PART_INDICATOR | LV_STATE_DEFAULT);
        lv_obj_add_style(sl, &style_slider_indicator[found], LV_PART_KNOB | LV_STATE_DEFAULT);
        return;
    }

    // Fallback: manually set colors if we couldn't match a semantic slot.
    lv_obj_set_style_bg_color     (sl, lv_color_mix(lv_color_hex(primary), lv_color_hex(0xFFFFFF), 64), LV_PART_MAIN      | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa       (sl, 255,                                                         LV_PART_MAIN      | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color     (sl, lv_color_hex(primary),                                      LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa       (sl, 255,                                                         LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(sl, lv_color_hex(primary),                                      LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color     (sl, lv_color_hex(primary),                                      LV_PART_KNOB      | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa       (sl, 255,                                                         LV_PART_KNOB      | LV_STATE_DEFAULT);
}

// ---------------------------------------------------------------------------
// Apply a color scheme to the entire UI
// ---------------------------------------------------------------------------

void applyColorScheme(int index) {
    if (index < 0 || index >= COLOR_SCHEME_COUNT) index = 0;
    g_active_color_scheme = index;
    const uint32_t THEME_P = COLOR_SCHEMES[index].title_bar;
    const uint32_t THEME_S = COLOR_SCHEMES[index].slider1;
    const uint32_t BTN = COLOR_SCHEMES[index].button_l;
    const uint32_t BTN_FOC = COLOR_SCHEMES[index].button_m;
    const uint32_t SL_P = COLOR_SCHEMES[index].slider1;
    const uint32_t SL_S = COLOR_SCHEMES[index].slider2;

    // --- 1. Re-init the LVGL default theme ---
    lv_disp_t *dispp = lv_disp_get_default();
    if (dispp) {
        lv_theme_t *theme = lv_theme_default_init(
            dispp, lv_color_hex(THEME_P), lv_color_hex(THEME_S), dark_mode, LV_FONT_DEFAULT);
        lv_disp_set_theme(dispp, theme);
    }

    // Update shared styles to match this scheme (so widgets using styles
    // will automatically reflect the selected scheme).
    styles_apply_scheme(index);

    // --- 2. Buttons: primary bg (default state) ---
    // Apply semantic left/mid/right colors to the obvious L/M/R widgets so
    // the UI respects per-button semantics (rather than forcing a single
    // color everywhere).
    // Start screen
    rs_btnPrimary(ui_StartButtonL, COLOR_SCHEMES[index].button_l);
    rs_btnPrimary(ui_StartButtonM, COLOR_SCHEMES[index].button_m);
    rs_btnPrimary(ui_StartButtonR, COLOR_SCHEMES[index].button_r);

    // Home screen (L + R only, M is start/stop green and handled separately in ui.c)
    rs_btnPrimary(ui_HomeButtonL, COLOR_SCHEMES[index].button_l);
    rs_btnPrimary(ui_HomeButtonR, COLOR_SCHEMES[index].button_r);

    // Pattern screen
    rs_btnPrimary(ui_PatternButtonL, COLOR_SCHEMES[index].button_l);
    rs_btnPrimary(ui_PatternButtonM, COLOR_SCHEMES[index].button_m);
    rs_btnPrimary(ui_PatternButtonR, COLOR_SCHEMES[index].button_r);

    // Torque screen
    // Torque screen: removed — do not reference Torqe UI symbols here

    // Eject screen
    rs_btnPrimary(ui_EJECTButtonL, COLOR_SCHEMES[index].button_l);
    rs_btnPrimary(ui_EJECTButtonM, COLOR_SCHEMES[index].button_m);
    rs_btnPrimary(ui_EJECTButtonR, COLOR_SCHEMES[index].button_r);

    // Settings screen
    rs_btnPrimary(ui_SettingsButtonL, COLOR_SCHEMES[index].button_l);
    rs_btnPrimary(ui_SettingsButtonM, COLOR_SCHEMES[index].button_m);
    rs_btnPrimary(ui_SettingsButtonR, COLOR_SCHEMES[index].button_r);

    // Menu screen — bottom controls
    rs_btnPrimary(ui_MenuButtonL, COLOR_SCHEMES[index].button_l);
    rs_btnPrimary(ui_MenuButtonM, COLOR_SCHEMES[index].button_m);
    rs_btnPrimary(ui_MenuButtonR, COLOR_SCHEMES[index].button_r);

    // Streaming screen (do NOT change the MX/middle button color here —
    // MX behavior is controlled dynamically in `buttonhandler.cpp`)
    rs_btnPrimary(ui_StreamingButtonL, COLOR_SCHEMES[index].button_l);
    rs_btnPrimary(ui_StreamingButtonR, COLOR_SCHEMES[index].button_r);

    // Addons screen — nav buttons + items (items use the left/item color)
    rs_btnPrimary(ui_AddonsButtonL, COLOR_SCHEMES[index].button_l);
    rs_btnPrimary(ui_AddonsButtonM, COLOR_SCHEMES[index].button_m);
    rs_btnPrimary(ui_AddonsButtonR, COLOR_SCHEMES[index].button_r);
    rs_btnPrimary(ui_AddonsItem0, COLOR_SCHEMES[index].button_l);
    rs_btnPrimary(ui_AddonsItem1, COLOR_SCHEMES[index].button_l);
    rs_btnPrimary(ui_AddonsItem2, COLOR_SCHEMES[index].button_l);

    // Colors screen — back button
    rs_btnPrimary(s_backBtn, COLOR_SCHEMES[index].button_l);

    // --- 3. Menu big tiles: assign explicit per-tile slider accent colors ---
    // Map: TL -> slider1, TR -> slider2, ML -> slider3, BL -> slider4
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
    if (ui_MenuButtonBL) {
        lv_obj_add_style(ui_MenuButtonBL, &style_slider_indicator[3], LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_style(ui_MenuButtonBL, &style_button_m, LV_PART_MAIN | LV_STATE_FOCUSED);
        lv_obj_add_style(ui_MenuButtonBL, &style_button_m_disabled, LV_PART_MAIN | LV_STATE_DISABLED);
    }

    // Ensure focused state uses the scheme's focussed_element accent (e.g., pink for Rainbow)
    uint32_t focusAccent = COLOR_SCHEMES[index].focussed_element;
    if (ui_MenuButtonTL) rs_btnFocused(ui_MenuButtonTL, focusAccent);
    if (ui_MenuButtonTR) rs_btnFocused(ui_MenuButtonTR, focusAccent);
    if (ui_MenuButtonML) rs_btnFocused(ui_MenuButtonML, focusAccent);
    if (ui_MenuButtonBL) rs_btnFocused(ui_MenuButtonBL, focusAccent);

    // --- 4. Addons items: focused state ---
    rs_btnFocused(ui_AddonsItem0, BTN_FOC);
    rs_btnFocused(ui_AddonsItem1, BTN_FOC);
    rs_btnFocused(ui_AddonsItem2, BTN_FOC);

    // --- 5. Sliders ---
    // Apply each slider its intended semantic color so we don't overwrite
    // the per-slider assignments done in ui.c.
    rs_slider(ui_homespeedslider, COLOR_SCHEMES[index].slider1, COLOR_SCHEMES[index].slider1);
    rs_slider(ui_homedepthslider, COLOR_SCHEMES[index].slider2, COLOR_SCHEMES[index].slider2);
    rs_slider(ui_homestrokeslider, COLOR_SCHEMES[index].slider3, COLOR_SCHEMES[index].slider3);
    rs_slider(ui_homesensationslider, COLOR_SCHEMES[index].slider4, COLOR_SCHEMES[index].slider4);

    // outtro/intro sliders removed from UI; no-op here

    rs_slider(ui_streamingspeedslider, COLOR_SCHEMES[index].slider1, COLOR_SCHEMES[index].slider1);
    rs_slider(ui_streamingdepthslider, COLOR_SCHEMES[index].slider2, COLOR_SCHEMES[index].slider2);
    rs_slider(ui_streamingstrokeslider, COLOR_SCHEMES[index].slider3, COLOR_SCHEMES[index].slider3);
    rs_slider(ui_brightness_slider, COLOR_SCHEMES[index].slider1, COLOR_SCHEMES[index].slider1);

    // --- 6. Pattern roller menu ---
    if (ui_PatternS) {
        // Keep the roller main bg transparent so the three band objects
        // underneath are the single source of visual color for the options.
        lv_obj_set_style_bg_opa(ui_PatternS, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        // Do NOT set an opaque selected-part color here — it was causing
        // the selected/option areas to paint on top of the bands (appearing
        // as an extra red overlay in some schemes). Leave selected part
        // transparent so the bands remain visible.
        lv_obj_set_style_bg_opa(ui_PatternS, LV_OPA_TRANSP, LV_PART_SELECTED | LV_STATE_DEFAULT);
    }

    // --- 7. Logo / header outlines ---
    lv_obj_t *logos[] = {
        ui_Logo, ui_Logo2, ui_Logo1, /* ui_Logo4 removed */ ui_Logo5, ui_Logo6,
        ui_LogoMenu, ui_LogoStreaming, ui_LogoAddons,
        s_colorsLogo,
    };
    for (lv_obj_t *logo : logos) {
        rs_outline(logo, THEME_P);
    }

    // Header/logo text and small status labels (battery/icons) now get
    // `style_text_primary` at creation time in `ui.c`. Removing redundant
    // runtime re-attachment here so `styles_apply_scheme()` remains the
    // single source-of-truth for style values.
    // Individual small status labels are styled at creation in `ui.c`.

    // --- 8. Colors screen visible scheme buttons ---
    updateVisibleSchemeButtons();

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
        return COLOR_SCHEMES[0].title_bar;
    }
    return COLOR_SCHEMES[g_active_color_scheme].title_bar;
}

extern "C" uint32_t getActiveSecondaryColor() {
    if (g_active_color_scheme < 0 || g_active_color_scheme >= COLOR_SCHEME_COUNT) {
        return COLOR_SCHEMES[0].slider1;
    }
    return COLOR_SCHEMES[g_active_color_scheme].slider1;
}

extern "C" uint32_t getActiveTextPrimaryColor() {
    if (g_active_color_scheme < 0 || g_active_color_scheme >= COLOR_SCHEME_COUNT) {
        return COLOR_SCHEMES[0].text_primary;
    }
    return COLOR_SCHEMES[g_active_color_scheme].text_primary;
}

extern "C" uint32_t getActiveTextSecondaryColor() {
    if (g_active_color_scheme < 0 || g_active_color_scheme >= COLOR_SCHEME_COUNT) {
        return COLOR_SCHEMES[0].text_secondary;
    }
    return COLOR_SCHEMES[g_active_color_scheme].text_secondary;
}

// Simple semantic accessors
extern "C" uint32_t getActiveTitleBarColor(void)        { return COLOR_SCHEMES[g_active_color_scheme].title_bar; }
extern "C" uint32_t getActiveButtonLColor(void)         { return COLOR_SCHEMES[g_active_color_scheme].button_l; }
extern "C" uint32_t getActiveButtonMColor(void)         { return COLOR_SCHEMES[g_active_color_scheme].button_m; }
extern "C" uint32_t getActiveButtonRColor(void)         { return COLOR_SCHEMES[g_active_color_scheme].button_r; }
extern "C" uint32_t getActiveSlider1Color(void)         { return COLOR_SCHEMES[g_active_color_scheme].slider1; }
extern "C" uint32_t getActiveSlider2Color(void)         { return COLOR_SCHEMES[g_active_color_scheme].slider2; }
extern "C" uint32_t getActiveSlider3Color(void)         { return COLOR_SCHEMES[g_active_color_scheme].slider3; }
extern "C" uint32_t getActiveSlider4Color(void)         { return COLOR_SCHEMES[g_active_color_scheme].slider4; }
extern "C" uint32_t getActiveBatteryMainColor(void)     { return COLOR_SCHEMES[g_active_color_scheme].battery_main; }
extern "C" uint32_t getActiveBatteryIndicatorColor(void){ return COLOR_SCHEMES[g_active_color_scheme].battery_indicator; }
extern "C" uint32_t getActiveRollerColor(void)          { return COLOR_SCHEMES[g_active_color_scheme].roller; }
extern "C" uint32_t getActiveBackgroundColor(void)      { return COLOR_SCHEMES[g_active_color_scheme].background; }

// ---------------------------------------------------------------------------
// Public API (extern "C" for ui.c compatibility)
// ---------------------------------------------------------------------------

extern "C" void colorSchemeScreenLoaded() {
    updateVisibleSchemeButtons();
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

void handleColorsScreen(const ButtonEvents &events) {
    // Encoder 4: carousel through scheme list (only if more schemes than visible)
    if (COLOR_SCHEME_COUNT > VISIBLE_SCHEME_COUNT) {
        if (encoder4.getCount() > encoder4_enc + 2) {
            s_focus_scheme_index = wrapSchemeIndex(s_focus_scheme_index + 1);
            updateVisibleSchemeButtons();
            encoder4_enc = encoder4.getCount();
        } else if (encoder4.getCount() < encoder4_enc - 2) {
            s_focus_scheme_index = wrapSchemeIndex(s_focus_scheme_index - 1);
            updateVisibleSchemeButtons();
            encoder4_enc = encoder4.getCount();
        }
    }

    // Left button: back to Addons
    if (events.leftShort) {
        _ui_screen_change(ui_Addons, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
        clearButtonFlags();
        return;
    }

    // MX / Right button: apply currently focused carousel scheme
    if (events.mxShort || events.rightShort) {
        colorSchemeSelectIndex(s_focus_scheme_index);
        clearButtonFlags();
    }
}

// ---------------------------------------------------------------------------
// Colors screen: LVGL event handlers
// ---------------------------------------------------------------------------

static void ev_schemeBtn(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_SHORT_CLICKED) return;
    lv_obj_t *target = reinterpret_cast<lv_obj_t *>(lv_event_get_target(e));
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
        _ui_screen_change(ui_Addons, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
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
    lv_obj_set_style_outline_color(s_colorsLogo, lv_color_hex(getActiveTitleBarColor()), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_opa  (s_colorsLogo, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(s_colorsLogo, 2,   LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_pad  (s_colorsLogo, 5,   LV_PART_MAIN | LV_STATE_DEFAULT);

    // ── Scheme selection buttons (carousel view, max 5 visible) ───────────
    const int btnW   = 300;
    const int btnH   = 24;
    const int pitch  = 28;   // btnH (24) + gap (4)
    const int startY = -52;  // y of first button's center from screen center

    ui_g_colors = lv_group_create();

    s_focus_scheme_index = g_active_color_scheme;

    for (int slot = 0; slot < VISIBLE_SCHEME_COUNT; slot++) {
        const int yPos = startY + slot * pitch;
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

        // If there is no scheme for this slot, hide the button and skip styling.
        if (scheme < 0) {
            lv_obj_add_flag(btn, LV_OBJ_FLAG_HIDDEN);
        } else {
            // This scheme's own primary as button background — visually distinctive
            lv_obj_set_style_bg_color(btn,
                lv_color_hex(COLOR_SCHEMES[scheme].title_bar), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(btn, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

            // Secondary color when encoder-focused
            lv_obj_set_style_bg_color(btn,
                lv_color_hex(COLOR_SCHEMES[scheme].button_m), LV_PART_MAIN | LV_STATE_FOCUSED);
        }

        lv_obj_add_event_cb(btn, ev_schemeBtn, LV_EVENT_SHORT_CLICKED, NULL);

        // Label: scheme name, left-aligned in button
        lv_obj_t *lbl = lv_label_create(btn);
        lv_obj_set_align(lbl, LV_ALIGN_LEFT_MID);
        lv_obj_set_x(lbl, 8);
        if (scheme < 0) {
            lv_label_set_text(lbl, "");
        } else {
            lv_label_set_text(lbl, COLOR_SCHEMES[scheme].name);
        }
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font (lbl, &lv_font_montserrat_14,  LV_PART_MAIN | LV_STATE_DEFAULT);

        s_schemeBtn[slot] = btn;
        s_schemeLbl[slot] = lbl;

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
        lv_color_hex(getActiveTitleBarColor()), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(s_backBtn, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(s_backBtn, ev_backBtn, LV_EVENT_SHORT_CLICKED, NULL);

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
        lv_color_hex(getActiveTitleBarColor()), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(selectBtn, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Apply currently focused carousel scheme
    lv_obj_add_event_cb(selectBtn, ev_selectBtn, LV_EVENT_SHORT_CLICKED, NULL);
    
    // Add to group so encoder can navigate to it and select via button
    lv_group_add_obj(ui_g_colors, selectBtn);

    lv_obj_t *selectLbl = lv_label_create(selectBtn);
    lv_obj_center(selectLbl);
    lv_label_set_text(selectLbl, T_SELECT);
    lv_obj_set_style_text_color(selectLbl,
        lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);

    updateVisibleSchemeButtons();
}

