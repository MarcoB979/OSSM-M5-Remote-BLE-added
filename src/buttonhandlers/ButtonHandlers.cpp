// ---------------------------------------------------------------------------
// ButtonHandlers.cpp
// Implements OneButton callbacks for the three physical buttons and the
// vibration motor helper.
// ---------------------------------------------------------------------------
#include <M5Unified.h>
#include "ButtonHandlers.h"
#include <lvgl.h>
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include "../ui/ui.h"        // for ui_vibrate
#include "../config/config_pins.h"  // for ENC_x_CLK/DT pin defines (no object definitions)

// ---------------------------------------------------------------------------
// Button press state flags
// Cleared at the end of every loop() iteration in main.cpp
// ---------------------------------------------------------------------------
bool mxclick_short_waspressed  = false;
bool mxclick_long_waspressed   = false;
bool click2_short_waspressed   = false;
bool click2_long_waspressed    = false;
bool click2_double_waspressed  = false;
bool click3_short_waspressed   = false;
bool click3_long_waspressed    = false;
bool click3_double_waspressed  = false;

// FreeRTOS timer handle for haptic pulse timeout
static TimerHandle_t g_hapticTimer = nullptr;
static int g_hapticIntensity = 0;

// ---------------------------------------------------------------------------
// Haptic timer callback - turns off motor after pulse duration
// ---------------------------------------------------------------------------
static void hapticTimerCallback(TimerHandle_t xTimer) {
    (void)xTimer;  // Unused parameter
    M5.Power.setVibration(0);
    g_hapticIntensity = 0;
}

// ---------------------------------------------------------------------------
// Haptic feedback
// ---------------------------------------------------------------------------
void vibrate(int vbr_Intensity, int vbr_Length) {
    if (lv_obj_has_state(ui_vibrate, LV_STATE_CHECKED) == 1) {
        if (vbr_Length < 1) vbr_Length = 1;
        
        g_hapticIntensity = vbr_Intensity;
        M5.Power.setVibration(g_hapticIntensity);
        
        // Create timer on first call, or reuse existing
        if (g_hapticTimer == nullptr) {
            g_hapticTimer = xTimerCreate(
                "HapticTimer",           // Timer name
                pdMS_TO_TICKS(1),        // Initial period (will be updated)
                pdFALSE,                 // Don't auto-reload
                nullptr,                 // Timer ID
                hapticTimerCallback      // Callback function
            );
        }
        
        // Restart timer with new duration
        if (g_hapticTimer != nullptr) {
            xTimerChangePeriod(g_hapticTimer, pdMS_TO_TICKS(vbr_Length), 0);
        }
    }
}

// ---------------------------------------------------------------------------
// Initialisation
// ---------------------------------------------------------------------------
void buttonInit() {
    encoder1.attachHalfQuad(ENC_1_CLK, ENC_1_DT);
    encoder2.attachHalfQuad(ENC_2_CLK, ENC_2_DT);
    encoder3.attachHalfQuad(ENC_3_CLK, ENC_3_DT);
    encoder4.attachHalfQuad(ENC_4_CLK, ENC_4_DT);

    // Tune click timing for encoder push-buttons:
    // - Shorter click window makes double-click recognition feel snappier.
    // - Keep long-press threshold unchanged.
    // - Slightly lower debounce keeps fast clicks reliable without phantom presses.
    Button2.setDebounceMs(30);
    Button3.setDebounceMs(30);
    Button2.setClickMs(200);
    Button3.setClickMs(200);
    Button2.setPressMs(800);
    Button3.setPressMs(800);
    Button1.setDebounceMs(30);
    Button1.setClickMs(50);  //candidate for speed improvement: 100ms click window for encoder push-button makes double-click recognition feel snappier.
    Button1.setPressMs(800);

    Button1.attachClick(mxclick);
    Button1.attachLongPressStart(mxlong);
    Button2.attachClick(click2);
    Button2.attachLongPressStart(click2long);
    Button2.attachDoubleClick(c2double);
    Button3.attachClick(click3);
    Button3.attachLongPressStart(c3long);
    Button3.attachDoubleClick(c3double);
}

// ---------------------------------------------------------------------------
// OneButton callbacks
// ---------------------------------------------------------------------------
void mxclick() {
    mxclick_short_waspressed = true;
    vibrate(200, 200);
}

void mxlong() {
    mxclick_long_waspressed = true;
    vibrate(200, 200);
}

void click2() {
    click2_short_waspressed = true;
    vibrate(200, 200);
}

void click2long() {
    click2_long_waspressed = true;
    vibrate(200, 200);
}

void c2double() {
    click2_double_waspressed = true;
    vibrate(200, 200);
}

void click3() {
    click3_short_waspressed = true;
    vibrate(200, 200);
}

void c3long() {
    click3_long_waspressed = true;
    vibrate(200, 200);
}

void c3double() {
    click3_double_waspressed = true;
    vibrate(200, 200);
}
