// PowerManagement.cpp — screensaver, deep sleep, battery UI/sampling, power tick.
//
// Extracted from ScreenHandler.cpp during the Phase B split.

#include "ScreenHandler.h"
#include "ScreenHandler_internal.h"

#include <lvgl.h>
#include <M5Unified.h>
#include <esp_sleep.h>
#include <Preferences.h>
#include <Arduino.h>

#include "../ui/ui.h"
#include "../ui/ui_helpers.h"
#include "../main.h"
#include "../config/config_power.h"
#include "../config/debug.h"
#include "../display/colors.h"
#include "../buttonhandlers/ButtonHandlers.h"
#include "../addons/AP-mode.h"
#include "../addons/AP-v2.h"
#include "language.h"

// -------------------------------------------------------
// Power Management / Sleep Helpers
// -------------------------------------------------------

void screensaver_check_activity()
{
    last_activity_ms = millis();
    if (screensaver_active) {
        M5.Lcd.setBrightness(screensaver_prev_brightness);
        screensaver_active = false;
    }
}

bool canEnterDeepSleep()
{
    // Do not enter deep sleep while the OSSM is actively running.
    return !onoff;
}

static bool areWakeButtonsReleased()
{
    return (digitalRead(Button1.pin()) == LOW) &&
           (digitalRead(Button2.pin()) == LOW) &&
           (digitalRead(Button3.pin()) == LOW);
}

static bool waitWakeButtonsReleasedStable(uint32_t stableMs, uint32_t timeoutMs)
{
    const uint32_t startMs = millis();
    uint32_t releasedSinceMs = 0;

    while ((millis() - startMs) < timeoutMs) {
        const bool released = areWakeButtonsReleased();
        if (released) {
            if (releasedSinceMs == 0) releasedSinceMs = millis();
            if ((millis() - releasedSinceMs) >= stableMs) return true;
        } else {
            releasedSinceMs = 0;
        }
        delay(5);
    }
    return false;
}

extern "C" void RestartM5()
{
    ESP.restart();
}

void enterDeepSleep()
{
    gpio_num_t mxPin    = static_cast<gpio_num_t>(Button1.pin());
    gpio_num_t leftPin  = static_cast<gpio_num_t>(Button2.pin());
    gpio_num_t rightPin = static_cast<gpio_num_t>(Button3.pin());
    uint64_t wakeMask   = (1ULL << mxPin) | (1ULL << leftPin) | (1ULL << rightPin);

    LogDebug("Entering deep sleep (wake on MX/left/right)");
    M5.Display.setBrightness(0);
    M5.Power.setVibration(0);

    // Guard against instant wake when any wake button is still held.
    if (!waitWakeButtonsReleasedStable(120, 1200)) {
        LogDebugFormatted("Deep sleep canceled: wake button(s) still active\n");
        screensaver_check_activity();
        return;
    }

    esp_sleep_enable_ext1_wakeup(wakeMask, ESP_EXT1_WAKEUP_ANY_HIGH);
    delay(50);
    esp_deep_sleep_start();
}

// -------------------------------------------------------
// -------------------------------------------------------
// Battery UI Helpers
// -------------------------------------------------------
static const char* battery_symbol_for_level(int level, bool isCharging)
{
    if (level < 0) level = 0;
    if (level > 100) level = 100;

    // Thresholds aligned to the non-linear Li-ion curve bins:
    // 0..7, 8..27, 28..57, 58..87, 88..100
    const char* baseSymbol = LV_SYMBOL_BATTERY_EMPTY;
    if (level >= 88) {
        baseSymbol = LV_SYMBOL_BATTERY_FULL;
    } else if (level >= 58) {
        baseSymbol = LV_SYMBOL_BATTERY_3;
    } else if (level >= 28) {
        baseSymbol = LV_SYMBOL_BATTERY_2;
    } else if (level >= 8) {
        baseSymbol = LV_SYMBOL_BATTERY_1;
    }

    if (!isCharging) {
        return baseSymbol;
    }

    // Show both fill level and charging state when plugged in. (now only charging symbol with percentage)
    static char chargingSymbol[24];
    snprintf(chargingSymbol, sizeof(chargingSymbol), "%s", LV_SYMBOL_CHARGE);
    return chargingSymbol;
}

