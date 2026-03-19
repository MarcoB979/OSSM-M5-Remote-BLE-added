#pragma once

#include <Arduino.h>

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

enum class OssmBleReadyState {
	Unknown,
	WaitingForStrokeEngine,
	Ready,
};

enum class OssmBleHomeToggleResult {
	BlockedNotReady,
	Paused,
	Resumed,
	Started,
	Failed,
};

bool OssmBleTryConnect(void (*postInitCallback)() = nullptr);
bool OssmBleConnected();

void OssmBleSetMode(bool enabled);
bool OssmBleIsMode();
void OssmBleBeginConnectFlow();
OssmBleReadyState OssmBleUpdateReadyState(bool forceRefresh = true);
bool OssmBleIsWaitingForReady();
bool OssmBleCanSendControlCommands();
OssmBleHomeToggleResult OssmBleHandleHomeToggle(bool isRunning, float currentSpeed);

bool OssmBleSendText(const String& command, String* response = nullptr);
bool OssmBleSendCommand(OssmBleCommand command, float value, float speedValue, float maxDepthMm, float maxSpeedValue, String* response = nullptr);
bool OssmBleSendAppCommand(int appCommand, float value, float currentSpeed, float currentDepth, float currentStroke, bool isRunning, float maxDepthMm, float maxSpeedValue, String* response = nullptr);
bool OssmBleSendMappedAppCommand(int appCommand, float value, float speedValue, bool clearDepthAndStrokeFirst, float maxDepthMm, float maxSpeedValue, String* response = nullptr);
bool OssmBleExecutePulloutStop(float currentSpeed, float maxDepthMm, float maxSpeedValue);
bool OssmBleGoToStrokeEngine();
bool OssmBleGoToSimplePenetration();
bool OssmBleGoToStreaming();
bool OssmBleGoToMenu();
bool OssmBleSetBuffer(float value);
bool OssmBleSetWifi(const String& ssid, const String& password);
bool OssmBleStreamPosition(float position, int timeMs);
bool OssmBleReadState(String* stateText);
bool OssmBleGetCurrentState(OssmBleMachineState* outState, bool forceRefresh = true);
bool OssmBlePollLimits(float* outMaxDepthMm, float* outMaxSpeedValue);
const char* OssmBleMachineModeName(OssmBleMachineMode mode);
bool OssmBleIsHoming(const OssmBleMachineState& state);
bool OssmBleIsReadyForStrokeEngine(const OssmBleMachineState& state);

// BLE-specific pause/resume helpers
void OssmBleStoreUnpauseSpeed(float speed);
float OssmBleGetUnpauseSpeed();
bool OssmBlePause();
bool OssmBleResume();
// Compatibility shim only. OSSM BLE does not support set:ispaused.
bool OssmBleSendIsPaused(int paused);
// Compatibility shim only. OSSM BLE does not support set:pausedSpeed.
bool OssmBleSendPausedSpeed(float speed);
