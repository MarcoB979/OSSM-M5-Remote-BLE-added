#pragma once

#include <Arduino.h>

// Serial debug toggle for BLE polling/state-machine visibility.
extern bool showBlePollSerial;

// BLE transport lifecycle
void bleCommInit();
bool bleCommTryConnect();
bool bleCommIsConnected();

// RADR-compatible app command mapping (SPEED/DEPTH/... constants from EspNowComm.h)
bool bleCommSendAppCommand(int appCommand, float value, float currentSpeed,
                           float currentDepth, float currentStroke,
                           float maxDepthMm, float maxSpeedValue);

// Send "go:menu" to put OSSM in menu/idle state
bool bleCommGoToMenu();

// Send "go:streaming" to put OSSM in streaming mode
bool bleCommGoToStreaming();

// Send "go:strokeEngine" to put OSSM in stroke engine mode
bool bleCommGoToStrokeEngine();

// Send streaming move command in OSSM streaming mode: "stream:<position>:<durationMs>"
// Returns false when not connected or when OSSM is not currently in streaming mode.
bool bleCommSendStreamCommand(int position, int durationMs);

// State-machine polling helpers
void bleCommSetEnabled(bool enabled);
bool bleCommIsEnabled();
bool bleCommIsHoming();
int bleCommGetHomingDirection();
