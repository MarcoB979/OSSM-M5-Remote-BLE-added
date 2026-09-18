#pragma once

// ToyHub.h — BLE toy hub. Scans for and connects known Buttplug-config BLE toys
// (Lovense built-in; the architecture also supports runtime "toy profiles"
// added through the web page), then registers each connected toy with the
// Buttplug client so Intiface can drive it. Each toy declares a feature list
// (vibrate, rotate, …) that drives both the Buttplug DeviceAdded registration
// and the on-device UI.

#include <Arduino.h>
#include <stdint.h>
#include <vector>

enum class ToyFeatureType : uint8_t {
    Vibrate, Rotate, Oscillate, Constrict, Spray, Temperature, Led, Position
};

struct ToyFeature {
    ToyFeatureType type;
    int minValue;
    int maxValue;
    String commandTemplate;  // e.g. "Vibrate:%d;"
    int currentValue;
    bool useRotateCmd;   // true = RotateCmd, false = ScalarCmd
    uint32_t bpIndex;    // index within its command
};

void toyHubInit();
void toyHubLoop();
// Blocking scan+connect. Runs in the background task, never on the UI loop.
void toyHubScanAndConnectOnce(uint32_t durationMs);
// Adopt toys the background task connected (registers them on the UI loop).
void toyHubDrainPendingToys();
void toyHubRequestScan();  // ask toyHubLoop to scan on the next tick (web "Scan now")
String toyHubScanReport();  // thread-safe copy of the last scan's discovered devices
int  toyHubConnectedCount();

// Called by the Buttplug command dispatcher for non-OSSM device indices.
void toyHubHandleScalar(uint32_t deviceIndex, uint32_t featureIndex, double scalar01);
void toyHubHandleRotate(uint32_t deviceIndex, uint32_t featureIndex, double speed01, bool clockwise);
void toyHubHandleStop(uint32_t deviceIndex);

// On-device UI accessors.
int toyHubGetToyCount();
const char* toyHubGetToyName(int index);
int toyHubGetFeatureCount(int index);
const ToyFeature* toyHubGetFeature(int index, int featureIndex);
void toyHubSetFeatureRaw(int index, int featureIndex, int raw);
void toyHubStopToy(int index);
