// AP-v2.cpp — "Advanced Penetration v2" addon (M5 layer).
//
// This screen renders DIRECTLY to the M5 display via an M5Canvas sprite (the
// M5GFX equivalent of RADR's GFXcanvas16 + tft.drawRGBBitmap). It bypasses
// LVGL widgets so the look matches the RADR remote 1:1: tabs, the bezier
// curve + modifier traces, two encoder bars, and three bottom buttons.
//
// The Advanced Penetration logic lives in radr/OSSMAdvanced.hpp (a
// self-contained copy of the RADR advancedPenetration.hpp), so upstream
// updates can be re-copied without touching this layer.
//
// Encoder mapping (Option A):
//   Enc1 = Speed   Enc2 = selected value   Enc3 = prev tab   Enc4 = next tab
//   Left button = Presets, Middle (MX) = Start/Stop, Right = Modifier

#include "AP-v2.h"

#include <Arduino.h>
#include <M5Unified.h>

#include <algorithm>
#include <string>
#include <vector>

#include "buttonhandlers/ButtonHandlers.h"
#include "communication/BleComm.h"
#include "config/debug.h"
#include "language.h"
#include "radr/OSSMAdvanced.hpp"
#include "screens/ScreenHandler.h"
#include "ui/ui.h"
#include "ui/ui_helpers.h"

extern "C" const int APV2_ID = 5;

