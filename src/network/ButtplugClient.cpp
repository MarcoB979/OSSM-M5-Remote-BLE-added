// ButtplugClient.cpp — Buttplug.io v3 device-side client over WebSocket.
//
// Device role: the M5 connects to an Intiface/Buttplug server (ws://host:port),
// answers RequestServerInfo / RequestDeviceList, announces its devices via
// DeviceAdded, and executes ScalarCmd / LinearCmd / RotateCmd / StopDeviceCmd /
// StopAllDevices by forwarding them to the registered handlers.

#include "ButtplugClient.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <WebSocketsClient.h>
#include <WiFi.h>
#include <string.h>

#include <vector>

#include "WifiStation.h"
#include "../config/debug.h"

namespace {

constexpr const char* PREFS_NS = "network";
constexpr const char* PREFS_HOST = "bp_host";
constexpr const char* PREFS_PORT = "bp_port";
constexpr const char* SERVER_NAME = "OSSM M5 Remote";
constexpr uint32_t MESSAGE_VERSION = 3;

WebSocketsClient webSocket;
bool s_initialized = false;
bool s_connected = false;

String s_host;     // empty = not configured
uint16_t s_port = 12345;

uint32_t s_nextMessageId = 1;
uint32_t s_nextDeviceIndex = 0;

struct RegisteredDevice {
    ButtplugDeviceDef def;
    uint32_t index;
};
std::vector<RegisteredDevice> s_devices;

ButtplugHandlers s_handlers;

bool s_beginSent = false;
bool s_devicesAnnounced = false;

uint32_t nextMessageId() { return s_nextMessageId++; }

// Normalise a user-entered Intiface address to a bare host (no scheme/path).
// Returns "" for empty or loopback values, because the Intiface server can
// never live on the M5 itself and a bad value would otherwise drive the
// WebSocket client's blocking DNS-reconnect loop (stalling the UI).
String normalizeHost(const String& raw) {
    String h = raw;
    h.trim();
    if (h.startsWith("ws://"))         h = h.substring(5);
    else if (h.startsWith("wss://"))   h = h.substring(6);
    else if (h.startsWith("http://"))  h = h.substring(7);
    else if (h.startsWith("https://")) h = h.substring(8);

    const int slash = h.indexOf('/');
    if (slash >= 0) h = h.substring(0, slash);
    h.trim();

    if (h.length() == 0 || h.equalsIgnoreCase("localhost") ||
        h == "127.0.0.1" || h == "::1") {
        return String();
    }
    return h;
}

void sendJson(JsonDocument& doc) {
    String out;
    serializeJson(doc, out);
    webSocket.sendTXT(out);
}

void sendOk(uint32_t id) {
    JsonDocument d;
    d["Ok"]["Id"] = id;
    sendJson(d);
}

void sendServerInfo(uint32_t id) {
    JsonDocument d;
    JsonObject body = d["ServerInfo"].to<JsonObject>();
    body["Id"] = id;
    body["ServerName"] = SERVER_NAME;
    body["MessageVersion"] = MESSAGE_VERSION;
    body["MaxPingTime"] = 0;
    sendJson(d);
}

void sendDeviceList(uint32_t id) {
    JsonDocument d;
    JsonObject body = d["DeviceList"].to<JsonObject>();
    body["Id"] = id;
    body["Devices"].to<JsonArray>();  // empty; devices announced via DeviceAdded
    sendJson(d);
}

void sendDeviceAdded(const RegisteredDevice& dev) {
    JsonDocument d;
    JsonObject body = d["DeviceAdded"].to<JsonObject>();
    body["Id"] = nextMessageId();
    body["DeviceName"] = dev.def.name;
    body["DeviceIndex"] = dev.index;

    JsonObject messages = body["DeviceMessages"].to<JsonObject>();
    if (!dev.def.scalarFeatures.empty()) {
        JsonArray arr = messages["ScalarCmd"].to<JsonArray>();
        for (const ButtplugScalarFeature& f : dev.def.scalarFeatures) {
            JsonObject m = arr.add<JsonObject>();
            m["FeatureCount"] = f.featureCount;
            if (dev.def.stepCount > 0) m["StepCount"] = dev.def.stepCount;
            if (f.actuatorType) m["ActuatorType"] = f.actuatorType;
        }
    }
    if (dev.def.hasLinear) {
        JsonObject m = messages["LinearCmd"].to<JsonObject>();
        m["FeatureCount"] = dev.def.linearFeatureCount;
        if (dev.def.stepCount > 0) m["StepCount"] = dev.def.stepCount;
    }
    if (dev.def.hasRotate) {
        JsonObject m = messages["RotateCmd"].to<JsonObject>();
        m["FeatureCount"] = dev.def.rotateFeatureCount;
        if (dev.def.stepCount > 0) m["StepCount"] = dev.def.stepCount;
    }
    sendJson(d);
}

void announceAllDevices() {
    for (const RegisteredDevice& dev : s_devices) {
        sendDeviceAdded(dev);
    }
}

void announceDevicesOnce() {
    if (s_devicesAnnounced) return;
    s_devicesAnnounced = true;
    announceAllDevices();
}

void handleText(const uint8_t* payload, size_t length) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, (const char*)payload, length);
    if (err) return;  // ignore malformed frames

    JsonObject root = doc.as<JsonObject>();
    for (JsonPair kv : root) {
        const char* type = kv.key().c_str();
        JsonObject body = kv.value().as<JsonObject>();
        uint32_t id = body["Id"] | 0;

        if (strcmp(type, "RequestServerInfo") == 0) {
            sendServerInfo(id);
            announceDevicesOnce();
        } else if (strcmp(type, "RequestDeviceList") == 0) {
            sendDeviceList(id);
            announceDevicesOnce();
        } else if (strcmp(type, "Ping") == 0) {
            sendOk(id);
        } else if (strcmp(type, "ScalarCmd") == 0) {
            uint32_t deviceIndex = body["DeviceIndex"] | 0;
            JsonArray scalars = body["Scalars"].as<JsonArray>();
            for (JsonVariant sv : scalars) {
                JsonObject s = sv.as<JsonObject>();
                uint32_t featureIndex = s["Index"] | 0;
                double scalar = s["Scalar"] | 0.0;
                const char* actuatorType = s["ActuatorType"] | "Unknown";
                if (s_handlers.scalar) s_handlers.scalar(deviceIndex, featureIndex, scalar, actuatorType);
            }
            sendOk(id);
        } else if (strcmp(type, "LinearCmd") == 0) {
            uint32_t deviceIndex = body["DeviceIndex"] | 0;
            JsonArray vectors = body["Vectors"].as<JsonArray>();
            for (JsonVariant vv : vectors) {
                JsonObject v = vv.as<JsonObject>();
                uint32_t featureIndex = v["Index"] | 0;
                uint32_t duration = v["Duration"] | 0;
                double position = v["Position"] | 0.0;
                if (s_handlers.linear) s_handlers.linear(deviceIndex, featureIndex, duration, position);
            }
            sendOk(id);
        } else if (strcmp(type, "RotateCmd") == 0) {
            uint32_t deviceIndex = body["DeviceIndex"] | 0;
            JsonArray rotations = body["Rotations"].as<JsonArray>();
            for (JsonVariant rv : rotations) {
                JsonObject r = rv.as<JsonObject>();
                uint32_t featureIndex = r["Index"] | 0;
                double speed = r["Speed"] | 0.0;
                bool clockwise = r["Clockwise"] | true;
                if (s_handlers.rotate) s_handlers.rotate(deviceIndex, featureIndex, speed, clockwise);
            }
            sendOk(id);
        } else if (strcmp(type, "StopDeviceCmd") == 0) {
            uint32_t deviceIndex = body["DeviceIndex"] | 0;
            if (s_handlers.stop) s_handlers.stop(deviceIndex);
            sendOk(id);
        } else if (strcmp(type, "StopAllDevices") == 0) {
            if (s_handlers.stop) {
                for (const RegisteredDevice& dev : s_devices) s_handlers.stop(dev.index);
            }
            sendOk(id);
        }
        // Unknown/unsolicited messages are ignored (logging at debug level only).
    }
}

void onWebSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_CONNECTED:
            s_connected = true;
            LogDebug("[Buttplug] WebSocket connected");
            break;
        case WStype_DISCONNECTED:
            s_connected = false;
            s_devicesAnnounced = false;
            LogDebug("[Buttplug] WebSocket disconnected");
            break;
        case WStype_TEXT:
            handleText(payload, length);
            break;
        case WStype_ERROR:
            LogDebug("[Buttplug] WebSocket error");
            break;
        default:
            break;
    }
}

}  // namespace

void buttplugInit() {
    if (s_initialized) return;
    s_initialized = true;

    Preferences prefs;
    prefs.begin(PREFS_NS, true);
    s_host = normalizeHost(prefs.getString(PREFS_HOST, ""));
    s_port = (uint16_t)prefs.getUShort(PREFS_PORT, 12345);
    prefs.end();

    webSocket.onEvent(onWebSocketEvent);
    webSocket.setReconnectInterval(3000);
}

void buttplugLoop() {
    if (!s_initialized) return;

    const bool wifiUp = wifiStationIsConnected();

    // Unconfigured/invalid host: keep the socket idle. A bad host (e.g. a bare
    // "localhost") makes the WebSocket client retry a blocking DNS lookup in
    // loop(), which stalls button/encoder handling for seconds at a time.
    if (s_host.length() == 0) {
        if (s_beginSent) {
            s_beginSent = false;
            webSocket.disconnect();
            s_connected = false;
        }
        return;
    }

    webSocket.loop();

    // Begin once while WiFi is up and a server host is configured. After that
    // the WebSocketsClient auto-reconnects on its own interval.
    if (wifiUp && !s_beginSent) {
        s_beginSent = true;
        webSocket.begin(s_host.c_str(), s_port, "/");
    }
    if (!wifiUp && s_beginSent) {
        s_beginSent = false;
        webSocket.disconnect();
        s_connected = false;
    }
}

