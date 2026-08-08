#include "ScreenHandler.h"
#include <lvgl.h>
#include <Preferences.h>
#include <esp_sleep.h>
#include "../config/config_power.h"
#include "../ui/ui.h"
#include "../ui/ui_helpers.h"
#include "../main.h"
#include "../config/debug.h"
#include "../config/config_pins.h"
#include "../config/config_ids.h"
#include "../PatternMath.h"
#include "../buttonhandlers/ButtonHandlers.h"
#include "../addons/Eject.h"
#include "../addons/FistIT.h"
#include "../addons/AP-mode.h"
#include "../addons/addonsStreaming.h"
#include "../communication/EspNowComm.h"
#include "../communication/CommManager.h"
#include "../communication/BleComm.h"
#include "../display/colors.h"
#include "../display/styles.h"
#include "../addons/strokeMode.h"
#include "../screens/icons.h"
#include "language.h"
#include <M5Unified.h>
#include <cmath>
#include <string>

// Screen resolution constants (same as backup firmware main.h)
#ifndef HOR_RES
#define HOR_RES 320
#define VER_RES 240
#endif

// -------------------------------------------------------
// Screen and control state
// -------------------------------------------------------
int   st_screens  = ST_UI_START;
int   menuestatus = 0;

float speed     = 0.0f;
float depth     = 0.0f;
float stroke    = 0.0f;
float sensation = 0.0f;
float torqe_f   = 100.0f;
float torqe_r   = -180.0f;
float cum_time  = 0.0f;
float cum_speed = 0.0f;
float cum_size  = 0.0f;
float cum_accel = 0.0f;

long speedenc     = 0;
long depthenc     = 0;
long strokeenc    = 0;
long sensationenc = 0;
long torqe_f_enc  = 0;
long torqe_r_enc  = 0;
long cum_t_enc    = 0;
long cum_si_enc   = 0;
long cum_s_enc    = 0;
long cum_a_enc    = 0;
long encoder3_enc = 0;
long encoder4_enc = 0;

int  pattern = 2;
char patternstr[20];
lv_obj_t *g_pattern_return_screen = nullptr;
lv_obj_t *g_addon_return_screen = nullptr;
static int  s_prev_st_screens = -1;
// Set when menu entry sends go:menu (from streaming exit or ble_force_homeing).
// Home/stroke screen checks this flag before sending go:strokeEngine, so that
// home→menu→home does NOT cause an unnecessary re-home.
static bool s_ble_menu_requires_stroke_reentry = false;
// Sent once after startup when Home is first opened while connected
static bool s_initial_pattern_sent = false;
static bool  s_motion_command_cache_valid = false;
static float s_last_motion_speed = 0.0f;
static float s_last_motion_depth = 0.0f;
static float s_last_motion_stroke = 0.0f;
static bool  s_zero_stroke_depth_jog_active = false;
static float s_zero_stroke_depth_target = 0.0f;
static int   s_zero_stroke_depth_direction = 0;
static bool  s_visual_speed_lock = false;
static bool  s_visual_speed_ratio_valid = false;
static float s_visual_speed_stroke_product = 0.0f;
static float s_visual_speed_last_commanded = -1.0f;
enum SpeedBehavior {
    SPEED_BEHAVIOR_STANDARD = 0,
    SPEED_BEHAVIOR_NATURAL = 1,
    SPEED_BEHAVIOR_TAMED = 2,
};
static int s_speed_behavior_profile = SPEED_BEHAVIOR_STANDARD;
static bool  s_stroke_influences_depth = false;
static float s_manual_rail_length_mm = 0.0f;
static bool s_home_speed_ramp_active = false;
static int s_home_speed_ramp_current = 0;
static int s_home_speed_ramp_target = 0;
static int s_home_speed_ramp_step = 0;
static uint32_t s_home_speed_ramp_interval_ms = 0;
static uint32_t s_home_speed_ramp_next_ms = 0;
static bool s_force_home_restore_pending = false;
static uint32_t s_zero_stroke_depth_jog_start_ms = 0;
static uint32_t s_zero_stroke_debug_log_ms = 0;
static uint32_t s_visual_speed_log_ms = 0;
static bool s_home_toggle_fired_this_loop = false;
static bool s_consume_next_mx_short_click = false;

// Variables for natural speed curve (to make speed drop at low stroke).
// Runtime profile selection:
//   Standard (legacy): strongest suppression at low stroke
//   Natural: balanced distance-based behavior
//   Tamed: between Standard and Natural
static constexpr float VIS_SPEED_STANDARD_MIN_FACTOR = 0.14f;
static constexpr float VIS_SPEED_STANDARD_KNEE_STROKE = 308.0f;
static constexpr float VIS_SPEED_STANDARD_POWER = 0.30f;

static constexpr float VIS_SPEED_NATURAL_MIN_FACTOR = 0.32f;
static constexpr float VIS_SPEED_NATURAL_KNEE_STROKE = 22.0f;
static constexpr float VIS_SPEED_NATURAL_POWER = 1.05f;

static constexpr float VIS_SPEED_TAMED_MIN_FACTOR = 0.40f;
static constexpr float VIS_SPEED_TAMED_KNEE_STROKE = 18.0f;
static constexpr float VIS_SPEED_TAMED_POWER = 0.90f;

//Higher value = stronger suppression at low/mid stroke, and longer before speed recovers.
//Example: 2.0 -> 2.8 or 3.2.
static constexpr float VIS_SPEED_CURVE_MAX_STEP = 2.0f;


static constexpr bool ENABLE_ZERO_STROKE_DEPTH_JOG = false;


static constexpr float ZERO_STROKE_DEPTH_TOLERANCE = 1.0f;
static constexpr uint32_t ZERO_STROKE_DEPTH_JOG_TIMEOUT_MS = 25000;
static constexpr int SETTINGS_CAROUSEL_VISIBLE_COUNT = 4;
enum EncRampProfile {
    ENCODER_RAMP_NONE = 0,
    ENCODER_RAMP_MEDIUM = 1,
    ENCODER_RAMP_HIGH = 2,
    ENCODER_RAMP_AGGRESSIVE = 3,
};
static int s_settings_focus_index = 0;
static int s_settings_scroll_offset = 0;
static lv_obj_t* s_manual_rail_length_setting = nullptr;
static lv_obj_t* s_encoder_ramp_profile_setting = nullptr;
static bool s_manual_rail_length_ui_syncing = false;
static bool s_speed_behavior_ui_syncing = false;
static int s_encoder_ramp_profile = ENCODER_RAMP_MEDIUM;
static void resetVisualSpeedRatioState();
static void updateVisualSpeedRatioFromUi(bool uiSpeedChanged, float uiSpeed, float uiStroke);
static float resolveVisualCompensatedSpeed(float uiSpeed, float uiStroke);
static void ensureManualRailLengthSetting();
static void ensureEncRampProfileSetting();
static int collectSettingsOptionObjects(lv_obj_t** outObjects, int maxObjects);
static void refreshSettingsCarousel();
static lv_obj_t* getSettingsFocusedObject();

static void syncManualRailLengthSettingUi();
static void syncEncRampProfileSettingUi();
static void syncSpeedBehaviorSettingUi();
static void persistManualRailLengthSetting();
static void persistEncRampProfileSetting();
static void persistSpeedBehaviorSetting();
static void runManualRailLengthCalibrationWorkflow();
static void manualRailLengthSetting_event_cb(lv_event_t* e);
static void EncRampProfile_event_cb(lv_event_t* e);
static void SpeedBehavior_event_cb(lv_event_t* e);
static const char* getSpeedBehaviorName(int profile);
static void applySpeedBehavior(int profile);
static float getDefaultManualRailLengthMm();

bool dynamicStroke  = false;
bool eject_status   = false;
bool vibrate_mode   = true;
bool touch_home     = false;
bool strokeinvert_mode = false;
bool ble_force_homeing = false;
bool touch_disabled = true;  // TOUCH TEMPRARILY DISABLED BECAUSE OF PROBLEMS IN CODE (FAKE MX CLICKS)
bool SafeStartStop   = false;
bool onoff          = false;
bool rstate         = false;
bool EJECT_On       = false;

// ---- Screensaver / Power management (ported from backup firmware) ----
int            g_brightness_value       = 180;
unsigned long  last_activity_ms         = 0;
int            screensaver_prev_brightness = 180;
bool           screensaver_active       = false;
int            screensaver_timeout_ms   = SCREENSAVER_TIMEOUT_MS_DEFAULT;
int            screensaver_dim_brightness = SCREENSAVER_DIM_BRIGHTNESS_DEFAULT;
uint32_t       deep_sleep_timeout_ms    = DEEP_SLEEP_TIMEOUT_MS_DEFAULT;

// Notification touch result (set by LVGL button callbacks inside showNotification)
static volatile int g_notification_touch_result = NOTIFICATION_RESULT_NONE;
static volatile bool g_status_strip_refresh_requested = true;
static uint32_t s_encoder_last_step_ms[4] = {0, 0, 0, 0};

static constexpr uint32_t ENCODER_RAMP_MEDIUM_MS = 120;
static constexpr uint32_t ENCODER_RAMP_FAST_MS = 45;

static int homeStepFromEncoderCount(int encoderIndex, long count)
{
    if (encoderIndex < 0 || encoderIndex >= 4) return 0;

    const long magnitude = labs(count);
    if (magnitude < 2) return 0;

    if (s_encoder_ramp_profile == ENCODER_RAMP_NONE) {
        return (count > 0) ? 1 : -1;
    }

    const uint32_t nowMs = millis();
    const uint32_t lastMs = s_encoder_last_step_ms[encoderIndex];
    const uint32_t elapsedMs = (lastMs == 0U) ? UINT32_MAX : (nowMs - lastMs);
    s_encoder_last_step_ms[encoderIndex] = nowMs;

    int step = 1;
    if (elapsedMs <= ENCODER_RAMP_FAST_MS) {
        if (s_encoder_ramp_profile == ENCODER_RAMP_MEDIUM) {
            step = 3;
        } else if (s_encoder_ramp_profile == ENCODER_RAMP_HIGH) {
            step = 4;
        } else if (s_encoder_ramp_profile == ENCODER_RAMP_AGGRESSIVE) {
            step = 6;
        }
    } else if (elapsedMs <= ENCODER_RAMP_MEDIUM_MS) {
        if (s_encoder_ramp_profile == ENCODER_RAMP_MEDIUM) {
            step = 2;
        } else if (s_encoder_ramp_profile == ENCODER_RAMP_HIGH) {
            step = 3;
        } else if (s_encoder_ramp_profile == ENCODER_RAMP_AGGRESSIVE) {
            step = 4;
        }
    }

    if (step < 1) step = 1;
    if (step > 6) step = 6;

    return (count > 0) ? step : -step;
}

static constexpr int EJECT_ICON_W = 14;
static constexpr int EJECT_ICON_H = 20;
static constexpr int FIST_ICON_W = 17;
static constexpr int FIST_ICON_H = 18;
static constexpr int HOME_ICON_W = 18;
static constexpr int HOME_ICON_H = 22;
static constexpr int ESP_ICON_W = 21;
static constexpr int ESP_ICON_H = 18;

static const char* const EJECT_ICON_MASK[EJECT_ICON_H] = {
"...###.........",
"...####.....###",
"....###....##..",
".....##...#....",
".....##.......",
".....###..###.",
"..............",
"..............",
".....###......",
"...########...",
"..##########..",
".############.",
".############.",
"...########...",
"....######....",
"....######....",
"....######....",
"....######....",
"....######....",
"....######...."
};

static const char* const FIST_ICON_MASK[FIST_ICON_H] = {

"...........##....",
"..##..####.####..",
"##..##..#..##..#.",
"##..##..#..##..#.",
"#....#.........#.",
"#..............#.",
".#.............##.",
".#..#..#..##..#.#",
".#..#..#..##..#.#",
".#..#..#..##..#.#",
".#..#..#..##..#.#",
".#..#..#..##..#.#",
".##############.#",
"......#.........#",
"......#.........#",
".....##.......##.",
"......########...",
"................."
  };

static const char* const HOME_ICON_MASK[HOME_ICON_H] = {
  "......#####.......",
  "....#########.....",
  "..#####....#####..",
  "#####........#####",
  "..................",
  "..................",
  "....###....###....",
  "....###....###....",
  "....###....###....",
  "....###....###....",
  "....##########....",
  "....##########....",
  "....###....###....",
  "....###....###....",
  "....###....###....",
  "....###....###....",
  "..................",
  "..................",
  "#####........#####",
  "..#####....#####..",
  "....#########.....",
  "......#####......."
};

static const char* const ESP_ICON_MASK[ESP_ICON_H] = {
"......########.......",
"....##........##.....",
"...#...........##....",
"..#..#####......##...",
".#.......####....##..",
".#..##....####.....#.",
".#.####.....###.....#",
"#..##.####...###....#",
"#.##.....###..###...#",
"#..#####..###..###..#",
"#....###..###..###..#",
".#.....###..##..##..#",
"..#.##..###.##..##..#",
"..#.##..###.##..##..#",
"..#....###,.##..##.#.",
"...#...##...##....#..",
"....##.........##....",
"......#########......"
};


// -------------------------------------------------------
// Status Strip Rendering Helpers
// -------------------------------------------------------

