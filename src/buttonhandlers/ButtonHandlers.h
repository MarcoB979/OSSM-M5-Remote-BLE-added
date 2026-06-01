#pragma once

#include <ESP32Encoder.h>
#include "OneButton.h"

// ---------------------------------------------------------------------------
// ButtonHandlers.h
// Handles physical button press events and the vibration motor feedback.
// Future: add BLE button mappings here alongside ESP-NOW ones.
// ---------------------------------------------------------------------------

// Encoder and button objects defined in main.cpp / config.h
extern ESP32Encoder encoder1;
extern ESP32Encoder encoder2;
extern ESP32Encoder encoder3;
extern ESP32Encoder encoder4;
extern OneButton Button1;
extern OneButton Button2;
extern OneButton Button3;

// Button press state flags – set by ISR-like callbacks, cleared in loop()
extern bool mxclick_short_waspressed;
extern bool mxclick_long_waspressed;
extern bool click2_short_waspressed;
extern bool click2_long_waspressed;
extern bool click3_short_waspressed;
extern bool click3_long_waspressed;
extern bool click3_double_waspressed;

// Haptic feedback helper (defaults: medium intensity, short pulse)
void vibrate(int vbr_Intensity = 200, int vbr_Length = 100);

// Initialise encoders and attach OneButton callbacks – call once from setup()
void buttonInit();

// OneButton callbacks
void mxclick();
void mxlong();
void click2();
void click2long();
void click3();
void c3long();
void c3double();
