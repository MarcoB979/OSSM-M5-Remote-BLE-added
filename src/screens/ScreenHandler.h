#pragma once
#include <lvgl.h>

// ---- Screen states ----
#define ST_UI_START         0
#define ST_UI_HOME          1
#define ST_UI_MENU          10
#define ST_UI_MENUE         ST_UI_MENU   // legacy alias
#define ST_UI_PATTERN       11
#define ST_UI_Torqe         12
#define ST_UI_EJECTSETTINGS 13
#define ST_UI_STROKE        14
#define ST_UI_COLORS        15
#define ST_UI_STREAMING     16
#define ST_UI_ADDONS        17
#define ST_UI_SETTINGS      20

// ---- Shared screen state (defined in ScreenHandler.cpp) ----
extern int   st_screens;
extern float speed, depth, stroke, sensation;
extern float torqe_f, torqe_r;
extern int   pattern;
extern char  patternstr[20];
extern bool  dynamicStroke;
extern bool  eject_status;
extern bool  vibrate_mode;
extern bool  touch_home;
extern bool  strokeinvert_mode;
extern bool  ble_force_homeing;
extern bool  touch_disabled;
extern bool  EJECT_On;

// ---- Lifecycle ----
void screenInit();     // Load NVS settings and apply to UI; call after ui_init() + buttonInit()
void handleScreens();  // Battery display + screen state machine + flag clear; call from loop()
