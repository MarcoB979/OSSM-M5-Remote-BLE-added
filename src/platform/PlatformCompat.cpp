#include "PlatformCompat.h"

#ifdef CYD
  #include <SPI.h>
  #include <TFT_eSPI.h>
  #include <XPT2046_Touchscreen.h>

static TFT_eSPI s_tft;

#if defined(TOUCH_IRQ)
static XPT2046_Touchscreen s_touch(TOUCH_CS, TOUCH_IRQ);
#else
static XPT2046_Touchscreen s_touch(TOUCH_CS);
#endif

static bool s_touchReady = false;

static int16_t clamp16(int32_t v, int16_t lo, int16_t hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return (int16_t)v;
}

void platformInit()
{
    s_tft.init();
    s_tft.setRotation(1);
    s_tft.fillScreen(TFT_BLACK);
    s_tft.setSwapBytes(true);

    s_touch.begin();
    s_touch.setRotation(1);
    s_touchReady = true;
}

void platformUpdate() {}

void platformSetChargeCurrent(uint16_t milliAmps) { (void)milliAmps; }
void platformSetVibration(uint8_t intensity) { (void)intensity; }
void platformSetBrightness(uint8_t brightness) { (void)brightness; }

void platformPowerOff()
{
    esp_deep_sleep_start();
}

bool platformIsCharging() { return false; }
int  platformGetBatteryCurrent() { return 0; }
float platformGetBatteryVoltage() { return 0.0f; }
int  platformGetBatteryLevel() { return 100; }

void platformDisplayPushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t* pixels)
{
    s_tft.pushImage(x, y, w, h, const_cast<uint16_t*>(pixels));
}

int32_t platformDisplayWidth() { return s_tft.width(); }
int32_t platformDisplayHeight() { return s_tft.height(); }
uint8_t platformDisplayGetRotation() { return (uint8_t)s_tft.getRotation(); }
void platformDisplaySetRotation(uint8_t rotation) { s_tft.setRotation(rotation); }
void platformDisplaySetEpdFastest() {}

bool platformTouchRead(PlatformTouchPoint& point)
{
    if (!s_touchReady || !s_touch.touched()) return false;

    TS_Point p = s_touch.getPoint();
    // Typical CYD XPT2046 calibration range; tune in build flags if needed.
    constexpr int32_t xMin = 200;
    constexpr int32_t xMax = 3900;
    constexpr int32_t yMin = 200;
    constexpr int32_t yMax = 3900;

    const int32_t mappedX = map((int32_t)p.x, xMin, xMax, 0, platformDisplayWidth() - 1);
    const int32_t mappedY = map((int32_t)p.y, yMin, yMax, 0, platformDisplayHeight() - 1);
    point.x = clamp16(mappedX, 0, (int16_t)(platformDisplayWidth() - 1));
    point.y = clamp16(mappedY, 0, (int16_t)(platformDisplayHeight() - 1));
    return true;
}

#else

  #include <M5Unified.h>

void platformInit()
{
    auto cfg = M5.config();
    M5.begin(cfg);
}

void platformUpdate() { M5.update(); }

void platformSetChargeCurrent(uint16_t milliAmps)
{
    M5.Power.setChargeCurrent(milliAmps);
}

void platformSetVibration(uint8_t intensity)
{
    M5.Power.setVibration(intensity);
}

void platformSetBrightness(uint8_t brightness)
{
    M5.Display.setBrightness(brightness);
    M5.Lcd.setBrightness(brightness);
}

void platformPowerOff()
{
    M5.Power.powerOff();
}

bool platformIsCharging()
{
    auto state = M5.Power.isCharging();
    return state == m5::Power_Class::is_charging;
}

int platformGetBatteryCurrent() { return M5.Power.getBatteryCurrent(); }
float platformGetBatteryVoltage() { return M5.Power.getBatteryVoltage(); }
int platformGetBatteryLevel() { return M5.Power.getBatteryLevel(); }

void platformDisplayPushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t* pixels)
{
    M5.Display.pushImageDMA<uint16_t>(x, y, w, h, const_cast<uint16_t*>(pixels));
}

int32_t platformDisplayWidth() { return M5.Display.width(); }
int32_t platformDisplayHeight() { return M5.Display.height(); }
uint8_t platformDisplayGetRotation() { return M5.Display.getRotation(); }
void platformDisplaySetRotation(uint8_t rotation) { M5.Display.setRotation(rotation); }
void platformDisplaySetEpdFastest() { M5.Display.setEpdMode(epd_mode_t::epd_fastest); }

bool platformTouchRead(PlatformTouchPoint& point)
{
    if (M5.Touch.getCount() == 0) return false;
    auto touch = M5.Touch.getDetail(0);
    point.x = touch.x;
    point.y = touch.y;
    return true;
}

#endif