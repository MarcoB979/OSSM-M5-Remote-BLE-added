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

// Release/restore the NimBLE host + BT controller. The ESP32 (Core2) has only
// ~320 KB of internal SRAM, which is not enough for NimBLE and the WiFi
// captive portal (DHCP/DNS/WebServer) at the same time: a client connecting to
// the AP exhausts the heap (esp_timer_create -> ESP_ERR_NO_MEM). Suspend tears
// BLE down completely before the portal opens; resume re-initialises it once
// the portal is closed and the normal auto-reconnect path takes over.
void bleCommSuspend();
void bleCommResume();

// Shared NimBLE-radio gate. The WiFi captive portal suspends the whole radio
// (NimBLE host + controller); every NimBLE consumer (OSSM BleComm, ToyHub and
// the addons) must check this and not re-init NimBLE while suspended.
bool bleRadioIsSuspended();

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

// ---- Firmware identification (variant + version) ----
enum class OssmFirmwareVariant {
    Unknown = 0,
    OssmLite,      // exposes the new 4f53534d-… service + Device Info (0x2A26)
    OssmStandard,  // legacy 522b443a-… service only
    OssmRs,        // legacy service, advertises "OSSM-rs"
};
OssmFirmwareVariant bleCommGetFirmwareVariant();
const char* bleCommGetFirmwareVersion();      // e.g. "2.3"; "" when unknown
const char* bleCommGetFirmwareDescription();  // "OSSM-Lite v2.3" / "OSSM-RS" / "OSSM"

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