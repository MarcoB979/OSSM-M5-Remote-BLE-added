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
#include "addons/addons.h"
#include "communication/CommManager.h"
#include "communication/BleComm.h"
#include "communication/BleBackground.h"
#include "screens/ScreenHandler.h"
#include "display/DisplaySetup.h"
#include <M5Unified.h>
#include "language.h"
#include "network/WifiStation.h"
#include "network/ButtplugClient.h"
#include "network/ButtplugHub.h"
#include "network/ToyHub.h"
#include "network/OtaServer.h"
#include "network/ToyConfigWeb.h"

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
  bleBackgroundInit();
  addonsInit();
  commInit();
  displayInit();  // display, LVGL, touchpad
  languageInit(); // load persisted language before building UI
  ui_init();
  LogDebug("\n ui initialized");

  buttonInit();
  screenInit();  // Load NVS settings and apply to UI
  wifiStationInit();
  buttplugInit();
  buttplugHubInit();
  toyHubInit();
  otaServerInit();
  toyConfigWebInit();

  LogDebug("\n End setup");
}

void loop()
{
  // Tick the physical buttons first, every iteration, so a heavy render or
  // network tick later in the loop never delays click detection.
  Button1.tick();
  Button2.tick();
  Button3.tick();

  screen_power_tick();
  M5.update();
  lv_task_handler();
  addonsTick();
  wifiStationLoop();
  buttplugLoop();
  toyHubLoop();
  otaServerLoop();
  handleScreens();
  delay(1);
}

// Screen event callbacks and handler moved to src/screens/ScreenHandler.cpp
