// WifiStation.cpp — WiFi STA management via WiFiManager captive portal.
//
// Kept deliberately small: it only owns radio bring-up and the credential
// portal. The buttplug/OTA features will build on wifiStationIsConnected().

#include "WifiStation.h"

#include <Preferences.h>
#include <WiFi.h>
#include <WiFiManager.h>

#include "ButtplugClient.h"
#include "../communication/BleComm.h"

namespace {

constexpr const char* AP_NAME = "OSSM-M5-Remote";
constexpr const char* HOSTNAME = "m5-remote";
constexpr const char* PREFS_NS = "network";
constexpr const char* PREFS_BP_HOST = "bp_host";
constexpr const char* PREFS_BP_PORT = "bp_port";

WiFiManager wm;
WiFiManagerParameter* paramBpHost = nullptr;
WiFiManagerParameter* paramBpPort = nullptr;
bool s_initialized = false;
bool s_configured = false;
char s_status[48] = "off";
char s_ip[16] = "";

}  // namespace

void wifiStationInit() {
    if (s_initialized) return;
    s_initialized = true;

    // Our own "has the user ever configured WiFi" flag. WiFiManager stores the
    // actual credentials itself; this just tells us whether to auto-connect.
    Preferences prefs;
    prefs.begin(PREFS_NS, true);
    s_configured = prefs.getBool("wifi_configured", false);
    const String savedHost = prefs.getString(PREFS_BP_HOST, "");
    const uint16_t savedPort = prefs.getUShort(PREFS_BP_PORT, 12345);
    prefs.end();

    WiFi.useStaticBuffers(true);
    // NOTE: do NOT call WiFi.setSleep(false) here — WiFi is not started yet
    // (autoConnect runs below). Disabling modem sleep before the WiFi driver is
    // up corrupts the coexistence state and makes esp_bt_controller_enable()
    // abort in coex_core_enable when the toy hub initialises NimBLE later in
    // setup(). It is applied safely in otaServerLoop() once WiFi is connected.
    wm.setConfigPortalBlocking(false);
    wm.setConnectTimeout(15);
    wm.setConnectRetries(3);
    wm.setEnableConfigPortal(true);
    wm.setTitle("OSSM M5 Remote");

    // Also collect the Intiface server address in the same portal form.
    paramBpHost = new WiFiManagerParameter(PREFS_BP_HOST, "Intiface host", savedHost.c_str(), 40);
    paramBpPort = new WiFiManagerParameter(PREFS_BP_PORT, "Intiface port", String(savedPort).c_str(), 6);
    wm.addParameter(paramBpHost);
    wm.addParameter(paramBpPort);

    wm.setSaveConfigCallback([]() {
        Preferences p;
        p.begin(PREFS_NS, false);
        p.putBool("wifi_configured", true);
        p.end();

        String host = paramBpHost->getValue();
        host.trim();
        int port = atoi(paramBpPort->getValue());
        if (port <= 0 || port > 65535) port = 12345;
        buttplugSetServer(host.c_str(), (uint16_t)port);
    });

    // Only bring the radio up once credentials exist; otherwise WiFi stays off
    // until the user opens the portal from Settings.
    if (s_configured) {
        WiFi.setHostname(HOSTNAME);  // helps some routers resolve the bare name
        wm.autoConnect(AP_NAME);
    }
}

void wifiStationLoop() {
    if (!s_initialized) return;
    if (WiFi.getMode() == WIFI_MODE_NULL) return;
    wm.process();
}

void wifiStationStartPortal() {
    if (!s_initialized) wifiStationInit();
    // The ESP32 (Core2) cannot hold NimBLE and the WiFi captive portal
    // (DHCP/DNS/WebServer) in its ~320 KB of internal SRAM at the same time: a
    // client connecting to the AP starves the heap (esp_timer_create ->
    // ESP_ERR_NO_MEM). Release BLE before bringing the portal up, then restore
    // it once the portal is closed.
    bleCommSuspend();
    wm.setConfigPortalBlocking(false);
    wm.startConfigPortal(AP_NAME);
}

void wifiStationStopPortal() {
    // Turn the radio fully off so BLE gets the 2.4 GHz radio back. A failed or
    // abandoned captive-portal attempt otherwise leaves the M5 in AP mode,
    // which breaks BLE connections (host reset / characteristic discovery rc=7).
    wm.stopConfigPortal();
    WiFi.disconnect(false, false);
    WiFi.mode(WIFI_OFF);
    bleCommResume();
}

bool wifiStationIsWifiActive() {
    return WiFi.getMode() != WIFI_MODE_NULL;
}

bool wifiStationIsConnected() {
    return WiFi.status() == WL_CONNECTED;
}

bool wifiStationHasCredentials() {
    return s_configured;
}

const char* wifiStationIp() {
    if (wifiStationIsConnected()) {
        snprintf(s_ip, sizeof(s_ip), "%s", WiFi.localIP().toString().c_str());
    } else {
        s_ip[0] = '\0';
    }
    return s_ip;
}

const char* wifiStationStatus() {
    if (wifiStationIsConnected()) {
        snprintf(s_status, sizeof(s_status), "connected %s", wifiStationIp());
    } else if (WiFi.getMode() == WIFI_MODE_AP) {
        snprintf(s_status, sizeof(s_status), "AP: %s", AP_NAME);
    } else {
        snprintf(s_status, sizeof(s_status), "off");
    }
    return s_status;
}
