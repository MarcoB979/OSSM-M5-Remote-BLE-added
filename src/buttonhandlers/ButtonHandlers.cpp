// ---------------------------------------------------------------------------
// ButtonHandlers.cpp
// Implements OneButton callbacks for the three physical buttons and the
// vibration motor helper.
// ---------------------------------------------------------------------------

#include "ButtonHandlers.h"
#include <M5Unified.h>
#include <lvgl.h>
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
bool click3_short_waspressed   = false;
bool click3_long_waspressed    = false;
bool click3_double_waspressed  = false;

// ---------------------------------------------------------------------------
// Haptic feedback
// ---------------------------------------------------------------------------
void vibrate(int vbr_Intensity, int vbr_Length) {
    if (lv_obj_has_state(ui_vibrate, LV_STATE_CHECKED) == 1) {
        M5.Power.setVibration(vbr_Intensity);
        vTaskDelay(vbr_Length);
        M5.Power.setVibration(0);
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
    Button1.attachClick(mxclick);
    Button1.attachLongPressStart(mxlong);
    Button2.attachClick(click2);
    Button2.attachLongPressStart(click2long);
    Button3.attachClick(click3);
    Button3.attachLongPressStart(c3long);
    Button3.attachDoubleClick(c3double);
}

// ---------------------------------------------------------------------------
// OneButton callbacks
// ---------------------------------------------------------------------------
void mxclick() {
    vibrate();
    mxclick_short_waspressed = true;
}

void mxlong() {
    vibrate(200, 200);
    mxclick_long_waspressed = true;
}

void click2() {
    vibrate();
    click2_short_waspressed = true;
}

void click2long() {
    vibrate(200, 200);
    click2_long_waspressed = true;
}

void click3() {
    vibrate();
    click3_short_waspressed = true;
}

void c3long() {
    vibrate();
    click3_long_waspressed = true;
}

void c3double() {
    vibrate();
    click3_double_waspressed = true;
}
