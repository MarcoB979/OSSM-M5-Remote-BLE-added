// ButtplugHub.cpp — maps Buttplug commands to peripherals.
//
// Currently registers the OSSM as a Buttplug device:
//   - LinearCmd (position 0..1 over a duration) -> OSSM stream:pos:duration
//   - ScalarCmd (ActuatorType "Position", 0..1)  -> OSSM immediate position
//   - StopDeviceCmd / StopAllDevices             -> OSSM OFF
//
// BLE toys are added in a later phase; their ButtplugDeviceDef entries will be
// registered here the same way and routed by device index.

#include "ButtplugHub.h"

#include "ButtplugClient.h"
#include "ToyHub.h"
#include "../communication/BleComm.h"
#include "../communication/CommManager.h"
#include "../config/config_ids.h"
#include "../config/debug.h"

namespace {

uint32_t s_ossmIndex = 0xFFFFFFFF;
bool s_streamingEntered = false;

uint32_t posToPercent(double position) {
    if (position <= 0.0) return 0;
    if (position >= 1.0) return 100;
    return (uint32_t)(position * 100.0 + 0.5);
}

void ensureStreaming() {
    if (!s_streamingEntered) {
        s_streamingEntered = bleCommGoToStreaming();
    }
}

void onScalar(uint32_t deviceIndex, uint32_t featureIndex, double scalar,
              const char* actuatorType) {
    (void)actuatorType;
    if (deviceIndex == s_ossmIndex) {
        (void)featureIndex;
        ensureStreaming();
        bleCommSendStreamCommand((int)posToPercent(scalar), 0);
    } else {
        toyHubHandleScalar(deviceIndex, featureIndex, scalar);
    }
}

void onLinear(uint32_t deviceIndex, uint32_t featureIndex, uint32_t durationMs,
              double position) {
    (void)featureIndex;
    if (deviceIndex != s_ossmIndex) return;
    ensureStreaming();
    bleCommSendStreamCommand((int)posToPercent(position), (int)durationMs);
}

void onRotate(uint32_t deviceIndex, uint32_t featureIndex, double speed, bool clockwise) {
    (void)featureIndex;
    if (deviceIndex != s_ossmIndex) {
        toyHubHandleRotate(deviceIndex, featureIndex, speed, clockwise);
    }
    // The OSSM has no rotate feature; ignore.
}

void onStop(uint32_t deviceIndex) {
    if (deviceIndex == s_ossmIndex) {
        SendCommand(OFF, 0.0f, OSSM_ID);
    } else {
        toyHubHandleStop(deviceIndex);
    }
}

}  // namespace

void buttplugHubInit() {
    ButtplugDeviceDef ossm;
    ossm.name = "OSSM";
    ButtplugScalarFeature pos;
    pos.actuatorType = "Position";
    pos.featureCount = 1;
    ossm.scalarFeatures.push_back(pos);
    ossm.hasLinear = true;
    ossm.linearFeatureCount = 1;
    ossm.stepCount = 100;
    s_ossmIndex = buttplugRegisterDevice(ossm);

    ButtplugHandlers handlers;
    handlers.scalar = onScalar;
    handlers.linear = onLinear;
    handlers.rotate = onRotate;
    handlers.stop = onStop;
    buttplugSetHandlers(handlers);

    LogDebugFormatted("[Hub] OSSM registered as Buttplug device index %u\n",
                      s_ossmIndex);
}
