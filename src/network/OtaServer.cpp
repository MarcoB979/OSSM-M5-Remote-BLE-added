// OtaServer.cpp — ElegantOTA upload server, active only while WiFi is up.

#include "OtaServer.h"

#include <ElegantOTA.h>
#include <ESPmDNS.h>
#include <ESPAsyncWebServer.h>

#include "WifiStation.h"
#include "../config/debug.h"

namespace {

constexpr const char* HOSTNAME = "m5-remote";

AsyncWebServer server(80);
bool s_initialized = false;
bool s_running = false;

}  // namespace

void otaServerInit() {
    if (s_initialized) return;
    s_initialized = true;
    ElegantOTA.begin(&server);  // registers the /update routes
    LogDebug("[OTA] ElegantOTA registered");
}

void otaServerLoop() {
    if (!s_initialized) return;

    const bool wifiUp = wifiStationIsConnected();
    if (wifiUp && !s_running) {
        s_running = true;
        // NOTE: do NOT call WiFi.setSleep(false) here. The ESP32 classic
        // requires WiFi modem sleep to stay ENABLED whenever Bluetooth is also
        // enabled (radio coexistence) — the WiFi driver aborts with
        // "Should enable WiFi modem sleep when both WiFi and Bluetooth are
        // enabled" if it is turned off.
        MDNS.begin(HOSTNAME);
        server.begin();
        LogDebugFormatted("[OTA] update page: http://%s.local/update  (ip %s)\n", HOSTNAME, wifiStationIp());
        LogDebugFormatted("[ToyHub] add toys:   http://%s.local/toys  (ip %s)\n", HOSTNAME, wifiStationIp());
    } else if (!wifiUp && s_running) {
        s_running = false;
        server.end();
        MDNS.end();
    }

    if (s_running) {
        // AsyncWebServer serves requests in its own background task — no
        // handleClient() pump is needed. ElegantOTA.loop() only handles its
        // post-update reboot delay.
        ElegantOTA.loop();
    }
}

AsyncWebServer& otaServerWebServer() { return server; }
