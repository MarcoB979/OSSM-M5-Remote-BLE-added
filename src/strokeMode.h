#pragma once

// Forward-declare ButtonEvents used by screen handlers.
struct ButtonEvents;

// Build the LVGL Stroke screen and per-loop handler for encoder input.
void ui_Stroke_screen_init(void);
void handleStrokeScreen(const ButtonEvents &events);
void refreshStrokeStartStopUi();