static lv_obj_t* createStatusIconBase(lv_obj_t* parent, int width, int height) {
    if (!parent) return nullptr;
    lv_obj_t* icon = lv_canvas_create(parent);
    lv_obj_remove_style_all(icon);
    lv_obj_set_size(icon, width, height);
    lv_obj_clear_flag(icon, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(icon, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_opa(icon, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(icon, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    return icon;
}

static lv_obj_t* createStatusESPIcon(lv_obj_t* parent) {
    static uint8_t iconBuffer[LV_CANVAS_BUF_SIZE(32, 32, 32, LV_DRAW_BUF_STRIDE_ALIGN)];
    static bool iconReady = false;
    lv_obj_t* icon = createStatusIconBase(parent, ESP_ICON_W, ESP_ICON_H);
    if (!icon) return nullptr;
    icons_render_mask_canvas(icon, iconBuffer, iconReady, ESP_ICON_MASK, ESP_ICON_W, ESP_ICON_H,
                             getActiveBackgroundColor(), getActiveTextPrimaryColor());
    return icon;
}

static lv_obj_t* createStatusEjectIcon(lv_obj_t* parent) {
    static uint8_t iconBuffer[LV_CANVAS_BUF_SIZE(32, 32, 32, LV_DRAW_BUF_STRIDE_ALIGN)];
    static bool iconReady = false;
    lv_obj_t* icon = createStatusIconBase(parent, EJECT_ICON_W, EJECT_ICON_H);
    if (!icon) return nullptr;
    icons_render_mask_canvas(icon, iconBuffer, iconReady, EJECT_ICON_MASK, EJECT_ICON_W, EJECT_ICON_H,
                             getActiveBackgroundColor(), getActiveTextPrimaryColor());
    return icon;
}

static lv_obj_t* createStatusFistIcon(lv_obj_t* parent) {
    static uint8_t iconBuffer[LV_CANVAS_BUF_SIZE(32, 32, 32, LV_DRAW_BUF_STRIDE_ALIGN)];
    static bool iconReady = false;
    lv_obj_t* icon = createStatusIconBase(parent, FIST_ICON_W, FIST_ICON_H);
    if (!icon) return nullptr;
    icons_render_mask_canvas(icon, iconBuffer, iconReady, FIST_ICON_MASK, FIST_ICON_W, FIST_ICON_H,
                             getActiveBackgroundColor(), getActiveTextPrimaryColor());
    return icon;
}

static lv_obj_t* createStatusHomeIcon(lv_obj_t* parent) {
    static uint8_t iconBuffer[LV_CANVAS_BUF_SIZE(32, 32, 32, LV_DRAW_BUF_STRIDE_ALIGN)];
    static bool iconReady = false;
    lv_obj_t* icon = createStatusIconBase(parent, HOME_ICON_W, HOME_ICON_H);
    if (!icon) return nullptr;
    icons_render_mask_canvas(icon, iconBuffer, iconReady, HOME_ICON_MASK, HOME_ICON_W, HOME_ICON_H,
                             getActiveBackgroundColor(), getActiveTextPrimaryColor());
    return icon;
}


static void updateStatusStrip() {
    static lv_obj_t* statusLabels[12] = { nullptr };
    static lv_obj_t* statusEjectIcons[12] = { nullptr };
    static lv_obj_t* statusFistIcons[12] = { nullptr };
    static lv_obj_t* statusHomeIcons[12] = { nullptr };
    static lv_obj_t* statusESPIcons[12] = { nullptr };
    lv_obj_t* statusScreens[12] = {
        ui_Start,
        ui_Home,
        ui_Pattern,
        ui_EJECTSettings,
        ui_Settings,
        ui_Menu,
        ui_Streaming,
        ui_Addons,
        ui_Colors,
        ui_FistIT,
        ui_Stroke,
        APModeGetScreen(),
    };

    for (size_t i = 0; i < 12; ++i) {
        if (statusLabels[i] != nullptr) continue;
        if (statusScreens[i] == nullptr) continue;

        if (statusScreens[i] == ui_Home && ui_connect != nullptr) {
            statusLabels[i] = ui_connect;
        } else {
            statusLabels[i] = lv_label_create(statusScreens[i]);
        }

        lv_obj_set_width(statusLabels[i], LV_SIZE_CONTENT);
        lv_obj_set_height(statusLabels[i], LV_SIZE_CONTENT);
        lv_obj_set_align(statusLabels[i], LV_ALIGN_LEFT_MID);
        lv_obj_set_x(statusLabels[i], 10);
        lv_obj_set_y(statusLabels[i], -102);
        lv_obj_add_style(statusLabels[i], &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(statusLabels[i], &lv_font_montserrat_22, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(statusLabels[i], LV_OBJ_FLAG_HIDDEN);


        if (statusESPIcons[i] == nullptr) {
            statusESPIcons[i] = createStatusESPIcon(statusScreens[i]);
        }
        if (statusEjectIcons[i] == nullptr) {
            statusEjectIcons[i] = createStatusEjectIcon(statusScreens[i]);
        }
        if (statusFistIcons[i] == nullptr) {
            statusFistIcons[i] = createStatusFistIcon(statusScreens[i]);
        }
        if (statusHomeIcons[i] == nullptr) {
            statusHomeIcons[i] = createStatusHomeIcon(statusScreens[i]);
        }
    }

    char labelText[48];
    size_t pos = 0;
    labelText[0] = '\0';

    auto appendToken = [&](const char* token) {
        if (token == nullptr || token[0] == '\0' || pos >= sizeof(labelText) - 1) return;
        if (pos > 0 && pos < sizeof(labelText) - 1) {
            labelText[pos++] = ' ';
            labelText[pos] = '\0';
        }
        int written = snprintf(labelText + pos, sizeof(labelText) - pos, "%s", token);
        if (written > 0) {
            pos += (size_t)written;
            if (pos >= sizeof(labelText)) pos = sizeof(labelText) - 1;
        }
    };

    if (bleCommIsConnected()) {
        appendToken(LV_SYMBOL_BLUETOOTH);
    }

    if (labelText[0] == '\0') {
        snprintf(labelText, sizeof(labelText), " ");
    }

    const bool ejectPaired = espNowIsEjectConnected();
    const bool fistPaired = espNowIsFistConnected();
    const bool espPaired = espNowIsPaired();
    for (size_t i = 0; i < 12; ++i) {
        lv_obj_t* label = statusLabels[i];
        if (label == nullptr) continue;

        lv_obj_add_style(label, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_22, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(label, labelText);
        lv_obj_update_layout(label);

        int iconX = 10 + lv_obj_get_width(label) + 4;

        if (statusESPIcons[i] != nullptr) {
            lv_obj_set_align(statusESPIcons[i], LV_ALIGN_LEFT_MID);
            lv_obj_set_x(statusESPIcons[i], iconX);
            lv_obj_set_y(statusESPIcons[i], -102);
                if (espPaired) {
                lv_obj_clear_flag(statusESPIcons[i], LV_OBJ_FLAG_HIDDEN);
                iconX += lv_obj_get_width(statusESPIcons[i]) + 2;
            } else {
                lv_obj_add_flag(statusESPIcons[i], LV_OBJ_FLAG_HIDDEN);
            }
        }

        if (statusFistIcons[i] != nullptr) {
            lv_obj_set_align(statusFistIcons[i], LV_ALIGN_LEFT_MID);
            lv_obj_set_x(statusFistIcons[i], iconX);
            lv_obj_set_y(statusFistIcons[i], -102);
            if (fistPaired) {
                lv_obj_clear_flag(statusFistIcons[i], LV_OBJ_FLAG_HIDDEN);
                iconX += lv_obj_get_width(statusFistIcons[i]) + 2;
            } else {
                lv_obj_add_flag(statusFistIcons[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
 
        if (statusEjectIcons[i] != nullptr) {
            lv_obj_set_align(statusEjectIcons[i], LV_ALIGN_LEFT_MID);
            lv_obj_set_x(statusEjectIcons[i], iconX);
            lv_obj_set_y(statusEjectIcons[i], -102);
            if (ejectPaired) {
                lv_obj_clear_flag(statusEjectIcons[i], LV_OBJ_FLAG_HIDDEN);
                iconX += lv_obj_get_width(statusEjectIcons[i]) + 2;
            } else {
                lv_obj_add_flag(statusEjectIcons[i], LV_OBJ_FLAG_HIDDEN);
            }
        }

        if (statusHomeIcons[i] != nullptr) {
            lv_obj_set_align(statusHomeIcons[i], LV_ALIGN_LEFT_MID);
            lv_obj_set_x(statusHomeIcons[i], iconX);
            lv_obj_set_y(statusHomeIcons[i], -102);
            if (bleCommIsHoming()) {
                lv_obj_clear_flag(statusHomeIcons[i], LV_OBJ_FLAG_HIDDEN);
                iconX += lv_obj_get_width(statusHomeIcons[i]) + 2;
            } else {
                lv_obj_add_flag(statusHomeIcons[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}

void screenRequestStatusStripRefresh() {
    g_status_strip_refresh_requested = true;
}

void screenForceStatusStripRefreshNow() {
    g_status_strip_refresh_requested = true;
    updateStatusStrip();
    g_status_strip_refresh_requested = false;
}

static int rangeFromLimit(float limitValue) {
    int limit = (int)(limitValue + 0.5f);
    if (limit < 1) limit = 1;
    return limit;
}

static void syncHomeSliderRangesToLimits() {
    if (!ui_homespeedslider || !ui_homedepthslider || !ui_homestrokeslider) return;

    const int speedMax = rangeFromLimit(speedlimit);
    const int depthMax = rangeFromLimit(maxdepthinmm);

    // Keep slider ranges aligned with current transport limits (BLE vs ESP-NOW).
    if (lv_slider_get_max_value(ui_homespeedslider) != speedMax) {
        lv_slider_set_range(ui_homespeedslider, 0, speedMax);
    }
    if (lv_slider_get_max_value(ui_homedepthslider) != depthMax) {
        lv_slider_set_range(ui_homedepthslider, 0, depthMax);
    }
    if (lv_slider_get_max_value(ui_homestrokeslider) != depthMax) {
        lv_slider_set_range(ui_homestrokeslider, 0, depthMax);
    }

    if (speed > speedMax) speed = (float)speedMax;
    if (depth > depthMax) depth = (float)depthMax;
    if (stroke > depthMax) stroke = (float)depthMax;
    if (stroke > depth) stroke = depth;
}

static void syncHomeSensationSliderToTransport() {
    if (!ui_homesensationslider) return;

    const bool bleMode = commIsBleMode();
//    const int desiredMin = bleMode ? 0 : -100;
    const int desiredMin = -100;
    const int desiredMax = 100;
    const lv_slider_mode_t desiredMode = bleMode ? LV_SLIDER_MODE_NORMAL : LV_SLIDER_MODE_SYMMETRICAL;

    const bool rangeChanged =
        (lv_slider_get_min_value(ui_homesensationslider) != desiredMin) ||
        (lv_slider_get_max_value(ui_homesensationslider) != desiredMax);

    if (rangeChanged) {
        lv_slider_set_range(ui_homesensationslider, desiredMin, desiredMax);
    }

    if (lv_slider_get_mode(ui_homesensationslider) != desiredMode) {
        lv_slider_set_mode(ui_homesensationslider, desiredMode);
    }

    if (rangeChanged) {
        sensation = bleMode ? 50.0f : 0.0f;
    }

    if (sensation < desiredMin) sensation = (float)desiredMin;
    if (sensation > desiredMax) sensation = (float)desiredMax;
    lv_slider_set_value(ui_homesensationslider, (int)sensation, LV_ANIM_OFF);
}

// -------------------------------------------------------
// Power Management / Sleep Helpers
// -------------------------------------------------------

void screensaver_check_activity()
{
    last_activity_ms = millis();
    if (screensaver_active) {
        M5.Lcd.setBrightness(screensaver_prev_brightness);
        screensaver_active = false;
    }
}

bool canEnterDeepSleep()
{
    // Do not enter deep sleep while the OSSM is actively running.
    return !onoff;
}

static bool areWakeButtonsReleased()
{
    return (digitalRead(Button1.pin()) == LOW) &&
           (digitalRead(Button2.pin()) == LOW) &&
           (digitalRead(Button3.pin()) == LOW);
}

static bool waitWakeButtonsReleasedStable(uint32_t stableMs, uint32_t timeoutMs)
{
    const uint32_t startMs = millis();
    uint32_t releasedSinceMs = 0;

    while ((millis() - startMs) < timeoutMs) {
        const bool released = areWakeButtonsReleased();
        if (released) {
            if (releasedSinceMs == 0) releasedSinceMs = millis();
            if ((millis() - releasedSinceMs) >= stableMs) return true;
        } else {
            releasedSinceMs = 0;
        }
        delay(5);
    }
    return false;
}

extern "C" void RestartM5()
{
    ESP.restart();
}

void enterDeepSleep()
{
    gpio_num_t mxPin    = static_cast<gpio_num_t>(Button1.pin());
    gpio_num_t leftPin  = static_cast<gpio_num_t>(Button2.pin());
    gpio_num_t rightPin = static_cast<gpio_num_t>(Button3.pin());
    uint64_t wakeMask   = (1ULL << mxPin) | (1ULL << leftPin) | (1ULL << rightPin);

    LogDebug("Entering deep sleep (wake on MX/left/right)");
    M5.Display.setBrightness(0);
    M5.Power.setVibration(0);

    // Guard against instant wake when any wake button is still held.
    if (!waitWakeButtonsReleasedStable(120, 1200)) {
        LogDebugFormatted("Deep sleep canceled: wake button(s) still active\n");
        screensaver_check_activity();
        return;
    }

    esp_sleep_enable_ext1_wakeup(wakeMask, ESP_EXT1_WAKEUP_ANY_HIGH);
    delay(50);
    esp_deep_sleep_start();
}

// -------------------------------------------------------
// Notification Overlay Helpers
// -------------------------------------------------------
static void notification_left_button_cb(lv_event_t *e)
{
    (void)e;
    g_notification_touch_result = NOTIFICATION_RESULT_LEFT;
}

static void notification_right_button_cb(lv_event_t *e)
{
    (void)e;
    g_notification_touch_result = NOTIFICATION_RESULT_RIGHT;
}

// ---------------------------------------------------------------------------
// showNotification() — blocking modal overlay (ported from backup firmware)
// ---------------------------------------------------------------------------
int showNotification(const char *title,
                     const char *text,
                     uint32_t duration,
                     bool showLeftButton,
                     const char *leftButtonText,
                     bool showRightButton,
                     const char *rightButtonText,
                     bool showFullScreen)
{
    const bool hasButtons = showLeftButton || showRightButton;
    const bool prevTouchDisabled = touch_disabled;
    const bool shouldBlockTouch  = !hasButtons;
    const uint32_t startMs = millis();
    int result = NOTIFICATION_RESULT_NONE;
    g_notification_touch_result = NOTIFICATION_RESULT_NONE;

    // Derive color scheme values
    uint32_t schemePrimary       = getActivePrimaryColor();
    uint32_t schemeSecondary     = getActiveSecondaryColor();
    uint32_t schemeTextPrimary   = getActiveTextPrimaryColor();
    uint32_t schemeTextSecondary = getActiveTextSecondaryColor();
    uint8_t pr = (schemePrimary >> 16) & 0xFF;
    uint8_t pg = (schemePrimary >>  8) & 0xFF;
    uint8_t pb =  schemePrimary        & 0xFF;
    uint32_t schemeDarker = (((pr >> 1) & 0xFF) << 16) |
                            (((pg >> 1) & 0xFF) <<  8) |
                             ((pb >> 1) & 0xFF);

    if (shouldBlockTouch) touch_disabled = true;

    // Drain stale button states before opening the modal.
    mxpress_waspressed       = false;
    mxclick_short_waspressed  = false;
    mxclick_long_waspressed   = false;
    click2_short_waspressed   = false;
    click2_long_waspressed    = false;
    click3_short_waspressed   = false;
    click3_long_waspressed    = false;
    click3_double_waspressed  = false;

    lv_obj_t *overlay = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(overlay);
    lv_obj_set_size(overlay, HOR_RES, VER_RES);
    lv_obj_center(overlay);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_50, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(overlay, lv_color_hex(schemeDarker), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(overlay, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *panel = lv_obj_create(overlay);
    if (showFullScreen) {
        const int topOffset     = 32;
        const int bottomPadding = 5;
        lv_obj_set_size(panel, 310, VER_RES - topOffset - bottomPadding);
        lv_obj_set_pos(panel, 5, topOffset);
    } else {
        lv_obj_set_size(panel, (HOR_RES * 90) / 100, (VER_RES * 75) / 100);
        lv_obj_align(panel, LV_ALIGN_CENTER, 0, 5);
    }
    lv_obj_set_style_radius(panel, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(panel, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(panel, lv_color_hex(schemePrimary), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(panel, lv_color_hex(schemeSecondary), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *titleBar = lv_obj_create(panel);
    lv_obj_remove_style_all(titleBar);
    lv_obj_set_size(titleBar, lv_pct(100), 32);
    lv_obj_align(titleBar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(titleBar, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(titleBar, lv_color_hex(schemeDarker), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *titleLabel = lv_label_create(titleBar);
    lv_label_set_text(titleLabel, (title != nullptr && title[0] != '\0') ? title : "Notification");
    lv_obj_set_style_text_align(titleLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(titleLabel, lv_color_hex(schemeTextPrimary), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(titleLabel, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(titleLabel);

    lv_obj_t *bodyLabel = lv_label_create(panel);
    lv_obj_set_width(bodyLabel, lv_pct(90));
    lv_label_set_long_mode(bodyLabel, LV_LABEL_LONG_WRAP);
    lv_label_set_text(bodyLabel, (text != nullptr) ? text : "");
    lv_obj_set_style_text_align(bodyLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(bodyLabel, lv_color_hex(schemeTextPrimary), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(bodyLabel, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(bodyLabel, LV_ALIGN_TOP_MID,0, 38);

    if (hasButtons) {
        lv_obj_t *buttonRow = lv_obj_create(panel);
        lv_obj_remove_style_all(buttonRow);
        lv_obj_set_size(buttonRow, lv_pct(94), 44);
        lv_obj_align(buttonRow, LV_ALIGN_BOTTOM_MID, 0, -5);
        lv_obj_set_style_bg_opa(buttonRow, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(buttonRow, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(buttonRow, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        if (showLeftButton) {
            lv_obj_t *leftBtn = lv_btn_create(buttonRow);
            lv_obj_set_size(leftBtn, 120, 36);
            lv_obj_align(leftBtn, LV_ALIGN_LEFT_MID, 0, 0);
            lv_obj_add_style(leftBtn, &style_button_l, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_style(leftBtn, &style_button_l_pressed, LV_PART_MAIN | LV_STATE_PRESSED);
            lv_obj_t *leftLbl = lv_label_create(leftBtn);
            lv_label_set_text(leftLbl, (leftButtonText != nullptr && leftButtonText[0] != '\0') ? leftButtonText : "Left");
            lv_obj_set_style_text_color(leftLbl, lv_color_hex(schemeTextPrimary), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_center(leftLbl);
            lv_obj_add_event_cb(leftBtn, notification_left_button_cb, LV_EVENT_SHORT_CLICKED, nullptr);
        }

        if (showRightButton) {
            lv_obj_t *rightBtn = lv_btn_create(buttonRow);
            lv_obj_set_size(rightBtn, 120, 36);
            lv_obj_align(rightBtn, LV_ALIGN_RIGHT_MID, 0, 0);
            lv_obj_add_style(rightBtn, &style_button_l, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_style(rightBtn, &style_button_l_pressed, LV_PART_MAIN | LV_STATE_PRESSED);
            lv_obj_t *rightLbl = lv_label_create(rightBtn);
            lv_label_set_text(rightLbl, (rightButtonText != nullptr && rightButtonText[0] != '\0') ? rightButtonText : "Right");
            lv_obj_set_style_text_color(rightLbl, lv_color_hex(schemeTextPrimary), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_center(rightLbl);
            lv_obj_add_event_cb(rightBtn, notification_right_button_cb, LV_EVENT_SHORT_CLICKED, nullptr);
        }
    }

    while (true) {
        M5.update();
        lv_task_handler();
        Button1.tick();
        Button2.tick();
        Button3.tick();

        if (duration > 0 && (millis() - startMs) >= duration) {
            result = NOTIFICATION_RESULT_NONE;
            break;
        }

        if (hasButtons) {
            if (g_notification_touch_result != NOTIFICATION_RESULT_NONE) {
                result = g_notification_touch_result;
                break;
            }
            if (showLeftButton && click2_short_waspressed) {
                result = NOTIFICATION_RESULT_LEFT;
                break;
            }
            if (showRightButton && click3_short_waspressed) {
                result = NOTIFICATION_RESULT_RIGHT;
                break;
            }
        }

        // Consume all button events so the current screen never sees stale flags.
        mxpress_waspressed       = false;
        mxclick_short_waspressed  = false;
        mxclick_long_waspressed   = false;
        click2_short_waspressed   = false;
        click2_long_waspressed    = false;
        click3_short_waspressed   = false;
        click3_long_waspressed    = false;
        click3_double_waspressed  = false;
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    lv_obj_del(overlay);

    // Clear flags after modal closes.
    mxpress_waspressed       = false;
    mxclick_short_waspressed  = false;
    mxclick_long_waspressed   = false;
    click2_short_waspressed   = false;
    click2_long_waspressed    = false;
    click3_short_waspressed   = false;
    click3_long_waspressed    = false;
    click3_double_waspressed  = false;

    if (shouldBlockTouch) touch_disabled = prevTouchDisabled;

    return result;
}

// -------------------------------------------------------
// Battery UI Helpers
// -------------------------------------------------------
static const char* battery_symbol_for_level(int level, bool isCharging)
{
    if (level < 0) level = 0;
    if (level > 100) level = 100;

    // Thresholds aligned to the non-linear Li-ion curve bins:
    // 0..7, 8..27, 28..57, 58..87, 88..100
    const char* baseSymbol = LV_SYMBOL_BATTERY_EMPTY;
    if (level >= 88) {
        baseSymbol = LV_SYMBOL_BATTERY_FULL;
    } else if (level >= 58) {
        baseSymbol = LV_SYMBOL_BATTERY_3;
    } else if (level >= 28) {
        baseSymbol = LV_SYMBOL_BATTERY_2;
    } else if (level >= 8) {
        baseSymbol = LV_SYMBOL_BATTERY_1;
    }

    if (!isCharging) {
        return baseSymbol;
    }

    // Show both fill level and charging state when plugged in. (now only charging symbol with percentage)
    static char chargingSymbol[24];
    snprintf(chargingSymbol, sizeof(chargingSymbol), "%s", LV_SYMBOL_CHARGE);
    return chargingSymbol;
}

static void update_battery_icons_all_screens(int level, bool isCharging)
{
    const int valueLabelX = isCharging ? -25 : -40;

    lv_obj_t *batteryTitleLabels[] = {
        ui_Batt, ui_Batt1, ui_Batt2, ui_Batt3, ui_Batt4,
        ui_Batt5, ui_Batt6, ui_Batt7, ui_Batt8, ui_Batt9,
        APModeGetBatteryTitleLabel()
    };
    lv_obj_t *batteryValueLabels[] = {
        ui_BattValue, ui_BattValue1, ui_BattValue2, ui_BattValue3, ui_BattValue4,
        ui_BattValue5, ui_BattValue6, ui_BattValue7, ui_BattValue8, ui_BattValue9,
        APModeGetBatteryValueLabel()
    };
    lv_obj_t *batteryBars[] = {
        ui_Battery, ui_Battery1, ui_Battery2, ui_Battery3, ui_Battery4,
        ui_Battery5, ui_Battery6, ui_Battery7, ui_Battery8, ui_Battery9,
        APModeGetBatteryBar()
    };

    for (lv_obj_t *label : batteryValueLabels) {
        if (label != nullptr) {
            lv_obj_clear_flag(label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_align(label, LV_ALIGN_RIGHT_MID);
            lv_obj_set_y(label, 0);
            lv_obj_set_style_text_color(label, lv_color_hex(getActiveTextPrimaryColor()), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(label, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
    for (lv_obj_t *bar : batteryBars) {
        if (bar != nullptr) {
            lv_obj_add_flag(bar, LV_OBJ_FLAG_HIDDEN);
        }
    }
    for (lv_obj_t *label : batteryTitleLabels) {
        if (label != nullptr) {
            lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(label, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }

    const char *symbol = battery_symbol_for_level(level, isCharging);
    //shows charging icon right from battery icon, but not the percentage
    char percentText[8];
    snprintf(percentText, sizeof(percentText), "%d%%", level);

    for (lv_obj_t *label : batteryTitleLabels) {
        if (label != nullptr) lv_label_set_text(label, symbol);
    }
    for (lv_obj_t *label : batteryValueLabels) {
        if (label != nullptr) {
            lv_obj_set_x(label, valueLabelX);
            lv_label_set_text(label, percentText);
        }
    }
}

// -------------------------------------------------------
// Battery Sampling / Smoothing Helpers
// -------------------------------------------------------
static bool detectChargingNow()
{
    auto chargingState = M5.Power.isCharging();
    if (chargingState == m5::Power_Class::is_charging) return true;
    if (chargingState == m5::Power_Class::is_discharging) return false;
    // Fallback for unsupported/unknown PMIC charging state.
    return M5.Power.getBatteryCurrent() > 15;
}

static bool getStableChargingState()
{
    static bool initialized  = false;
    static bool rawState     = false;
    static bool stableState  = false;
    static uint32_t rawSinceMs  = 0;
    static uint32_t lastPollMs  = 0;

    const uint32_t nowMs = millis();

    // Only poll I2C every 250 ms to avoid unnecessary I2C traffic on the
    // power bus, which can couple noise into the input-only GPIO group.
    if (!initialized || (nowMs - lastPollMs) >= 250UL) {
        lastPollMs = nowMs;
    } else {
        return stableState;
    }

    const bool nowRaw = detectChargingNow();

    if (!initialized) {
        initialized  = true;
        rawState     = nowRaw;
        stableState  = nowRaw;
        rawSinceMs   = nowMs;
        return stableState;
    }

    if (nowRaw != rawState) {
        rawState   = nowRaw;
        rawSinceMs = nowMs;
    }

    if (stableState != rawState && (nowMs - rawSinceMs) >= 800U) {
        stableState = rawState;
    }

    return stableState;
}

static void maybeShowChargingWarning(bool isCharging)
{
    return; // Disable warning for now since the PMIC behavior seems stable and the warning can be confusing if it shows up due to a single noisy reading.
    
    static bool shownForCurrentChargeSession = false;

    if (!isCharging) {
        shownForCurrentChargeSession = false;
        return;
    }
    if (shownForCurrentChargeSession) return;

    shownForCurrentChargeSession = true;
    showNotification(
        T_CHARGING_WARNING_TITLE,
        T_CHARGING_WARNING_TEXT,
        5000,
        false, nullptr,
        false, nullptr,
        false);
}

static int estimateBatteryPercentFromVoltageMv(float batteryMv)
{

    // 1. Get raw voltage and charging state from Core2 AXP192
    bool isCharging = detectChargingNow();
    //return batteryMv;
    // 2. Corrected Hardware Offset Calibration
    if (!isCharging) {
        // When running on battery, load drags voltage down. 
        // Adding 60mV brings it back to the true chemical state.
        batteryMv += 60.0f; 
    } else {
        // When charging, voltage reads high. Reduce slightly to match real capacity.
        batteryMv -= 40.0f;
    }

    // Non-linear Li-ion OCV-inspired mapping (mV -> percent), then interpolate.
    // This avoids the "too optimistic" mid-range values from linear mapping.
    struct BatteryCurvePoint {
        float mv;
        int pct;
    };
    // 3. Your optimized curve for Core2 hardware
    static const BatteryCurvePoint curve[] = {
        {3400.0f, 0}, {3500.0f, 5}, {3600.0f, 15}, {3650.0f, 30}, {3700.0f, 50},
        {3750.0f, 65}, {3800.0f, 80}, {3950.0f, 95}, {4100.0f, 100}
    };

    const int n = (int)(sizeof(curve) / sizeof(curve[0]));
    if (batteryMv <= curve[0].mv) return curve[0].pct;
    if (batteryMv >= curve[n - 1].mv) return curve[n - 1].pct;

    for (int i = 0; i < n - 1; ++i) {
        const BatteryCurvePoint &a = curve[i];
        const BatteryCurvePoint &b = curve[i + 1];
        if (batteryMv >= a.mv && batteryMv <= b.mv) {
            const float t = (batteryMv - a.mv) / (b.mv - a.mv);
            int pct = (int)(a.pct + t * (float)(b.pct - a.pct) + 0.5f);
            return (pct < 0) ? 0 : (pct > 100) ? 100 : pct;
        }
    }
    return 0;
}

static int readBatteryPercentForUi(bool isCharging)
{
    const float battMv = M5.Power.getBatteryVoltage();
    return estimateBatteryPercentFromVoltageMv(battMv); //temporary change to check if percentage is handled better now
    if (!isCharging) {
        if (battMv > 1000.0f) {
            return estimateBatteryPercentFromVoltageMv(battMv);
        }
    }
    LogDebugFormatted("Battery voltage too low or charging, using M5.Power.getBatteryLevel() instead\n");
    return M5.Power.getBatteryLevel();
}

static int getSmoothedBatteryLevel(bool isCharging)
{
    static uint32_t lastSampleMs   = 0;
    static bool     wasCharging    = false;
    static uint32_t disconnectedMs = 0;
    static float    emaLevel       = -1.0f;
    static int      displayedLevel = -1;

    const uint32_t now = millis();

    if (wasCharging && !isCharging) disconnectedMs = now;
    wasCharging = isCharging;

    const bool inSettlingWindow = (!isCharging && (now - disconnectedMs) < 120000UL);

    if (emaLevel < 0.0f) {
        emaLevel       = (float)readBatteryPercentForUi(isCharging);
        displayedLevel = (int)(emaLevel + 0.5f);
        lastSampleMs   = now;
        if (!isCharging) disconnectedMs = now;
    }

    if (inSettlingWindow) return displayedLevel;

    if (now - lastSampleMs >= 10000UL || lastSampleMs == 0) {
        lastSampleMs = now;
        const float raw = (float)readBatteryPercentForUi(isCharging);
        emaLevel       = 0.1f * raw + 0.9f * emaLevel;
        displayedLevel = (int)(emaLevel + 0.5f);
        if (displayedLevel < 0)   displayedLevel = 0;
        if (displayedLevel > 100) displayedLevel = 100;
    }

    return displayedLevel;
}

// -------------------------------------------------------
// Screen Power Tick
// -------------------------------------------------------
void screen_power_tick()
{
    // Only treat encoder movement as activity when the counts change since
    // the last tick. Some encoder implementations leave a non-zero count
    // value until explicitly cleared which would otherwise constantly
    // retrigger the screensaver activity check.
    static long s_prev_encoder_counts[4] = {0, 0, 0, 0};
    long c1 = encoder1.getCount();
    long c2 = encoder2.getCount();
    long c3 = encoder3.getCount();
    long c4 = encoder4.getCount();
    if (c1 != s_prev_encoder_counts[0] || c2 != s_prev_encoder_counts[1] ||
        c3 != s_prev_encoder_counts[2] || c4 != s_prev_encoder_counts[3]) {
        screensaver_check_activity();
        s_prev_encoder_counts[0] = c1;
        s_prev_encoder_counts[1] = c2;
        s_prev_encoder_counts[2] = c3;
        s_prev_encoder_counts[3] = c4;
    }

    if (!screensaver_active && (millis() - last_activity_ms > (unsigned long)screensaver_timeout_ms)) {
        screensaver_prev_brightness = g_brightness_value;
        M5.Lcd.setBrightness(screensaver_dim_brightness);
        screensaver_active = true;
    }

#if AUTO_IDLE_DEEP_SLEEP_ENABLED == 1
    if (millis() - last_activity_ms > deep_sleep_timeout_ms) {
        if (canEnterDeepSleep()) {
            vibrate(1000, 255);
                const int result = showNotification(
                T_SHUTDOWN_SLEEP_TITLE,
                T_SHUTDOWN_SLEEP_TEXT,
            60000,
            true,  T_CANCEL,
            false,  nullptr,
            false);

            if (result != NOTIFICATION_RESULT_LEFT) {
                M5.Power.powerOff();
            }
        }
    }
#endif
}

// -------------------------------------------------------
// Settings Menu Actions
// -------------------------------------------------------
extern "C" void menuSleepAction(void)
{
    const int result = showNotification(
        "Enter Deep-Sleep",
        "Are you sure you want to enter deep-sleep mode? This will stop all connections.",
        0,
        true,  "Yes",
        true,  "No",
        false);

    if (result == NOTIFICATION_RESULT_LEFT) {
        enterDeepSleep();
    }
}

extern "C" void menuRestartAction(void)
{
    const int result = showNotification(
        "Restart",
        "Are you sure you want to perform a restart?",
        0,
        true,  "Yes",
        true,  "No",
        false);

    if (result == NOTIFICATION_RESULT_LEFT) {
        esp_restart();
    }
}

// -------------------------------------------------------
// Motion / Manual-Rail Helper Utilities
// -------------------------------------------------------
static void syncMotionCommandCache(float motionSpeed, float motionDepth, float motionStroke)
{
    s_last_motion_speed = motionSpeed;
    s_last_motion_depth = motionDepth;
    s_last_motion_stroke = motionStroke;
    s_motion_command_cache_valid = true;
}

static void persistManualRailLengthSetting()
{
    Preferences prefs;
    prefs.begin("m5-ctnr", false);
    prefs.putFloat("RailLengthMm", s_manual_rail_length_mm);
    prefs.end();
}

static void persistEncRampProfileSetting()
{
    Preferences prefs;
    prefs.begin("m5-ctnr", false);
    prefs.putInt("EncRampProfile", s_encoder_ramp_profile);
    prefs.end();
}

static void persistSpeedBehaviorSetting()
{
    Preferences prefs;
    prefs.begin("m5-ctnr", false);
    prefs.putInt("SpeedBehavior", s_speed_behavior_profile);
    prefs.putBool("VisualSpeedLock", s_speed_behavior_profile != SPEED_BEHAVIOR_STANDARD);
    prefs.end();
}

static float getDefaultManualRailLengthMm()
{
#ifdef RAIL_LENGTH
    return (float)RAIL_LENGTH;
#else
    return 0.0f;
#endif
}

static void updateManualRailLengthSettingLabel()
{
    if (!s_manual_rail_length_setting) return;

    char label[64];
    if (s_manual_rail_length_mm > 0.0f) {
        snprintf(label, sizeof(label), "Set Rail length : %.0f", s_manual_rail_length_mm);
    } else {
        snprintf(label, sizeof(label), "Set Rail length");
    }
    lv_checkbox_set_text(s_manual_rail_length_setting, label);
}

static void syncManualRailLengthSettingUi()
{
    if (!s_manual_rail_length_setting) return;

    s_manual_rail_length_ui_syncing = true;
    updateManualRailLengthSettingLabel();
    if (s_manual_rail_length_mm > 0.0f) {
        lv_obj_add_state(s_manual_rail_length_setting, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(s_manual_rail_length_setting, LV_STATE_CHECKED);
    }
    s_manual_rail_length_ui_syncing = false;
}

static const char* getEncRampProfileName(int profile)
{
    switch (profile) {
        case ENCODER_RAMP_NONE:
            return "None";
        case ENCODER_RAMP_HIGH:
            return "High";
        case ENCODER_RAMP_AGGRESSIVE:
            return "Aggressive";
        case ENCODER_RAMP_MEDIUM:
        default:
            return "Medium";
    }
}

static void syncEncRampProfileSettingUi()
{
    if (!s_encoder_ramp_profile_setting) return;
    char label[64];
    snprintf(label, sizeof(label), "Encoder ramp : %s", getEncRampProfileName(s_encoder_ramp_profile));
    lv_checkbox_set_text(s_encoder_ramp_profile_setting, label);
    lv_obj_add_state(s_encoder_ramp_profile_setting, LV_STATE_CHECKED);
}

static const char* getSpeedBehaviorName(int profile)
{
    switch (profile) {
        case SPEED_BEHAVIOR_NATURAL:
            return "Natural";
        case SPEED_BEHAVIOR_TAMED:
            return "Tamed";
        case SPEED_BEHAVIOR_STANDARD:
        default:
            return "Standard";
    }
}

static void applySpeedBehavior(int profile)
{
    if (profile < SPEED_BEHAVIOR_STANDARD || profile > SPEED_BEHAVIOR_TAMED) {
        profile = SPEED_BEHAVIOR_STANDARD;
    }
    s_speed_behavior_profile = profile;
    s_visual_speed_lock = (s_speed_behavior_profile != SPEED_BEHAVIOR_STANDARD);

    if (!s_visual_speed_lock) {
        resetVisualSpeedRatioState();
    } else if (stroke > 0.001f) {
        s_visual_speed_stroke_product = speed;
        s_visual_speed_ratio_valid = true;
    }
}

static void syncSpeedBehaviorSettingUi()
{
    if (!ui_visualSpeedLock) return;

    s_speed_behavior_ui_syncing = true;
    char label[64];
    snprintf(label, sizeof(label), "Speed behaviour : %s", getSpeedBehaviorName(s_speed_behavior_profile));
    lv_checkbox_set_text(ui_visualSpeedLock, label);

    if (s_speed_behavior_profile == SPEED_BEHAVIOR_STANDARD) {
        lv_obj_clear_state(ui_visualSpeedLock, LV_STATE_CHECKED);
    } else {
        lv_obj_add_state(ui_visualSpeedLock, LV_STATE_CHECKED);
    }
    s_speed_behavior_ui_syncing = false;
}

static void ensureManualRailLengthSetting()
{
    if (!ui_Settings || s_manual_rail_length_setting) return;

    s_manual_rail_length_setting = lv_checkbox_create(ui_Settings);
    lv_checkbox_set_text(s_manual_rail_length_setting, "Set manual Rail length");
    lv_obj_set_width(s_manual_rail_length_setting, LV_SIZE_CONTENT);
    lv_obj_set_height(s_manual_rail_length_setting, LV_SIZE_CONTENT);
    lv_obj_set_x(s_manual_rail_length_setting, 10);
    lv_obj_set_y(s_manual_rail_length_setting, 120);
    lv_obj_set_align(s_manual_rail_length_setting, LV_ALIGN_LEFT_MID);
    lv_obj_add_flag(s_manual_rail_length_setting, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_set_style_text_font(s_manual_rail_length_setting, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
    uiApplyCheckboxStyles(s_manual_rail_length_setting);
    lv_obj_add_event_cb(s_manual_rail_length_setting, manualRailLengthSetting_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    syncManualRailLengthSettingUi();
}

static void ensureEncRampProfileSetting()
{
    if (!ui_Settings || s_encoder_ramp_profile_setting) return;

    s_encoder_ramp_profile_setting = lv_checkbox_create(ui_Settings);
    lv_checkbox_set_text(s_encoder_ramp_profile_setting, "Encoder ramp : Medium");
    lv_obj_set_width(s_encoder_ramp_profile_setting, LV_SIZE_CONTENT);
    lv_obj_set_height(s_encoder_ramp_profile_setting, LV_SIZE_CONTENT);
    lv_obj_set_x(s_encoder_ramp_profile_setting, 10);
    lv_obj_set_y(s_encoder_ramp_profile_setting, 150);
    lv_obj_set_align(s_encoder_ramp_profile_setting, LV_ALIGN_LEFT_MID);
    lv_obj_add_flag(s_encoder_ramp_profile_setting, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    
    lv_obj_set_style_text_font(s_encoder_ramp_profile_setting, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
    uiApplyCheckboxStyles(s_encoder_ramp_profile_setting);
    lv_obj_add_event_cb(s_encoder_ramp_profile_setting, EncRampProfile_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    syncEncRampProfileSettingUi();
}

static void manualRailLengthSetting_event_cb(lv_event_t* e)
{
    if (!e || s_manual_rail_length_ui_syncing) return;
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED || !s_manual_rail_length_setting) return;

    const bool checked = lv_obj_has_state(s_manual_rail_length_setting, LV_STATE_CHECKED);
    if (!checked) {
        s_manual_rail_length_mm = 0.0f;
        persistManualRailLengthSetting();
        syncManualRailLengthSettingUi();
        refreshSettingsCarousel();
        lv_refr_now(NULL);
        return;
    }

    runManualRailLengthCalibrationWorkflow();
}

static void EncRampProfile_event_cb(lv_event_t* e)
{
    if (!e || !s_encoder_ramp_profile_setting) return;

    const lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_VALUE_CHANGED) return;

    s_encoder_ramp_profile = (s_encoder_ramp_profile + 1) % 4;
    persistEncRampProfileSetting();
    syncEncRampProfileSettingUi();
    refreshSettingsCarousel();
    lv_refr_now(NULL);
}

static void SpeedBehavior_event_cb(lv_event_t* e)
{
    if (!e || !ui_visualSpeedLock || s_speed_behavior_ui_syncing) return;

    const lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_VALUE_CHANGED) return;

    int nextProfile = s_speed_behavior_profile + 1;
    if (nextProfile > SPEED_BEHAVIOR_TAMED) {
        nextProfile = SPEED_BEHAVIOR_STANDARD;
    }

    applySpeedBehavior(nextProfile);
    syncSpeedBehaviorSettingUi();
    persistSpeedBehaviorSetting();
    refreshSettingsCarousel();
    lv_refr_now(NULL);
}

static void runManualRailLengthCalibrationWorkflow()
{
    if (!bleCommIsConnected() && !bleCommTryConnect()) {
        showNotification("Moving to MAX", "BLE is not connected, so calibration cannot start.", 0, true, "OK", false, nullptr, false);
        syncManualRailLengthSettingUi();
        refreshSettingsCarousel();
        lv_refr_now(NULL);
        return;
    }

    while (true) {
        const int startResult = showNotification(
            "Moving to MAX",
            "The OSSM will now move to max depth and store this as a setting. Move away from your OSSM and press start",
            0,
            true,  "Cancel",
            true,  "Start",
            false);

        if (startResult == NOTIFICATION_RESULT_LEFT) {
            syncManualRailLengthSettingUi();
            refreshSettingsCarousel();
            lv_refr_now(NULL);
            return;
        }
        if (startResult != NOTIFICATION_RESULT_RIGHT) {
            syncManualRailLengthSettingUi();
            refreshSettingsCarousel();
            lv_refr_now(NULL);
            return;
        }

        SendCommand(DEPTH, 100.0f, OSSM_ID);
        SendCommand(STROKE, 1.0f, OSSM_ID);
        SendCommand(SPEED, 5.0f, OSSM_ID);
        SendCommand(SENSATION, 0.0f, OSSM_ID);
        SendCommand(ON, 5.0f, OSSM_ID);

        const uint32_t moveStartMs = millis();
        while ((millis() - moveStartMs) < 10000U) {
            M5.update();
            lv_task_handler();
            vTaskDelay(pdMS_TO_TICKS(20));
        }

        SendCommand(OFF, 0.0f, OSSM_ID);
        vTaskDelay(pdMS_TO_TICKS(250));

        const int confirmResult = showNotification(
            "Confirm max depth",
            "Confirm if your OSSM is indeed at the maximum position",
            0,
            true,  "Confirm",
            true,  "Retry",
            false);

        if (confirmResult == NOTIFICATION_RESULT_RIGHT) {
            continue;
        }

        if (confirmResult == NOTIFICATION_RESULT_LEFT) {
            const float confirmedPosition = bleCommGetConfirmedPosition();
            if (confirmedPosition > 0.0f) {
                s_manual_rail_length_mm = confirmedPosition;
                persistManualRailLengthSetting();
                syncManualRailLengthSettingUi();
                refreshSettingsCarousel();
                lv_refr_now(NULL);
            }
            return;
        }

        return;
    }
}

static void resetVisualSpeedRatioState()
{
    s_visual_speed_ratio_valid = false;
    s_visual_speed_stroke_product = 0.0f;
    s_visual_speed_last_commanded = -1.0f;
}

static void updateVisualSpeedRatioFromUi(bool uiSpeedChanged, float uiSpeed, float uiStroke)
{
    if (!s_visual_speed_lock) {
        resetVisualSpeedRatioState();
        return;
    }

    if (uiStroke <= 0.001f) return;

    if (uiSpeedChanged || !s_visual_speed_ratio_valid) {
        s_visual_speed_stroke_product = uiSpeed;
        s_visual_speed_ratio_valid = true;
    }
}

static float resolveVisualCompensatedSpeed(float uiSpeed, float uiStroke)
{
    if (!s_visual_speed_lock) return uiSpeed;
    if (!s_visual_speed_ratio_valid) return uiSpeed;
    if (uiStroke <= 0.001f) return uiSpeed;

    float minFactor = VIS_SPEED_STANDARD_MIN_FACTOR;
    float kneeStroke = VIS_SPEED_STANDARD_KNEE_STROKE;
    float curvePower = VIS_SPEED_STANDARD_POWER;
    if (s_speed_behavior_profile == SPEED_BEHAVIOR_NATURAL) {
        minFactor = VIS_SPEED_NATURAL_MIN_FACTOR;
        kneeStroke = VIS_SPEED_NATURAL_KNEE_STROKE;
        curvePower = VIS_SPEED_NATURAL_POWER;
    } else if (s_speed_behavior_profile == SPEED_BEHAVIOR_TAMED) {
        minFactor = VIS_SPEED_TAMED_MIN_FACTOR;
        kneeStroke = VIS_SPEED_TAMED_KNEE_STROKE;
        curvePower = VIS_SPEED_TAMED_POWER;
    }

    const float strokeNorm = fminf(1.0f, fmaxf(0.0f, uiStroke / kneeStroke));
    const float factor = minFactor + (1.0f - minFactor) * powf(strokeNorm, curvePower);
    float compensatedSpeed = s_visual_speed_stroke_product * factor;
    if (compensatedSpeed < 0.0f) compensatedSpeed = 0.0f;
    if (compensatedSpeed > speedlimit) compensatedSpeed = speedlimit;
    return compensatedSpeed;
}

static int collectSettingsOptionObjects(lv_obj_t** outObjects, int maxObjects)
{
    if (!outObjects || maxObjects <= 0) return 0;

    ensureEncRampProfileSetting();

    int count = 0;
    auto addObj = [&](lv_obj_t* obj) {
        if (!obj || count >= maxObjects) return;
        outObjects[count++] = obj;
    };

    addObj(ui_vibrate);
    addObj(ui_safeStartStop);
    addObj(ui_strokeinvert);
    addObj(ui_forceHome);
    addObj(ui_visualSpeedLock);
    addObj(ui_strokeDepthLink);
    addObj(s_encoder_ramp_profile_setting);
    return count;
}

static void refreshSettingsCarousel()
{
    lv_obj_t* options[9] = {};
    const int optionCount = collectSettingsOptionObjects(options, 9);
    if (optionCount <= 0) {
        if (ui_Logo1) {
            lv_label_set_text(ui_Logo1, T_SCREEN_SETTINGS);
        }
        return;
    }

    if (s_settings_focus_index < 0) s_settings_focus_index = 0;
    if (s_settings_focus_index >= optionCount) s_settings_focus_index = optionCount - 1;

    if (s_settings_focus_index < s_settings_scroll_offset) {
        s_settings_scroll_offset = s_settings_focus_index;
    }
    if (s_settings_focus_index >= (s_settings_scroll_offset + SETTINGS_CAROUSEL_VISIBLE_COUNT)) {
        s_settings_scroll_offset = s_settings_focus_index - SETTINGS_CAROUSEL_VISIBLE_COUNT + 1;
    }

    if (s_settings_scroll_offset < 0) s_settings_scroll_offset = 0;
    const int maxOffset = (optionCount > SETTINGS_CAROUSEL_VISIBLE_COUNT)
                          ? (optionCount - SETTINGS_CAROUSEL_VISIBLE_COUNT)
                          : 0;
    if (s_settings_scroll_offset > maxOffset) s_settings_scroll_offset = maxOffset;

    const int slotY[SETTINGS_CAROUSEL_VISIBLE_COUNT] = {-60, -30, 0, 30};
    for (int i = 0; i < optionCount; ++i) {
        lv_obj_t* obj = options[i];
        if (!obj) continue;

        const bool visible = (i >= s_settings_scroll_offset) &&
                             (i < (s_settings_scroll_offset + SETTINGS_CAROUSEL_VISIBLE_COUNT));
        if (visible) {
            const int slot = i - s_settings_scroll_offset;
            lv_obj_set_y(obj, slotY[slot]);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }

        if (i == s_settings_focus_index) {
            lv_obj_add_state(obj, LV_STATE_FOCUSED);
        } else {
            lv_obj_clear_state(obj, LV_STATE_FOCUSED);
        }
    }

    if (ui_Logo1) {
        char title[64];
        snprintf(title, sizeof(title), "%s %d/%d", T_SCREEN_SETTINGS, s_settings_focus_index + 1, optionCount);
        lv_label_set_text(ui_Logo1, title);
    }
}

static lv_obj_t* getSettingsFocusedObject()
{
    lv_obj_t* options[9] = {};
    const int optionCount = collectSettingsOptionObjects(options, 9);
    if (optionCount <= 0) return nullptr;

    if (s_settings_focus_index < 0) s_settings_focus_index = 0;
    if (s_settings_focus_index >= optionCount) s_settings_focus_index = optionCount - 1;
    return options[s_settings_focus_index];
}

// -------------------------------------------------------
// Screen Lifecycle / Event Entry Points
// -------------------------------------------------------
void screenInit() {
    bleCommResetPatternReadState();

    Preferences prefs;
    prefs.begin("m5-ctnr", false);
    eject_status = prefs.getBool("ejectAddon", false);
    vibrate_mode = prefs.getBool("Vibrate", true);
    SafeStartStop   = prefs.getBool("SafeStartStop", true);
    strokeinvert_mode = prefs.getBool("StrokeInvert", true);
    ble_force_homeing = prefs.getBool("BleForceHomeing", true);
    s_speed_behavior_profile = SPEED_BEHAVIOR_STANDARD;
    if (prefs.isKey("SpeedBehavior")) {
        s_speed_behavior_profile = prefs.getInt("SpeedBehavior", SPEED_BEHAVIOR_STANDARD);
    }
    applySpeedBehavior(s_speed_behavior_profile);
    s_stroke_influences_depth = prefs.getBool("DepthToStroke", false);
    s_encoder_ramp_profile = prefs.getInt("EncRampProfile", ENCODER_RAMP_MEDIUM);
    if (s_encoder_ramp_profile < ENCODER_RAMP_NONE || s_encoder_ramp_profile > ENCODER_RAMP_AGGRESSIVE) {
        s_encoder_ramp_profile = ENCODER_RAMP_MEDIUM;
    }
    s_manual_rail_length_mm = getDefaultManualRailLengthMm();
    if (prefs.isKey("RailLengthMm")) {
        s_manual_rail_length_mm = prefs.getFloat("RailLengthMm", s_manual_rail_length_mm);
    }
    int brightness = prefs.getInt("Brightness", 180);
    if (brightness < 5) brightness = 5;
    if (brightness > 255) brightness = 255;
    prefs.end();

    g_brightness_value = brightness;
    last_activity_ms = millis();
    M5.Display.setBrightness(brightness);
    M5.Lcd.setBrightness(brightness);

    if (eject_status) {
        lv_obj_add_state(ui_ejectaddon,    LV_STATE_CHECKED);
        lv_obj_clear_state(ui_HomeButtonL, LV_STATE_DISABLED);
    }
    if (vibrate_mode) { lv_obj_add_state(ui_vibrate,  LV_STATE_CHECKED); }
    if (SafeStartStop)   { lv_obj_add_state(ui_safeStartStop,    LV_STATE_CHECKED); }
    if (strokeinvert_mode && ui_strokeinvert) { lv_obj_add_state(ui_strokeinvert, LV_STATE_CHECKED); }
    if (ble_force_homeing && ui_forceHome)    { lv_obj_add_state(ui_forceHome, LV_STATE_CHECKED); }
    if (ui_visualSpeedLock) {
        lv_obj_add_event_cb(ui_visualSpeedLock, SpeedBehavior_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
        syncSpeedBehaviorSettingUi();
    }
    if (s_stroke_influences_depth && ui_strokeDepthLink) { lv_obj_add_state(ui_strokeDepthLink, LV_STATE_CHECKED); }
    ensureEncRampProfileSetting();
    syncEncRampProfileSettingUi();
    if (ui_brightness_slider) {
        lv_slider_set_value(ui_brightness_slider, brightness, LV_ANIM_OFF);
    }

    lv_roller_set_selected(ui_PatternS, 2, LV_ANIM_OFF);
    lv_roller_get_selected_str(ui_PatternS, patternstr, 0);
    lv_label_set_text(ui_HomePatternLabel, patternstr);

    colors_init();

    // Initialise the battery icon display for all screens immediately so that
    // the very first lv_task_handler() render (which runs before handleScreens()
    // in loop()) already shows the icon + percentage instead of the old bar.
    {
        const bool c = detectChargingNow();
        update_battery_icons_all_screens(readBatteryPercentForUi(c), c);
    }
}

void brightness_slider_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED || !ui_brightness_slider) return;

    int val = lv_slider_get_value(ui_brightness_slider);
    if (val < 5) val = 5;
    if (val > 255) val = 255;

    g_brightness_value = val;
    M5.Display.setBrightness(val);
    M5.Lcd.setBrightness(val);

    Preferences pref;
    pref.begin("m5-ctnr", false);
    pref.putInt("Brightness", val);
    pref.end();
}

// -------------------------------------------------------
// Screen event callbacks (registered via ui.c / ui.h)
// -------------------------------------------------------

void screenmachine(lv_event_t * e) {
    // Clear any lingering LVGL touch-press tracking from the previous screen.
    // Without this, a button tap that causes a screen transition can leak into
    // the new screen if a button there occupies the same pixel coordinates
    // (e.g. PatternButtonM and HomeButtonM are both at lv_pct(0) / y=100).
    lv_indev_t *_indev = lv_indev_get_next(NULL);
    while (_indev) {
        lv_indev_reset(_indev, NULL);
        _indev = lv_indev_get_next(_indev);
    }

    if (lv_scr_act() == ui_Start) {
        st_screens = ST_UI_START;
    } else if (lv_scr_act() == ui_Home) {
        if (st_screens != ST_UI_HOME) {
            resetEncoderCounts();
        }
        st_screens = ST_UI_HOME;
        syncHomeSliderRangesToLimits();
        syncHomeSensationSliderToTransport();
        speed = lv_slider_get_value(ui_homespeedslider);
        // If this is the very first time Home is opened while connected,
        // send Pattern 2 (Simple Stroke) to OSSM and update UI labels.
        if (!s_initial_pattern_sent) {
            if (bleCommIsConnected() || espNowIsPaired()) {
                s_initial_pattern_sent = true;
                // Update roller/labels to reflect pattern 2 if UI objects exist
                if (ui_PatternS) {
                    lv_roller_set_selected(ui_PatternS, 2, LV_ANIM_OFF);
                    lv_roller_get_selected_str(ui_PatternS, patternstr, sizeof(patternstr));
                    if (ui_HomePatternLabel) lv_label_set_text(ui_HomePatternLabel, patternstr);
                    if (ui_StrokePatternLabel) lv_label_set_text(ui_StrokePatternLabel, patternstr);
                }
                // Send pattern index 2 to OSSM
                SendCommand(PATTERN, 2.0f, OSSM_ID);
                SendCommand(SENSATION, 0.0f, OSSM_ID);
            }
        }
        //LogDebug(speedenc);
        //LogDebug(speed);
    } else if (lv_scr_act() == ui_Menu) {

        // requestMenuEntryAction equivalent (mirrors backup firmware logic).
        // At this point st_screens is STILL the PREVIOUS screen value.
        {
            const bool cameFromHome = (st_screens == ST_UI_HOME || st_screens == ST_UI_STROKE || st_screens == ST_UI_PATTERN);
            const bool cameFromStreaming = (st_screens == ST_UI_STREAMING);
            if (!bleCommIsConnected()) {
                s_ble_menu_requires_stroke_reentry = false;
            } else if (cameFromHome) {
                // home/stroke → menu: OSSM stays in strokeEngine; suppress re-home on return.
                s_ble_menu_requires_stroke_reentry = false;
            } else if (cameFromStreaming) {
                // streaming → menu: go:menu was already queued by SCREEN_UNLOAD_START;
                // arm the flag so home screen re-homes after the mode change.
                s_ble_menu_requires_stroke_reentry = true;
            } else if (ble_force_homeing) {
                // Force-homing enabled: navigated from a non-strokeEngine screen (settings, etc.)
                bleCommGoToMenu();
                s_ble_menu_requires_stroke_reentry = true;
            } else {
                s_ble_menu_requires_stroke_reentry = false;
            }
        }
        if (st_screens != ST_UI_MENU) {
            resetEncoderCounts();
        }

        st_screens = ST_UI_MENU;
    } else if (lv_scr_act() == ui_Pattern) {
        if (st_screens != ST_UI_PATTERN) {
            resetEncoderCounts();
        }
        st_screens = ST_UI_PATTERN;

        if (commIsBleMode()) {
            if (readPatternsFromOSSM() && newPatternIsReadFromOSSM && patternString.length() > 0) {
                lv_roller_set_options(ui_PatternS, patternString.c_str(), LV_ROLLER_MODE_NORMAL);
                uint16_t optionCount = (uint16_t)lv_roller_get_option_count(ui_PatternS);
                if (optionCount > 0) {
                    if (pattern < 0 || pattern >= (int)optionCount) {
                        pattern = 0;
                    }
                    lv_roller_set_selected(ui_PatternS, pattern, LV_ANIM_OFF);
                }
                newPatternIsReadFromOSSM = false;
            }
        } else {
            // Keep ESP-NOW flow on default UI-defined patterns.
            newPatternIsReadFromOSSM = false;
        }
    } else if (lv_scr_act() == ui_Torqe) {
//        if (st_screens != ST_UI_Torqe) {
//            resetEncoderCounts();
//        }

        st_screens = ST_UI_Torqe;
        torqe_f = lv_slider_get_value(ui_outtroqeslider);
        torqe_f_enc = fscale(50, 200, 0, Encoder_MAP, torqe_f, 0);
        encoder1.setCount(torqe_f_enc);
        torqe_r = lv_slider_get_value(ui_introqeslider);
        torqe_r_enc = fscale(20, 200, 0, Encoder_MAP, torqe_r, 0);
        encoder4.setCount(torqe_r_enc);
    } else if (lv_scr_act() == ui_EJECTSettings) {
//        if (st_screens != ST_UI_EJECTSETTINGS) {
//            resetEncoderCounts();
//        }
        st_screens = ST_UI_EJECTSETTINGS;
    } else if (lv_scr_act() == ui_Settings) {
        st_screens = ST_UI_SETTINGS;
        s_settings_focus_index = 0;
        s_settings_scroll_offset = 0;
        refreshSettingsCarousel();
    } else if (lv_scr_act() == ui_Stroke) {
        if (st_screens != ST_UI_STROKE) {
            resetEncoderCounts();
        }
        st_screens = ST_UI_STROKE;
        refreshStrokeStartStopUi();
    } else if (lv_scr_act() == ui_Colors) {
        st_screens = ST_UI_COLORS;
    } else if (lv_scr_act() == ui_Streaming) {
        if (st_screens != ST_UI_STREAMING) {
            resetEncoderCounts();
        }
        st_screens = ST_UI_STREAMING;
    } else if (lv_scr_act() == ui_Addons) {
//        if (st_screens != ST_UI_ADDONS) {
//            resetEncoderCounts();
//        }
        st_screens = ST_UI_ADDONS;
        addonsSyncSelectionVisual();
    } else if (lv_scr_act() == ui_FistIT) {
//        if (st_screens != ST_UI_FISTIT) {
//            resetEncoderCounts();
//        }
        st_screens = ST_UI_FISTIT;
    } else if (APModeOwnsActiveScreen()) {
//        if (st_screens != ST_UI_APMODE) {
//            resetEncoderCounts();
//        }
        st_screens = ST_UI_APMODE;
    }
}

// -------------------------------------------------------
// Settings Persistence / Action Callbacks
// -------------------------------------------------------
void savesettings(lv_event_t * e) {
    Preferences prefs;
    prefs.begin("m5-ctnr", false);
    if (lv_obj_has_state(ui_vibrate, LV_STATE_CHECKED) == 1) {
        prefs.putBool("Vibrate", true);
    } else {
        prefs.putBool("Vibrate", false);
    }

    if (lv_obj_has_state(ui_safeStartStop, LV_STATE_CHECKED) == 1) {
        prefs.putBool("SafeStartStop", true);
    } else {
        prefs.putBool("SafeStartStop", false);
    }

    if (ui_strokeinvert && lv_obj_has_state(ui_strokeinvert, LV_STATE_CHECKED) == 1) {
        prefs.putBool("StrokeInvert", true);
        strokeinvert_mode = true;
    } else {
        prefs.putBool("StrokeInvert", false);
        strokeinvert_mode = false;
    }

    if (ui_forceHome && lv_obj_has_state(ui_forceHome, LV_STATE_CHECKED) == 1) {
        prefs.putBool("BleForceHomeing", true);
        ble_force_homeing = true;
    } else {
        prefs.putBool("BleForceHomeing", false);
        ble_force_homeing = false;
    }

    prefs.putInt("SpeedBehavior", s_speed_behavior_profile);
    prefs.putBool("VisualSpeedLock", s_speed_behavior_profile != SPEED_BEHAVIOR_STANDARD);

    if (ui_strokeDepthLink && lv_obj_has_state(ui_strokeDepthLink, LV_STATE_CHECKED) == 1) {
        prefs.putBool("DepthToStroke", true);
        s_stroke_influences_depth = true;
    } else {
        prefs.putBool("DepthToStroke", false);
        s_stroke_influences_depth = false;
    }

    prefs.putInt("EncRampProfile", s_encoder_ramp_profile);

    prefs.putFloat("RailLengthMm", s_manual_rail_length_mm);

    if (ui_brightness_slider) {
        int brightness = lv_slider_get_value(ui_brightness_slider);
        if (brightness < 5) brightness = 5;
        if (brightness > 255) brightness = 255;
        prefs.putInt("Brightness", brightness);
        M5.Display.setBrightness(brightness);
        M5.Lcd.setBrightness(brightness);
    }

    prefs.end();
    delay(100);
    vibrate(225, 75);
}

void pullOut(lv_event_t * e) {
    if (speed > 20) {
        speed = 20;
        SendCommand(SPEED, speed, OSSM_ID);
    }
    int speed_time = (5000*(20/speed));
    SendCommand(DEPTH, 0, OSSM_ID);
    SendCommand(STROKE, 0.1, OSSM_ID); // set a tiny stroke to ensure we exit the stroke pattern if active
    speed = 0;
    stroke = 0;
    depth = 0;
    lv_slider_set_value(ui_homespeedslider, speed, LV_ANIM_OFF);
    lv_slider_set_value(ui_homestrokeslider, stroke, LV_ANIM_OFF);
    lv_slider_set_value(ui_homedepthslider, depth, LV_ANIM_OFF);
    //std::string speedStr = std::to_string(speed);
    lv_label_set_text(ui_homespeedvalue, "0");
    //std::string strokeStr = std::to_string(stroke);
    lv_label_set_text(ui_homestrokevalue, "0");
    //std::string depthStr = std::to_string(depth);
    lv_label_set_text(ui_homedepthvalue, "0");
    showNotification(T_PULLING_OUT, T_PULLING_OUT_TEXT, speed_time, false, nullptr, false, nullptr, false);
    lv_refr_now(NULL);  // force immediate render — lv_task_handler() is re-entrant-blocked inside an event callback
    SendCommand(SPEED, 0, OSSM_ID);
    SendCommand(STROKE, 0, OSSM_ID);
    screenmachine(e);
}

void emergencyStop(lv_event_t * e) {
    pullOut(e);
    EJECT_On = false;
    
    // turn off fist-it and send that command to Fist-IT, regardless of whatever screen we are in
    SendCommand(OFF, 0.0, FIST_ID);
    SendCommand(OFF, 0.0, OSSM_ID);
    SendCommand(OFF, 0.0, EJECT_ID);    
}

void ejectcreampie(lv_event_t * e) {
    if (EJECT_On == true) {
        lv_obj_clear_state(ui_HomeButtonL, LV_STATE_CHECKED);
        SendCommand(ON, 0.0, EJECT_ID);    
        EJECT_On = false;
    } else {
        lv_obj_clear_state(ui_HomeButtonL, LV_STATE_CHECKED);
//        depth  = 0;
//        speed  = 0;
//        stroke = 0;
//        SendCommand(SETUP_D_I, 0.0, OSSM_ID);
//        SendCommand(DEPTH, depth, OSSM_ID);
//        screenmachine(e);
        SendCommand(ON, 0.0, EJECT_ID);    
        EJECT_On = true;
    }
}

void toggleFistIT(lv_event_t * e) {
    if (EJECT_On == true) {
        lv_obj_clear_state(ui_HomeButtonR, LV_STATE_CHECKED);
        SendCommand(ON, 0.0, EJECT_ID);    
        EJECT_On = false;
    } else {
        lv_obj_clear_state(ui_HomeButtonR, LV_STATE_CHECKED);
        SendCommand(ON, 0.0, EJECT_ID);    
        EJECT_On = true;
    }
}

void savepattern(lv_event_t * e) {
    pattern = lv_roller_get_selected(ui_PatternS);
    lv_roller_get_selected_str(ui_PatternS, patternstr, 0);
    lv_label_set_text(ui_HomePatternLabel, patternstr);
    if (ui_StrokePatternLabel) lv_label_set_text(ui_StrokePatternLabel, patternstr);
    
    //LogDebug(pattern);
    float patterns = pattern;
    SendCommand(PATTERN, patterns, OSSM_ID);
    SendCommand(SENSATION, 0.0, OSSM_ID);
    lv_slider_get_value(ui_homesensationslider);
    lv_slider_set_value(ui_homesensationslider, 0, LV_ANIM_OFF);
    sensation = 0;
}

static void applyHomeButtonMState(const char* text, lv_style_t* defaultStyle, lv_style_t* pressedStyle, bool forceFocused = false) {
    if (!ui_HomeButtonM || !ui_HomeButtonMText) return;

    lv_obj_remove_style(ui_HomeButtonM, &style_button_m, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_remove_style(ui_HomeButtonM, &style_button_m_pressed, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_remove_style(ui_HomeButtonM, &style_button_running, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_remove_style(ui_HomeButtonM, &style_button_running_pressed, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_remove_style(ui_HomeButtonM, &style_button_stopped, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_remove_style(ui_HomeButtonM, &style_button_stopped_pressed, LV_PART_MAIN | LV_STATE_PRESSED);

    lv_obj_add_style(ui_HomeButtonM, defaultStyle, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_HomeButtonM, pressedStyle, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_add_style(ui_HomeButtonM, &style_button_m_focused, LV_PART_MAIN | LV_STATE_FOCUSED);

    lv_label_set_text(ui_HomeButtonMText, text);
    lv_obj_refresh_style(ui_HomeButtonM, LV_PART_MAIN, LV_STYLE_PROP_ANY);
    lv_obj_invalidate(ui_HomeButtonM);
}

static void updateHomeButtonMState() {
    bool isMoving = speed > 0 and stroke > 0 and depth > 0 and OSSM_On;
    
    if (isMoving) {
        applyHomeButtonMState(T_STOP, &style_button_running, &style_button_running_pressed);
    } else {
        applyHomeButtonMState(T_RESUME, &style_button_stopped, &style_button_stopped_pressed);
    }
}


static void syncHomeMotionUi(bool invertStroke)
{
    if (ui_homedepthslider) {
        lv_slider_set_value(ui_homedepthslider, depth, LV_ANIM_OFF);
    }
    if (ui_homedepthvalue) {
        char depth_v[7];
        dtostrf(depth, 6, 0, depth_v);
        lv_label_set_text(ui_homedepthvalue, depth_v);
    }

    if (ui_homestrokeslider) {
        if (invertStroke) {
            if (lv_bar_get_mode(ui_homestrokeslider) != LV_BAR_MODE_RANGE) {
                lv_bar_set_mode(ui_homestrokeslider, LV_BAR_MODE_RANGE);
            }
            lv_bar_set_start_value(ui_homestrokeslider, depth - stroke, LV_ANIM_OFF);
            lv_slider_set_value(ui_homestrokeslider, depth, LV_ANIM_OFF);
        } else {
            if (lv_bar_get_mode(ui_homestrokeslider) != LV_BAR_MODE_NORMAL) {
                lv_bar_set_mode(ui_homestrokeslider, LV_BAR_MODE_NORMAL);
            }
            lv_bar_set_start_value(ui_homestrokeslider, 0, LV_ANIM_OFF);
            lv_slider_set_value(ui_homestrokeslider, stroke, LV_ANIM_OFF);
        }
    }
    if (ui_homestrokevalue) {
        char stroke_v[7];
        dtostrf(stroke, 6, 0, stroke_v);
        lv_label_set_text(ui_homestrokevalue, stroke_v);
    }
}

static void rampHomeStartSpeed(int targetSpeed)
{
    s_home_speed_ramp_active = true;
    s_home_speed_ramp_current = HOME_START_RAMP_THRESHOLD + 1;
    s_home_speed_ramp_target = targetSpeed;
    s_home_speed_ramp_step = 1;
    s_home_speed_ramp_interval_ms = HOME_START_RAMP_INTERVAL_MS;
    s_home_speed_ramp_next_ms = millis() + HOME_START_RAMP_INTERVAL_MS;
}

static void rampHomeStopSpeed(int startSpeed)
{
    s_home_speed_ramp_active = true;
    s_home_speed_ramp_current = startSpeed - 1;
    s_home_speed_ramp_target = HOME_START_RAMP_THRESHOLD;
    s_home_speed_ramp_step = -1;
    s_home_speed_ramp_interval_ms = HOME_START_RAMP_INTERVAL_MS / 2;
    s_home_speed_ramp_next_ms = millis() + s_home_speed_ramp_interval_ms;
}

static void serviceHomeSpeedRamp()
{
    if (!s_home_speed_ramp_active) return;

    const uint32_t now = millis();
    if (now < s_home_speed_ramp_next_ms) return;

    const bool reachedTarget =
        (s_home_speed_ramp_step > 0 && s_home_speed_ramp_current > s_home_speed_ramp_target) ||
        (s_home_speed_ramp_step < 0 && s_home_speed_ramp_current < s_home_speed_ramp_target);
    if (reachedTarget) {
        s_home_speed_ramp_active = false;
        return;
    }

    SendCommand(SPEED, (float)s_home_speed_ramp_current, OSSM_ID);
    s_home_speed_ramp_current += s_home_speed_ramp_step;
    s_home_speed_ramp_next_ms = now + s_home_speed_ramp_interval_ms;
}

void homebuttonmevent(lv_event_t * e) {
    //LogDebug("HomeButton");
        SafeStartStop = (lv_obj_has_state(ui_safeStartStop, LV_STATE_CHECKED) == 1);
    if (OSSM_On == false) {
        if (speed == 0 || stroke == 0 || depth == 0) return;
        applyHomeButtonMState(T_STOP, &style_button_running, &style_button_running_pressed);
        lv_refr_now(NULL);
        const float startCommandedSpeed = resolveVisualCompensatedSpeed(speed, stroke);
        const int targetSpeed = (int)(startCommandedSpeed + 0.5f);
        const int startSpeed = (targetSpeed > HOME_START_RAMP_THRESHOLD) ? HOME_START_RAMP_THRESHOLD : targetSpeed;
        LogDebugFormatted("Starting OSSM Safe start is active: %s\n Starting at startingspeed %d", SafeStartStop ? "true" : "false", startSpeed);
        if (SafeStartStop) {
            s_home_speed_ramp_active = (targetSpeed > HOME_START_RAMP_THRESHOLD);
            //SendCommand(SPEED, 1, OSSM_ID);
            SendCommand(ON, (float)startSpeed, OSSM_ID);
            //OSSM_On = true;
            if (targetSpeed > HOME_START_RAMP_THRESHOLD) {
                rampHomeStartSpeed(targetSpeed);
            }
        } else {
            SendCommand(SPEED, (float)targetSpeed, OSSM_ID);
            SendCommand(ON, (float)targetSpeed, OSSM_ID);
            //OSSM_On = true;
        }
    } else {
        const int resumeSpeed = (int)(speed + 0.5f);
        const bool wasRamping = s_home_speed_ramp_active;
        LogDebugFormatted("Stopping OSSM Safe start is active: %s\n", SafeStartStop ? "true" : "false");
        LogDebugFormatted("BLE: Unpause speed %.1f\n", bleCommGetUnpauseSpeed());
        applyHomeButtonMState(T_RESUME, &style_button_stopped, &style_button_stopped_pressed);
        lv_refr_now(NULL);
        s_home_speed_ramp_active = false;
        if (SafeStartStop) {
            if (!wasRamping && resumeSpeed > HOME_START_RAMP_THRESHOLD) {
//                rampHomeStopSpeed(resumeSpeed);
                SendCommand(SPEED, (float)resumeSpeed, OSSM_ID);
            }
            if (wasRamping && resumeSpeed > HOME_START_RAMP_THRESHOLD) {
//                rampHomeStopSpeed(resumeSpeed);
                //SendCommand(SPEED, 1, OSSM_ID);
            }
        } else {
            SendCommand(SPEED, (float)resumeSpeed, OSSM_ID);
        }
        SendCommand(OFF, 0.0, OSSM_ID);
        bleCommSetUnpauseSpeed(resumeSpeed);
    }
    // Stroke screen watches OSSM_On itself and will refresh its Start/Stop UI.
}

static bool requestHomeButtonToggleOnce()
{
    if (s_home_toggle_fired_this_loop) return false;
    s_home_toggle_fired_this_loop = true;
    homebuttonmevent(nullptr);
    return true;
}

void setupDepthInter(lv_event_t * e) {
    SendCommand(SETUP_D_I, 0.0, OSSM_ID);
}

void setupdepthF(lv_event_t * e) {
    SendCommand(SETUP_D_I_F, 0.0, OSSM_ID);
}

void resetEncoderCounts() {
    encoder1.setCount(0);
    encoder2.setCount(0);
    encoder3.setCount(0);
    encoder4.setCount(0);
    s_encoder_last_step_ms[0] = 0;
    s_encoder_last_step_ms[1] = 0;
    s_encoder_last_step_ms[2] = 0;
    s_encoder_last_step_ms[3] = 0;
    encoder3_enc = 0;
    encoder4_enc = 0;}

// -------------------------------------------------------
// Zero-Stroke Depth Jog + Motion Command Flush
// -------------------------------------------------------
static void startZeroStrokeDepthJog(float previousDepth, float targetDepth)
{
    s_zero_stroke_depth_jog_active = true;
    s_zero_stroke_depth_target = targetDepth;
    s_zero_stroke_depth_jog_start_ms = millis();
    s_zero_stroke_debug_log_ms = 0;
    if (targetDepth > previousDepth) {
        s_zero_stroke_depth_direction = 1;
    } else if (targetDepth < previousDepth) {
        s_zero_stroke_depth_direction = -1;
    } else {
        s_zero_stroke_depth_direction = 0;
    }
}

static void stopZeroStrokeDepthJog()
{
    s_zero_stroke_depth_jog_active = false;
    s_zero_stroke_depth_target = 0.0f;
    s_zero_stroke_depth_jog_start_ms = 0;
    s_zero_stroke_debug_log_ms = 0;
    s_zero_stroke_depth_direction = 0;
}

static void serviceZeroStrokeDepthJog()
{
    if (!ENABLE_ZERO_STROKE_DEPTH_JOG) {
        if (s_zero_stroke_depth_jog_active) {
            stopZeroStrokeDepthJog();
        }
        return;
    }

    if (!s_zero_stroke_depth_jog_active) return;

    if (s_manual_rail_length_mm <= 0.0f) {
        LogDebugFormatted("Error depth jogging - no rail length set\n");
        SendCommand(STROKE, 0.0f, OSSM_ID);
        SendCommand(SPEED, 0.0f, OSSM_ID);
        stopZeroStrokeDepthJog();
        return;
    }
    bleCommGetConfirmedPosition(); // refresh BLE state
    const bool stillEligible = commIsBleMode() && speed > 0.5f && stroke <= 0.001f && depth > 0.0f && s_manual_rail_length_mm > 0.0f;
    if (!stillEligible) {
        SendCommand(STROKE, 0.0f, OSSM_ID);
        stopZeroStrokeDepthJog();
        return;
    }

    const bool timedOut = s_zero_stroke_depth_jog_start_ms != 0 &&
                          (millis() - s_zero_stroke_depth_jog_start_ms) > ZERO_STROKE_DEPTH_JOG_TIMEOUT_MS;
    const float currentPositionMm = bleCommGetConfirmedPosition();
    float targetMm = (s_zero_stroke_depth_target * s_manual_rail_length_mm) / 100.0f;
    bool reachedDepth = false;
    if (bleCommHasFreshState() && currentPositionMm >= 0.0f) {
        if (s_zero_stroke_depth_direction > 0) {
//            reachedDepth = currentPositionMm >= targetMm;
            float targetMm = ((s_zero_stroke_depth_target-1) * s_manual_rail_length_mm) / 100.0f;
            reachedDepth = currentPositionMm >= targetMm;
        } else if (s_zero_stroke_depth_direction < 0) {
//            reachedDepth = currentPositionMm <= targetMm;
            float targetMm = ((s_zero_stroke_depth_target+1) * s_manual_rail_length_mm) / 100.0f;
            reachedDepth = currentPositionMm <= targetMm;
        } else {
            reachedDepth = fabsf(currentPositionMm - targetMm) <= ZERO_STROKE_DEPTH_TOLERANCE;
        }
    }

    const uint32_t nowMs = millis();
    if ((nowMs - s_zero_stroke_debug_log_ms) >= 1000U) {
        const char* dir = (s_zero_stroke_depth_direction > 0) ? "out" :
                          (s_zero_stroke_depth_direction < 0) ? "in" : "none";
        LogDebugFormatted(
            "BLE: zero-stroke jog dir=%s targetDepth=%.1f targetMm=%.1f currentMm=%.1f reached=%d timeout=%d\n",
            dir,
            s_zero_stroke_depth_target,
            targetMm,
            currentPositionMm,
            reachedDepth ? 1 : 0,
            timedOut ? 1 : 0);
        s_zero_stroke_debug_log_ms = nowMs;
    }

    if (reachedDepth || timedOut) {
        const float targetDepth = s_zero_stroke_depth_target;
        SendCommand(STROKE, 0.0f, OSSM_ID);
        stopZeroStrokeDepthJog();
        if (timedOut) {
            LogDebugFormatted("BLE: zero-stroke depth jog timeout at target %.1f\n", targetDepth);
        }
    }
}

static void flushMotionCommands(float motionSpeed,
                                float motionDepth,
                                float motionStroke,
                                bool  motionValueChanged,
                                bool  allowSend)
{
    if (!motionValueChanged) return;

    if (allowSend && motionValueChanged) {
        const float commandedSpeed = resolveVisualCompensatedSpeed(motionSpeed, motionStroke);
        const bool speedChanged = !s_motion_command_cache_valid || motionSpeed != s_last_motion_speed;
        const bool depthChanged = !s_motion_command_cache_valid || motionDepth != s_last_motion_depth;
        const bool strokeChanged = !s_motion_command_cache_valid || motionStroke != s_last_motion_stroke;
        bool sendSpeedForVsl = !speedChanged &&
                               s_visual_speed_lock && s_visual_speed_ratio_valid &&
                               strokeChanged && motionStroke > 0.001f &&
                               OSSM_On && !s_home_speed_ramp_active;
        const bool wantsZeroStrokeJog = ENABLE_ZERO_STROKE_DEPTH_JOG && commIsBleMode() && motionSpeed > 0.5f && motionStroke <= 0.001f && motionDepth > 0.0f && depthChanged;

        if (wantsZeroStrokeJog && s_manual_rail_length_mm <= 0.0f) {
            LogDebugFormatted("Error depth jogging - no rail length set\n");
            SendCommand(STROKE, 0.0f, OSSM_ID);
            SendCommand(SPEED, 0.0f, OSSM_ID);
            stopZeroStrokeDepthJog();
            syncMotionCommandCache(0.0f, motionDepth, 0.0f);
            return;
        }

        const bool zeroStrokeDepthJog = ENABLE_ZERO_STROKE_DEPTH_JOG && commIsBleMode() && motionSpeed > 0.5f && motionStroke <= 0.001f &&
                        motionDepth > 0.0f && s_manual_rail_length_mm > 0.0f && depthChanged;
        const bool forceRunForZeroStrokeJog = zeroStrokeDepthJog && !OSSM_On;
        const float previousDepth = s_motion_command_cache_valid ? s_last_motion_depth : 0.0f;
        float speedToSend = commandedSpeed;

        if (sendSpeedForVsl && s_visual_speed_last_commanded >= 0.0f) {
            const float delta = speedToSend - s_visual_speed_last_commanded;
            if (fabsf(delta) < 0.1f) {
                sendSpeedForVsl = false;
            } else if (delta > VIS_SPEED_CURVE_MAX_STEP) {
                speedToSend = s_visual_speed_last_commanded + VIS_SPEED_CURVE_MAX_STEP;
            } else if (delta < -VIS_SPEED_CURVE_MAX_STEP) {
                speedToSend = s_visual_speed_last_commanded - VIS_SPEED_CURVE_MAX_STEP;
            }
        }

        if (forceRunForZeroStrokeJog) {
            SendCommand(ON, commandedSpeed, OSSM_ID);
            LogDebugFormatted("BLE: zero-stroke depth jog forcing ON at speed %.1f\n", commandedSpeed);
        } else if ((speedChanged || sendSpeedForVsl) && !s_home_speed_ramp_active) {
            SendCommand(SPEED, speedToSend, OSSM_ID);
            s_visual_speed_last_commanded = speedToSend;
        }
        if (depthChanged) { SendCommand(DEPTH, motionDepth, OSSM_ID); }
        if (zeroStrokeDepthJog) {
            SendCommand(STROKE, 1.0f, OSSM_ID);
            startZeroStrokeDepthJog(previousDepth, motionDepth);
        } else if (strokeChanged) {
            SendCommand(STROKE, motionStroke, OSSM_ID);
            stopZeroStrokeDepthJog();
        }
        if(speedChanged ) {
            LogDebugFormatted("BLE: flushMotionCommands speed %.1f depth %.1f stroke %.1f\n Previous speed: %.1f. Speed changed: %s", motionSpeed, motionDepth, motionStroke, s_last_motion_speed, speedChanged ? "true" : "false");
            if (s_visual_speed_lock && s_visual_speed_ratio_valid && motionStroke > 0.001f) {
                const uint32_t nowMs = millis();
                if ((nowMs - s_visual_speed_log_ms) >= 250U) {
                    LogDebugFormatted("VSL: uiSpeed=%.2f stroke=%.2f cmdSpeed=%.2f\n",
                                      motionSpeed, motionStroke, commandedSpeed);
                    s_visual_speed_log_ms = nowMs;
                }
            }
            if (s_last_motion_speed == 0.0f && commandedSpeed > 0.0f) {
                LogDebugFormatted("BLE: Unpause speed %.1f\n", bleCommGetUnpauseSpeed());
                requestHomeButtonToggleOnce(); // simulate a press of the HomeButtonM to resume motion
//            SendCommand(ON, bleCommGetUnpauseSpeed(), OSSM_ID);
            }
        }

        syncMotionCommandCache(motionSpeed, motionDepth, motionStroke);

    }
}

// -------------------------------------------------------
// BLE Connection Guard
// -------------------------------------------------------
static void checkBleDisconnectError()
{
    // ESP-NOW connections don't use BLE — this check is BLE-specific.
    if (commIsEspNowMode()) return;

    static bool          s_was_connected      = false;
    static unsigned long s_disconnect_ms      = 0;
    static bool          s_notification_shown = false;

    const bool connected = bleCommIsConnected();

    if (connected) {
        // Live connection — reset all disconnect tracking.
        s_was_connected      = true;
        s_disconnect_ms      = 0;
        s_notification_shown = false;
        return;
    }

    // Not connected.
    if (!s_was_connected) {
        // Never successfully connected yet (startup) — don't trigger.
        return;
    }

    // Was connected, now disconnected.
    if (s_disconnect_ms == 0) {
        s_disconnect_ms = millis();
        if (s_disconnect_ms == 0) s_disconnect_ms = 1; // avoid sentinel zero
    }

    // Still within the 3-second grace window — allow background reconnect.
    if ((millis() - s_disconnect_ms) < 3000UL) return;

    // Already showed the notification this disconnect episode.
    if (s_notification_shown) return;

    // Don't trigger while the user is still on the start/connect screen.
    if (st_screens == ST_UI_START) return;

    // Grace window expired — try one explicit reconnect before giving up.
    // bleCommTryConnect() has its own BLE_CONNECT_COOLDOWN_MS guard so this
    // won't double-scan if the background task already attempted recently.
    if (bleCommTryConnect()) {
        // Reconnected successfully — reset timer and let normal flow resume.
        s_disconnect_ms = 0;
        return;
    }

    s_notification_shown = true;

    const int result = showNotification(
        T_BLE_COMM_ERROR_TITLE,
        T_BLE_COMM_ERROR_TEXT,
        0,      // no auto-dismiss
        true,   T_RESTART,
        true,   T_TURN_OFF,
        false);

    if (result == NOTIFICATION_RESULT_LEFT) {
        esp_restart();
    } else if (result == NOTIFICATION_RESULT_RIGHT) {
        M5.Power.powerOff();
    }
    // If neither button was pressed (shouldn't happen), fall through —
    // s_notification_shown stays true so we don't spam the notification.
}

// -------------------------------------------------------
// Main Screen State Machine Loop
// -------------------------------------------------------

void SetInitialValues() {
        //if either speed, stroke or depth has no value, set it to 0, so that the stroke screen will not start with a value that is not valid
        if (speed <= 0.0f) {
            speed = 0.0f;
            SendCommand(SPEED, speed, OSSM_ID);
        }
        if (stroke <= 0.0f) {
            stroke = 0.0f;
            SendCommand(STROKE, stroke, OSSM_ID);
        }    
        if (depth <= 0.0f) {
            depth = 50.0f;
            SendCommand(DEPTH, depth, OSSM_ID);
        }
}

void handleScreens() {
    checkBleDisconnectError();
    serviceHomeSpeedRamp();

    s_home_toggle_fired_this_loop = false;
    const bool mxPressCanToggleOssm =
        (st_screens == ST_UI_HOME) ||
        (st_screens == ST_UI_STROKE) ||
        (st_screens == ST_UI_STREAMING);

    if (!mxPressCanToggleOssm) {
        s_consume_next_mx_short_click = false;
    }

    if (mxpress_waspressed && mxPressCanToggleOssm) {
        const bool motionReady = (speed > 0.0f && stroke > 0.0f && depth > 0.0f);
        if ((OSSM_On || motionReady) && requestHomeButtonToggleOnce()) {
            s_consume_next_mx_short_click = true;
        }
    }
    if (mxclick_long_waspressed) {
        s_consume_next_mx_short_click = false;
    }
    if (mxclick_short_waspressed && s_consume_next_mx_short_click) {
        mxclick_short_waspressed = false;
        s_consume_next_mx_short_click = false;
    }

    if (s_zero_stroke_depth_jog_active && st_screens != ST_UI_HOME) {
        SendCommand(STROKE, 0.0f, OSSM_ID);
        stopZeroStrokeDepthJog();
    }

    {
        static bool s_prev_ble_connected = false;
        static bool s_prev_espnow_paired = false;
        static bool s_prev_eject_paired = false;
        static bool s_prev_fist_paired = false;
        static bool s_prev_homing = false;
        static int  s_prev_homing_dir = 0;

        const bool bleConnected = bleCommIsConnected();
        const bool espNowPaired = espNowIsPaired();
        const bool ejectPaired = espNowIsEjectConnected();
        const bool fistPaired = espNowIsFistConnected();
        const bool isHoming = bleCommIsHoming();
        const int homingDir = isHoming ? bleCommGetHomingDirection() : 0;

        if (bleConnected != s_prev_ble_connected ||
            espNowPaired != s_prev_espnow_paired ||
            ejectPaired != s_prev_eject_paired ||
            fistPaired != s_prev_fist_paired ||
            isHoming != s_prev_homing ||
            homingDir != s_prev_homing_dir) {
            g_status_strip_refresh_requested = true;
        }

        if (g_status_strip_refresh_requested) {
            updateStatusStrip();
            g_status_strip_refresh_requested = false;
            s_prev_ble_connected = bleConnected;
            s_prev_espnow_paired = espNowPaired;
            s_prev_eject_paired = ejectPaired;
            s_prev_fist_paired = fistPaired;
            s_prev_homing = isHoming;
            s_prev_homing_dir = homingDir;
        }
    }

    // ---- Battery display (icon + percentage, same as backup firmware) ----
    const bool isCharging = getStableChargingState();
    update_battery_icons_all_screens(getSmoothedBatteryLevel(isCharging), isCharging);
    maybeShowChargingWarning(isCharging);

    // ---- Screen state machine ----
    switch (st_screens) {

    case ST_UI_START:
    {
//        if (lv_obj_has_state(ui_TouchDisable, LV_STATE_CHECKED) == 1) {
//            touch_disabled = true;
//        }
        touch_disabled = false;
        SetInitialValues();
        if (click2_short_waspressed) {
            lv_obj_send_event(ui_StartButtonL, LV_EVENT_CLICKED, NULL);
        } else if (mxclick_short_waspressed) {
            lv_obj_send_event(ui_StartButtonM, LV_EVENT_CLICKED, NULL);
        } else if (click3_short_waspressed) {
            lv_obj_send_event(ui_StartButtonR, LV_EVENT_CLICKED, NULL);
        }
        if (AtStartup){
            connectbutton(nullptr);
            AtStartup = false;
        }
        resetEncoderCounts();
    }
    break;

    case ST_UI_HOME:
    {
//        if (lv_obj_has_state(ui_TouchDisable, LV_STATE_CHECKED) == 1) {
//            touch_disabled = true;
//        }
        touch_disabled = true;
        const bool invertStroke = ui_strokeinvert && lv_obj_has_state(ui_strokeinvert, LV_STATE_CHECKED);
        const bool depthSliderDragged = lv_slider_is_dragged(ui_homedepthslider);
        const bool strokeSliderDragged = lv_slider_is_dragged(ui_homestrokeslider);

        const bool wasMotionReady = (speed > 0.0f && stroke > 0.0f && depth > 0.0f);
        bool homeMotionValueChanged = false;
        bool homeSpeedValueChanged = false;
        bool homeStrokeValueChanged = false;

        // On first entry from a non-strokeEngine screen, tell OSSM to switch to strokeEngine.
        // Only re-home when we know it is needed:
        //   - initial connect (from start screen)
        //   - direct entry from streaming (rare; go:menu safety net in bleCommGoToStrokeEngine)
        //   - returning from menu AFTER a go:menu was deliberately sent (streaming exit or force-homing)
        // home→menu→home intentionally excluded to prevent unnecessary re-homing.
        if (s_prev_st_screens != ST_UI_HOME && s_prev_st_screens != ST_UI_STROKE && s_prev_st_screens != ST_UI_PATTERN) {
            const bool fromStart      = (s_prev_st_screens == ST_UI_START || s_prev_st_screens < 0);
            const bool fromStreaming  = (s_prev_st_screens == ST_UI_STREAMING);
            const bool fromMenuArmed = (s_prev_st_screens == ST_UI_MENU && s_ble_menu_requires_stroke_reentry);
            if (fromStart || fromStreaming || fromMenuArmed) {
                const bool transitioned = bleCommGoToStrokeEngine();
                if (fromMenuArmed && transitioned) {
                    // A force re-home changes OSSM internals; invalidate cache so
                    // the next home loop re-applies UI values to OSSM explicitly.
                    s_motion_command_cache_valid = false;
                    s_force_home_restore_pending = true;
                }
                s_ble_menu_requires_stroke_reentry = false;
            }
        }

        syncHomeSliderRangesToLimits();

        // Encoder 1 — Speed
        bool changed = false;
        //bool updateMXbutton = false;
        if (lv_slider_is_dragged(ui_homespeedslider) == false) {
            changed = false;
            const int speedStep = homeStepFromEncoderCount(0, encoder1.getCount());
            if (speedStep != 0) {
                changed = true;
                speed += speedStep;
                encoder1.setCount(0);
            }
            if (speed <= 0)          { changed = true; speed = 0; }
            if (speed > speedlimit) { changed = true; speed = speedlimit; }
            if (changed) { 
                lv_slider_set_value(ui_homespeedslider, speed, LV_ANIM_OFF);
                //updateMXbutton=true;
            }
        } else if (lv_slider_get_value(ui_homespeedslider) != speed) {
            speed = lv_slider_get_value(ui_homespeedslider);
            changed = true;
            
            //updateMXbutton=true;
        }
        homeMotionValueChanged = homeMotionValueChanged || changed || (lv_slider_get_value(ui_homespeedslider) != speed);
            homeSpeedValueChanged = homeSpeedValueChanged || changed;
        char speed_v[7]; dtostrf(speed, 6, 0, speed_v);
        lv_label_set_text(ui_homespeedvalue, speed_v);

        // Encoder 2 — Depth
        if (!depthSliderDragged) {
            changed = false;
            const float prevDepth = depth;
            const float prevStroke = stroke;
            const int depthStep = homeStepFromEncoderCount(1, encoder2.getCount());
            if (depthStep != 0) {
                changed = true;
                depth += depthStep;
                if (dynamicStroke) {
                    stroke += depthStep;
                    if (stroke >= depth) stroke = depth;
                }
                encoder2.setCount(0);
            }
            if (depth <= 0)            { changed = true; depth = 0; stroke = 0; }   //here is the error
            if (depth > maxdepthinmm) { changed = true; depth = maxdepthinmm; }
            if (stroke > depth)         { changed = true; stroke = depth; }
            if (changed && (depth != prevDepth || stroke != prevStroke)) {
                lv_slider_set_value(ui_homedepthslider, depth, LV_ANIM_OFF);
            }
        } else {
            const float touchDepth = lv_slider_get_value(ui_homedepthslider);
            if (touchDepth != depth) {
                depth = touchDepth;
                changed = true;
            }
            if (stroke > depth) {
                stroke = depth;
                changed = true;
            }
        }
        homeMotionValueChanged = homeMotionValueChanged || changed || (lv_slider_get_value(ui_homedepthslider) != depth);

        // Encoder 3 — Stroke
        if (!strokeSliderDragged) {
            changed = false;
            const float prevDepth = depth;
            const float prevStroke = stroke;
            const int strokeStep = homeStepFromEncoderCount(2, encoder3.getCount());
            if (strokeStep != 0) {
                changed = true;
                stroke += invertStroke ? -strokeStep : strokeStep;
                encoder3.setCount(0);
            }
            if (stroke <= 0)            { changed = true; stroke = 0; }
            if (stroke > maxdepthinmm) { changed = true; stroke = maxdepthinmm; }
            
            if (s_stroke_influences_depth) {
                if (stroke > depth) {
                    changed = true;
                    depth = stroke;
                }
            } else {
                if (stroke > depth) {
                    changed = true;
                    stroke = depth;
                }
            }

            if (invertStroke) {
                if(lv_bar_get_mode(ui_homestrokeslider) != LV_BAR_MODE_RANGE) {
                    lv_bar_set_mode(ui_homestrokeslider, LV_BAR_MODE_RANGE);
                }
                lv_bar_set_start_value(ui_homestrokeslider, depth - stroke, LV_ANIM_OFF);
                lv_slider_set_value(ui_homestrokeslider, depth, LV_ANIM_OFF);
            }
            else {
                if(lv_bar_get_mode(ui_homestrokeslider) != LV_BAR_MODE_NORMAL) {
                    lv_bar_set_mode(ui_homestrokeslider, LV_BAR_MODE_NORMAL);
                }
                lv_bar_set_start_value(ui_homestrokeslider, 0, LV_ANIM_OFF);
                lv_slider_set_value(ui_homestrokeslider, stroke, LV_ANIM_OFF);
            }

            if (changed && (depth != prevDepth || stroke != prevStroke)) {
                //LogDebug("Possible error 3");
            }
        } else {
            const float touchStroke = invertStroke ? (depth - lv_slider_get_left_value(ui_homestrokeslider))
                                                   : lv_slider_get_value(ui_homestrokeslider);
            if (touchStroke != stroke) {
                stroke = touchStroke;
                changed = true;
            }
            if (!s_stroke_influences_depth && stroke > depth) {
                stroke = depth;
                changed = true;
            }
        }
            homeMotionValueChanged = homeMotionValueChanged || changed;
            homeStrokeValueChanged = homeStrokeValueChanged || changed;
        syncHomeMotionUi(invertStroke);

        if (homeStrokeValueChanged && stroke <= 0.001f) {
            resetVisualSpeedRatioState();
        }
        updateVisualSpeedRatioFromUi(homeSpeedValueChanged, speed, stroke);

        // Encoder 4 — Sensation
        if (lv_slider_is_dragged(ui_homesensationslider) == false) {
            changed = false;
            lv_slider_set_value(ui_homesensationslider, sensation, LV_ANIM_OFF);
            const int sensationStep = homeStepFromEncoderCount(3, encoder4.getCount());
            if (sensationStep != 0) {
                changed = true;
                sensation += sensationStep;
                encoder4.setCount(0);
            }
            if (sensation < -100)   { changed = true; sensation = -100; }
            if (sensation > 100) { changed = true; sensation = 100; }
            if (changed) { SendCommand(SENSATION, sensation, OSSM_ID); }
        } else if (lv_slider_get_value(ui_homesensationslider) != sensation) {
            sensation = lv_slider_get_value(ui_homesensationslider);
            SendCommand(SENSATION, sensation, OSSM_ID);
        }

        if (s_force_home_restore_pending) {
            // One-shot full restore from UI -> OSSM after re-home completion.
            // Keep it explicit here (all 4 channels) for readability and safety.
            // Also block same-loop MX short-click from starting with stale values.
            mxclick_short_waspressed = false;
            s_consume_next_mx_short_click = false;

            const float restoredSpeed = resolveVisualCompensatedSpeed(speed, stroke);
            SendCommand(SPEED, restoredSpeed, OSSM_ID);
            SendCommand(DEPTH, depth, OSSM_ID);
            SendCommand(STROKE, stroke, OSSM_ID);
            SendCommand(SENSATION, sensation, OSSM_ID);

            syncMotionCommandCache(speed, depth, stroke);
            s_visual_speed_last_commanded = restoredSpeed;
            s_force_home_restore_pending = false;
        }

        if (click2_long_waspressed) {
            lv_obj_send_event(ui_HomeButtonL, LV_EVENT_LONG_PRESSED, NULL);
        } else if (click2_double_waspressed) {
            lv_obj_send_event(ui_HomeButtonL, LV_EVENT_DOUBLE_CLICKED, NULL);
        } else if (click2_short_waspressed) {
            lv_obj_send_event(ui_HomeButtonL, LV_EVENT_CLICKED, NULL);
        } else if (mxclick_short_waspressed) {
            requestHomeButtonToggleOnce();
        } else if (mxclick_long_waspressed) {
            lv_obj_send_event(ui_HomeButtonM, LV_EVENT_LONG_PRESSED, NULL);
            sensation = 0;
            speed = 0;
            stroke = 0;
            depth = 0;
            SendCommand(SPEED, speed, OSSM_ID);
            SendCommand(STROKE, stroke, OSSM_ID);
            SendCommand(DEPTH, depth, OSSM_ID);
            SendCommand(SENSATION, sensation, OSSM_ID);
            bleCommGoToMenu();
            _ui_screen_change(ui_Menu, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
            st_screens = ST_UI_MENU;
        } else if (click3_long_waspressed) {
            //LogDebug("HomeButtonR long pressed - checking for FistIT addon");
            if (addonsIsFistITEnabled() && FistITPaired()) {
                //LogDebug("Fist-IT addon is paired - opening Fist-IT screen");
                g_addon_return_screen = lv_scr_act();
                FistITPrepareScreen();
                _ui_screen_change(FistITGetScreen(), LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
            }
            sensation = 0;
        } else if (click3_double_waspressed) {
            if (addonsIsFistITEnabled() && FistITPaired()) {
                FistITToggle();
            }
        } else if (click3_short_waspressed) {
            lv_obj_send_event(ui_HomeButtonR, LV_EVENT_CLICKED, NULL);
        }
        const bool isMotionReady = (speed > 0.0f && stroke > 0.0f && depth > 0.0f);
        flushMotionCommands(speed, depth, stroke, homeMotionValueChanged, true);
        serviceZeroStrokeDepthJog();

        if (homeMotionValueChanged && !wasMotionReady && isMotionReady && !OSSM_On) {
            requestHomeButtonToggleOnce();
            LogDebug ("HomeButtonM auto-started OSSM due to motion values being set");
        } else if (homeMotionValueChanged && wasMotionReady && !isMotionReady && OSSM_On) {
            requestHomeButtonToggleOnce();
            LogDebug ("HomeButtonM auto-stopped OSSM due to motion values being cleared");
        }

        updateHomeButtonMState();

        if (FistITPaired()) {
            lv_label_set_text(ui_HomeButtonRText, T_PATTERN_Button "   F");
        }

        if (EjectIsPaired()) {
            lv_label_set_text(ui_HomeButtonLText, T_HOMEL "       E");
        }
    }
    break;

    case ST_UI_MENU:
    {
//        if (lv_obj_has_state(ui_ui_TouchDisable, LV_STATE_CHECKED) == 1) {
//           touch_disabled = true;
//        }
        touch_disabled = false;
        if (encoder4.getCount() > encoder4_enc + 1) {
            lv_group_focus_next(ui_g_menu);
            encoder4_enc = encoder4.getCount();
        } else if (encoder4.getCount() < encoder4_enc - 1) {
            lv_group_focus_prev(ui_g_menu);
            encoder4_enc = encoder4.getCount();
        }
        if (click2_short_waspressed) {
            lv_obj_send_event(ui_MenuButtonL, LV_EVENT_SHORT_CLICKED, NULL);
        } else if (mxclick_short_waspressed) {
            lv_obj_send_event(ui_MenuButtonM, LV_EVENT_SHORT_CLICKED, NULL);
        } else if (click3_short_waspressed) {
            lv_obj_send_event(ui_MenuButtonR, LV_EVENT_SHORT_CLICKED, NULL);
        } else if (click3_long_waspressed) {
            SendCommand(REBOOT, 0, OSSM_ID);
        }
    }
    break;

    case ST_UI_STROKE:
    {
        touch_disabled = false;

        const bool shouldRehome = (s_prev_st_screens != ST_UI_STROKE && s_prev_st_screens != ST_UI_HOME && s_prev_st_screens != ST_UI_PATTERN) && (
            (s_prev_st_screens == ST_UI_START || s_prev_st_screens < 0) ||
            (s_prev_st_screens == ST_UI_STREAMING) ||
            (s_prev_st_screens == ST_UI_MENU && s_ble_menu_requires_stroke_reentry)
        );
        const bool resetToSimpleStroke = (s_prev_st_screens != ST_UI_STROKE && s_prev_st_screens != ST_UI_PATTERN);
        strokeScreenHandle(shouldRehome, resetToSimpleStroke);
    }
    break;

    case ST_UI_COLORS:
    {
        touch_disabled = false;
        if (encoder4.getCount() > encoder4_enc + 2) {
            colorsScrollFocus(1);
            encoder4_enc = encoder4.getCount();
        } else if (encoder4.getCount() < encoder4_enc - 2) {
            colorsScrollFocus(-1);
            encoder4_enc = encoder4.getCount();
        }
        if (click2_short_waspressed) {
            _ui_screen_change(ui_Menu, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
        } else if (mxclick_short_waspressed || click3_short_waspressed) {
            colorSchemeSelectIndex(colorsGetFocusIndex());
        }
    }
    break;

    case ST_UI_STREAMING:
    {
        touch_disabled = true;
        const bool firstEntry = (s_prev_st_screens != ST_UI_STREAMING);
        streamingScreenHandle(firstEntry);
    }
    break;

    case ST_UI_ADDONS:
    {
        touch_disabled = false;
        if (encoder4.getCount() > encoder4_enc + 2) {
            addonsMoveSelection(1);
            encoder4_enc = encoder4.getCount();
        } else if (encoder4.getCount() < encoder4_enc - 2) {
            addonsMoveSelection(-1);
            encoder4_enc = encoder4.getCount();
        }
        if (click2_short_waspressed) {
            lv_obj_send_event(ui_AddonsButtonL, LV_EVENT_SHORT_CLICKED, NULL);
        } else if (mxclick_short_waspressed) {
            lv_obj_send_event(ui_AddonsButtonM, LV_EVENT_SHORT_CLICKED, NULL);
        } else if (click3_short_waspressed) {
            lv_obj_send_event(ui_AddonsButtonR, LV_EVENT_SHORT_CLICKED, NULL);
        }
    }
    break;


    case ST_UI_PATTERN:
    {
//        if (lv_obj_has_state(ui_TouchDisable, LV_STATE_CHECKED) == 1) {
//            touch_disabled = true;
//        }
        touch_disabled = false;
        if (encoder4.getCount() > encoder4_enc + 2) {
            //LogDebug("next");
            uint32_t t = LV_KEY_DOWN;
            lv_obj_send_event(ui_PatternS, LV_EVENT_KEY, &t);
            encoder4_enc = encoder4.getCount();
        } else if (encoder4.getCount() < encoder4_enc - 2) {
            uint32_t t = LV_KEY_UP;
            lv_obj_send_event(ui_PatternS, LV_EVENT_KEY, &t);
            //LogDebug("Preview");
            encoder4_enc = encoder4.getCount();
        }
        if (click2_short_waspressed) {
            lv_obj_send_event(ui_PatternButtonL, LV_EVENT_CLICKED, NULL);
        } else if (mxclick_short_waspressed) {
            resetEncoderCounts();
            lv_obj_send_event(ui_PatternButtonM, LV_EVENT_CLICKED, NULL);
        } else if (click3_short_waspressed) {
            resetEncoderCounts();
            lv_obj_send_event(ui_PatternButtonR, LV_EVENT_CLICKED, NULL);
        }
    }
    break;

    case ST_UI_Torqe:
    {
//        if (lv_obj_has_state(ui_safeStartStop, LV_STATE_CHECKED) == 1) {
//            touch_disabled = true;
//        }

        // Encoder 1 — Torque Out
        if (lv_slider_is_dragged(ui_outtroqeslider) == false) {
            if (encoder1.getCount() != torqe_f_enc) {
                lv_slider_set_value(ui_outtroqeslider, torqe_f, LV_ANIM_OFF);
                if      (encoder1.getCount() <= 0)          encoder1.setCount(0);
                else if (encoder1.getCount() >= Encoder_MAP) encoder1.setCount(Encoder_MAP);
                torqe_f_enc = encoder1.getCount();
                torqe_f = fscale(0, Encoder_MAP, 50, 200, torqe_f_enc, 0);
                SendCommand(TORQE_F, torqe_f, OSSM_ID);
            }
        } else if (lv_slider_get_value(ui_outtroqeslider) != torqe_f) {
            torqe_f_enc = fscale(50, 200, 0, Encoder_MAP, torqe_f, 0);
            encoder1.setCount(torqe_f_enc);
            torqe_f = lv_slider_get_value(ui_outtroqeslider);
            SendCommand(TORQE_F, torqe_f, OSSM_ID);
        }
        char torqe_f_v[7]; dtostrf((torqe_f * -1), 6, 0, torqe_f_v);
        lv_label_set_text(ui_outtroqevalue, torqe_f_v);

        // Encoder 4 — Torque In
        if (lv_slider_is_dragged(ui_introqeslider) == false) {
            if (encoder4.getCount() != torqe_r_enc) {
                lv_slider_set_value(ui_introqeslider, torqe_r, LV_ANIM_OFF);
                if      (encoder4.getCount() <= 0)          encoder4.setCount(0);
                else if (encoder4.getCount() >= Encoder_MAP) encoder4.setCount(Encoder_MAP);
                torqe_r_enc = encoder4.getCount();
                torqe_r = fscale(0, Encoder_MAP, 20, 200, torqe_r_enc, 0);
                SendCommand(TORQE_R, torqe_r, OSSM_ID);
            }
        } else if (lv_slider_get_value(ui_introqeslider) != torqe_r) {
            torqe_r_enc = fscale(20, 200, 0, Encoder_MAP, torqe_r, 0);
            encoder4.setCount(torqe_r_enc);
            torqe_r = lv_slider_get_value(ui_introqeslider);
            SendCommand(TORQE_R, torqe_r, OSSM_ID);
        }
        char torqe_r_v[7]; dtostrf(torqe_r, 6, 0, torqe_r_v);
        lv_label_set_text(ui_introqevalue, torqe_r_v);

        if (click2_short_waspressed) {
            lv_obj_send_event(ui_TorqeButtonL, LV_EVENT_CLICKED, NULL);
        } else if (mxclick_short_waspressed) {
            lv_obj_send_event(ui_TorqeButtonM, LV_EVENT_CLICKED, NULL);
        } else if (click3_short_waspressed) {
            lv_obj_send_event(ui_TorqeButtonR, LV_EVENT_CLICKED, NULL);
        }
    }
    break;

    case ST_UI_EJECTSETTINGS:
    {
        touch_disabled = true;
        ButtonEvents events = {
            click2_short_waspressed,
            mxclick_short_waspressed,
            click3_short_waspressed
        };
        EjectHandleScreen(events);
    }
    break;

    case ST_UI_FISTIT:
    {
        touch_disabled = true;
        ButtonEvents events = {
            click2_short_waspressed,
            mxclick_short_waspressed,
            click3_short_waspressed
        };
        FistITHandleScreen(events);
    }
    break;

    case ST_UI_APMODE:
    {
        touch_disabled = true;
        ButtonEvents events = {
            click2_short_waspressed,
            mxclick_short_waspressed,
            click3_short_waspressed
        };
        APModeHandleScreen(events);
    }
    break;

    case ST_UI_SETTINGS:
    {
        touch_disabled = false;
        refreshSettingsCarousel();
        if (encoder3.getCount() > encoder3_enc + 2) {
            if (ui_brightness_slider) {
                int val = lv_slider_get_value(ui_brightness_slider);
                int mx = lv_slider_get_max_value(ui_brightness_slider);
                if (val < mx) {
                    int newVal = (val + 5 <= mx) ? val + 5 : mx;
                    lv_slider_set_value(ui_brightness_slider, newVal, LV_ANIM_OFF);
                    M5.Display.setBrightness(newVal);
                    M5.Lcd.setBrightness(newVal);
                }
            }
            encoder3_enc = encoder3.getCount();
        } else if (encoder3.getCount() < encoder3_enc - 2) {
            if (ui_brightness_slider) {
                int val = lv_slider_get_value(ui_brightness_slider);
                int mn = lv_slider_get_min_value(ui_brightness_slider);
                if (val > mn) {
                    int newVal = (val - 5 >= mn) ? val - 5 : mn;
                    lv_slider_set_value(ui_brightness_slider, newVal, LV_ANIM_OFF);
                    M5.Display.setBrightness(newVal);
                    M5.Lcd.setBrightness(newVal);
                }
            }
            encoder3_enc = encoder3.getCount();
        }

        if (encoder4.getCount() > encoder4_enc + 2) {
            lv_obj_t* options[9] = {};
            const int optionCount = collectSettingsOptionObjects(options, 9);
            if (optionCount > 0 && s_settings_focus_index < (optionCount - 1)) {
                ++s_settings_focus_index;
            }
            refreshSettingsCarousel();
            encoder4_enc = encoder4.getCount();
        } else if (encoder4.getCount() < encoder4_enc - 2) {
            if (s_settings_focus_index > 0) {
                --s_settings_focus_index;
            }
            refreshSettingsCarousel();
            encoder4_enc = encoder4.getCount();
        }
        if (click2_short_waspressed) {
            lv_obj_send_event(ui_SettingsButtonL, LV_EVENT_CLICKED, NULL);
        } else if (mxclick_short_waspressed) {
            lv_obj_send_event(ui_SettingsButtonM, LV_EVENT_CLICKED, NULL);
        } else if (click3_short_waspressed) {
            lv_obj_t *focused = getSettingsFocusedObject();
            if (focused) {
                if (focused == ui_visualSpeedLock) {
                    lv_obj_send_event(focused, LV_EVENT_VALUE_CHANGED, NULL);
                } else {
                    bool isToggle = (focused == ui_vibrate || focused == ui_safeStartStop ||
                                     focused == ui_strokeinvert || focused == ui_forceHome ||
                                     focused == ui_strokeDepthLink ||
                                     focused == s_manual_rail_length_setting);
                    if (isToggle) {
                        if (lv_obj_has_state(focused, LV_STATE_CHECKED)) {
                            lv_obj_clear_state(focused, LV_STATE_CHECKED);
                        } else {
                            lv_obj_add_state(focused, LV_STATE_CHECKED);
                        }
                        lv_obj_send_event(focused, LV_EVENT_VALUE_CHANGED, NULL);
                    } else {
                        if (focused == s_encoder_ramp_profile_setting) {
                            lv_obj_send_event(focused, LV_EVENT_VALUE_CHANGED, NULL);
                        } else {
                            lv_obj_send_event(focused, LV_EVENT_SHORT_CLICKED, NULL);
                        }
                    }
                }
            }
        }
    }
    break;

    } // end switch(st_screens)

    // ---- Clear button flags ----
    mxpress_waspressed       = false;
    mxclick_long_waspressed  = false;
    mxclick_short_waspressed = false;
    click2_short_waspressed  = false;
    click2_long_waspressed   = false;
    click2_double_waspressed = false;
    click3_short_waspressed  = false;
    click3_long_waspressed   = false;
    click3_double_waspressed = false;
    s_prev_st_screens = st_screens;
}
