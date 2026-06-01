#pragma once
#include <lvgl.h>
#include <stdint.h>

// ---- Notification result values for modal overlay prompts ----
typedef enum NotificationResult {
    NOTIFICATION_RESULT_NONE  = 0,
    NOTIFICATION_RESULT_LEFT  = 1,
    NOTIFICATION_RESULT_RIGHT = 2,
} NotificationResult;

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
#define ST_UI_FISTIT        18
#define ST_UI_SETTINGS      20

// ---- Shared screen state (defined in ScreenHandler.cpp) ----
extern int   st_screens;
extern float speed, depth, stroke, sensation;
extern float torqe_f, torqe_r;
extern int   pattern;
extern char  patternstr[20];
extern lv_obj_t *g_pattern_return_screen;
extern bool  dynamicStroke;
extern bool  eject_status;
extern bool  vibrate_mode;
extern bool  touch_home;
extern bool  strokeinvert_mode;
extern bool  ble_force_homeing;
extern bool  touch_disabled;
extern bool  EJECT_On;
extern bool  rstate;

// ---- Lifecycle ----
void screenInit();     // Load NVS settings and apply to UI; call after ui_init() + buttonInit()
void handleScreens();  // Battery display + screen state machine + flag clear; call from loop()

// ---- Screensaver / Power management ----
extern int            g_brightness_value;
extern unsigned long  last_activity_ms;
extern bool           screensaver_active;
extern int            screensaver_prev_brightness;
extern int            screensaver_timeout_ms;
extern int            screensaver_dim_brightness;
extern uint32_t       deep_sleep_timeout_ms;

void screensaver_check_activity();
bool canEnterDeepSleep();
void enterDeepSleep();
void screen_power_tick();

// ---- Notification overlay ----
#ifdef __cplusplus
int showNotification(const char *title,
                     const char *text,
                     uint32_t duration,
                     bool showLeftButton = false,
                     const char *leftButtonText = nullptr,
                     bool showRightButton = false,
                     const char *rightButtonText = nullptr,
                     bool showFullScreen = false);
#endif

// ---- Sleep / Restart actions (C linkage for LVGL UI callbacks) ----
#ifdef __cplusplus
extern "C" {
#endif
void menuSleepAction(void);
void menuRestartAction(void);
#ifdef __cplusplus
}
#endif