namespace {

// ---- RADR logic instance ----
static OSSMAdvanced s_ap;

// ---- Minimal LVGL screen (for lv_scr_act() detection only) ----
static lv_obj_t *s_screen = nullptr;

// ---- Full-screen render canvas ----
static M5Canvas s_canvas;
static bool s_canvas_ready = false;

// ---- State ----
static bool s_enabled       = false;
static bool s_modifier_view = false;
static long s_enc1 = 0, s_enc2 = 0, s_enc3 = 0, s_enc4 = 0;

static bool     s_live_ap_available       = false;
static uint32_t s_last_bootstrap_attempt  = 0;
static uint32_t s_last_status_poll        = 0;
static uint32_t s_last_unavailable_log_ms = 0;

static std::vector<std::string> s_preset_names;
static int s_preset_cursor = -1;

// RADR layout constants (320x240).
static constexpr int CURVE_X = 10;
static constexpr int CURVE_Y = 70;

// ---- Helpers ----
static int detentsFromEncoder(ESP32Encoder &enc, long *state) {
    if (!state) return 0;
    long count = enc.getCount();
    if (count >= (*state + 2)) { *state = count; return 1; }
    if (count <= (*state - 2)) { *state = count; return -1; }
    return 0;
}

static void resetLocalEncoders() {
    s_enc1 = encoder1.getCount();
    s_enc2 = encoder2.getCount();
    s_enc3 = encoder3.getCount();
    s_enc4 = encoder4.getCount();
}

static void goBackToAddons() {
    _ui_screen_change(ui_Addons, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
}

static bool writeControl(const std::string &payload) {
    if (!s_live_ap_available || payload.empty()) return false;
    if (!bleCommWriteAdvancedControl(String(payload.c_str()))) {
        s_live_ap_available = false;
        return false;
    }
    return true;
}

static bool tryBootstrapModelFromLiveBle() {
    const uint32_t nowMs = millis();
    if ((nowMs - s_last_bootstrap_attempt) < 1500U) return s_live_ap_available;
    s_last_bootstrap_attempt = nowMs;

    String configFrame;
    if (!bleCommReadAdvancedConfig(&configFrame) || configFrame.length() == 0) {
        const uint32_t now = millis();
        if ((now - s_last_unavailable_log_ms) > 5000U) {
            LogDebug("[APv2] Live config read unavailable");
            s_last_unavailable_log_ms = now;
        }
        s_live_ap_available = false;
        return false;
    }

    s_ap.parseConfig(std::string(configFrame.c_str()));

    String statusFrame;
    if (bleCommReadAdvancedStatus(&statusFrame) && statusFrame.length() > 0) {
        s_ap.parseStatus(std::string(statusFrame.c_str()));
    }

    String presetsFrame;
    s_preset_names.clear();
    if (bleCommReadAdvancedPresets(&presetsFrame) && presetsFrame.length() > 0) {
        std::string list = presetsFrame.c_str();
        size_t start = 0, p = list.find(',');
        while (p != std::string::npos) {
            std::string n = list.substr(start, p - start);
            if (!n.empty()) s_preset_names.push_back(n);
            start = p + 1;
            p = list.find(',', start);
        }
    }

    s_live_ap_available = !s_ap.controlNames.empty();
    return s_live_ap_available;
}

static void refreshFromLiveStatusIfDue() {
    if (!s_live_ap_available) return;
    const uint32_t nowMs = millis();
    if ((nowMs - s_last_status_poll) < 250U) return;
    s_last_status_poll = nowMs;
    String statusFrame;
    if (!bleCommReadAdvancedStatus(&statusFrame) || statusFrame.length() == 0) return;
    s_ap.parseStatus(std::string(statusFrame.c_str()));
}

// ---- Direct rendering (RADR-style) ----
static void drawText(const char *s, int x, int y, uint16_t fg, uint16_t bg, float size) {
    s_canvas.setTextSize(size);
    s_canvas.setTextColor(fg, bg);
    s_canvas.drawString(s, x, y);
}

static void drawCenterText(const char *s, int cx, int y, uint16_t fg, uint16_t bg, float size) {
    s_canvas.setTextSize(size);
    s_canvas.setTextColor(fg, bg);
    s_canvas.drawCenterString(s, cx, y);
}

static void drawCurve() {
    OSSMAdvanced::Render r = s_modifier_view ? s_ap.buildModifierDisplay() : s_ap.buildCurveDisplay();
    for (const OSSMAdvanced::Polyline &pl : r.lines) {
        if (pl.dashed) {
            for (const OSSMAdvanced::Pt &p : pl.pts) {
                s_canvas.drawPixel(CURVE_X + p.x, CURVE_Y + p.y, pl.color565);
            }
        } else if (pl.segments) {
            for (size_t i = 0; i + 1 < pl.pts.size(); i += 2) {
                s_canvas.drawLine(CURVE_X + pl.pts[i].x, CURVE_Y + pl.pts[i].y,
                                  CURVE_X + pl.pts[i + 1].x, CURVE_Y + pl.pts[i + 1].y, pl.color565);
            }
        } else {
            for (size_t i = 1; i < pl.pts.size(); ++i) {
                s_canvas.drawLine(CURVE_X + pl.pts[i - 1].x, CURVE_Y + pl.pts[i - 1].y,
                                  CURVE_X + pl.pts[i].x, CURVE_Y + pl.pts[i].y, pl.color565);
            }
        }
    }
    if (r.hasGuide && !s_modifier_view) {
        s_canvas.fillRect(CURVE_X + r.guideX, CURVE_Y + r.guideY,
                          r.guideW > 0 ? r.guideW : 1, r.guideH > 0 ? r.guideH : 1, r.guideColor);
    }
}

static void drawEncoderBar(int x, int y, int w, int h, float value01, uint16_t color) {
    s_canvas.fillRect(x, y, w, h, 0x0000);
    const int innerW = w - 2, innerH = h - 2;
    const int fillH = (int)(innerH * value01);
    s_canvas.fillRoundRect(x + 1, y + 1 + (innerH - fillH), innerW, fillH, 2, color);
    s_canvas.drawRoundRect(x, y, w, h, 2, 0xFFFF);
}

static void drawScreen() {
    if (!s_canvas_ready) return;

    s_canvas.fillScreen(0x0000);

    // Title
    drawCenterText(s_modifier_view ? T_APV2_MODS : T_APV2, 160, 4, 0xFFFF, 0x0000, 2.0f);

    // Battery percentage (top-right)
    char batt[12];
    snprintf(batt, sizeof(batt), "%d%%", M5.Power.getBatteryLevel());
    s_canvas.setTextSize(1.0f);
    s_canvas.setTextColor(0xFFFF, 0x0000);
    s_canvas.drawRightString(batt, 312, 10);

    // Tabs (6)
    const int tabCount = s_modifier_view ? s_ap.modifierTabCount() : s_ap.baseTabCount();
    const int tabW = 47, tabGap = 4, tabY = 30;
    const int tabStartX = (320 - (tabCount * tabW + (tabCount - 1) * tabGap)) / 2;
    for (int i = 0; i < tabCount; ++i) {
        const int v = s_modifier_view ? (int)(s_ap.modifierValue(i) + 0.5f) : (int)(s_ap.baseValue(i) + 0.5f);
        const bool active = s_modifier_view ? (s_ap.modifierIndex == (uint8_t)i) : (s_ap.baseIndex == (uint8_t)i);
        const int x = tabStartX + i * (tabW + tabGap);
        const uint16_t bg = active ? s_ap.advancedColors[s_ap.baseIndex % s_ap.advancedColors.size()] : 0x1082;
        s_canvas.fillRoundRect(x, tabY, tabW, 20, 4, bg);
        char num[8];
        snprintf(num, sizeof(num), "%d", v);
        drawCenterText(num, x + tabW / 2, tabY + 5, active ? 0x0000 : 0xFFFF, bg, 1.0f);
    }

    // Speed + selected name labels
    char speedTxt[20];
    snprintf(speedTxt, sizeof(speedTxt), "%s %d", T_APV2_SPEED, (int)(s_ap.speedValue() + 0.5f));
    drawText(speedTxt, 8, 54, 0xFFFF, 0x0000, 1.0f);
    const char *name = "?";
    if (s_modifier_view) {
        if (s_ap.modifierIndex < s_ap.modifierNames.size()) name = s_ap.modifierNames[s_ap.modifierIndex].c_str();
    } else {
        name = s_ap.selectedName();
    }
    s_canvas.setTextSize(1.0f);
    s_canvas.setTextColor(0xFFFF, 0x0000);
    s_canvas.drawRightString(name, 312, 54);

    // Curve area border + curve
    s_canvas.drawRect(CURVE_X - 1, CURVE_Y - 1, 302, 150, 0x3341);
    drawCurve();

    // Encoder bars (left = Speed, right = selected value)
    const float speed01 = s_ap.speedValue() / 100.0f;
    const float value01 = (s_modifier_view ? s_ap.selectedValue() : s_ap.selectedBaseValue()) / 100.0f;
    drawEncoderBar(1, CURVE_Y, 8, 124, speed01, s_ap.advancedColors[6]);
    drawEncoderBar(311, CURVE_Y, 8, 124, value01, s_ap.selectedColor());

    // Bottom buttons: Presets / Start-Stop / Modifier
    const int btnY = 214, btnH = 20, btnW = 100;
    const bool running = s_ap.speedValue() > 0.0f;

    s_canvas.fillRoundRect(6, btnY, btnW, btnH, 5, 0x4aac);
    drawCenterText(T_PRESETS, 6 + btnW / 2, btnY + 4, 0xFFFF, 0x4aac, 1.5f);

    const uint16_t midBg = running ? 0xF800 : 0x4aac;  // red when running
    s_canvas.fillRoundRect(110, btnY, btnW, btnH, 5, midBg);
    drawCenterText(running ? T_STOP : T_START, 110 + btnW / 2, btnY + 4, 0xFFFF, midBg, 1.5f);

    s_canvas.fillRoundRect(214, btnY, btnW, btnH, 5, 0x4aac);
    drawCenterText(s_modifier_view ? T_BACK : T_APV2_MODS, 214 + btnW / 2, btnY + 4, 0xFFFF, 0x4aac, 1.5f);

    // Fallback hint
    if (!s_live_ap_available) {
        s_canvas.setTextSize(1.0f);
        s_canvas.setTextColor(0xFBE0, 0x0000);
        s_canvas.drawString(T_FALLBACK, 8, 236);
    }

    s_canvas.pushSprite(&M5.Display, 0, 0);
}

}  // namespace

// ---- Public API ----
void APV2ModeSetAddonEnabled(bool enabled) { s_enabled = enabled; }
bool APV2ModeIsAddonEnabled() { return s_enabled; }

lv_obj_t *APV2ModeGetScreen() {
    if (!s_screen) {
        s_screen = lv_obj_create(nullptr);
        lv_obj_clear_flag(s_screen, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(s_screen, screenmachine, LV_EVENT_SCREEN_LOADED, nullptr);
        lv_obj_set_style_bg_color(s_screen, lv_color_hex(0x000000), 0);
    }
    return s_screen;
}

bool APV2ModeOwnsActiveScreen() {
    return (s_screen != nullptr) && (lv_scr_act() == s_screen);
}

void APV2ModePrepareScreen() {
    APV2ModeGetScreen();
    s_ap.sendControlFn = writeControl;
    resetLocalEncoders();
    s_modifier_view = false;
    s_ap.baseIndex = 0;
    s_ap.modifierIndex = 0;

    if (!s_canvas_ready) {
        s_canvas.setPsram(true);
        s_canvas.setColorDepth(16);
        s_canvas_ready = (s_canvas.createSprite(320, 240) != nullptr);
    }

    if (!tryBootstrapModelFromLiveBle()) {
        s_ap.parseConfig("Top(0/100):Amount(0/100):StepA(0/100):StepB(0/100):StepC(0/100):StepD(0/100):Phase(0/100),"
                         "Bottom(0/100):Amount(0/100):StepA(0/100):StepB(0/100):StepC(0/100):StepD(0/100):Phase(0/100),"
                         "Rise(0/100):Amount(0/100):StepA(0/100):StepB(0/100):StepC(0/100):StepD(0/100):Phase(0/100),"
                         "Fall(0/100):Amount(0/100):StepA(0/100):StepB(0/100):StepC(0/100):StepD(0/100):Phase(0/100),"
                         "CurveIn(0/100):Amount(0/100):StepA(0/100):StepB(0/100):StepC(0/100):StepD(0/100):Phase(0/100),"
                         "CurveOut(0/100):Amount(0/100):StepA(0/100):StepB(0/100):StepC(0/100):StepD(0/100):Phase(0/100),"
                         "Speed(0/100):Amount(0/100):StepA(0/100):StepB(0/100):StepC(0/100):StepD(0/100):Phase(0/100)");
    }
    drawScreen();
}

void APV2ModeHandleScreen(const ButtonEvents &events) {
    if (!s_enabled) {
        goBackToAddons();
        return;
    }

    if (events.leftShort) {
        if (!s_preset_names.empty()) {
            s_preset_cursor = (s_preset_cursor + 1) % (int)s_preset_names.size();
            writeControl(":" + s_preset_names[s_preset_cursor]);
        }
        drawScreen();
        return;
    }
    if (events.rightShort) {
        s_modifier_view = !s_modifier_view;
        if (s_modifier_view) s_ap.modifierIndex = 0;
        drawScreen();
        return;
    }
    if (events.mxShort) {
        if (s_ap.speedValue() > 0.0f) s_ap.setSpeed(0);
        else s_ap.setSpeed(50);
        drawScreen();
        return;
    }

    if (!s_live_ap_available) tryBootstrapModelFromLiveBle();
    refreshFromLiveStatusIfDue();

    const int d1 = detentsFromEncoder(encoder1, &s_enc1);
    const int d2 = detentsFromEncoder(encoder2, &s_enc2);
    const int d3 = detentsFromEncoder(encoder3, &s_enc3);
    const int d4 = detentsFromEncoder(encoder4, &s_enc4);

    bool changed = false;
    if (d1 != 0) {
        int v = (int)(s_ap.speedValue() + 0.5f) + d1 * 2;
        if (v < 0) v = 0;
        s_ap.setSpeed(v);
        changed = true;
    }
    if (d2 != 0) {
        if (s_modifier_view) {
            int v = (int)(s_ap.selectedValue() + 0.5f) + d2;
            if (v < 0) v = 0;
            s_ap.setModifierValue(v);
        } else {
            int v = (int)(s_ap.selectedBaseValue() + 0.5f) + d2;
            if (v < 0) v = 0;
            s_ap.setBaseValue(v);
        }
        changed = true;
    }
    if (d3 != 0) {
        if (s_modifier_view) {
            int n = s_ap.modifierTabCount();
            if (n > 0) s_ap.modifierIndex = (s_ap.modifierIndex + n - 1) % n;
        } else {
            int n = s_ap.baseTabCount();
            if (n > 0) s_ap.baseIndex = (s_ap.baseIndex + n - 1) % n;
        }
        changed = true;
    }
    if (d4 != 0) {
        if (s_modifier_view) {
            int n = s_ap.modifierTabCount();
            if (n > 0) s_ap.modifierIndex = (s_ap.modifierIndex + 1) % n;
        } else {
            int n = s_ap.baseTabCount();
            if (n > 0) s_ap.baseIndex = (s_ap.baseIndex + 1) % n;
        }
        changed = true;
    }

    if (changed) drawScreen();
}

extern "C" void APV2ModeHandleScreen(const struct ButtonEvents *events) {
    if (!events) return;
    APV2ModeHandleScreen(*events);
}

lv_obj_t *APV2ModeGetBatteryTitleLabel() { return nullptr; }
lv_obj_t *APV2ModeGetBatteryValueLabel() { return nullptr; }
lv_obj_t *APV2ModeGetBatteryBar() { return nullptr; }
