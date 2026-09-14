// Coyote addon — synchronizes a DG-Labs Coyote 2/3 eStim device's
// frequency/intensity to live OSSM rail telemetry (railPos/strokeSpeed/
// railAccel), via the vendored CoyoStim library (lib/CoyoStim).
//
// UI model: 4 range sliders (Frequency, Intensity, Speed Sens., Accel
// Sens.), each with a min and max handle.
//   - encoder1 selects which slider is active (highlighted).
//   - encoder2 adjusts the active slider's min handle.
//   - encoder3 adjusts the active slider's max handle.
// Frequency/Intensity are the output ceilings sent to the Coyote. Speed/
// Accel Sens. are the input ranges (of strokeSpeed%/railAccel%) that get
// mapped onto those output ceilings.

#include "Coyote.h"

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <Preferences.h>
#include <math.h>

#include "coyote.hpp"

#include "main.h"
#include "language.h"
#include "display/styles.h"
#include "ui/ui.h"
#include "ui/ui_helpers.h"
#include "buttonhandlers/ButtonHandlers.h"
#include "screens/ScreenHandler.h"
#include "communication/BleComm.h"
#include "config/debug.h"

namespace {

// ---- Persistent calibration (Preferences namespace "coyote") ----------
struct CoyoteSettings {
    float freqMin = 10.0f;       // Hz (10-50 Hz range for tactile e-stim)
    float freqMax = 35.0f;
    float intensityMin = 10.0f;  // % (0-100%)
    float intensityMax = 50.0f;
    float sensation = 50.0f;     // % (0-100%): controls motion wave ramp slope
    float channelMode = 2.0f;    // 0 = A, 1 = B, 2 = A-B (Both)
};

static CoyoteSettings s_settings;

static void loadSettings() {
    Preferences prefs;
    prefs.begin("coyote", true);
    s_settings.freqMin = prefs.getFloat("freqMin", s_settings.freqMin);
    s_settings.freqMax = prefs.getFloat("freqMax", s_settings.freqMax);
    s_settings.intensityMin = prefs.getFloat("intMin", s_settings.intensityMin);
    s_settings.intensityMax = prefs.getFloat("intMax", s_settings.intensityMax);
    s_settings.sensation = prefs.getFloat("sens", s_settings.sensation);
    s_settings.channelMode = prefs.getFloat("chanMode", s_settings.channelMode);
    prefs.end();
}

static void saveSettings() {
    Preferences prefs;
    prefs.begin("coyote", false);
    prefs.putFloat("freqMin", s_settings.freqMin);
    prefs.putFloat("freqMax", s_settings.freqMax);
    prefs.putFloat("intMin", s_settings.intensityMin);
    prefs.putFloat("intMax", s_settings.intensityMax);
    prefs.putFloat("sens", s_settings.sensation);
    prefs.putFloat("chanMode", s_settings.channelMode);
    prefs.end();
}

// ---- Slider row definitions --------------------------------------------
enum SliderRowType {
    ROW_TYPE_RANGE,   // dual handles (min..max)
    ROW_TYPE_SINGLE,  // single scalar slider (0..100)
    ROW_TYPE_OPTION   // discrete options (0="A", 1="B", 2="A-B")
};

struct SliderRowDef {
    const char* label;
    float* minValue;
    float* maxValue;
    int rangeMin;
    int rangeMax;
    SliderRowType type;
};

static SliderRowDef s_sliderDefs[4] = {
    { T_COYOTE_FREQUENCY, &s_settings.freqMin,      &s_settings.freqMax,      10, 50,  ROW_TYPE_RANGE },
    { T_COYOTE_INTENSITY, &s_settings.intensityMin, &s_settings.intensityMax,  0, 100, ROW_TYPE_RANGE },
    { T_COYOTE_SENSATION, &s_settings.sensation,    &s_settings.sensation,    0, 100, ROW_TYPE_SINGLE },
    { T_COYOTE_CHANNEL,   &s_settings.channelMode,  &s_settings.channelMode,  0, 2,   ROW_TYPE_OPTION },
};
static constexpr int NUM_RANGE_SLIDERS = 4;
static int s_activeSlider = 0;

// ---- Screen widgets ------------------------------------------------------
static lv_obj_t *s_screen = nullptr;
static lv_obj_t *s_title = nullptr;
static lv_obj_t *s_batt_title = nullptr;
static lv_obj_t *s_batt_value = nullptr;
static lv_obj_t *s_row_label[NUM_RANGE_SLIDERS] = { nullptr, nullptr, nullptr, nullptr };
static lv_obj_t *s_row_slider[NUM_RANGE_SLIDERS] = { nullptr, nullptr, nullptr, nullptr };
static lv_obj_t *s_row_value[NUM_RANGE_SLIDERS] = { nullptr, nullptr, nullptr, nullptr };
static lv_obj_t *s_button_left = nullptr;
static lv_obj_t *s_button_mid = nullptr;
static lv_obj_t *s_button_right = nullptr;
static lv_obj_t *s_button_left_text = nullptr;
static lv_obj_t *s_button_mid_text = nullptr;
static lv_obj_t *s_button_right_text = nullptr;

static long s_enc1 = 0;
static long s_enc2 = 0;
static long s_enc3 = 0;
static bool s_flush_buttons_once = false;
static bool s_is_on = false;

// ---- Connection state ----------------------------------------------------
static bool s_addon_enabled = false;
static bool s_ble_init = false;
static Coyote* s_coyote = nullptr;   // never deleted: destructor derefs bleClient unconditionally
static bool s_is_paired = false;
static bool s_channels_armed = false; // custom mode function + baseline power set
static uint32_t s_last_connect_attempt_ms = 0;
static constexpr uint32_t COYOTE_CONNECT_RETRY_MS = 4000;
static constexpr uint32_t COYOTE_BG_SCAN_MS = 150;
static constexpr uint32_t COYOTE_FG_SCAN_MS = 2500;

// Live mapped output, read by the custom mode functions on the library's own
// internal 100ms timer. Updated by CoyoteBackgroundTick() whenever fresh
// rail telemetry arrives.
static volatile int s_liveFrequencyHz = 10;
static volatile int s_liveIntensityPercent = 0;

static coyote_pattern coyoteCustomModeFnA(uint32_t &waveclock, uint32_t &cyclecount) {
    (void)waveclock;
    (void)cyclecount;
    coyote_pattern out;
    const int mode = (int)(s_settings.channelMode + 0.5f);
    if (mode == 0 || mode == 2) { // 0 = Channel A, 2 = A-B (Both)
        out.frequency = s_liveFrequencyHz;
        out.amplitude = s_liveIntensityPercent;
    } else {
        out.frequency = 0;
        out.amplitude = 0;
    }
    return out;
}

static coyote_pattern coyoteCustomModeFnB(uint32_t &waveclock, uint32_t &cyclecount) {
    (void)waveclock;
    (void)cyclecount;
    coyote_pattern out;
    const int mode = (int)(s_settings.channelMode + 0.5f);
    if (mode == 1 || mode == 2) { // 1 = Channel B, 2 = A-B (Both)
        out.frequency = s_liveFrequencyHz;
        out.amplitude = s_liveIntensityPercent;
    } else {
        out.frequency = 0;
        out.amplitude = 0;
    }
    return out;
}

static void coyoteInitBleOnce() {
    if (s_ble_init) return;
    if (!NimBLEDevice::isInitialized()) {
        NimBLEDevice::init("M5-OSSM-Remote");
    }
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);
    s_ble_init = true;
}

