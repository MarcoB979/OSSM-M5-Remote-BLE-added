#pragma once

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern lv_obj_t *ui_FistIT;

void addonsMoveSelection(int delta);
void addonsActivateSelection(void);
void addonsSyncSelectionVisual(void);
bool addonsIsFistITEnabled(void);
bool addonsIsEjectEnabled(void);
bool ejectPaired(void);
bool FistITPaired(void);
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
// Update streaming value labels (speed, depth, stroke, sensation) for the addon-owned screen loop.
void streamingUpdateValueLabels(float spd, float dep, float str, float sen);
// Pause state — managed by addonsStreaming.cpp and used by the addon-owned screen loop.
bool streamingIsPaused();
void streamingResetPause();
void streamingRememberResumeSpeed(float speed);
float streamingGetResumeSpeed();
// Start/cancel the streaming BLE init sequence and consume completion signal.
void streamingBeginInitSequence();
void streamingCancelInitSequence();
bool streamingConsumeInitCompleted();
// Handle one Streaming screen tick.
void streamingScreenHandle(bool firstEntry);
#endif
