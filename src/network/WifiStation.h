#pragma once

// WifiStation.h — WiFi STA management for the M5 remote's network features
// (buttplug.io bridge + OTA). Credentials are entered through a WiFiManager
// captive portal; once saved, the M5 auto-connects on boot. WiFi stays off
// until the user has configured it, so BLE/ESP-NOW radio time and battery are
// not wasted.

void wifiStationInit();            // call once from setup()
void wifiStationLoop();            // call every loop() iteration
void wifiStationStartPortal();     // open the captive portal (non-blocking)
void wifiStationStopPortal();      // stop the portal and switch WiFi off (frees radio for BLE)
bool wifiStationIsWifiActive();    // true when the WiFi radio is on (any mode)
bool wifiStationIsConnected();     // true when WL_CONNECTED
bool wifiStationHasCredentials();  // true when an SSID has been saved
const char* wifiStationStatus();   // short human-readable status
const char* wifiStationIp();       // dotted-quad IP or "" when offline
