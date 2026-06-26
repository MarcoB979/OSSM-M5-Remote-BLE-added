#pragma once

#if __has_include("lvgl.h")
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

// Build the LVGL Stroke screen.
// Stroke screen handling now lives here; ScreenHandler.cpp only routes into it.
#ifdef __cplusplus
extern "C" {
#endif

void ui_Stroke_screen_init(void);
void refreshStrokeStartStopUi(void);
void strokeScreenHandle(bool shouldRehome, bool resetToSimpleStroke);

// Stroke-screen slider and value label objects (extern so ScreenHandler.cpp can drive them)
extern lv_obj_t *ui_StrokeSpeedSlider;
extern lv_obj_t *ui_StrokeStrokeSlider;
extern lv_obj_t *ui_StrokeSensationSlider;
extern lv_obj_t *ui_StrokeSpeedValue;
extern lv_obj_t *ui_StrokeStrokeValue;
extern lv_obj_t *ui_StrokeSensationValue;

#ifdef __cplusplus
}
#endif
