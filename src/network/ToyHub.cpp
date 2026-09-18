// ToyHub.cpp — BLE toy hub (Lovense first).
//
// Lovense toys advertise names "LVS-*" / "LOVE-*" and a service whose UUID ends
// in "0001-…"; its writable characteristic is the command channel, driven with
// "Vibrate:<0..20>;". We find the first writable characteristic after connect,
// which is the command channel on every Lovense toy. Each connected toy is
// registered with the Buttplug client as a ScalarCmd ("Vibrate") device.

#include "ToyHub.h"

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <freertos/semphr.h>

#include <vector>

#include "ButtplugClient.h"
#include "ToyProfiles.h"
#include "../config/debug.h"
#include "../communication/BleComm.h"
#include "../communication/BleBackground.h"
#include "../screens/ScreenHandler.h"  // last_activity_ms

namespace {

constexpr uint32_t SCAN_INTERVAL_MS = 60000;    // auto-scan cadence (was 20 s)
constexpr uint32_t BACKGROUND_SCAN_MS = 300;    // brief auto-scan (was 1500 ms)
constexpr uint32_t FOREGROUND_SCAN_MS = 2500;   // explicit "Scan now" from the web page
constexpr uint32_t INPUT_QUIET_MS = 1000;
constexpr int MAX_TOYS = 2;

struct ConnectedToy {
    NimBLEClient* client = nullptr;
    NimBLERemoteCharacteristic* tx = nullptr;
    String name;
    std::vector<ToyFeature> features;
    uint32_t buttplugIndex = 0xFFFFFFFF;
};

std::vector<ConnectedToy> s_toys;
std::vector<ConnectedToy> s_pendingToys;  // connected by the background task, awaiting UI-loop registration
SemaphoreHandle_t s_pendingMutex = nullptr;
bool s_initialized = false;
uint32_t s_lastScanMs = 0;
bool s_scanRequested = false;
String s_lastScanReport;  // guarded by s_pendingMutex; shown on the /toys page

void bleInitOnce() {
    if (bleRadioIsSuspended()) return;  // WiFi portal owns the radio/RAM
    if (!NimBLEDevice::isInitialized()) {
        NimBLEDevice::init("M5-ToyHub");
    }
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);
}

bool alreadyConnected(const NimBLEAddress& addr) {
    for (const ConnectedToy& t : s_toys) {
        if (t.client && t.client->isConnected() && t.client->getPeerAddress() == addr) {
            return true;
        }
    }
    return false;
}

const char* actuatorTypeString(ToyFeatureType t) {
    switch (t) {
        case ToyFeatureType::Vibrate: return "Vibrate";
        case ToyFeatureType::Rotate: return "Rotate";
        case ToyFeatureType::Oscillate: return "Oscillate";
        case ToyFeatureType::Constrict: return "Constrict";
        case ToyFeatureType::Spray: return "Spray";
        case ToyFeatureType::Temperature: return "Temperature";
        case ToyFeatureType::Led: return "Led";
        case ToyFeatureType::Position: return "Position";
    }
    return "Vibrate";
}

void sendFeature(ConnectedToy& toy, ToyFeature& f, int raw) {
    if (!toy.client || !toy.client->isConnected() || !toy.tx) return;
    if (raw < f.minValue) raw = f.minValue;
    if (raw > f.maxValue) raw = f.maxValue;
    f.currentValue = raw;
    char cmd[40];
    snprintf(cmd, sizeof(cmd), f.commandTemplate.c_str(), raw);
    toy.tx->writeValue((uint8_t*)cmd, strlen(cmd), false);
}