static void armChannelsOnce() {
    if (!s_coyote || s_channels_armed) return;
    s_coyote->chan_a().put_power_pc(100);
    s_coyote->chan_b().put_power_pc(100);
    s_coyote->chan_a().put_setmode(coyoteCustomModeFnA);
    s_coyote->chan_b().put_setmode(coyoteCustomModeFnB);
    s_channels_armed = true;
}

static bool coyoteTryConnect(bool force) {
    if (!s_addon_enabled) return false;

    if (s_coyote && s_coyote->get_isconnected()) {
        s_is_paired = true;
        armChannelsOnce();
        return true;
    }
    s_is_paired = false;
    s_channels_armed = false;

    const uint32_t nowMs = millis();
    if (!force && s_last_connect_attempt_ms != 0 && (nowMs - s_last_connect_attempt_ms) < COYOTE_CONNECT_RETRY_MS) {
        return false;
    }
    s_last_connect_attempt_ms = nowMs;


    coyoteInitBleOnce();
    if (!s_coyote) {
        s_coyote = new Coyote();
    }

    NimBLEScan* scanner = NimBLEDevice::getScan();
    if (!scanner) {
        return false;
    }

    scanner->stop();
    scanner->clearResults();
    scanner->setActiveScan(true);
    // Duty-cycled (interval > window) so the OSSM's own GATT connection
    // isn't starved while this addon scans in the background.
    scanner->setInterval(96);
    scanner->setWindow(32);

    const uint32_t scanMs = force ? COYOTE_FG_SCAN_MS : COYOTE_BG_SCAN_MS;
    NimBLEScanResults results = scanner->getResults(scanMs, false);

    NimBLEAdvertisedDevice* target = nullptr;
    for (int i = 0; i < results.getCount(); ++i) {
        NimBLEAdvertisedDevice* device = const_cast<NimBLEAdvertisedDevice*>(results.getDevice(i));
        if (device && Coyote::is_coyote(device)) {
            target = device;
            break;
        }
    }

    if (!target) {
        scanner->clearResults();
        return false;
    }

    const bool connected = s_coyote->connect_to_device(target);
    scanner->clearResults();

    if (!connected) {
        return false;
    }

    s_is_paired = true;
    armChannelsOnce();
    screenRequestStatusStripRefresh();
    return true;
}

