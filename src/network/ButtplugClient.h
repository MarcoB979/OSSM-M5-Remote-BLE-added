#pragma once

// ButtplugClient.h — Buttplug.io v3 *device-side* client over WebSocket.
//
// The M5 connects to an Intiface/Buttplug server and registers devices (the
// OSSM and any BLE toys) so that Buttplug apps/games can drive them. Incoming
// command messages are decoded and dispatched through the handler callbacks
// installed with buttplugSetHandlers().

#include <stdint.h>

#include <functional>
#include <vector>

// One ScalarCmd actuator type (e.g. Vibrate, Position) with its feature count.
struct ButtplugScalarFeature {
    const char* actuatorType;
    uint32_t featureCount;
};

// Describes the capabilities of one logical Buttplug device. Only the fields
// for enabled capabilities are serialized into the DeviceAdded message.
struct ButtplugDeviceDef {
    const char* name;

    std::vector<ButtplugScalarFeature> scalarFeatures;  // empty = no ScalarCmd

    bool hasLinear = false;
    uint32_t linearFeatureCount = 1;

    bool hasRotate = false;
    uint32_t rotateFeatureCount = 1;

    uint32_t stepCount = 0;  // optional hint; 0 = omit from the message
};

// Command dispatch callbacks. deviceIndex is the Buttplug device index assigned
// by buttplugRegisterDevice(); featureIndex is the axis/feature within it.
struct ButtplugHandlers {
    std::function<void(uint32_t deviceIndex, uint32_t featureIndex, double scalar,
                       const char* actuatorType)> scalar;
    std::function<void(uint32_t deviceIndex, uint32_t featureIndex, uint32_t durationMs,
                       double position)> linear;
    std::function<void(uint32_t deviceIndex, uint32_t featureIndex, double speed,
                       bool clockwise)> rotate;
    std::function<void(uint32_t deviceIndex)> stop;
};

void buttplugInit();
void buttplugLoop();
bool buttplugIsConnected();

void buttplugSetServer(const char* host, uint16_t port);
const char* buttplugGetHost();
uint16_t buttplugGetPort();

void buttplugConnect();
void buttplugDisconnect();

// Register a device and return its Buttplug device index.
uint32_t buttplugRegisterDevice(const ButtplugDeviceDef& def);
void buttplugUnregisterDevice(uint32_t deviceIndex);
void buttplugSetHandlers(const ButtplugHandlers& handlers);
