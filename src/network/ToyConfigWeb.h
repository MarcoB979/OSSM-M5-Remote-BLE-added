#pragma once

// ToyConfigWeb.h — novice web page for adding BLE toys at runtime.
// Serves http://<m5-ip>/toys while WiFi is connected.

void toyConfigWebInit();  // call once from setup(), after otaServerInit()
