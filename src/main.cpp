#pragma GCC optimize ("Ofast")
#include <M5Unified.h>
#include <ESP32Encoder.h>
#include <Arduino.h>
#include <esp_heap_caps.h>
#include <Wire.h>
#include <lvgl.h>
#include <SPI.h>
#include "OneButton.h"    // must be before config.h which uses the OneButton type
#include "config.h"
#include "debug.h"
#include "main.h"
#include "ui/ui.h"
#include "buttonhandlers/ButtonHandlers.h"
#include "communication/EspNowComm.h"
#include "communication/CommManager.h"
#include "screens/ScreenHandler.h"
#include "display/DisplaySetup.h"

// Shared state (defined here, declared extern in main.h)
bool dark_mode = false;

// Shared numeric limits (defined here, declared extern in main.h)
float maxdepthinmm = 400.0f;
float speedlimit   = 300.0f;

// Encoder objects (declared extern in buttonhandlers/ButtonHandlers.h)
ESP32Encoder encoder1;
ESP32Encoder encoder2;
ESP32Encoder encoder3;
ESP32Encoder encoder4;

static void logSetupHeap(const char* tag) {
  const uint32_t freeInternal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
  const uint32_t largestInternal = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
  const uint32_t free8bit = heap_caps_get_free_size(MALLOC_CAP_8BIT);
  LogDebugFormatted("[SETUP][HEAP] %s: free_internal=%lu largest_internal=%lu free_8bit=%lu\n",
                    tag,
                    (unsigned long)freeInternal,
                    (unsigned long)largestInternal,
                    (unsigned long)free8bit);
}

void setup(){
  auto cfg = M5.config();
  M5.begin(cfg);

  M5.Power.setChargeCurrent(BATTERY_CHARGE_CURRENT);
  LogDebug("\n Starting");      // Start LogDebug
  logSetupHeap("start");

  espNowInit();
  logSetupHeap("after-espNowInit");
  commInit();
  logSetupHeap("after-commInit");
  displayInit();  // display, LVGL, touchpad
  logSetupHeap("after-displayInit");
  ui_init();
  logSetupHeap("after-ui_init");
  LogDebug("\n ui initialized");

  buttonInit();
  logSetupHeap("after-buttonInit");
  screenInit();  // Load NVS settings and apply to UI
  logSetupHeap("after-screenInit");

  LogDebug("\n End setup");
}

void loop()
{
  M5.update();
  lv_task_handler();
  Button1.tick();
  Button2.tick();
  Button3.tick();
  handleScreens();
  delay(5);
}

// Screen event callbacks and handler moved to src/screens/ScreenHandler.cpp
// ESP-NOW communication moved to src/communication/EspNowComm.cpp