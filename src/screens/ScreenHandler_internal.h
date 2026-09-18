#pragma once
#include <lvgl.h>
#include <stdint.h>

// Shared internal declarations for the ScreenHandler* translation units
// (split during Phase B). Public API stays in ScreenHandler.h; everything here
// is implementation detail shared between the routing/power/notifications/
// settings/motion files.

// ---- Screen resolution constants (same as backup firmware main.h) ----
#ifndef HOR_RES
#define HOR_RES 320
#define VER_RES 240
#endif

// ---- Shared enums (defined here; moved out of ScreenHandler.cpp) ----
enum SpeedBehavior {
    SPEED_BEHAVIOR_STANDARD = 0,
    SPEED_BEHAVIOR_NATURAL = 1,
    SPEED_BEHAVIOR_TAMED = 2,
};
enum EncRampProfile {
    ENCODER_RAMP_NONE = 0,
    ENCODER_RAMP_MEDIUM = 1,
    ENCODER_RAMP_HIGH = 2,
    ENCODER_RAMP_AGGRESSIVE = 3,
};

// ---- Shared constants (defined in ScreenHandler.cpp) ----
extern const int      HOME_START_RAMP_THRESHOLD;
extern const uint32_t HOME_START_RAMP_INTERVAL_MS;
extern const float    VIS_SPEED_CURVE_MAX_STEP;

// ---- Shared state (defined in ScreenHandler.cpp) ----
extern bool           onoff;
extern bool           s_motion_command_cache_valid;
extern float          s_last_motion_speed;
extern float          s_last_motion_depth;
extern float          s_last_motion_stroke;
extern uint32_t       s_last_local_motion_input_ms;
extern bool           s_zero_stroke_depth_jog_active;
extern float          s_zero_stroke_depth_target;
extern int            s_zero_stroke_depth_direction;
extern uint32_t       s_zero_stroke_depth_jog_start_ms;
extern uint32_t       s_zero_stroke_debug_log_ms;
extern bool           s_visual_speed_lock;
extern bool           s_visual_speed_ratio_valid;
extern float          s_visual_speed_stroke_product;
extern float          s_visual_speed_last_commanded;
extern uint32_t       s_visual_speed_log_ms;
extern float          s_manual_rail_length_mm;
extern bool           s_home_speed_ramp_active;
extern int            s_home_speed_ramp_current;
extern int            s_home_speed_ramp_target;
extern int            s_home_speed_ramp_step;
extern uint32_t       s_home_speed_ramp_interval_ms;
extern uint32_t       s_home_speed_ramp_next_ms;
extern int            s_encoder_ramp_profile;
extern uint32_t       s_encoder_last_step_ms[4];
extern int            s_settings_focus_index;
extern int            s_settings_scroll_offset;
extern lv_obj_t*      s_manual_rail_length_setting;
extern lv_obj_t*      s_encoder_ramp_profile_setting;
extern lv_obj_t*      s_language_setting;
extern lv_obj_t*      s_wifi_setting;
extern bool           s_manual_rail_length_ui_syncing;
extern bool           s_speed_behavior_ui_syncing;
extern int            s_speed_behavior_profile;

// ---- MotionControl.cpp ----
void syncHomeSliderRangesToLimits();
void syncHomeSensationSliderToTransport();
void syncHomeValuesFromOssm(bool speedDragged, bool depthDragged,
                            bool strokeDragged, bool sensationDragged);
void serviceHomeSpeedRamp();
void serviceZeroStrokeDepthJog();
void flushMotionCommands(float motionSpeed, float motionDepth, float motionStroke,
                         bool motionValueChanged, bool allowSend);
void stopZeroStrokeDepthJog();
void syncMotionCommandCache(float motionSpeed, float motionDepth, float motionStroke);

// ---- SettingsScreen.cpp ----
void      applySpeedBehavior(int profile);
void      resetVisualSpeedRatioState();
void      updateVisualSpeedRatioFromUi(bool uiSpeedChanged, float uiSpeed, float uiStroke);
float     resolveVisualCompensatedSpeed(float uiSpeed, float uiStroke);
int       collectSettingsOptionObjects(lv_obj_t** outObjects, int maxObjects);
void      refreshSettingsCarousel();
lv_obj_t* getSettingsFocusedObject();
void      ensureEncRampProfileSetting();
void      syncEncRampProfileSettingUi();
void      syncSpeedBehaviorSettingUi();
void      ensureLanguageSetting();
void      syncLanguageSettingUi();
void      syncWifiSettingUi();

// ---- MotionControl.cpp (continued) ----
void syncHomeMotionUi(bool invertStroke);
void rampHomeStartSpeed(int targetSpeed);
void rampHomeStopSpeed(int startSpeed);

// ---- PowerManagement.cpp ----
bool detectChargingNow();
int  readBatteryPercentForUi(bool isCharging);
void update_battery_icons_all_screens(int level, bool isCharging);
bool getStableChargingState();
void maybeShowChargingWarning(bool isCharging);
int  getSmoothedBatteryLevel(bool isCharging);

// ---- SettingsScreen.cpp (continued) ----
float getDefaultManualRailLengthMm();
void  SpeedBehavior_event_cb(lv_event_t* e);
// ---- ScreenHandler.cpp (routing) ----
bool requestHomeButtonToggleOnce();
