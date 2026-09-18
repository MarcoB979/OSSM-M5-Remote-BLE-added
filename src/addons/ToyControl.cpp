#include "ToyControl.h"

#include <Arduino.h>
#include <M5Unified.h>

#include <algorithm>
#include <string>

#include "buttonhandlers/ButtonHandlers.h"
#include "display/styles.h"
#include "language.h"
#include "network/ToyHub.h"
#include "screens/ScreenHandler.h"
#include "ui/ui.h"
#include "ui/ui_helpers.h"

extern "C" const int TOYCONTROL_ID = 6;

namespace {

static lv_obj_t* s_screen = nullptr;
static lv_obj_t* s_title = nullptr;
static lv_obj_t* s_row_label[4] = {nullptr, nullptr, nullptr, nullptr};
static lv_obj_t* s_row_value[4] = {nullptr, nullptr, nullptr, nullptr};
static lv_obj_t* s_btn_l = nullptr;
static lv_obj_t* s_btn_m = nullptr;
static lv_obj_t* s_btn_r = nullptr;
static lv_obj_t* s_btn_l_label = nullptr;
static lv_obj_t* s_btn_m_label = nullptr;
static lv_obj_t* s_btn_r_label = nullptr;

static bool s_enabled = false;
static int s_selectedToy = 0;
static long s_enc1 = 0;
static long s_enc2 = 0;
static long s_enc3 = 0;
static long s_enc4 = 0;

int detentsFromEncoder(ESP32Encoder& enc, long* stored) {
    if (!stored) return 0;
    long count = enc.getCount();
    if (count >= (*stored + 2)) { *stored = count; return 1; }
    if (count <= (*stored - 2)) { *stored = count; return -1; }
    return 0;
}

void resetLocalEncoders() {
    s_enc1 = encoder1.getCount();
    s_enc2 = encoder2.getCount();
    s_enc3 = encoder3.getCount();
    s_enc4 = encoder4.getCount();
}

void goBackToAddons() {
    _ui_screen_change(ui_Addons, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
}

const char* featureName(ToyFeatureType t) {
    switch (t) {
        case ToyFeatureType::Vibrate: return "Vibrate";
        case ToyFeatureType::Rotate: return "Rotate";
        case ToyFeatureType::Oscillate: return "Oscillate";
        case ToyFeatureType::Constrict: return "Constrict";
        case ToyFeatureType::Spray: return "Spray";
        case ToyFeatureType::Temperature: return "Temperature";
        case ToyFeatureType::Led: return "LED";
        case ToyFeatureType::Position: return "Position";
    }
    return "?";
}

void drawScreen() {
    if (!s_screen) return;

    const int toyCount = toyHubGetToyCount();
    if (toyCount <= 0) {
        if (s_title) lv_label_set_text(s_title, languageGet(Tk_TOYS));
        if (s_row_label[0]) lv_label_set_text(s_row_label[0], languageGet(Tk_NO_TOYS));
        if (s_row_value[0]) lv_label_set_text(s_row_value[0], "");
        for (int i = 1; i < 4; ++i) {
            if (s_row_label[i]) lv_label_set_text(s_row_label[i], "");
            if (s_row_value[i]) lv_label_set_text(s_row_value[i], "");
        }
        return;
    }

    if (s_selectedToy < 0) s_selectedToy = 0;
    if (s_selectedToy >= toyCount) s_selectedToy = toyCount - 1;

    if (s_title) lv_label_set_text(s_title, toyHubGetToyName(s_selectedToy));

    for (int i = 0; i < 4; ++i) {
        if (!s_row_label[i] || !s_row_value[i]) continue;
        const ToyFeature* f = toyHubGetFeature(s_selectedToy, i);
        if (!f) {
            lv_label_set_text(s_row_label[i], "");
            lv_label_set_text(s_row_value[i], "");
            continue;
        }
        lv_label_set_text(s_row_label[i], featureName(f->type));
        lv_label_set_text_fmt(s_row_value[i], "%d", f->currentValue);
    }
}

void createScreenIfNeeded() {
    if (s_screen) return;

    s_screen = lv_obj_create(nullptr);
    lv_obj_clear_flag(s_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(s_screen, screenmachine, LV_EVENT_SCREEN_LOADED, nullptr);
    lv_obj_set_style_bg_color(s_screen, lv_color_hex(0x0A0A12), LV_PART_MAIN | LV_STATE_DEFAULT);

    // Title (same as other addon screens).
    s_title = lv_label_create(s_screen);
    lv_obj_set_align(s_title, LV_ALIGN_TOP_MID);
    lv_obj_set_x(s_title, 0);
    lv_obj_set_y(s_title, 8);
    lv_obj_set_style_text_font(s_title, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(s_title, &style_title_bar, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(s_title, languageGet(Tk_TOYS));

    // Four feature rows (label left, value right).
    for (int i = 0; i < 4; ++i) {
        s_row_label[i] = lv_label_create(s_screen);
        lv_obj_set_align(s_row_label[i], LV_ALIGN_TOP_LEFT);
        lv_obj_set_x(s_row_label[i], 12);
        lv_obj_set_y(s_row_label[i], 36 + i * 30);
        lv_obj_set_style_text_font(s_row_label[i], &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_style(s_row_label[i], &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

        s_row_value[i] = lv_label_create(s_screen);
        lv_obj_set_align(s_row_value[i], LV_ALIGN_TOP_RIGHT);
        lv_obj_set_x(s_row_value[i], -12);
        lv_obj_set_y(s_row_value[i], 36 + i * 30);
        lv_obj_set_style_text_font(s_row_value[i], &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_style(s_row_value[i], &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    // Bottom buttons: Menu (left), Stop (middle), Next (right) — standard style.
    s_btn_l = lv_btn_create(s_screen);
    lv_obj_set_size(s_btn_l, 100, 24);
    lv_obj_set_align(s_btn_l, LV_ALIGN_BOTTOM_LEFT);
    lv_obj_set_x(s_btn_l, 8);
    lv_obj_set_y(s_btn_l, -4);
    lv_obj_add_style(s_btn_l, &style_button_l, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(s_btn_l, &style_button_l_pressed, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_add_style(s_btn_l, &style_button_l_focused, LV_PART_MAIN | LV_STATE_FOCUSED);
    s_btn_l_label = lv_label_create(s_btn_l);
    lv_obj_center(s_btn_l_label);
    lv_label_set_text(s_btn_l_label, T_MENU);

    s_btn_m = lv_btn_create(s_screen);
    lv_obj_set_size(s_btn_m, 100, 24);
    lv_obj_set_align(s_btn_m, LV_ALIGN_BOTTOM_MID);
    lv_obj_set_y(s_btn_m, -4);
    lv_obj_add_style(s_btn_m, &style_button_m, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(s_btn_m, &style_button_m_pressed, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_add_style(s_btn_m, &style_button_m_focused, LV_PART_MAIN | LV_STATE_FOCUSED);
    s_btn_m_label = lv_label_create(s_btn_m);
    lv_obj_center(s_btn_m_label);
    lv_label_set_text(s_btn_m_label, T_STOP);

    s_btn_r = lv_btn_create(s_screen);
    lv_obj_set_size(s_btn_r, 100, 24);
    lv_obj_set_align(s_btn_r, LV_ALIGN_BOTTOM_RIGHT);
    lv_obj_set_x(s_btn_r, -8);
    lv_obj_set_y(s_btn_r, -4);
    lv_obj_add_style(s_btn_r, &style_button_r, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(s_btn_r, &style_button_r_pressed, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_add_style(s_btn_r, &style_button_r_focused, LV_PART_MAIN | LV_STATE_FOCUSED);
    s_btn_r_label = lv_label_create(s_btn_r);
    lv_obj_center(s_btn_r_label);
    lv_label_set_text(s_btn_r_label, T_NEXT);
}

}  // namespace

void ToyControlSetAddonEnabled(bool enabled) { s_enabled = enabled; }
bool ToyControlIsAddonEnabled() { return s_enabled; }

lv_obj_t* ToyControlGetScreen() {
    createScreenIfNeeded();
    return s_screen;
}

bool ToyControlOwnsActiveScreen() {
    return s_screen && lv_scr_act() == s_screen;
}

void ToyControlPrepareScreen() {
    createScreenIfNeeded();
    resetLocalEncoders();
    s_selectedToy = 0;
    drawScreen();
}

void ToyControlHandleScreen(const ButtonEvents& events) {
    if (!s_enabled) {
        goBackToAddons();
        return;
    }

    const int toyCount = toyHubGetToyCount();

    if (events.leftShort) {
        goBackToAddons();
        return;
    }
    if (events.rightShort) {
        if (toyCount > 0) {
            s_selectedToy = (s_selectedToy + 1) % toyCount;
            resetLocalEncoders();
            drawScreen();
        }
        return;
    }
    if (events.mxShort) {
        if (toyCount > 0) {
            toyHubStopToy(s_selectedToy);
            drawScreen();
        }
        return;
    }

    if (toyCount <= 0) return;

    const int deltas[4] = {
        detentsFromEncoder(encoder1, &s_enc1),
        detentsFromEncoder(encoder2, &s_enc2),
        detentsFromEncoder(encoder3, &s_enc3),
        detentsFromEncoder(encoder4, &s_enc4),
    };

    bool changed = false;
    for (int i = 0; i < 4; ++i) {
        if (deltas[i] == 0) continue;
        const ToyFeature* f = toyHubGetFeature(s_selectedToy, i);
        if (!f) continue;
        int step = (deltas[i] > 0) ? 1 : -1;
        if (f->type == ToyFeatureType::Rotate) step *= 2;  // coarser over -20..20
        toyHubSetFeatureRaw(s_selectedToy, i, f->currentValue + step);
        changed = true;
    }
    if (changed) drawScreen();
}

extern "C" void ToyControlHandleScreen(const struct ButtonEvents* events) {
    if (!events) return;
    ToyControlHandleScreen(*events);
}
