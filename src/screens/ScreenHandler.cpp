#include "ScreenHandler.h"
#include "ScreenHandler_internal.h"
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
#include "../addons/addons.h"
#include "../addons/Eject.h"
#include "../addons/FistIT.h"
#include "../addons/Coyote.h"
#include "../addons/AP-mode.h"
#include "../addons/AP-v2.h"
#include "../addons/ToyControl.h"
#include "../addons/addonsStreaming.h"
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

// Home screen speed-ramp parameters. Moved here from ScreenHandler.h so they
// have a single definition (previously each including TU got its own copy).
const int      HOME_START_RAMP_THRESHOLD   = 10;
const uint32_t HOME_START_RAMP_INTERVAL_MS = 10;

// -------------------------------------------------------
// Screen and control state
// -------------------------------------------------------
int   st_screens  = ST_UI_START;
int   menuestatus = 0;

float speed     = 0.0f;
float depth     = 0.0f;
float stroke    = 0.0f;
float sensation = 0.0f;
float minPos    = 0.0f;
float maxPos    = 100.0f;
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
bool  s_motion_command_cache_valid = false;
float s_last_motion_speed = 0.0f;
float s_last_motion_depth = 0.0f;
float s_last_motion_stroke = 0.0f;
// Suppresses syncHomeValuesFromOssm() briefly after local input, so the
// user's own change isn't immediately overwritten by not-yet-updated confirmed state.
uint32_t s_last_local_motion_input_ms = 0;
bool  s_zero_stroke_depth_jog_active = false;
float s_zero_stroke_depth_target = 0.0f;
int   s_zero_stroke_depth_direction = 0;
bool  s_visual_speed_lock = false;
bool  s_visual_speed_ratio_valid = false;
float s_visual_speed_stroke_product = 0.0f;
float s_visual_speed_last_commanded = -1.0f;
int s_speed_behavior_profile = SPEED_BEHAVIOR_STANDARD;
static bool  s_stroke_influences_depth = false;
float s_manual_rail_length_mm = 0.0f;
bool s_home_speed_ramp_active = false;
int s_home_speed_ramp_current = 0;
int s_home_speed_ramp_target = 0;
int s_home_speed_ramp_step = 0;
uint32_t s_home_speed_ramp_interval_ms = 0;
uint32_t s_home_speed_ramp_next_ms = 0;
static bool s_force_home_restore_pending = false;
uint32_t s_zero_stroke_depth_jog_start_ms = 0;
uint32_t s_zero_stroke_debug_log_ms = 0;
uint32_t s_visual_speed_log_ms = 0;
static bool s_home_toggle_fired_this_loop = false;
static bool s_consume_next_mx_short_click = false;

const float VIS_SPEED_CURVE_MAX_STEP = 2.0f;


int s_settings_focus_index = 0;
int s_settings_scroll_offset = 0;
lv_obj_t* s_manual_rail_length_setting = nullptr;
lv_obj_t* s_encoder_ramp_profile_setting = nullptr;
lv_obj_t* s_language_setting = nullptr;
lv_obj_t* s_wifi_setting = nullptr;
bool s_manual_rail_length_ui_syncing = false;
bool s_speed_behavior_ui_syncing = false;
int s_encoder_ramp_profile = ENCODER_RAMP_MEDIUM;
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

// ---- Screensaver / Power management (ported from backup firmware) ----
int            g_brightness_value       = 180;
unsigned long  last_activity_ms         = 0;
int            screensaver_prev_brightness = 180;
bool           screensaver_active       = false;
int            screensaver_timeout_ms   = SCREENSAVER_TIMEOUT_MS_DEFAULT;
int            screensaver_dim_brightness = SCREENSAVER_DIM_BRIGHTNESS_DEFAULT;
uint32_t       deep_sleep_timeout_ms    = DEEP_SLEEP_TIMEOUT_MS_DEFAULT;

// Notification touch result (set by LVGL button callbacks inside showNotification)
static volatile bool g_status_strip_refresh_requested = true;
uint32_t s_encoder_last_step_ms[4] = {0, 0, 0, 0};


