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

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
// Update streaming value labels (speed, depth, stroke, sensation) — called from ScreenHandler encoder logic
void streamingUpdateValueLabels(float spd, float dep, float str, float sen);
// Pause state — managed by addonsStreaming.cpp, polled by ScreenHandler
bool streamingIsPaused();
void streamingResetPause();
// Start/cancel the streaming BLE init sequence and consume completion signal.
void streamingBeginInitSequence();
void streamingCancelInitSequence();
bool streamingConsumeInitCompleted();
#endif
