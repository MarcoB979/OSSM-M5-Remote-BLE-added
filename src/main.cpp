#pragma GCC optimize ("Ofast")
#include <ESP32Encoder.h>
#include <Arduino.h>
#include <Wire.h>
#include <lvgl.h>
#include <SPI.h>
#include "OneButton.h"    // must be before config.h which uses the OneButton type
#include "config/config.h"
#include "config/debug.h"
#include "main.h"
#include "ui/ui.h"
#include "buttonhandlers/ButtonHandlers.h"
#include "addons/Eject.h"
#include "addons/FistIT.h"
#include "addons/AP-mode.h"
#include "addons/addonsStreaming.h"
#include "communication/CommManager.h"
#include "communication/BleComm.h"
#include "screens/ScreenHandler.h"
#include "display/DisplaySetup.h"
#include <M5Unified.h>

// Shared state (defined here, declared extern in main.h)
bool dark_mode = false;
bool AtStartup = true;

// Shared numeric limits (defined here, declared extern in main.h)
float maxdepthinmm = 400.0f;
float speedlimit   = 300.0f;

// Encoder objects (declared extern in buttonhandlers/ButtonHandlers.h)
ESP32Encoder encoder1;
ESP32Encoder encoder2;
ESP32Encoder encoder3;
ESP32Encoder encoder4;

void setup(){
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Power.setChargeCurrent(BATTERY_CHARGE_CURRENT);
  LogDebug("\n Starting");      // Start LogDebug

  bleCommRegisterMainTask();
  EjectSetAddonEnabled(addonsIsEjectEnabled());
  FistITSetAddonEnabled(addonsIsFistITEnabled());
  APModeSetAddonEnabled(true);
  commInit();
  displayInit();  // display, LVGL, touchpad
  ui_init();
  LogDebug("\n ui initialized");

  buttonInit();
  screenInit();  // Load NVS settings and apply to UI

  LogDebug("\n End setup");
}

void loop()
{
  screen_power_tick();
  //LogDebug("Loop tick");
  M5.update();
  //LogDebug("M5 update done");
  lv_task_handler();
  //LogDebug("LVGL task handler done"); 
  Button1.tick();
  //LogDebug("Button1 tick done");
  Button2.tick();
  //LogDebug("Button2 tick done");
  Button3.tick();
  //LogDebug("Button3 tick done");
  handleScreens();
  //LogDebug("handleScreens done");
  delay(5);
}

// Screen event callbacks and handler moved to src/screens/ScreenHandler.cpp
