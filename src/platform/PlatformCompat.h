#pragma once

#include <Arduino.h>

struct PlatformTouchPoint {
    int16_t x;
    int16_t y;
};

void platformInit();
void platformUpdate();

void platformSetChargeCurrent(uint16_t milliAmps);
void platformSetVibration(uint8_t intensity);
void platformSetBrightness(uint8_t brightness);
void platformPowerOff();

bool  platformIsCharging();
int   platformGetBatteryCurrent();
float platformGetBatteryVoltage();
int   platformGetBatteryLevel();

void    platformDisplayPushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t* pixels);
int32_t platformDisplayWidth();
int32_t platformDisplayHeight();
uint8_t platformDisplayGetRotation();
void    platformDisplaySetRotation(uint8_t rotation);
void    platformDisplaySetEpdFastest();

bool platformTouchRead(PlatformTouchPoint& point);