// ---- Mapping: rail telemetry -> Coyote frequency/intensity --------------
static float mapClamped(float value, float inMin, float inMax, float outMin, float outMax) {
    if (inMax <= inMin) return outMin;
    float v = value;
    if (v < inMin) v = inMin;
    if (v > inMax) v = inMax;
    const float t = (v - inMin) / (inMax - inMin);
    return outMin + t * (outMax - outMin);
}

static void updateMapping() {
    if (!s_is_paired || !s_is_on) {
        s_liveIntensityPercent = 0;
#ifdef DEBUG_POSITION_STREAM
        static uint32_t s_lastGateLogMs = 0;
        const uint32_t nowMs = millis();
        if (nowMs - s_lastGateLogMs >= 1000) {
            s_lastGateLogMs = nowMs;
            LogDebugFormatted("[COYOTE] gated: paired=%d on=%d\n", s_is_paired, s_is_on);
        }
#endif
        return;
    }

    BleConfirmedValues confValues;
    const bool haveConfirmed = bleCommGetConfirmedValues(&confValues);
    BleRailTelemetry telemetry;
    const bool haveTelemetry = bleCommGetRailTelemetry(&telemetry);

    // Safety: no confirmed state, or OSSM not actively running -> silence output.
    if (!haveConfirmed || !OSSM_On) {
        s_liveIntensityPercent = 0;
#ifdef DEBUG_POSITION_STREAM
        static uint32_t s_lastGateLogMs2 = 0;
        const uint32_t nowMs2 = millis();
        if (nowMs2 - s_lastGateLogMs2 >= 1000) {
            s_lastGateLogMs2 = nowMs2;
            LogDebugFormatted("[COYOTE] silenced: haveConfirmed=%d OSSM_On=%d\n", haveConfirmed, OSSM_On);
        }
#endif
        return;
    }

    // 1. Distance d from nearest stroke endpoint (0% to 50%)
    float pos = telemetry.railPos;
    if (pos < 0.0f) pos = 50.0f; // fallback if telemetry position not yet received
    if (pos > 100.0f) pos = 100.0f;
    const float distFromEnd = fminf(pos, 100.0f - pos);

    // 2. Wave ramp threshold dRamp based on Sensation (0..100):
    //    Sensation = 100 -> instant ramp (dRamp = 1.0%)
    //    Sensation = 0   -> gradual ramp across half-stroke (dRamp = 50.0%)
    const float sensNorm = mapClamped(s_settings.sensation, 0.0f, 100.0f, 0.0f, 1.0f);
    const float dRamp = fmaxf(1.0f, 50.0f * (1.0f - sensNorm));

    // 3. Stroke wave ramp progress (0.0 at rail turnaround ends, 1.0 during active travel)
    const float rampProgress = mapClamped(distFromEnd, 0.0f, dRamp, 0.0f, 1.0f);

    // 4. OSSM Speed dampening: lower overall OSSM setpoint speed dampens max intensity
    const float speedScale = confValues.speed / 100.0f; // 0.0 to 1.0

    // 5. Calculate live Frequency and Intensity
    // Frequency ramps from freqMin (at turnaround) to freqMax (during travel)
    const float currentFreqHz = s_settings.freqMin + (s_settings.freqMax - s_settings.freqMin) * rampProgress;

    // Intensity ramps from intensityMin to effective intensityMax (dampened by OSSM speed)
    const float effIntensityMax = s_settings.intensityMin + (s_settings.intensityMax - s_settings.intensityMin) * speedScale;
    const float currentIntensityPct = s_settings.intensityMin + (effIntensityMax - s_settings.intensityMin) * rampProgress;

    int rawFreqByte = (int)(currentFreqHz + 0.5f);
    if (rawFreqByte < 10) rawFreqByte = 10;
    if (rawFreqByte > 240) rawFreqByte = 240;

    int intensityOut = (int)(currentIntensityPct + 0.5f);
    if (intensityOut < 0) intensityOut = 0;
    if (intensityOut > 100) intensityOut = 100;

    s_liveFrequencyHz = rawFreqByte;
    s_liveIntensityPercent = intensityOut;

#ifdef DEBUG_POSITION_STREAM
    static float s_lastLoggedRailPos = -999.0f;
    if (pos != s_lastLoggedRailPos) {
        s_lastLoggedRailPos = pos;
        LogDebugFormatted("[COYOTE] pos=%.1f d=%.1f ramp=%.2f ossmSpeed=%.0f -> freqHz=%d intensity=%d%% (chanMode=%d)\n",
                          pos, distFromEnd, rampProgress, confValues.speed, rawFreqByte, intensityOut, (int)(s_settings.channelMode + 0.5f));
    }
#endif
}

