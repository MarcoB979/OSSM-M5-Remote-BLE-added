#pragma once
#include <Arduino.h>

// RADR-style BLE comms for OSSM
enum class OssmBleMachineMode {
  Unknown,
  HomingForward,
  HomingBackward,
  StrokeEngineIdle,
  StrokeEngineActive,
  SimplePenetration,
  Streaming,
  Menu,
  Other,
};

struct OssmBleMachineState {
  bool valid = false;
  String raw;
  String stateName;
  OssmBleMachineMode mode = OssmBleMachineMode::Unknown;
  int speed = 0;
  int stroke = 0;
  int sensation = 0;
  int depth = 0;
  int pattern = 0;
};

enum class OssmBleCommand {
  SetSpeed,
  SetDepth,
  SetStroke,
  SetSensation,
  SetPattern,
  Off,
  On,
};

enum class OssmBleHomeToggleResult {
  BlockedNotReady,
  Paused,
  Resumed,
  Started,
  Failed,
};

// Public API (RADR-style: enqueue + optional ACK)
bool OssmBleTryConnect(void (*postInitCallback)() = nullptr);
bool OssmBleConnected();
void OssmBleSetMode(bool enabled);
bool OssmBleIsMode();
bool OssmBleCanSendControlCommands();
OssmBleHomeToggleResult OssmBleHandleHomeToggle(bool isRunning, float currentSpeed);
OssmBleHomeToggleResult OssmBleHandleStreamingToggle(bool isRunning, float currentSpeed);

// Core send APIs. If a caller passes a non-null response pointer,
// the call will wait for an ACK/read-back (behaves like RADR requireAck).
bool OssmBleSendText(const String& command, String* response = nullptr);
bool OssmBleSendCommand(OssmBleCommand command, float value, float speedValue, float maxDepthMm, float maxSpeedValue, String* response = nullptr);
bool OssmBleSendAppCommand(int appCommand, float value, float currentSpeed, float currentDepth, float currentStroke, bool isRunning, float maxDepthMm, float maxSpeedValue, String* response = nullptr);
bool OssmBleSendMappedAppCommand(int appCommand, float value, float speedValue, bool clearDepthAndStrokeFirst, float maxDepthMm, float maxSpeedValue, String* response = nullptr);

// Higher-level helpers
bool OssmBleExecutePulloutStop(float currentSpeed, float maxDepthMm, float maxSpeedValue);
bool OssmBleGoToStrokeEngine();
bool OssmBleGoToSimplePenetration();
bool OssmBleGoToStreaming();
bool OssmBleGoToMenu();
bool OssmBleSetBuffer(float value);
bool OssmBleSetWifi(const String& ssid, const String& password);
bool OssmBleStreamPosition(float position, int timeMs);

// State polling / parsing
bool OssmBleReadState(String* stateText, bool logState = true);
bool OssmBleGetCurrentState(OssmBleMachineState* outState, bool forceRefresh = true);
bool OssmBlePollLimits(float* outMaxDepthMm, float* outMaxSpeedValue);

// Pause/resume
void OssmBleStoreUnpauseSpeed(float speed);
float OssmBleGetUnpauseSpeed();
bool OssmBlePause();
bool OssmBleResume();
bool OssmBleSendIsPaused(int paused);
bool OssmBleSendPausedSpeed(float speed);

// UI suppression helper: used to prevent transient UI state changes when
// OssmBLE intentionally pauses the device as a workaround (e.g. before
// issuing a stroke=0). `observedStroke` is the latest stroke value from
// the machine state; when it reaches 0 the suppression is cleared.
bool OssmBleShouldSuppressUiPause(int observedStroke);

#ifdef __cplusplus
extern "C" {
#endif
void OssmBleSetMode_c(int enabled);
#ifdef __cplusplus
}
#endif
