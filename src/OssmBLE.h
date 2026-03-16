#pragma once

#include <Arduino.h>

bool OssmBleTryConnect();
bool OssmBleConnected();

void OssmBleSetMode(bool enabled);
bool OssmBleIsMode();

bool OssmBleSendText(const String& command, String* response = nullptr);
bool OssmBleSendCommand(int command, float value, float speedValue, float maxDepthMm, float maxSpeedValue, String* response = nullptr);
bool OssmBlePrepareStrokeEngine();
bool OssmBleReadState(String* stateText);
bool OssmBlePollLimits(float* outMaxDepthMm, float* outMaxSpeedValue);

// BLE-specific pause/resume helpers
void OssmBleStoreUnpauseSpeed(float speed);
float OssmBleGetUnpauseSpeed();
bool OssmBlePause();
bool OssmBleResume();
// Send BLE pause state (0 = not paused, 1 = paused)
bool OssmBleSendIsPaused(int paused);
// Send BLE paused speed (percent 0-100)
bool OssmBleSendPausedSpeed(float speed);
