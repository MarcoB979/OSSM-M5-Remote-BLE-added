// SettingsScreen.cpp — settings carousel, encoder ramp profile, visual speed
// behaviour and manual rail length calibration.
//
// Extracted from ScreenHandler.cpp during the Phase B split.

#include "ScreenHandler.h"
#include "ScreenHandler_internal.h"

#include <lvgl.h>
#include <Preferences.h>
#include <M5Unified.h>
#include <Arduino.h>
#include <cmath>

#include "../ui/ui.h"
#include "../ui/ui_helpers.h"
#include "../main.h"
#include "../config/debug.h"
#include "../communication/CommManager.h"
#include "../config/config_ids.h"
#include "../communication/BleComm.h"
#include "../display/styles.h"
#include "language.h"
#include "../network/WifiStation.h"

// Natural speed curve (visual speed behaviour). Moved from ScreenHandler.cpp.
static constexpr float VIS_SPEED_STANDARD_MIN_FACTOR = 0.14f;
static constexpr float VIS_SPEED_STANDARD_KNEE_STROKE = 308.0f;
static constexpr float VIS_SPEED_STANDARD_POWER = 0.30f;
static constexpr float VIS_SPEED_NATURAL_MIN_FACTOR = 0.32f;
static constexpr float VIS_SPEED_NATURAL_KNEE_STROKE = 22.0f;
static constexpr float VIS_SPEED_NATURAL_POWER = 1.05f;
static constexpr float VIS_SPEED_TAMED_MIN_FACTOR = 0.40f;
static constexpr float VIS_SPEED_TAMED_KNEE_STROKE = 18.0f;
static constexpr float VIS_SPEED_TAMED_POWER = 0.90f;
static constexpr uint32_t ENCODER_RAMP_MEDIUM_MS = 120;
static constexpr uint32_t ENCODER_RAMP_FAST_MS = 45;
static constexpr int SETTINGS_CAROUSEL_VISIBLE_COUNT = 4;
static void runManualRailLengthCalibrationWorkflow();
static void manualRailLengthSetting_event_cb(lv_event_t* e);
static void EncRampProfile_event_cb(lv_event_t* e);
static void LanguageSetting_event_cb(lv_event_t* e);
static void WifiSetting_event_cb(lv_event_t* e);

// Selectable languages (Settings -> Language). Add new languages here once
// their table exists in language.cpp and LANGUAGE_XX is defined in language.h.
// The name is the NATIVE language name (never translated).
struct SelectableLanguage {
    LanguageId  id;
    const char* name;
};
static const SelectableLanguage s_selectable_languages[] = {
    { LANGUAGE_EN, "English" },
    // { LANGUAGE_DE, "Deutsch" },   // add when German is available
};
static constexpr int s_selectable_language_count =
    (int)(sizeof(s_selectable_languages) / sizeof(s_selectable_languages[0]));

int screenEncoderRampStep(int encoderIndex, long count)
{
    if (encoderIndex < 0 || encoderIndex >= 4) return 0;

    // ESP32Encoder attachHalfQuad() reports 2 raw counts per mechanical detent.
    // Convert to whole detents first so a fast flick that accumulates several
    // detents between loop iterations (loop() only polls every few ms) is
    // never silently dropped — only the *ramp multiplier* below depends on
    // timing, the base detent count is always honored in full.
    static constexpr long COUNTS_PER_DETENT = 2;
    const long magnitude = labs(count);
    const long detents = magnitude / COUNTS_PER_DETENT;
    if (detents < 1) return 0;

    if (s_encoder_ramp_profile == ENCODER_RAMP_NONE) {
        return (count > 0) ? (int)detents : -(int)detents;
    }

    const uint32_t nowMs = millis();
    const uint32_t lastMs = s_encoder_last_step_ms[encoderIndex];
    const uint32_t elapsedMs = (lastMs == 0U) ? UINT32_MAX : (nowMs - lastMs);
    s_encoder_last_step_ms[encoderIndex] = nowMs;

    int multiplier = 1;
    if (elapsedMs <= ENCODER_RAMP_FAST_MS) {
        if (s_encoder_ramp_profile == ENCODER_RAMP_MEDIUM) {
            multiplier = 3;
        } else if (s_encoder_ramp_profile == ENCODER_RAMP_HIGH) {
            multiplier = 4;
        } else if (s_encoder_ramp_profile == ENCODER_RAMP_AGGRESSIVE) {
            multiplier = 6;
        }
    } else if (elapsedMs <= ENCODER_RAMP_MEDIUM_MS) {
        if (s_encoder_ramp_profile == ENCODER_RAMP_MEDIUM) {
            multiplier = 2;
        } else if (s_encoder_ramp_profile == ENCODER_RAMP_HIGH) {
            multiplier = 3;
        } else if (s_encoder_ramp_profile == ENCODER_RAMP_AGGRESSIVE) {
            multiplier = 4;
        }
    }

    long step = detents * multiplier;
    if (step > 24) step = 24;

    return (count > 0) ? (int)step : -(int)step;
}

