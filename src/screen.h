#pragma once

#include <stdint.h>

// Shared screen/UI runtime state (single definitions in screen.cpp)
extern int g_brightness_value;
extern bool eject_status;
extern bool vibrate_mode;
extern bool touch_home;
extern char patternstr[20];

extern int screensaver_timeout_ms;
extern int screensaver_dim_brightness;
extern unsigned long deep_sleep_timeout_ms;

extern long speedenc;
extern long torqe_f_enc;
extern long torqe_r_enc;
extern long encoder3_enc;
extern long encoder4_enc;

void update_battery_icons_all_screens(int level);
void screen_power_tick();