// ---- Screen construction --------------------------------------------------
static void applyRowHighlight(int index) {
    for (int i = 0; i < NUM_RANGE_SLIDERS; ++i) {
        if (!s_row_label[i]) continue;
        if (i == index) {
            lv_obj_set_style_text_opa(s_row_label[i], LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
        } else {
            lv_obj_set_style_text_opa(s_row_label[i], LV_OPA_60, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
}

static void refreshRowValueLabel(int index) {
    if (!s_row_value[index]) return;
    char buf[24];
    SliderRowDef &def = s_sliderDefs[index];
    if (def.type == ROW_TYPE_RANGE) {
        snprintf(buf, sizeof(buf), "%d-%d", (int)(*def.minValue + 0.5f), (int)(*def.maxValue + 0.5f));
    } else if (def.type == ROW_TYPE_SINGLE) {
        snprintf(buf, sizeof(buf), "%d%%", (int)(*def.minValue + 0.5f));
    } else if (def.type == ROW_TYPE_OPTION) {
        const int mode = (int)(*def.minValue + 0.5f);
        if (mode == 0) snprintf(buf, sizeof(buf), "A");
        else if (mode == 1) snprintf(buf, sizeof(buf), "B");
        else snprintf(buf, sizeof(buf), "A-B");
    }
    lv_label_set_text(s_row_value[index], buf);
}

static void refreshAllValueLabels() {
    for (int i = 0; i < NUM_RANGE_SLIDERS; ++i) {
        if (s_row_slider[i]) {
            SliderRowDef &def = s_sliderDefs[i];
            if (def.type == ROW_TYPE_RANGE) {
                lv_slider_set_mode(s_row_slider[i], LV_SLIDER_MODE_RANGE);
                lv_slider_set_left_value(s_row_slider[i], (int)(*def.minValue + 0.5f), LV_ANIM_OFF);
                lv_slider_set_value(s_row_slider[i], (int)(*def.maxValue + 0.5f), LV_ANIM_OFF);
            } else {
                lv_slider_set_mode(s_row_slider[i], LV_SLIDER_MODE_NORMAL);
                lv_slider_set_value(s_row_slider[i], (int)(*def.minValue + 0.5f), LV_ANIM_OFF);
            }
        }
        refreshRowValueLabel(i);
    }
    applyRowHighlight(s_activeSlider);
}

static void applyMidButtonState() {
    if (!s_button_mid || !s_button_mid_text) return;

    lv_obj_remove_style(s_button_mid, &style_button_m, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_remove_style(s_button_mid, &style_button_running, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_remove_style(s_button_mid, &style_button_running_pressed, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_remove_style(s_button_mid, &style_button_stopped, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_remove_style(s_button_mid, &style_button_stopped_pressed, LV_PART_MAIN | LV_STATE_PRESSED);

    if (s_is_on) {
        lv_obj_add_style(s_button_mid, &style_button_running, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_style(s_button_mid, &style_button_running_pressed, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_label_set_text(s_button_mid_text, T_STOP);
    } else {
        lv_obj_add_style(s_button_mid, &style_button_stopped, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_style(s_button_mid, &style_button_stopped_pressed, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_label_set_text(s_button_mid_text, T_START);
    }
    lv_obj_add_style(s_button_mid, &style_button_m_focused, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_refresh_style(s_button_mid, LV_PART_MAIN, LV_STYLE_PROP_ANY);
    lv_obj_invalidate(s_button_mid);
}

static void createRangeSliderRow(int index, int y) {
    SliderRowDef &def = s_sliderDefs[index];

    s_row_label[index] = lv_label_create(s_screen);
    lv_obj_set_width(s_row_label[index], lv_pct(95));
    lv_obj_set_height(s_row_label[index], LV_SIZE_CONTENT);
    lv_obj_set_x(s_row_label[index], 0);
    lv_obj_set_y(s_row_label[index], y);
    lv_obj_set_align(s_row_label[index], LV_ALIGN_CENTER);
    lv_label_set_text(s_row_label[index], def.label);
    lv_obj_set_style_text_font(s_row_label[index], &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    s_row_slider[index] = lv_slider_create(s_row_label[index]);
    if (def.type == ROW_TYPE_RANGE) {
        lv_slider_set_mode(s_row_slider[index], LV_SLIDER_MODE_RANGE);
        lv_slider_set_range(s_row_slider[index], def.rangeMin, def.rangeMax);
        lv_slider_set_left_value(s_row_slider[index], (int)(*def.minValue + 0.5f), LV_ANIM_OFF);
        lv_slider_set_value(s_row_slider[index], (int)(*def.maxValue + 0.5f), LV_ANIM_OFF);
    } else {
        lv_slider_set_mode(s_row_slider[index], LV_SLIDER_MODE_NORMAL);
        lv_slider_set_range(s_row_slider[index], def.rangeMin, def.rangeMax);
        lv_slider_set_value(s_row_slider[index], (int)(*def.minValue + 0.5f), LV_ANIM_OFF);
    }
    lv_obj_set_width(s_row_slider[index], 130);
    lv_obj_set_height(s_row_slider[index], 12);
    lv_obj_set_x(s_row_slider[index], -15);
    lv_obj_set_align(s_row_slider[index], LV_ALIGN_RIGHT_MID);
    lv_obj_add_style(s_row_slider[index], &style_slider_track[index], LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(s_row_slider[index], &style_slider_indicator[index], LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_add_style(s_row_slider[index], &style_slider_indicator[index], LV_PART_KNOB | LV_STATE_DEFAULT);

    s_row_value[index] = lv_label_create(s_row_label[index]);
    lv_obj_set_width(s_row_value[index], LV_SIZE_CONTENT);
    lv_obj_set_height(s_row_value[index], LV_SIZE_CONTENT);
    lv_obj_set_x(s_row_value[index], 95);
    lv_obj_set_y(s_row_value[index], 0);
    lv_obj_set_align(s_row_value[index], LV_ALIGN_LEFT_MID);
    lv_label_set_text(s_row_value[index], "0-0");
    lv_obj_add_style(s_row_value[index], &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(s_row_value[index], &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void createScreenIfNeeded() {
    if (s_screen != nullptr) return;

    s_screen = lv_obj_create(nullptr);
    ui_Coyote = s_screen;
    lv_obj_clear_flag(s_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(s_screen, screenmachine, LV_EVENT_SCREEN_LOADED, nullptr);
    lv_obj_add_style(s_screen, &style_background, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(s_screen, &style_option_bg, LV_PART_MAIN | LV_STATE_DEFAULT);

    s_title = lv_label_create(s_screen);
    lv_obj_set_align(s_title, LV_ALIGN_TOP_MID);
    lv_obj_set_y(s_title, 8);
    lv_label_set_text(s_title, T_COYOTE_SCREEN);
    lv_obj_set_style_text_font(s_title, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(s_title, &style_title_bar, LV_PART_MAIN | LV_STATE_DEFAULT);

    s_batt_title = lv_label_create(s_screen);
    lv_obj_set_width(s_batt_title, 85);
    lv_obj_set_height(s_batt_title, 30);
    lv_obj_set_align(s_batt_title, LV_ALIGN_TOP_RIGHT);
    lv_obj_set_x(s_batt_title, -6);
    lv_obj_set_y(s_batt_title, 8);
    lv_label_set_text(s_batt_title, T_BATT);

    s_batt_value = lv_label_create(s_batt_title);
    lv_obj_set_width(s_batt_value, LV_SIZE_CONTENT);
    lv_obj_set_height(s_batt_value, LV_SIZE_CONTENT);
    lv_obj_set_align(s_batt_value, LV_ALIGN_RIGHT_MID);
    lv_obj_set_x(s_batt_value, 0);
    lv_obj_set_y(s_batt_value, -8);
    lv_label_set_text(s_batt_value, T_BLANK);
    lv_obj_add_style(s_batt_value, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    createRangeSliderRow(0, -60);
    createRangeSliderRow(1, -22);
    createRangeSliderRow(2, 16);
    createRangeSliderRow(3, 54);

    s_button_left = lv_btn_create(s_screen);
    lv_obj_set_size(s_button_left, 100, 30);
    lv_obj_set_align(s_button_left, LV_ALIGN_BOTTOM_LEFT);
    lv_obj_set_x(s_button_left, 8);
    lv_obj_set_y(s_button_left, -8);
    lv_obj_add_style(s_button_left, &style_button_l, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(s_button_left, &style_button_l_focused, LV_PART_MAIN | LV_STATE_FOCUSED);
    s_button_left_text = lv_label_create(s_button_left);
    lv_obj_center(s_button_left_text);
    lv_label_set_text(s_button_left_text, T_MENU);

    s_button_mid = lv_btn_create(s_screen);
    lv_obj_set_size(s_button_mid, 100, 30);
    lv_obj_set_align(s_button_mid, LV_ALIGN_BOTTOM_MID);
    lv_obj_set_y(s_button_mid, -8);
    lv_obj_add_style(s_button_mid, &style_button_m, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(s_button_mid, &style_button_m_focused, LV_PART_MAIN | LV_STATE_FOCUSED);
    s_button_mid_text = lv_label_create(s_button_mid);
    lv_obj_center(s_button_mid_text);
    lv_label_set_text(s_button_mid_text, T_START);

    s_button_right = lv_btn_create(s_screen);
    lv_obj_set_size(s_button_right, 100, 30);
    lv_obj_set_align(s_button_right, LV_ALIGN_BOTTOM_RIGHT);
    lv_obj_set_x(s_button_right, -8);
    lv_obj_set_y(s_button_right, -8);
    lv_obj_add_style(s_button_right, &style_button_r, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(s_button_right, &style_button_r_focused, LV_PART_MAIN | LV_STATE_FOCUSED);
    s_button_right_text = lv_label_create(s_button_right);
    lv_obj_center(s_button_right_text);
    lv_label_set_text(s_button_right_text, T_BACK);
}

static void clearButtonFlags() {
    click2_short_waspressed = false;
    click2_long_waspressed = false;
    click2_double_waspressed = false;
    mxclick_short_waspressed = false;
    mxclick_long_waspressed = false;
    click3_short_waspressed = false;
    click3_long_waspressed = false;
    click3_double_waspressed = false;
    resetEncoderCounts();
}

static int detentsFromDelta(ESP32Encoder &enc, long *state) {
    long count = enc.getCount();
    if (count >= *state + 2) { *state = count; return 1; }
    if (count <= *state - 2) { *state = count; return -1; }
    return 0;
}

}  // namespace

lv_obj_t *ui_Coyote = nullptr;

void CoyotePrepareScreen() {
    createScreenIfNeeded();
    refreshAllValueLabels();
    applyMidButtonState();
    s_flush_buttons_once = true;
}

lv_obj_t *CoyoteGetScreen() {
    createScreenIfNeeded();
    return s_screen;
}

lv_obj_t *CoyoteGetBatteryTitleLabel() { return s_batt_title; }
lv_obj_t *CoyoteGetBatteryValueLabel() { return s_batt_value; }

bool CoyoteIsPaired() {
    return s_addon_enabled && s_is_paired && s_coyote && s_coyote->get_isconnected();
}

bool CoyoteIsOn() {
    return CoyoteIsPaired() && s_is_on;
}

bool CoyoteTryConnectNow() {
    loadSettings();
    return coyoteTryConnect(true);
}

bool CoyoteTryConnectBackground() {
    return coyoteTryConnect(false);
}

void CoyoteSetAddonEnabled(bool enabled) {
    const bool wasEnabled = s_addon_enabled;
    s_addon_enabled = enabled;
    if (!enabled) {
        s_is_paired = false;
        s_channels_armed = false;
        s_liveIntensityPercent = 0;
    } else if (!wasEnabled) {
        loadSettings();
    }
}

void CoyoteBackgroundTick() {
    // BLE (re)connection attempts are handled by ScreenHandler's shared,
    // staggered addon-connect scheduler (serviceAddonBackgroundConnect),
    // same as Eject/FistIT — this tick only runs the cheap telemetry->
    // output mapping every loop, no scanning.
    if (!s_addon_enabled || !CoyoteIsPaired()) return;
    updateMapping();

    // Drive CoyoStim's BLE write cycle ourselves from this (adequately
    // stacked) task instead of its own internal FreeRTOS timer, which
    // crashed under load — see lib/CoyoStim/src/coyote.cpp.
    static uint32_t s_lastTickMs = 0;
    const uint32_t nowMs = millis();
    if (nowMs - s_lastTickMs >= 100) {
        s_lastTickMs = nowMs;
        s_coyote->timer_callback(nullptr);
    }
}

void CoyoteHandleScreen(const ButtonEvents &events) {
    createScreenIfNeeded();

    if (!CoyoteIsPaired()) {
        (void)coyoteTryConnect(false);
    }

    if (s_flush_buttons_once) {
        clearButtonFlags();
        s_flush_buttons_once = false;
    }

    // Encoder1: select active slider.
    const int selectDelta = detentsFromDelta(encoder1, &s_enc1);
    if (selectDelta != 0) {
        s_activeSlider = (s_activeSlider + selectDelta + NUM_RANGE_SLIDERS) % NUM_RANGE_SLIDERS;
        applyRowHighlight(s_activeSlider);
    }

    SliderRowDef &active = s_sliderDefs[s_activeSlider];
    bool changed = false;

    if (active.type == ROW_TYPE_RANGE) {
        // Encoder2: adjust active slider's min handle.
        const int minDelta = detentsFromDelta(encoder2, &s_enc2);
        if (minDelta != 0) {
            float next = *active.minValue + (float)minDelta;
            if (next < active.rangeMin) next = (float)active.rangeMin;
            if (next > *active.maxValue - 1) next = *active.maxValue - 1;
            *active.minValue = next;
            changed = true;
        }

        // Encoder3: adjust active slider's max handle.
        const int maxDelta = detentsFromDelta(encoder3, &s_enc3);
        if (maxDelta != 0) {
            float next = *active.maxValue + (float)maxDelta;
            if (next > active.rangeMax) next = (float)active.rangeMax;
            if (next < *active.minValue + 1) next = *active.minValue + 1;
            *active.maxValue = next;
            changed = true;
        }
    } else {
        // Single value or discrete option row: Encoder 2 or 3 adjusts value
        const int enc2Delta = detentsFromDelta(encoder2, &s_enc2);
        const int enc3Delta = detentsFromDelta(encoder3, &s_enc3);
        const int totalDelta = enc3Delta - enc2Delta;
        if (totalDelta != 0) {
            float next = *active.minValue + (float)totalDelta;
            if (next < active.rangeMin) next = (float)active.rangeMin;
            if (next > active.rangeMax) next = (float)active.rangeMax;
            *active.minValue = next;
            *active.maxValue = next;
            changed = true;
        }
    }

    if (changed) {
        if (active.type == ROW_TYPE_RANGE) {
            lv_slider_set_left_value(s_row_slider[s_activeSlider], (int)(*active.minValue + 0.5f), LV_ANIM_OFF);
            lv_slider_set_value(s_row_slider[s_activeSlider], (int)(*active.maxValue + 0.5f), LV_ANIM_OFF);
        } else {
            lv_slider_set_value(s_row_slider[s_activeSlider], (int)(*active.minValue + 0.5f), LV_ANIM_OFF);
        }
        refreshRowValueLabel(s_activeSlider);
    }

    if (events.leftShort) {
        saveSettings();
        _ui_screen_change(ui_Menu, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
        clearButtonFlags();
    } else if (events.mxShort) {
        s_is_on = !s_is_on;
        if (!s_is_on) {
            s_liveIntensityPercent = 0;
        }
        applyMidButtonState();
        LogDebugFormatted("[COYOTE] Start/Stop pressed: on=%d paired=%d\n", s_is_on, CoyoteIsPaired());
        clearButtonFlags();
    } else if (events.rightShort) {
        saveSettings();
        _ui_screen_change(ui_Addons, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
        clearButtonFlags();
    }
}

extern "C" void CoyoteHandleScreen(const struct ButtonEvents *events) {
    if (!events) return;
    CoyoteHandleScreen(*events);
}