static void persistManualRailLengthSetting()
{
    Preferences prefs;
    prefs.begin("m5-ctnr", false);
    prefs.putFloat("RailLengthMm", s_manual_rail_length_mm);
    prefs.end();
}

static void persistEncRampProfileSetting()
{
    Preferences prefs;
    prefs.begin("m5-ctnr", false);
    prefs.putInt("EncRampProfile", s_encoder_ramp_profile);
    prefs.end();
}

static void persistSpeedBehaviorSetting()
{
    Preferences prefs;
    prefs.begin("m5-ctnr", false);
    prefs.putInt("SpeedBehavior", s_speed_behavior_profile);
    prefs.putBool("VisualSpeedLock", s_speed_behavior_profile != SPEED_BEHAVIOR_STANDARD);
    prefs.end();
}

float getDefaultManualRailLengthMm()
{
#ifdef RAIL_LENGTH
    return (float)RAIL_LENGTH;
#else
    return 0.0f;
#endif
}

static void updateManualRailLengthSettingLabel()
{
    if (!s_manual_rail_length_setting) return;

    char label[64];
    if (s_manual_rail_length_mm > 0.0f) {
        snprintf(label, sizeof(label), T_RAIL_LENGTH_VALUE, s_manual_rail_length_mm);
    } else {
        snprintf(label, sizeof(label), "%s", T_RAIL_LENGTH_PLAIN);
    }
    lv_checkbox_set_text(s_manual_rail_length_setting, label);
}