// Runs in the background task: scan, connect, discover. Fills `out` with a
// fully-connected toy but does NOT touch the buttplug registry/WebSocket (that
// must happen on the UI loop, which owns the WebSocket client).
bool connectToyBle(const NimBLEAdvertisedDevice* device, ConnectedToy* out) {
    if (!device || !out || !device->haveName()) return false;
    const String name = device->getName().c_str();
    if (alreadyConnected(device->getAddress())) return false;

    // Built-in Lovense profile or a user-added runtime profile.
    std::vector<ToyFeature> features;
    if (name.startsWith("LVS-") || name.startsWith("LOVE-")) {
        features = {
            { ToyFeatureType::Vibrate, 0, 20, "Vibrate:%d;", 0, false, 0 },
            { ToyFeatureType::Rotate, -20, 20, "Rotate:%d;", 0, true, 0 },
        };
    } else {
        const int profileIndex = toyProfilesMatch(name.c_str());
        if (profileIndex < 0) return false;
        ToyProfile profile;
        if (!toyProfilesGet(profileIndex, profile)) return false;
        features.push_back({ profile.type, profile.minValue, profile.maxValue,
                             profile.commandTemplate, 0,
                             profile.type == ToyFeatureType::Rotate, 0 });
    }

    NimBLEClient* client = NimBLEDevice::createClient();
    if (!client) return false;
    client->setConnectionParams(12, 12, 0, 150);
    client->setConnectTimeout(5000);
    if (!client->connect(device->getAddress())) {
        NimBLEDevice::deleteClient(client);
        return false;
    }

    // Lovense exposes one writable characteristic: the command channel.
    NimBLERemoteCharacteristic* tx = nullptr;
    const std::vector<NimBLERemoteService*>& services = client->getServices(true);
    for (NimBLERemoteService* svc : services) {
        if (!svc) continue;
        const std::vector<NimBLERemoteCharacteristic*>& chars = svc->getCharacteristics(true);
        for (NimBLERemoteCharacteristic* chr : chars) {
            if (chr && chr->canWrite()) {
                tx = chr;
                break;
            }
        }
        if (tx) break;
    }
    if (!tx) {
        NimBLEDevice::deleteClient(client);
        return false;
    }

    out->client = client;
    out->tx = tx;
    out->name = name;
    out->features = features;
    out->buttplugIndex = 0xFFFFFFFF;
    return true;
}

// Runs on the UI loop: builds the buttplug device definition, registers it and
// moves the toy into the live list. Must not run in the background task because
// it touches the WebSocket client.
void registerConnectedToy(ConnectedToy& toy) {
    ButtplugDeviceDef def;
    def.name = toy.name.c_str();
    def.stepCount = 20;
    for (ToyFeature& f : toy.features) {
        if (f.type == ToyFeatureType::Rotate) {
            f.useRotateCmd = true;
            f.bpIndex = 0;
            def.hasRotate = true;
            def.rotateFeatureCount = 1;
        } else {
            f.useRotateCmd = false;
            f.bpIndex = (uint32_t)def.scalarFeatures.size();
            ButtplugScalarFeature sf;
            sf.actuatorType = actuatorTypeString(f.type);
            sf.featureCount = 1;
            def.scalarFeatures.push_back(sf);
        }
    }
    toy.buttplugIndex = buttplugRegisterDevice(def);
    s_toys.push_back(toy);
    LogDebugFormatted("[ToyHub] connected toy '%s' (buttplug index %u)\n",
                      toy.name.c_str(), toy.buttplugIndex);
}

}  // namespace

void toyHubInit() {
    if (s_initialized) return;
    s_initialized = true;
    if (!s_pendingMutex) s_pendingMutex = xSemaphoreCreateMutex();
    bleInitOnce();
}

void toyHubLoop() {
    if (!s_initialized) return;
    if (bleRadioIsSuspended()) return;  // do not scan while the portal is up

    // Adopt toys the background task connected (register on the UI loop, which
    // owns the WebSocket/buttplug registry).
    toyHubDrainPendingToys();

    if ((int)s_toys.size() >= MAX_TOYS) return;

    const uint32_t now = millis();
    const bool manual = s_scanRequested;
    if (!manual && (int32_t)(now - s_lastScanMs) < (int32_t)SCAN_INTERVAL_MS) return;
    if ((now - last_activity_ms) < INPUT_QUIET_MS) return;

    s_scanRequested = false;
    s_lastScanMs = now;

    // Hand the blocking scan off to the background task; never run it on the UI loop.
    // Keep the periodic auto-scan short; an explicit "Scan now" may run longer.
    bleBackgroundRequest(BleBgJob::ToyScan, manual ? FOREGROUND_SCAN_MS : BACKGROUND_SCAN_MS);
}

void toyHubRequestScan() {
    s_scanRequested = true;
}

