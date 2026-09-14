#pragma once

#include <Arduino.h>

// Serial debug toggle for BLE polling/state-machine visibility.
extern bool showBlePollSerial;

// BLE transport lifecycle
void bleCommInit();
bool bleCommTryConnect();
bool bleCommIsConnected();
// Records the calling task as the UI/main task so background BLE tasks know
// when it's unsafe to touch LVGL (call once from setup(), on the main task).
void bleCommRegisterMainTask();

bool bleCommSendAppCommand(int appCommand, float value, float currentSpeed,
                           float currentDepth, float currentStroke,
                           float maxDepthMm, float maxSpeedValue);

                           // Thread-safe BLE mutex access for cross-client operation synchronization

// State-machine polling helpers
void bleCommSetEnabled(bool enabled);
bool bleCommIsEnabled();
bool bleCommIsHoming();
bool bleCommHasFreshState();
void bleCommUnlock();

// Latest confirmed values reported by OSSM. The BLE task owns the cache;
// callers receive a consistent snapshot for use on the UI task.
struct BleConfirmedValues {
    uint32_t revision;
    float speed;
    float depth;
    float stroke;
    float sensation;
    int pattern;
    float minPosition;
    float maxPosition;
};
bool bleCommGetConfirmedValues(BleConfirmedValues* outValues);

// Live per-stroke rail telemetry (position/speed/acceleration), updated once
// per stroke by the OSSM's StrokeEngine telemetry callback. Returns false if
// no fresh state has been received yet. See OSSM_STROKE_TELEMETRY_PR.md.
struct BleRailTelemetry {
    float railPos = -1.0f;      // % of travel
    float strokeSpeed = -1.0f;  // % of max speed
    float railAccel = -1.0f;    // % of max acceleration
};
bool bleCommGetRailTelemetry(BleRailTelemetry* outValues);

int bleCommGetHomingDirection();
float bleCommGetConfirmedPosition();
int bleCommSetUnpauseSpeed(float speedValue);
int bleCommGetUnpauseSpeed();
// Mode/query helpers used by UI streaming flow.
bool bleCommIsMenu();
bool bleCommGoToMenu();
bool bleCommGoToStreaming();
bool bleCommGoToStrokeEngine();
bool bleCommEnsureStrokeEngineOrStreamingReady();
String bleCommGetMachineStateName(bool lowerCase);

// Unified BLE streaming command bridge.
bool bleCommSendStreamCommand(int position, int durationMs);

// Advanced Penetration characteristic bridge.
bool bleCommReadAdvancedConfig(String* outConfig);
bool bleCommReadAdvancedStatus(String* outStatus);
bool bleCommReadAdvancedPresets(String* outPresets);
bool bleCommWriteAdvancedControl(const String& payload);
bool bleCommWriteAdvancedPresets(const String& payload);

// BLE pattern catalog bridge (OSSM patterns characteristic)
extern bool newPatternIsReadFromOSSM;
extern String patternString;
bool readPatternsFromOSSM();
void bleCommResetPatternReadState();
// ---- Shared runtime variables ----
extern bool               Ossm_paired;
extern volatile bool      OSSM_On;
extern int pattern;
extern float speedlimit;
extern float maxdepthinmm;