static void syncManualRailLengthSettingUi()
{
    if (!s_manual_rail_length_setting) return;

    s_manual_rail_length_ui_syncing = true;
    updateManualRailLengthSettingLabel();
    if (s_manual_rail_length_mm > 0.0f) {
        lv_obj_add_state(s_manual_rail_length_setting, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(s_manual_rail_length_setting, LV_STATE_CHECKED);
    }
    s_manual_rail_length_ui_syncing = false;
}

static const char* getEncRampProfileName(int profile)
{
    switch (profile) {
        case ENCODER_RAMP_NONE:
            return T_RAMP_NONE;
        case ENCODER_RAMP_HIGH:
            return T_RAMP_HIGH;
        case ENCODER_RAMP_AGGRESSIVE:
            return T_RAMP_AGGRESSIVE;
        case ENCODER_RAMP_MEDIUM:
        default:
            return T_RAMP_MEDIUM;
    }
}

void syncEncRampProfileSettingUi()
{
    if (!s_encoder_ramp_profile_setting) return;
    char label[64];
    snprintf(label, sizeof(label), T_ENCODER_RAMP, getEncRampProfileName(s_encoder_ramp_profile));
    lv_checkbox_set_text(s_encoder_ramp_profile_setting, label);
    lv_obj_add_state(s_encoder_ramp_profile_setting, LV_STATE_CHECKED);
}

static const char* getSpeedBehaviorName(int profile)
{
    switch (profile) {
        case SPEED_BEHAVIOR_NATURAL:
            return T_BEHAVIOR_NATURAL;
        case SPEED_BEHAVIOR_TAMED:
            return T_BEHAVIOR_TAMED;
        case SPEED_BEHAVIOR_STANDARD:
        default:
            return T_BEHAVIOR_STANDARD;
    }
}

void applySpeedBehavior(int profile)
{
    if (profile < SPEED_BEHAVIOR_STANDARD || profile > SPEED_BEHAVIOR_TAMED) {
        profile = SPEED_BEHAVIOR_STANDARD;
    }
    s_speed_behavior_profile = profile;
    s_visual_speed_lock = (s_speed_behavior_profile != SPEED_BEHAVIOR_STANDARD);

    if (!s_visual_speed_lock) {
        resetVisualSpeedRatioState();
    } else if (stroke > 0.001f) {
        s_visual_speed_stroke_product = speed;
        s_visual_speed_ratio_valid = true;
    }
}

void syncSpeedBehaviorSettingUi()
{
    if (!ui_visualSpeedLock) return;

    s_speed_behavior_ui_syncing = true;
    char label[64];
    snprintf(label, sizeof(label), T_SPEED_BEHAVIOR, getSpeedBehaviorName(s_speed_behavior_profile));
    lv_checkbox_set_text(ui_visualSpeedLock, label);

    if (s_speed_behavior_profile == SPEED_BEHAVIOR_STANDARD) {
        lv_obj_clear_state(ui_visualSpeedLock, LV_STATE_CHECKED);
    } else {
        lv_obj_add_state(ui_visualSpeedLock, LV_STATE_CHECKED);
    }
    s_speed_behavior_ui_syncing = false;
}

static void ensureManualRailLengthSetting()
{
    if (!ui_Settings || s_manual_rail_length_setting) return;

    s_manual_rail_length_setting = lv_checkbox_create(ui_Settings);
    lv_checkbox_set_text(s_manual_rail_length_setting, T_SET_RAIL_LENGTH);
    lv_obj_set_width(s_manual_rail_length_setting, LV_SIZE_CONTENT);
    lv_obj_set_height(s_manual_rail_length_setting, LV_SIZE_CONTENT);
    lv_obj_set_x(s_manual_rail_length_setting, 10);
    lv_obj_set_y(s_manual_rail_length_setting, 120);
    lv_obj_set_align(s_manual_rail_length_setting, LV_ALIGN_LEFT_MID);
    lv_obj_add_flag(s_manual_rail_length_setting, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_set_style_text_font(s_manual_rail_length_setting, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
    uiApplyCheckboxStyles(s_manual_rail_length_setting);
    lv_obj_add_event_cb(s_manual_rail_length_setting, manualRailLengthSetting_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    syncManualRailLengthSettingUi();
}

void ensureEncRampProfileSetting()
{
    if (!ui_Settings || s_encoder_ramp_profile_setting) return;

    s_encoder_ramp_profile_setting = lv_checkbox_create(ui_Settings);
    char encLabel[64];
    snprintf(encLabel, sizeof(encLabel), T_ENCODER_RAMP, getEncRampProfileName(s_encoder_ramp_profile));
    lv_checkbox_set_text(s_encoder_ramp_profile_setting, encLabel);
    lv_obj_set_width(s_encoder_ramp_profile_setting, LV_SIZE_CONTENT);
    lv_obj_set_height(s_encoder_ramp_profile_setting, LV_SIZE_CONTENT);
    lv_obj_set_x(s_encoder_ramp_profile_setting, 10);
    lv_obj_set_y(s_encoder_ramp_profile_setting, 150);
    lv_obj_set_align(s_encoder_ramp_profile_setting, LV_ALIGN_LEFT_MID);
    lv_obj_add_flag(s_encoder_ramp_profile_setting, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    
    lv_obj_set_style_text_font(s_encoder_ramp_profile_setting, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
    uiApplyCheckboxStyles(s_encoder_ramp_profile_setting);
    lv_obj_add_event_cb(s_encoder_ramp_profile_setting, EncRampProfile_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    syncEncRampProfileSettingUi();
}

static void manualRailLengthSetting_event_cb(lv_event_t* e)
{
    if (!e || s_manual_rail_length_ui_syncing) return;
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED || !s_manual_rail_length_setting) return;

    const bool checked = lv_obj_has_state(s_manual_rail_length_setting, LV_STATE_CHECKED);
    if (!checked) {
        s_manual_rail_length_mm = 0.0f;
        persistManualRailLengthSetting();
        syncManualRailLengthSettingUi();
        refreshSettingsCarousel();
        lv_refr_now(NULL);
        return;
    }

    runManualRailLengthCalibrationWorkflow();
}

static void EncRampProfile_event_cb(lv_event_t* e)
{
    if (!e || !s_encoder_ramp_profile_setting) return;

    const lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_VALUE_CHANGED) return;

    s_encoder_ramp_profile = (s_encoder_ramp_profile + 1) % 4;
    persistEncRampProfileSetting();
    syncEncRampProfileSettingUi();
    refreshSettingsCarousel();
    lv_refr_now(NULL);
}

static int selectableLanguageIndex(LanguageId lang)
{
    for (int i = 0; i < s_selectable_language_count; ++i) {
        if (s_selectable_languages[i].id == lang) return i;
    }
    return 0;
}

static const char* getSelectableLanguageName(LanguageId lang)
{
    return s_selectable_languages[selectableLanguageIndex(lang)].name;
}

void syncLanguageSettingUi()
{
    if (!s_language_setting) return;
    char label[64];
    snprintf(label, sizeof(label), T_LANGUAGE, getSelectableLanguageName(languageGetCurrent()));
    lv_checkbox_set_text(s_language_setting, label);
    lv_obj_add_state(s_language_setting, LV_STATE_CHECKED);
}

void ensureLanguageSetting()
{
    if (!ui_Settings || s_language_setting) return;

    s_language_setting = lv_checkbox_create(ui_Settings);
    char label[64];
    snprintf(label, sizeof(label), T_LANGUAGE, getSelectableLanguageName(languageGetCurrent()));
    lv_checkbox_set_text(s_language_setting, label);
    lv_obj_set_width(s_language_setting, LV_SIZE_CONTENT);
    lv_obj_set_height(s_language_setting, LV_SIZE_CONTENT);
    lv_obj_set_x(s_language_setting, 10);
    lv_obj_set_y(s_language_setting, 180);
    lv_obj_set_align(s_language_setting, LV_ALIGN_LEFT_MID);
    lv_obj_add_flag(s_language_setting, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_set_style_text_font(s_language_setting, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
    uiApplyCheckboxStyles(s_language_setting);
    lv_obj_add_event_cb(s_language_setting, LanguageSetting_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    syncLanguageSettingUi();
}

static void LanguageSetting_event_cb(lv_event_t* e)
{
    if (!e || !s_language_setting) return;
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;

    const int idx = selectableLanguageIndex(languageGetCurrent());
    const int next = (idx + 1) % s_selectable_language_count;  // no-op while only one language
    languageSet(s_selectable_languages[next].id);
    languageStore(s_selectable_languages[next].id);

    syncLanguageSettingUi();
    refreshSettingsCarousel();
    lv_refr_now(NULL);
}

void SpeedBehavior_event_cb(lv_event_t* e)
{
    if (!e || !ui_visualSpeedLock || s_speed_behavior_ui_syncing) return;

    const lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_VALUE_CHANGED) return;

    int nextProfile = s_speed_behavior_profile + 1;
    if (nextProfile > SPEED_BEHAVIOR_TAMED) {
        nextProfile = SPEED_BEHAVIOR_STANDARD;
    }

    applySpeedBehavior(nextProfile);
    syncSpeedBehaviorSettingUi();
    persistSpeedBehaviorSetting();
    refreshSettingsCarousel();
    lv_refr_now(NULL);
}

static void runManualRailLengthCalibrationWorkflow()
{
    if (!bleCommIsConnected() && !bleCommTryConnect()) {
        showNotification(T_CALIBRATE_TITLE, T_CALIBRATE_NO_BLE, 0, true, T_OK, false, nullptr, false);
        syncManualRailLengthSettingUi();
        refreshSettingsCarousel();
        lv_refr_now(NULL);
        return;
    }

    while (true) {
        const int startResult = showNotification(
            T_CALIBRATE_TITLE,
            T_CALIBRATE_PROMPT,
            0,
            true,  T_CANCEL_SHORT,
            true,  T_RESUME,
            false);

        if (startResult == NOTIFICATION_RESULT_LEFT) {
            syncManualRailLengthSettingUi();
            refreshSettingsCarousel();
            lv_refr_now(NULL);
            return;
        }
        if (startResult != NOTIFICATION_RESULT_RIGHT) {
            syncManualRailLengthSettingUi();
            refreshSettingsCarousel();
            lv_refr_now(NULL);
            return;
        }

        SendCommand(DEPTH, 100.0f, OSSM_ID);
        SendCommand(STROKE, 1.0f, OSSM_ID);
        SendCommand(SPEED, 5.0f, OSSM_ID);
        SendCommand(SENSATION, 0.0f, OSSM_ID);
        SendCommand(ON, 5.0f, OSSM_ID);

        const uint32_t moveStartMs = millis();
        while ((millis() - moveStartMs) < 10000U) {
            M5.update();
            lv_task_handler();
            vTaskDelay(pdMS_TO_TICKS(20));
        }

        SendCommand(OFF, 0.0f, OSSM_ID);
        vTaskDelay(pdMS_TO_TICKS(250));

        const int confirmResult = showNotification(
            T_CALIBRATE_CONFIRM_TITLE,
            T_CALIBRATE_CONFIRM_TEXT,
            0,
            true,  T_CONFIRM,
            true,  T_RETRY,
            false);

        if (confirmResult == NOTIFICATION_RESULT_RIGHT) {
            continue;
        }

        if (confirmResult == NOTIFICATION_RESULT_LEFT) {
            const float confirmedPosition = bleCommGetConfirmedPosition();
            if (confirmedPosition > 0.0f) {
                s_manual_rail_length_mm = confirmedPosition;
                persistManualRailLengthSetting();
                syncManualRailLengthSettingUi();
                refreshSettingsCarousel();
                lv_refr_now(NULL);
            }
            return;
        }

        return;
    }
}

void resetVisualSpeedRatioState()
{
    s_visual_speed_ratio_valid = false;
    s_visual_speed_stroke_product = 0.0f;
    s_visual_speed_last_commanded = -1.0f;
}

void updateVisualSpeedRatioFromUi(bool uiSpeedChanged, float uiSpeed, float uiStroke)
{
    if (!s_visual_speed_lock) {
        resetVisualSpeedRatioState();
        return;
    }

    if (uiStroke <= 0.001f) return;

    if (uiSpeedChanged || !s_visual_speed_ratio_valid) {
        s_visual_speed_stroke_product = uiSpeed;
        s_visual_speed_ratio_valid = true;
    }
}

float resolveVisualCompensatedSpeed(float uiSpeed, float uiStroke)
{
    if (!s_visual_speed_lock) return uiSpeed;
    if (!s_visual_speed_ratio_valid) return uiSpeed;
    if (uiStroke <= 0.001f) return uiSpeed;

    float minFactor = VIS_SPEED_STANDARD_MIN_FACTOR;
    float kneeStroke = VIS_SPEED_STANDARD_KNEE_STROKE;
    float curvePower = VIS_SPEED_STANDARD_POWER;
    if (s_speed_behavior_profile == SPEED_BEHAVIOR_NATURAL) {
        minFactor = VIS_SPEED_NATURAL_MIN_FACTOR;
        kneeStroke = VIS_SPEED_NATURAL_KNEE_STROKE;
        curvePower = VIS_SPEED_NATURAL_POWER;
    } else if (s_speed_behavior_profile == SPEED_BEHAVIOR_TAMED) {
        minFactor = VIS_SPEED_TAMED_MIN_FACTOR;
        kneeStroke = VIS_SPEED_TAMED_KNEE_STROKE;
        curvePower = VIS_SPEED_TAMED_POWER;
    }

    const float strokeNorm = fminf(1.0f, fmaxf(0.0f, uiStroke / kneeStroke));
    const float factor = minFactor + (1.0f - minFactor) * powf(strokeNorm, curvePower);
    float compensatedSpeed = s_visual_speed_stroke_product * factor;
    if (compensatedSpeed < 0.0f) compensatedSpeed = 0.0f;
    if (compensatedSpeed > speedlimit) compensatedSpeed = speedlimit;
    return compensatedSpeed;
}

void syncWifiSettingUi()
{
    if (!s_wifi_setting) return;
    char label[64];
    snprintf(label, sizeof(label), "%s: %s", T_WIFI_SETUP, wifiStationStatus());
    lv_checkbox_set_text(s_wifi_setting, label);
    lv_obj_add_state(s_wifi_setting, LV_STATE_CHECKED);
}

void ensureWifiSetting()
{
    if (!ui_Settings || s_wifi_setting) return;

    s_wifi_setting = lv_checkbox_create(ui_Settings);
    lv_checkbox_set_text(s_wifi_setting, T_WIFI_SETUP);
    lv_obj_set_width(s_wifi_setting, LV_SIZE_CONTENT);
    lv_obj_set_height(s_wifi_setting, LV_SIZE_CONTENT);
    lv_obj_set_x(s_wifi_setting, 10);
    lv_obj_set_y(s_wifi_setting, 210);
    lv_obj_set_align(s_wifi_setting, LV_ALIGN_LEFT_MID);
    lv_obj_add_flag(s_wifi_setting, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_set_style_text_font(s_wifi_setting, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
    uiApplyCheckboxStyles(s_wifi_setting);
    lv_obj_add_event_cb(s_wifi_setting, WifiSetting_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    syncWifiSettingUi();
}

static void WifiSetting_event_cb(lv_event_t* e)
{
    if (!e || !s_wifi_setting) return;
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;

    // Toggle: turn WiFi off if it's on (releases the radio for BLE), else open
    // the captive portal. The portal itself never releases the radio on a
    // failed attempt, so this is the only way back out.
    if (wifiStationIsWifiActive()) {
        wifiStationStopPortal();
    } else {
        wifiStationStartPortal();
    }
    lv_obj_add_state(s_wifi_setting, LV_STATE_CHECKED);  // act as a button
    syncWifiSettingUi();
}

int collectSettingsOptionObjects(lv_obj_t** outObjects, int maxObjects)
{
    if (!outObjects || maxObjects <= 0) return 0;

    ensureEncRampProfileSetting();
    ensureLanguageSetting();
    ensureWifiSetting();

    int count = 0;
    auto addObj = [&](lv_obj_t* obj) {
        if (!obj || count >= maxObjects) return;
        outObjects[count++] = obj;
    };

    addObj(ui_vibrate);
    addObj(ui_safeStartStop);
    addObj(ui_strokeinvert);
    addObj(ui_forceHome);
    addObj(ui_visualSpeedLock);
    addObj(ui_strokeDepthLink);
    addObj(s_encoder_ramp_profile_setting);
    addObj(s_language_setting);
    addObj(s_wifi_setting);
    return count;
}

void refreshSettingsCarousel()
{
    lv_obj_t* options[9] = {};
    const int optionCount = collectSettingsOptionObjects(options, 9);
    if (optionCount <= 0) {
        if (ui_Logo1) {
            lv_label_set_text(ui_Logo1, T_SCREEN_SETTINGS);
        }
        return;
    }

    if (s_settings_focus_index < 0) s_settings_focus_index = 0;
    if (s_settings_focus_index >= optionCount) s_settings_focus_index = optionCount - 1;

    if (s_settings_focus_index < s_settings_scroll_offset) {
        s_settings_scroll_offset = s_settings_focus_index;
    }
    if (s_settings_focus_index >= (s_settings_scroll_offset + SETTINGS_CAROUSEL_VISIBLE_COUNT)) {
        s_settings_scroll_offset = s_settings_focus_index - SETTINGS_CAROUSEL_VISIBLE_COUNT + 1;
    }

    if (s_settings_scroll_offset < 0) s_settings_scroll_offset = 0;
    const int maxOffset = (optionCount > SETTINGS_CAROUSEL_VISIBLE_COUNT)
                          ? (optionCount - SETTINGS_CAROUSEL_VISIBLE_COUNT)
                          : 0;
    if (s_settings_scroll_offset > maxOffset) s_settings_scroll_offset = maxOffset;

    const int slotY[SETTINGS_CAROUSEL_VISIBLE_COUNT] = {-60, -30, 0, 30};
    for (int i = 0; i < optionCount; ++i) {
        lv_obj_t* obj = options[i];
        if (!obj) continue;

        const bool visible = (i >= s_settings_scroll_offset) &&
                             (i < (s_settings_scroll_offset + SETTINGS_CAROUSEL_VISIBLE_COUNT));
        if (visible) {
            const int slot = i - s_settings_scroll_offset;
            lv_obj_set_y(obj, slotY[slot]);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }

        if (i == s_settings_focus_index) {
            lv_obj_add_state(obj, LV_STATE_FOCUSED);
        } else {
            lv_obj_clear_state(obj, LV_STATE_FOCUSED);
        }
    }

    if (ui_Logo1) {
        char title[64];
        snprintf(title, sizeof(title), "%s %d/%d", T_SCREEN_SETTINGS, s_settings_focus_index + 1, optionCount);
        lv_label_set_text(ui_Logo1, title);
    }
}

lv_obj_t* getSettingsFocusedObject()
{
    lv_obj_t* options[9] = {};
    const int optionCount = collectSettingsOptionObjects(options, 9);
    if (optionCount <= 0) return nullptr;

    if (s_settings_focus_index < 0) s_settings_focus_index = 0;
    if (s_settings_focus_index >= optionCount) s_settings_focus_index = optionCount - 1;
    return options[s_settings_focus_index];
}

// -------------------------------------------------------