void toyHubScanAndConnectOnce(uint32_t durationMs) {
    if (!s_initialized) toyHubInit();
    bleInitOnce();
    if (bleRadioIsSuspended()) return;

    NimBLEScan* scanner = NimBLEDevice::getScan();
    if (!scanner) return;

    scanner->stop();
    scanner->clearResults();
    scanner->setActiveScan(true);
    // Duty-cycled scan so the active OSSM GATT connection keeps its radio time.
    scanner->setInterval(96);
    scanner->setWindow(32);

    const NimBLEScanResults results = scanner->getResults(durationMs, false);

    String report;
    report.reserve(256);
    report = "Scanned " + String(results.getCount()) + " device(s)";
    for (int i = 0; i < results.getCount(); ++i) {
        const NimBLEAdvertisedDevice* dev = results.getDevice(i);
        const String name = dev->haveName() ? String(dev->getName().c_str()) : String("(no name)");
        report += "\n  " + name + "  [" + String(dev->getAddress().toString().c_str()) + "]";

        ConnectedToy toy;
        if (connectToyBle(dev, &toy)) {
            report += "  -> connected";
            if (xSemaphoreTake(s_pendingMutex, portMAX_DELAY) == pdTRUE) {
                s_pendingToys.push_back(toy);
                xSemaphoreGive(s_pendingMutex);
            }
        }
    }
    if (xSemaphoreTake(s_pendingMutex, portMAX_DELAY) == pdTRUE) {
        s_lastScanReport = report;
        xSemaphoreGive(s_pendingMutex);
    }
    LogDebugFormatted("[ToyHub] scan report: %s\n", report.c_str());
    scanner->clearResults();
}

String toyHubScanReport() {
    if (!s_pendingMutex) return String();
    String out;
    if (xSemaphoreTake(s_pendingMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        out = s_lastScanReport;
        xSemaphoreGive(s_pendingMutex);
    }
    return out;
}

void toyHubDrainPendingToys() {
    if (!s_pendingMutex) return;
    std::vector<ConnectedToy> toRegister;
    if (xSemaphoreTake(s_pendingMutex, 0) == pdTRUE) {
        toRegister.swap(s_pendingToys);
        xSemaphoreGive(s_pendingMutex);
    }
    for (ConnectedToy& toy : toRegister) {
        registerConnectedToy(toy);
    }
}

int toyHubConnectedCount() {
    return (int)s_toys.size();
}

void toyHubHandleScalar(uint32_t deviceIndex, uint32_t featureIndex, double scalar01) {
    for (ConnectedToy& t : s_toys) {
        if (t.buttplugIndex != deviceIndex) continue;
        for (ToyFeature& f : t.features) {
            if (f.useRotateCmd || f.bpIndex != featureIndex) continue;
            const int raw = f.minValue + (int)(scalar01 * (f.maxValue - f.minValue) + 0.5);
            sendFeature(t, f, raw);
            return;
        }
    }
}

void toyHubHandleRotate(uint32_t deviceIndex, uint32_t featureIndex, double speed01, bool clockwise) {
    for (ConnectedToy& t : s_toys) {
        if (t.buttplugIndex != deviceIndex) continue;
        for (ToyFeature& f : t.features) {
            if (!f.useRotateCmd || f.bpIndex != featureIndex) continue;
            const int magnitude = (int)(speed01 * (f.maxValue - f.minValue) + 0.5);
            sendFeature(t, f, clockwise ? magnitude : -magnitude);
            return;
        }
    }
}

void toyHubHandleStop(uint32_t deviceIndex) {
    for (ConnectedToy& t : s_toys) {
        if (t.buttplugIndex != deviceIndex) continue;
        for (ToyFeature& f : t.features) sendFeature(t, f, 0);
        return;
    }
}

int toyHubGetToyCount() { return (int)s_toys.size(); }

const char* toyHubGetToyName(int index) {
    return (index >= 0 && index < (int)s_toys.size()) ? s_toys[index].name.c_str() : "";
}

int toyHubGetFeatureCount(int index) {
    return (index >= 0 && index < (int)s_toys.size()) ? (int)s_toys[index].features.size() : 0;
}

const ToyFeature* toyHubGetFeature(int index, int featureIndex) {
    if (index < 0 || index >= (int)s_toys.size()) return nullptr;
    if (featureIndex < 0 || featureIndex >= (int)s_toys[index].features.size()) return nullptr;
    return &s_toys[index].features[featureIndex];
}

void toyHubSetFeatureRaw(int index, int featureIndex, int raw) {
    if (index < 0 || index >= (int)s_toys.size()) return;
    if (featureIndex < 0 || featureIndex >= (int)s_toys[index].features.size()) return;
    sendFeature(s_toys[index], s_toys[index].features[featureIndex], raw);
}

void toyHubStopToy(int index) {
    if (index < 0 || index >= (int)s_toys.size()) return;
    for (ToyFeature& f : s_toys[index].features) sendFeature(s_toys[index], f, 0);
}