static constexpr int EJECT_ICON_W = 14;
static constexpr int EJECT_ICON_H = 20;
static constexpr int FIST_ICON_W = 17;
static constexpr int FIST_ICON_H = 18;
static constexpr int HOME_ICON_W = 18;
static constexpr int HOME_ICON_H = 22;
static constexpr int ESP_ICON_W = 21;
static constexpr int ESP_ICON_H = 18;
static constexpr int COYOTE_ICON_W = 20;
static constexpr int COYOTE_ICON_H = 20;

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

// Simple lightning-bolt glyph for the Coyote (e-stim) status icon.
static const char* const COYOTE_ICON_MASK[COYOTE_ICON_H] = {
 ".......######.......",
 ".....##......##.....",
 "...##..#....#..##...",
 "..#...##....##...#..",
 ".#....###..###....#.",
 "#....##########...#.",
 "#....##########...#.",
 "#....#.######.#...#.",
 "#...##..####..##..#.",
 "#...##...##...##..#.",
 "#...###......###..#.",
 "#....###..##.###..#.",
 "#....##########...#.",
 "#......######.....#.",
 ".#......####.....#..",
 "..#......##.....#...",
 "...##..........##...",
 ".....##......##.....",
 ".......######.......",
 "...................."
};

// Simple lightning-bolt glyph for the Coyote (e-stim) status icon.
static const char* const COYOTE_ON_ICON_MASK[COYOTE_ICON_H] = {
 "....................",
 "......########......",
 "....###.####.###....",
 "..####..####..####..",
 ".#####...##...#####.",
 "#####..........#####",
 "#####..........#####",
 "#####.#......#.#####",
 "####..##....##..####",
 "####..###..###..####",
 "####...######...####",
 "#####...##..#...####",
 "#####..........#####",
 "#######......#######",
 ".#######....#######.",
 "..#######..#######..",
 "....############....",
 "......########......",
 "....................",
 "...................."
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

static lv_obj_t* createStatusCoyoteIcon(lv_obj_t* parent) {
    static uint8_t iconBuffer[LV_CANVAS_BUF_SIZE(32, 32, 32, LV_DRAW_BUF_STRIDE_ALIGN)];
    static bool iconReady = false;
    lv_obj_t* icon = createStatusIconBase(parent, COYOTE_ICON_W, COYOTE_ICON_H);
    if (!icon) return nullptr;
    icons_render_mask_canvas(icon, iconBuffer, iconReady, COYOTE_ICON_MASK, COYOTE_ICON_W, COYOTE_ICON_H,
                             getActiveBackgroundColor(), getActiveTextPrimaryColor());
    return icon;
}

static lv_obj_t* createStatusCoyoteOnIcon(lv_obj_t* parent) {
    static uint8_t iconBuffer[LV_CANVAS_BUF_SIZE(32, 32, 32, LV_DRAW_BUF_STRIDE_ALIGN)];
    static bool iconReady = false;
    lv_obj_t* icon = createStatusIconBase(parent, COYOTE_ICON_W, COYOTE_ICON_H);
    if (!icon) return nullptr;
    icons_render_mask_canvas(icon, iconBuffer, iconReady, COYOTE_ON_ICON_MASK, COYOTE_ICON_W, COYOTE_ICON_H,
                             getActiveBackgroundColor(), getActiveTextPrimaryColor());
    return icon;
}


static void updateStatusStrip() {
    static lv_obj_t* statusLabels[14] = { nullptr };
    static lv_obj_t* statusEjectIcons[14] = { nullptr };
    static lv_obj_t* statusFistIcons[14] = { nullptr };
    static lv_obj_t* statusHomeIcons[14] = { nullptr };
    static lv_obj_t* statusESPIcons[14] = { nullptr };
    static lv_obj_t* statusCoyoteIcons[14] = { nullptr };
    static lv_obj_t* statusCoyoteOnIcons[14] = { nullptr };
    lv_obj_t* statusScreens[14] = {
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
        APV2ModeGetScreen(),
        ui_Coyote,
    };

    for (size_t i = 0; i < 14; ++i) {
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
        if (statusCoyoteIcons[i] == nullptr) {
            statusCoyoteIcons[i] = createStatusCoyoteIcon(statusScreens[i]);
        }
        if (statusCoyoteOnIcons[i] == nullptr) {
            statusCoyoteOnIcons[i] = createStatusCoyoteOnIcon(statusScreens[i]);
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

    const bool ejectPaired = EjectIsPaired();
    const bool fistPaired = FistITIsPaired();
    const bool coyotePaired = CoyoteIsPaired();
    const bool coyoteOn = CoyoteIsOn();
    for (size_t i = 0; i < 13; ++i) {
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
            lv_obj_add_flag(statusESPIcons[i], LV_OBJ_FLAG_HIDDEN);
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

        if (statusCoyoteIcons[i] != nullptr) {
            lv_obj_set_align(statusCoyoteIcons[i], LV_ALIGN_LEFT_MID);
            lv_obj_set_x(statusCoyoteIcons[i], iconX);
            lv_obj_set_y(statusCoyoteIcons[i], -102);
            if (coyotePaired && !coyoteOn) {
                lv_obj_clear_flag(statusCoyoteIcons[i], LV_OBJ_FLAG_HIDDEN);
                iconX += lv_obj_get_width(statusCoyoteIcons[i]) + 2;
            } else {
                lv_obj_add_flag(statusCoyoteIcons[i], LV_OBJ_FLAG_HIDDEN);
            }
        }

        if (statusCoyoteOnIcons[i] != nullptr) {
            lv_obj_set_align(statusCoyoteOnIcons[i], LV_ALIGN_LEFT_MID);
            lv_obj_set_x(statusCoyoteOnIcons[i], iconX);
            lv_obj_set_y(statusCoyoteOnIcons[i], -102);
            if (coyoteOn) {
                lv_obj_clear_flag(statusCoyoteOnIcons[i], LV_OBJ_FLAG_HIDDEN);
                iconX += lv_obj_get_width(statusCoyoteOnIcons[i]) + 2;
            } else {
                lv_obj_add_flag(statusCoyoteOnIcons[i], LV_OBJ_FLAG_HIDDEN);
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

    const int prevScreenForAddonGate = st_screens;

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
            if (bleCommIsConnected()) {
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
            if (newPatternIsReadFromOSSM && patternString.length() > 0) {
                lv_roller_set_options(ui_PatternS, patternString.c_str(), LV_ROLLER_MODE_NORMAL);
                uint16_t optionCount = (uint16_t)lv_roller_get_option_count(ui_PatternS);
                if (optionCount > 0) {
                    if (pattern < 0 || pattern >= (int)optionCount) {
                        pattern = 0;
                    }
                    lv_roller_set_selected(ui_PatternS, pattern, LV_ANIM_OFF);
                }
                newPatternIsReadFromOSSM = false;
            } else {
                uint16_t optionCount = (uint16_t)lv_roller_get_option_count(ui_PatternS);
                if (optionCount > 0) {
                    if (pattern < 0 || pattern >= (int)optionCount) {
                        pattern = 0;
                    }
                    lv_roller_set_selected(ui_PatternS, pattern, LV_ANIM_OFF);
                }
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
    } else if (lv_scr_act() == ui_Coyote) {
        st_screens = ST_UI_COYOTE;
    } else if (APModeOwnsActiveScreen()) {
//        if (st_screens != ST_UI_APMODE) {
//            resetEncoderCounts();
//        }
        st_screens = ST_UI_APMODE;
    } else if (APV2ModeOwnsActiveScreen()) {
        st_screens = ST_UI_APMODE_V2;
    } else if (ToyControlOwnsActiveScreen()) {
        st_screens = ST_UI_TOYCONTROL;
    }
    (void)prevScreenForAddonGate;
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
    
    // turn off fist-it and send that command to Fist-IT, regardless of whatever screen we are in
    SendCommand(OFF, 0.0, FIST_ID);
    SendCommand(OFF, 0.0, OSSM_ID);
    SendCommand(OFF, 0.0, EJECT_ID);    
}

void ejectcreampie(lv_event_t * e) {
    (void)e;
    if (ui_HomeButtonL) {
        lv_obj_clear_state(ui_HomeButtonL, LV_STATE_CHECKED);
    }
    EjectToggle();
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


void homebuttonmevent(lv_event_t * e) {
    LogDebug("HomeButton");
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
            OSSM_On = true;
            if (targetSpeed > HOME_START_RAMP_THRESHOLD) {
                rampHomeStartSpeed(targetSpeed);
            }
        } else {
            SendCommand(SPEED, (float)targetSpeed, OSSM_ID);
            SendCommand(ON, (float)targetSpeed, OSSM_ID);
            OSSM_On = true;
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
        OSSM_On = false;
        bleCommSetUnpauseSpeed(resumeSpeed);
    }
    // Stroke screen watches OSSM_On itself and will refresh its Start/Stop UI.
}

bool requestHomeButtonToggleOnce()
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

// BLE Connection Guard
// -------------------------------------------------------
static void checkBleDisconnectError()
{
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
        true,   T_RECONNECT,
        false);

    if (result == NOTIFICATION_RESULT_LEFT) {
        esp_restart();
    } else if (result == NOTIFICATION_RESULT_RIGHT) {
        //M5.Power.powerOff();
        connectbutton(nullptr);
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
        if(st_screens == ST_UI_STROKE) {
            if (depth <= 0.0f) {
                depth = 50.0f;
                SendCommand(DEPTH, depth, OSSM_ID);
            }
        } else {
            if (depth <= 0.0f) {
                depth = 0.0f;
                SendCommand(DEPTH, depth, OSSM_ID);
            }
        }    
}

void handleScreens() {
    checkBleDisconnectError();
    addonsBackgroundConnect();
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
        static bool s_prev_eject_paired = false;
        static bool s_prev_fist_paired = false;
        static bool s_prev_coyote_on = false;
        static bool s_prev_homing = false;
        static int  s_prev_homing_dir = 0;
        static uint32_t s_last_status_refresh_ms = 0;

        const bool bleConnected = bleCommIsConnected();
        const bool ejectPaired = EjectIsPaired();
        const bool fistPaired = FistITIsPaired();
        const bool coyoteOn = CoyoteIsOn();
        const bool isHoming = bleCommIsHoming();
        const int homingDir = isHoming ? bleCommGetHomingDirection() : 0;
        const uint32_t nowMs = millis();

        // Keep status visuals fresh while BLE polling updates machine state.
        if (bleConnected && (nowMs - s_last_status_refresh_ms) >= 250U) {
            g_status_strip_refresh_requested = true;
        }

        if (bleConnected != s_prev_ble_connected ||
            ejectPaired != s_prev_eject_paired ||
            fistPaired != s_prev_fist_paired ||
            coyoteOn != s_prev_coyote_on ||
            isHoming != s_prev_homing ||
            homingDir != s_prev_homing_dir) {
            g_status_strip_refresh_requested = true;
        }

        if (g_status_strip_refresh_requested) {
            updateStatusStrip();
            g_status_strip_refresh_requested = false;
            s_last_status_refresh_ms = nowMs;
            s_prev_ble_connected = bleConnected;
            s_prev_eject_paired = ejectPaired;
            s_prev_fist_paired = fistPaired;
            s_prev_coyote_on = coyoteOn;
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
        if (lv_scr_act() == ui_Start && bleCommIsConnected()) {
            if (ui_Welcome) {
                static char welcomeText[48];
                snprintf(welcomeText, sizeof(welcomeText), "%s%s", T_CONNECTED_TO, bleCommGetFirmwareDescription());
                lv_label_set_text(ui_Welcome, welcomeText);
            }
            _ui_screen_change(ui_Menu, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
            screenmachine(nullptr);
            break;
        }
        //SetInitialValues();
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
        const bool speedSliderDragged = lv_slider_is_dragged(ui_homespeedslider);
        const bool depthSliderDragged = lv_slider_is_dragged(ui_homedepthslider);
        const bool strokeSliderDragged = lv_slider_is_dragged(ui_homestrokeslider);
        const bool sensationSliderDragged = lv_slider_is_dragged(ui_homesensationslider);
        const bool speedEncoderPending = labs(encoder1.getCount()) >= 2;
        const bool depthEncoderPending = labs(encoder2.getCount()) >= 2;
        const bool strokeEncoderPending = labs(encoder3.getCount()) >= 2;
        const bool sensationEncoderPending = labs(encoder4.getCount()) >= 2;

        syncHomeValuesFromOssm(speedSliderDragged || speedEncoderPending,
                   depthSliderDragged || depthEncoderPending,
                   strokeSliderDragged || strokeEncoderPending,
                   sensationSliderDragged || sensationEncoderPending);

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
            const int speedStep = screenEncoderRampStep(0, encoder1.getCount());
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
            const int depthStep = screenEncoderRampStep(1, encoder2.getCount());
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
            const int strokeStep = screenEncoderRampStep(2, encoder3.getCount());
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

            // Widget push (bar mode/range recompute) is handled once below by
            // syncHomeMotionUi(), gated on an actual value change, instead of
            // unconditionally here every loop.

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

        // Only push depth/stroke to their LVGL widgets (bar mode/range recalculation
        // is noticeably heavier than a plain slider) when something actually moved —
        // doing this unconditionally every loop starved encoder polling and made
        // depth/stroke feel laggier than the speed encoder.
        static float s_last_synced_depth = -1.0f;
        static float s_last_synced_stroke = -1.0f;
        static int   s_last_synced_invert = -1;
        const bool depthStrokeUiStale =
            (depth != s_last_synced_depth) || (stroke != s_last_synced_stroke) ||
            ((int)invertStroke != s_last_synced_invert);
        if (depthStrokeUiStale) {
            syncHomeMotionUi(invertStroke);
            s_last_synced_depth = depth;
            s_last_synced_stroke = stroke;
            s_last_synced_invert = (int)invertStroke;
        }

        if (homeStrokeValueChanged && stroke <= 0.001f) {
            resetVisualSpeedRatioState();
        }
        updateVisualSpeedRatioFromUi(homeSpeedValueChanged, speed, stroke);

        // Encoder 4 — Sensation
        bool homeSensationValueChanged = false;
        if (lv_slider_is_dragged(ui_homesensationslider) == false) {
            changed = false;
            lv_slider_set_value(ui_homesensationslider, sensation, LV_ANIM_OFF);
            const int sensationStep = screenEncoderRampStep(3, encoder4.getCount());
            if (sensationStep != 0) {
                changed = true;
                sensation += sensationStep;
                encoder4.setCount(0);
            }
            if (sensation < -100)   { changed = true; sensation = -100; }
            if (sensation > 100) { changed = true; sensation = 100; }
            if (changed) { SendCommand(SENSATION, sensation, OSSM_ID); }
            homeSensationValueChanged = changed;
        } else if (lv_slider_get_value(ui_homesensationslider) != sensation) {
            sensation = lv_slider_get_value(ui_homesensationslider);
            SendCommand(SENSATION, sensation, OSSM_ID);
            homeSensationValueChanged = true;
        }

        if (homeMotionValueChanged || homeSensationValueChanged) {
            s_last_local_motion_input_ms = millis();
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
            lv_obj_send_event(ui_HomeButtonL, LV_EVENT_SHORT_CLICKED, NULL);
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

        if (ui_HomeButtonRText) {
            if (FistITPaired()) {
                lv_label_set_text_fmt(ui_HomeButtonRText, "%s   F", T_PATTERN_Button);
            } else {
                lv_label_set_text(ui_HomeButtonRText, T_PATTERN_Button);
            }
        }

        if (ui_HomeButtonLText) {
            if (EjectIsPaired()) {
                lv_label_set_text_fmt(ui_HomeButtonLText, "%s       E", T_HOMEL);
            } else {
                lv_label_set_text(ui_HomeButtonLText, T_HOMEL);
            }
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

    case ST_UI_COYOTE:
    {
        touch_disabled = true;
        ButtonEvents events = {
            click2_short_waspressed,
            mxclick_short_waspressed,
            click3_short_waspressed
        };
        CoyoteHandleScreen(events);
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

    case ST_UI_APMODE_V2:
    {
        touch_disabled = true;
        ButtonEvents events = {
            click2_short_waspressed,
            mxclick_short_waspressed,
            click3_short_waspressed
        };
        APV2ModeHandleScreen(events);
    }
    break;

    case ST_UI_TOYCONTROL:
    {
        touch_disabled = true;
        ButtonEvents events = {
            click2_short_waspressed,
            mxclick_short_waspressed,
            click3_short_waspressed
        };
        ToyControlHandleScreen(events);
    }
    break;

    case ST_UI_SETTINGS:
    {
        touch_disabled = false;
        refreshSettingsCarousel();
        {
            static uint32_t s_last_wifi_status_sync = 0;
            const uint32_t now = millis();
            if ((now - s_last_wifi_status_sync) > 1500) {
                s_last_wifi_status_sync = now;
                syncWifiSettingUi();
            }
        }
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
                        if (focused == s_encoder_ramp_profile_setting ||
                            focused == s_language_setting ||
                            focused == s_wifi_setting) {
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
