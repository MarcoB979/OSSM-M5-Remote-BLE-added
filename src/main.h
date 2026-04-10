#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <lvgl.h>

#ifdef __cplusplus
#include <OneButton.h>
#include <ESP32Encoder.h>
#endif

// Uncomment the following line if you wish to print DEBUG info
#define DEBUG

#ifdef DEBUG
#define LogDebug(...) Serial.println(__VA_ARGS__)
#define LogDebugFormatted(...) Serial.printf(__VA_ARGS__)
#else
#define LogDebug(...) ((void)0)
#define LogDebugFormatted(...) ((void)0)
#endif

// Uncomment the following line if you wish to print priority DEBUG info
#define DEBUGPRIO

#ifdef DEBUGPRIO
#define LogDebugPrio(...) Serial.println(__VA_ARGS__)
#define LogDebugPrioFormatted(...) Serial.printf(__VA_ARGS__)
#else
#define LogDebugPrio(...) ((void)0)
#define LogDebugPrioFormatted(...) ((void)0)
#endif

#define HOR_RES 320
#define VER_RES 240

#ifndef OSSM_ID
#define OSSM_ID 1
#endif

#define OFF 10
#define ON 11

#define ST_UI_START 0
#define ST_UI_HOME 1
#define ST_UI_MENUE 10
#define ST_UI_PATTERN 11
#define ST_UI_Torqe 12
#define ST_UI_EJECTSETTINGS 13
#define ST_UI_SETTINGS 20
#define ST_UI_MENU 21
#define ST_UI_STREAMING 22
#define ST_UI_ADDONS 23

#define state_OFF 0
#define state_ON 1
#define state_PAUSE 2
#define state_STOP 3
#define state_MENU 4
#define state_HOMING 5
#define state_STREAMING 6
#define state_SIMPLE_PENETRATION 7
#define state_STROKE_ENGINE 8
#define state_ERROR 9

#define state_FALSE state_OFF
#define state_TRUE state_ON

#define CONN 0
#define SPEED 1
#define DEPTH 2
#define STROKE 3
#define SENSATION 4
#define PATTERN 5
#define TORQE_F 6
#define TORQE_R 7
#define SETUP_D_I 12
#define SETUP_D_I_F 13
#define REBOOT 14

#define CUMSPEED 20
#define CUMTIME 21
#define CUMSIZE 22
#define CUMACCEL 23

#define CONNECT 88
#define HEARTBEAT 99

// Shared notification result values for modal overlay prompts.
typedef enum NotificationResult {
    NOTIFICATION_RESULT_NONE = 0,
    NOTIFICATION_RESULT_LEFT = 1,
    NOTIFICATION_RESULT_RIGHT = 2,
} NotificationResult;

#ifdef __cplusplus
inline bool isRunningUiState(int state)
{
    return state == state_TRUE || state == state_STREAMING;
}
#endif

extern float maxdepthinmm;
extern float speedlimit;
extern int g_brightness_value;
extern bool eject_status;
extern bool vibrate_mode;
extern bool touch_home;
extern char patternstr[20];
extern bool dark_mode;
extern bool touch_disabled;
extern int st_screens;
extern int OSSM_State;
extern float speed;
extern float depth;
extern float stroke;
extern float sensation;
extern float torqe_f;
extern float torqe_r;
extern uint32_t ossm_state_monitor_hold_until_ms;
extern unsigned long last_activity_ms;
extern int screensaver_prev_brightness;
extern bool screensaver_active;
extern long speedenc;
extern long torqe_f_enc;
extern long torqe_r_enc;
extern long encoder3_enc;
extern long encoder4_enc;
extern bool strokeinvert_mode;
extern bool dynamicStroke;
extern bool Ossm_paired;
extern bool ossm_espnow_connected;  // True if OSSM is connected via ESP_NOW (not an addon)
extern bool waiting_for_limits;
extern int pattern;
#ifdef __cplusplus
extern ESP32Encoder encoder1;
extern ESP32Encoder encoder2;
extern ESP32Encoder encoder3;
extern ESP32Encoder encoder4;
#endif

extern bool mxclick_short_waspressed;
extern bool mxclick_long_waspressed;
extern bool mxclick_double_waspressed;
extern bool clickLeft_short_waspressed;
extern bool clickLeft_double_waspressed;
extern bool clickRight_short_waspressed;
extern bool clickRight_long_waspressed;
extern bool clickRight_double_waspressed;

#ifdef __cplusplus
extern OneButton Button1;
extern OneButton Button2;
extern OneButton Button3;
#endif

extern bool g_streaming_entry_flow_pending;
extern bool g_streaming_controls_locked;

#ifdef __cplusplus
// Core command / state
bool SendCommand(int Command, float Value, int Target);
bool canEnterDeepSleep();
void enterDeepSleep();
void monitorOssmState(bool forceBlePoll = false);
void screensaver_check_activity();
uint32_t my_tick_function();
void my_display_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
void updateHomeTopLeftStateLabel();
void syncBleConnectUi(bool forceRefresh);

// Vibration
void vibrate(int vbr_Intensity = 200, int vbr_Length = 100);

// Button handlers
void mxclick();
void mxdouble();
void mxlong();
void clickLeft();
void clickLeftDouble();
void clickRight();
void clickRightLong();
void clickRightDouble();
void clearButtonFlags();
void updateMxReleaseStability();
void markEncoderActivityForMxFilter();
void register_event_debug_callbacks();

// Screen dispatch
void handleCurrentScreen();
void my_touchpad_read(lv_indev_t *drv, lv_indev_data_t *data);
void event_cb(lv_event_t *e);
void markStartScreenLoaded();
bool isStartScreenMinTimeElapsed();

// Addon trigger (used from screen handlers)
bool triggerAddonForSlot(int slot);

// Button action / UI
void streamingbuttonm_action(bool fromPhysicalMx);
void streamingStartOnlyAction();
void homebuttonm_action(bool fromPhysicalMx);
void refreshHomeAndStreamingStartStopUi();

// ESP-NOW
void EspNowInitCommunication();
void EspNowSendPairingHeartbeat();
void EspNowWaitForPairingOrTimeout(uint32_t timeoutMs, uint32_t heartbeatIntervalMs);
bool EspNowSendControlCommand(int command, float value, int target);
void EspNowProcessPendingUiUpdates();  // Process UI updates from ESP-NOW in thread-safe way
#endif

#ifdef __cplusplus
extern "C" {
#endif

void connectbutton(lv_event_t * e);
void requestStreamingEntryFlow(void);
void streamingReturnToMenu(void);
void requestMenuEntryAction(void);
void menuSleepAction(void);
void menuRestartAction(void);
void addonsScreenLoaded(void);
void addonsSelectIndex(int index);
void homebuttonmevent(lv_event_t * e);
void streamingbuttonmevent(lv_event_t * e);
void setupDepthInter(lv_event_t * e);
void setupdepthF(lv_event_t * e);
void homebuttonLevent(lv_event_t * e);
void savepattern(lv_event_t * e);
void savesettings(lv_event_t * e);
void brightness_slider_event_cb(lv_event_t * e);

#ifdef __cplusplus
}

int showNotification(const char *title,
                     const char *text,
                     uint32_t duration,
                     bool showLeftButton = false,
                     const char *leftButtonText = nullptr,
                     bool showRightButton = false,
                     const char *rightButtonText = nullptr,
                     bool showFullScreen = false);
void handleStreamingEntryFlow();
#endif

