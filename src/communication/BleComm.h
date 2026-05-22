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

// State-machine polling helpers
void bleCommSetEnabled(bool enabled);
bool bleCommIsEnabled();
