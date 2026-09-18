#pragma once

// OtaServer.h — ElegantOTA web upload server. Serves http://<m5-ip>/update over
// WiFi so new firmware can be flashed without USB, whenever WiFi is connected.
// The same server also hosts the novice toy-config page (/toys).

// Forward-declare only. The full <ESPAsyncWebServer.h> include lives in
// OtaServer.cpp: its literals.h defines T_XXX constants (T_CONNECT, T_GET…)
// that collide with the T_XXX language macros in language.h, which main.cpp
// includes first.
class AsyncWebServer;

void otaServerInit();
void otaServerLoop();
AsyncWebServer& otaServerWebServer();  // shared server for config pages