bool buttplugIsConnected() {
    return s_connected;
}

void buttplugSetServer(const char* host, uint16_t port) {
    s_host = normalizeHost(host ? host : "");
    s_port = port;
    Preferences prefs;
    prefs.begin(PREFS_NS, false);
    prefs.putString(PREFS_HOST, s_host);
    prefs.putUShort(PREFS_PORT, s_port);
    prefs.end();
    s_beginSent = false;
    webSocket.disconnect();
    s_connected = false;
}

const char* buttplugGetHost() {
    return s_host.c_str();
}

uint16_t buttplugGetPort() {
    return s_port;
}

void buttplugConnect() {
    if (s_host.length() == 0 || !wifiStationIsConnected()) return;
    s_beginSent = true;
    webSocket.begin(s_host.c_str(), s_port, "/");
}

void buttplugDisconnect() {
    s_beginSent = false;
    webSocket.disconnect();
    s_connected = false;
}

uint32_t buttplugRegisterDevice(const ButtplugDeviceDef& def) {
    RegisteredDevice dev;
    dev.def = def;
    dev.index = s_nextDeviceIndex++;
    s_devices.push_back(dev);

    if (s_connected) {
        sendDeviceAdded(dev);
    }
    return dev.index;
}

void buttplugUnregisterDevice(uint32_t deviceIndex) {
    for (auto it = s_devices.begin(); it != s_devices.end(); ++it) {
        if (it->index != deviceIndex) continue;
        if (s_connected) {
            JsonDocument d;
            JsonObject body = d["DeviceRemoved"].to<JsonObject>();
            body["Id"] = nextMessageId();
            body["DeviceIndex"] = deviceIndex;
            sendJson(d);
        }
        s_devices.erase(it);
        return;
    }
}

void buttplugSetHandlers(const ButtplugHandlers& handlers) {
    s_handlers = handlers;
}