void update_battery_icons_all_screens(int level, bool isCharging)
{
    const int valueLabelX = isCharging ? -25 : -40;

    lv_obj_t *batteryTitleLabels[] = {
        ui_Batt, ui_Batt1, ui_Batt2, ui_Batt3, ui_Batt4,
        ui_Batt5, ui_Batt6, ui_Batt7, ui_Batt8, ui_Batt9,
        APModeGetBatteryTitleLabel(),
        APV2ModeGetBatteryTitleLabel()
    };
    lv_obj_t *batteryValueLabels[] = {
        ui_BattValue, ui_BattValue1, ui_BattValue2, ui_BattValue3, ui_BattValue4,
        ui_BattValue5, ui_BattValue6, ui_BattValue7, ui_BattValue8, ui_BattValue9,
        APModeGetBatteryValueLabel(),
        APV2ModeGetBatteryValueLabel()
    };
    lv_obj_t *batteryBars[] = {
        ui_Battery, ui_Battery1, ui_Battery2, ui_Battery3, ui_Battery4,
        ui_Battery5, ui_Battery6, ui_Battery7, ui_Battery8, ui_Battery9,
        APModeGetBatteryBar(),
        APV2ModeGetBatteryBar()
    };

    for (lv_obj_t *label : batteryValueLabels) {
        if (label != nullptr) {
            lv_obj_clear_flag(label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_align(label, LV_ALIGN_RIGHT_MID);
            lv_obj_set_y(label, 0);
            lv_obj_set_style_text_color(label, lv_color_hex(getActiveTextPrimaryColor()), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(label, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
    for (lv_obj_t *bar : batteryBars) {
        if (bar != nullptr) {
            lv_obj_add_flag(bar, LV_OBJ_FLAG_HIDDEN);
        }
    }
    for (lv_obj_t *label : batteryTitleLabels) {
        if (label != nullptr) {
            lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(label, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }

    const char *symbol = battery_symbol_for_level(level, isCharging);
    //shows charging icon right from battery icon, but not the percentage
    char percentText[8];
    snprintf(percentText, sizeof(percentText), "%d%%", level);

    for (lv_obj_t *label : batteryTitleLabels) {
        if (label != nullptr) lv_label_set_text(label, symbol);
    }
    for (lv_obj_t *label : batteryValueLabels) {
        if (label != nullptr) {
            lv_obj_set_x(label, valueLabelX);
            lv_label_set_text(label, percentText);
        }
    }
}

// -------------------------------------------------------
// Battery Sampling / Smoothing Helpers
// -------------------------------------------------------
bool detectChargingNow()
{
    auto chargingState = M5.Power.isCharging();
    if (chargingState == m5::Power_Class::is_charging) return true;
    if (chargingState == m5::Power_Class::is_discharging) return false;
    // Fallback for unsupported/unknown PMIC charging state.
    return M5.Power.getBatteryCurrent() > 15;
}

bool getStableChargingState()
{
    static bool initialized  = false;
    static bool rawState     = false;
    static bool stableState  = false;
    static uint32_t rawSinceMs  = 0;
    static uint32_t lastPollMs  = 0;

    const uint32_t nowMs = millis();

    // Only poll I2C every 250 ms to avoid unnecessary I2C traffic on the
    // power bus, which can couple noise into the input-only GPIO group.
    if (!initialized || (nowMs - lastPollMs) >= 250UL) {
        lastPollMs = nowMs;
    } else {
        return stableState;
    }

    const bool nowRaw = detectChargingNow();

    if (!initialized) {
        initialized  = true;
        rawState     = nowRaw;
        stableState  = nowRaw;
        rawSinceMs   = nowMs;
        return stableState;
    }

    if (nowRaw != rawState) {
        rawState   = nowRaw;
        rawSinceMs = nowMs;
    }

    if (stableState != rawState && (nowMs - rawSinceMs) >= 800U) {
        stableState = rawState;
    }

    return stableState;
}

void maybeShowChargingWarning(bool isCharging)
{
    return; // Disable warning for now since the PMIC behavior seems stable and the warning can be confusing if it shows up due to a single noisy reading.
    
    static bool shownForCurrentChargeSession = false;

    if (!isCharging) {
        shownForCurrentChargeSession = false;
        return;
    }
    if (shownForCurrentChargeSession) return;

    shownForCurrentChargeSession = true;
    showNotification(
        T_CHARGING_WARNING_TITLE,
        T_CHARGING_WARNING_TEXT,
        5000,
        false, nullptr,
        false, nullptr,
        false);
}

static int estimateBatteryPercentFromVoltageMv(float batteryMv)
{

    // 1. Get raw voltage and charging state from Core2 AXP192
    bool isCharging = detectChargingNow();
    //return batteryMv;
    // 2. Corrected Hardware Offset Calibration
    if (!isCharging) {
        // When running on battery, load drags voltage down. 
        // Adding 60mV brings it back to the true chemical state.
        batteryMv += 60.0f; 
    } else {
        // When charging, voltage reads high. Reduce slightly to match real capacity.
        batteryMv -= 40.0f;
    }

    // Non-linear Li-ion OCV-inspired mapping (mV -> percent), then interpolate.
    // This avoids the "too optimistic" mid-range values from linear mapping.
    struct BatteryCurvePoint {
        float mv;
        int pct;
    };
    // 3. Your optimized curve for Core2 hardware
    static const BatteryCurvePoint curve[] = {
        {3400.0f, 0}, {3500.0f, 5}, {3600.0f, 15}, {3650.0f, 30}, {3700.0f, 50},
        {3750.0f, 65}, {3800.0f, 80}, {3950.0f, 95}, {4100.0f, 100}
    };

    const int n = (int)(sizeof(curve) / sizeof(curve[0]));
    if (batteryMv <= curve[0].mv) return curve[0].pct;
    if (batteryMv >= curve[n - 1].mv) return curve[n - 1].pct;

    for (int i = 0; i < n - 1; ++i) {
        const BatteryCurvePoint &a = curve[i];
        const BatteryCurvePoint &b = curve[i + 1];
        if (batteryMv >= a.mv && batteryMv <= b.mv) {
            const float t = (batteryMv - a.mv) / (b.mv - a.mv);
            int pct = (int)(a.pct + t * (float)(b.pct - a.pct) + 0.5f);
            return (pct < 0) ? 0 : (pct > 100) ? 100 : pct;
        }
    }
    return 0;
}

int readBatteryPercentForUi(bool isCharging)
{
    const float battMv = M5.Power.getBatteryVoltage();
    return estimateBatteryPercentFromVoltageMv(battMv); //temporary change to check if percentage is handled better now
    if (!isCharging) {
        if (battMv > 1000.0f) {
            return estimateBatteryPercentFromVoltageMv(battMv);
        }
    }
    LogDebugFormatted("Battery voltage too low or charging, using M5.Power.getBatteryLevel() instead\n");
    return M5.Power.getBatteryLevel();
}

int getSmoothedBatteryLevel(bool isCharging)
{
    static uint32_t lastSampleMs   = 0;
    static bool     wasCharging    = false;
    static uint32_t disconnectedMs = 0;
    static float    emaLevel       = -1.0f;
    static int      displayedLevel = -1;

    const uint32_t now = millis();

    if (wasCharging && !isCharging) disconnectedMs = now;
    wasCharging = isCharging;

    const bool inSettlingWindow = (!isCharging && (now - disconnectedMs) < 120000UL);

    if (emaLevel < 0.0f) {
        emaLevel       = (float)readBatteryPercentForUi(isCharging);
        displayedLevel = (int)(emaLevel + 0.5f);
        lastSampleMs   = now;
        if (!isCharging) disconnectedMs = now;
    }

    if (inSettlingWindow) return displayedLevel;

    if (now - lastSampleMs >= 10000UL || lastSampleMs == 0) {
        lastSampleMs = now;
        const float raw = (float)readBatteryPercentForUi(isCharging);
        emaLevel       = 0.1f * raw + 0.9f * emaLevel;
        displayedLevel = (int)(emaLevel + 0.5f);
        if (displayedLevel < 0)   displayedLevel = 0;
        if (displayedLevel > 100) displayedLevel = 100;
    }

    return displayedLevel;
}

// -------------------------------------------------------
// Screen Power Tick
// -------------------------------------------------------
void screen_power_tick()
{
    // Only treat encoder movement as activity when the counts change since
    // the last tick. Some encoder implementations leave a non-zero count
    // value until explicitly cleared which would otherwise constantly
    // retrigger the screensaver activity check.
    static long s_prev_encoder_counts[4] = {0, 0, 0, 0};
    long c1 = encoder1.getCount();
    long c2 = encoder2.getCount();
    long c3 = encoder3.getCount();
    long c4 = encoder4.getCount();
    if (c1 != s_prev_encoder_counts[0] || c2 != s_prev_encoder_counts[1] ||
        c3 != s_prev_encoder_counts[2] || c4 != s_prev_encoder_counts[3]) {
        screensaver_check_activity();
        s_prev_encoder_counts[0] = c1;
        s_prev_encoder_counts[1] = c2;
        s_prev_encoder_counts[2] = c3;
        s_prev_encoder_counts[3] = c4;
    }

    if (!screensaver_active && (millis() - last_activity_ms > (unsigned long)screensaver_timeout_ms)) {
        screensaver_prev_brightness = g_brightness_value;
        M5.Lcd.setBrightness(screensaver_dim_brightness);
        screensaver_active = true;
    }

#if AUTO_IDLE_DEEP_SLEEP_ENABLED == 1
    if (millis() - last_activity_ms > deep_sleep_timeout_ms) {
        if (canEnterDeepSleep()) {
            vibrate(1000, 255);
                const int result = showNotification(
                T_SHUTDOWN_SLEEP_TITLE,
                T_SHUTDOWN_SLEEP_TEXT,
            60000,
            true,  T_CANCEL,
            false,  nullptr,
            false);

            if (result != NOTIFICATION_RESULT_LEFT) {
                M5.Power.powerOff();
            }
        }
    }
#endif
}

// -------------------------------------------------------
// Settings Menu Actions
// -------------------------------------------------------
extern "C" void menuSleepAction(void)
{
    const int result = showNotification(
        T_SLEEP_CONFIRM_TITLE,
        T_SLEEP_CONFIRM_TEXT,
        0,
        true,  T_YES,
        true,  T_NO,
        false);

    if (result == NOTIFICATION_RESULT_LEFT) {
        enterDeepSleep();
    }
}

extern "C" void menuRestartAction(void)
{
    const int result = showNotification(
        T_RESTART,
        T_RESTART_CONFIRM_TEXT,
        0,
        true,  T_YES,
        true,  T_NO,
        false);

    if (result == NOTIFICATION_RESULT_LEFT) {
        esp_restart();
    }
}

// -------------------------------------------------------
