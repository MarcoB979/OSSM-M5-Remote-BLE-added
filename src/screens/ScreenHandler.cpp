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
#include "../addons/addonsStreaming.h"
#include "../communication/EspNowComm.h"
#include "../communication/CommManager.h"
#include "../communication/BleComm.h"
#include "../display/colors.h"
#include "../display/styles.h"
#include "../addons/strokeMode.h"
#include "../addons/addonsStreaming.h"
#include "../screens/icons.h"
#include "language.h"
#include <M5Unified.h>
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
static bool  s_motion_command_cache_valid = false;
static float s_last_motion_speed = 0.0f;
static float s_last_motion_depth = 0.0f;
static float s_last_motion_stroke = 0.0f;

bool dynamicStroke  = false;
bool eject_status   = false;
bool vibrate_mode   = true;
bool touch_home     = false;
bool strokeinvert_mode = false;
bool ble_force_homeing = false;
bool touch_disabled = false;
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

static unsigned long nowMs      = 0;
static int           rampMs     = 0;
static bool          rampEnabled = true;
static int           rampValue  = 1;
static int           rampUpValue = 2;
static int           rampTime   = 95;
static int           maxRamp    = 10;
static int           encId      = 0;
static int           activeEncId = 0;

static constexpr int EJECT_ICON_W = 15;
static constexpr int EJECT_ICON_H = 20;
static constexpr int FIST_ICON_W = 22;
static constexpr int FIST_ICON_H = 18;
static constexpr int HOME_ICON_W = 18;
static constexpr int HOME_ICON_H = 22;
static constexpr int ESP_ICON_W = 21;
static constexpr int ESP_ICON_H = 18;

static const char* const EJECT_ICON_MASK[EJECT_ICON_H] = {
".....###.........",
".....####.....###",
"......###....##..",
"..... ..##...#...",
".......##.......",
".......###..###.",
"................",
"................",
"........##......",
".......#..#.....",
"......######....",
".....#########..",
"....##########..",
".....########...",
"......######....",
"......######....",
"......######.....",
"......######....",
"......######....",
"......######...."
};

static const char* const FIST_ICON_MASK[FIST_ICON_H] = {

"...........##........",
"..##..#####..#####...",
"##..##...#...##...#..",
"##..##...#...##...#..",
"#....#............#..",
"#.................#..",
"#.................##.",
"#....#...#...##...#.#",
"#....#...#...##...#.#",
"#....#...#...##...#.#",
"#....#...#...##...#.#",
"#....#...#...##...#.#",
"###################.#",
"........#..........#.",
"........#..........#.",
".......##........##..",
"........#########....",
"....................."
  };

static const char* const HOME_ICON_MASK[HOME_ICON_H] = {
  "........#####........",
  "......#########......",
  "....#####...#####....",
  "..#####.......#####..",
  "#####...........#####",
  ".....................",
  ".....###.....###.....",
  ".....###.....###.....",
  ".....###.....###.....",
  ".....###.....###.....",
  ".....###########.....",
  ".....###########.....",
  ".....###.....###.....",
  ".....###.....###.....",
  ".....###.....###.....",
  ".....###.....###.....",
  ".....................",
  "#####...........#####",
  "..#####.......#####..",
  "....#####...#####....",
  "......#########......",
  "........#####........"
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
        nullptr,
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
    const int desiredMin = bleMode ? 0 : -100;
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
// screenInit() — load NVS settings and apply to UI
// ---------------------------------------------------------------------------
// Screensaver / Power management functions (ported from backup firmware)
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// Notification overlay callbacks
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// Battery icon display (ported from backup firmware screen.cpp)
// ---------------------------------------------------------------------------
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
    static bool batteryUiInitialized = false;
    const int valueLabelX = isCharging ? -25 : -40;

    lv_obj_t *batteryTitleLabels[] = {
        ui_Batt, ui_Batt1, ui_Batt2, ui_Batt3, ui_Batt4,
        ui_Batt5, ui_Batt6, ui_Batt7, ui_Batt8, ui_Batt9
    };
    lv_obj_t *batteryValueLabels[] = {
        ui_BattValue, ui_BattValue1, ui_BattValue2, ui_BattValue3, ui_BattValue4,
        ui_BattValue5, ui_BattValue6, ui_BattValue7, ui_BattValue8, ui_BattValue9
    };
    lv_obj_t *batteryBars[] = {
        ui_Battery, ui_Battery1, ui_Battery2, ui_Battery3, ui_Battery4,
        ui_Battery5, ui_Battery6, ui_Battery7, ui_Battery8, ui_Battery9
    };

    if (!batteryUiInitialized) {
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
        batteryUiInitialized = true;
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

// ---------------------------------------------------------------------------
// Battery level smoothing and charging detection (ported from backup firmware)
// ---------------------------------------------------------------------------
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
    /*
    // Non-linear Li-ion OCV-inspired mapping (mV -> percent), then interpolate.
    // This avoids the "too optimistic" mid-range values from linear mapping.
    struct BatteryCurvePoint {
        float mv;
        int pct;
    };

    static const BatteryCurvePoint curve[] = {
        {3200.0f, 0},
        {3300.0f, 3},
        {3400.0f, 7},
        {3500.0f, 15},
        {3600.0f, 28},
        {3700.0f, 42},
        {3800.0f, 58},
        {3900.0f, 74},
        {4000.0f, 87},
        {4100.0f, 95},
        {4200.0f, 100},
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
            if (pct < 0) pct = 0;
            if (pct > 100) pct = 100;
            return pct;
        }
    }

    return 0;
    */
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

// ---------------------------------------------------------------------------
// screen_power_tick() — called at top of loop() (ported from backup firmware)
// ---------------------------------------------------------------------------
void screen_power_tick()
{
    if (encoder1.getCount() + encoder2.getCount() + encoder3.getCount() + encoder4.getCount() != 0) {
        screensaver_check_activity();
    }

    if (!screensaver_active && (millis() - last_activity_ms > (unsigned long)screensaver_timeout_ms)) {
        screensaver_prev_brightness = g_brightness_value;
        M5.Lcd.setBrightness(screensaver_dim_brightness);
        screensaver_active = true;
    }

#if AUTO_IDLE_DEEP_SLEEP_ENABLED
    if (millis() - last_activity_ms > deep_sleep_timeout_ms) {
        if (canEnterDeepSleep()) {
            enterDeepSleep();
        }
    }
#endif
}

// ---------------------------------------------------------------------------
// Sleep / Restart confirmation actions — called from Settings screen UI
// ---------------------------------------------------------------------------
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

void screenInit() {
    Preferences prefs;
    prefs.begin("m5-ctnr", false);
    eject_status = prefs.getBool("ejectAddon", false);
    vibrate_mode = prefs.getBool("Vibrate",    true);
    touch_home   = prefs.getBool("Lefty",      false);
    strokeinvert_mode = prefs.getBool("StrokeInvert", true);
    ble_force_homeing = prefs.getBool("BleForceHomeing", true);
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
    if (touch_home)   { lv_obj_add_state(ui_lefty,    LV_STATE_CHECKED); }
    if (strokeinvert_mode && ui_strokeinvert) { lv_obj_add_state(ui_strokeinvert, LV_STATE_CHECKED); }
    if (ble_force_homeing && ui_forceHome)    { lv_obj_add_state(ui_forceHome, LV_STATE_CHECKED); }
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
        st_screens = ST_UI_HOME;
        syncHomeSliderRangesToLimits();
        syncHomeSensationSliderToTransport();
        speed = lv_slider_get_value(ui_homespeedslider);
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
        st_screens = ST_UI_MENU;
    } else if (lv_scr_act() == ui_Pattern) {
        st_screens = ST_UI_PATTERN;
    } else if (lv_scr_act() == ui_Torqe) {
        st_screens = ST_UI_Torqe;
        torqe_f = lv_slider_get_value(ui_outtroqeslider);
        torqe_f_enc = fscale(50, 200, 0, Encoder_MAP, torqe_f, 0);
        encoder1.setCount(torqe_f_enc);
        torqe_r = lv_slider_get_value(ui_introqeslider);
        torqe_r_enc = fscale(20, 200, 0, Encoder_MAP, torqe_r, 0);
        encoder4.setCount(torqe_r_enc);
    } else if (lv_scr_act() == ui_EJECTSettings) {
        st_screens = ST_UI_EJECTSETTINGS;
    } else if (lv_scr_act() == ui_Settings) {
        st_screens = ST_UI_SETTINGS;
    } else if (lv_scr_act() == ui_Stroke) {
        st_screens = ST_UI_STROKE;
        refreshStrokeStartStopUi();
    } else if (lv_scr_act() == ui_Colors) {
        st_screens = ST_UI_COLORS;
    } else if (lv_scr_act() == ui_Streaming) {
        st_screens = ST_UI_STREAMING;
    } else if (lv_scr_act() == ui_Addons) {
        st_screens = ST_UI_ADDONS;
        addonsSyncSelectionVisual();
    } else if (lv_scr_act() == ui_FistIT) {
        st_screens = ST_UI_FISTIT;
    }
}

void savesettings(lv_event_t * e) {
    Preferences prefs;
    prefs.begin("m5-ctnr", false);

    if (lv_obj_has_state(ui_vibrate, LV_STATE_CHECKED) == 1) {
        prefs.putBool("Vibrate", true);
    } else {
        prefs.putBool("Vibrate", false);
    }

    if (lv_obj_has_state(ui_lefty, LV_STATE_CHECKED) == 1) {
        prefs.putBool("Lefty", true);
    } else {
        prefs.putBool("Lefty", false);
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

    if (lv_obj_has_state(ui_ejectaddon, LV_STATE_CHECKED) == 1) {
        prefs.putBool("ejectAddon", true);
    } else {
        prefs.putBool("ejectAddon", false);
    }

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
    if (speed > 5) {
        SendCommand(SPEED, 5, OSSM_ID);
    }
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
    showNotification(T_PULLING_OUT, T_PULLING_OUT_TEXT, 3000, false, nullptr, false, nullptr, false);
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
    //LogDebug("HomeButton");
    if (OSSM_On == false) {
        if (speed == 0 || stroke == 0 || depth == 0) return;
        LogDebug("Starting OSSM");
        SendCommand(ON, 0.0, OSSM_ID);
        applyHomeButtonMState(T_STOP, &style_button_running, &style_button_running_pressed);
    } else {
        LogDebug("Stopping OSSM");
        LogDebugFormatted("BLE: Unpause speed %.1f\n", bleCommGetUnpauseSpeed());
        SendCommand(OFF, 0.0, OSSM_ID);
        applyHomeButtonMState(T_RESUME, &style_button_stopped, &style_button_stopped_pressed);
    }
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
}

static void syncMotionCommandCache(float motionSpeed, float motionDepth, float motionStroke)
{
    s_last_motion_speed = motionSpeed;
    s_last_motion_depth = motionDepth;
    s_last_motion_stroke = motionStroke;
    s_motion_command_cache_valid = true;
}

static void flushMotionCommands(float motionSpeed,
                                float motionDepth,
                                float motionStroke,
                                bool  motionValueChanged,
                                bool  allowSend,
                                bool  requestAutoStart)
{
    if (!motionValueChanged && !requestAutoStart) return;

    if (allowSend && motionValueChanged) {
        const bool speedChanged = !s_motion_command_cache_valid || motionSpeed != s_last_motion_speed;
        const bool depthChanged = !s_motion_command_cache_valid || motionDepth != s_last_motion_depth;
        const bool strokeChanged = !s_motion_command_cache_valid || motionStroke != s_last_motion_stroke;

        if (speedChanged) { SendCommand(SPEED, motionSpeed, OSSM_ID); }
        if (depthChanged) { SendCommand(DEPTH, motionDepth, OSSM_ID); }
        if (strokeChanged) { SendCommand(STROKE, motionStroke, OSSM_ID); }

        syncMotionCommandCache(motionSpeed, motionDepth, motionStroke);
    }

    if (requestAutoStart && speed > 0.0f && stroke > 0.0f && depth > 0.0f && !OSSM_On) {
        homebuttonmevent(nullptr);
    }
}
// -------------------------------------------------------
// checkBleDisconnectError() — show a fatal notification if BLE drops
// and does not recover within 3 seconds.
// Only active in BLE transport mode; skipped for ESP-NOW connections.
// Must only be called from the Arduino main-task context.
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
// handleScreens() — battery update + state machine + flag clear
// Called every loop iteration from main loop()
// -------------------------------------------------------
void handleScreens() {
    checkBleDisconnectError();

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
        if (lv_obj_has_state(ui_lefty, LV_STATE_CHECKED) == 1) {
            touch_disabled = true;
        }
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
    }
    break;

    case ST_UI_HOME:
    {
        if (lv_obj_has_state(ui_lefty, LV_STATE_CHECKED) == 1) {
            touch_disabled = true;
        }

        const bool wasMotionReady = (speed > 0.0f && stroke > 0.0f && depth > 0.0f);
        bool homeMotionValueChanged = false;

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
                bleCommGoToStrokeEngine();
                s_ble_menu_requires_stroke_reentry = false;
            }
        }

        syncHomeSliderRangesToLimits();

        // Ramp helper
        nowMs = millis();
        if (nowMs - rampMs <= (unsigned long)rampTime && rampEnabled == true) {
            if (rampValue <= maxRamp && encId == activeEncId) {
                rampValue += rampUpValue;
                //++rampValue;
            }
        } else {
            rampValue = 1;  //TEST AFTER EAU COMMENTS
            activeEncId = encId;
        }

        // Encoder 1 — Speed
        bool changed = false;
        //bool updateMXbutton = false;
        if (lv_slider_is_dragged(ui_homespeedslider) == false) {
            changed = false;
            if (encoder1.getCount() >= 2) {
                changed = true; speed += rampValue;
                encoder1.setCount(0); rampMs = millis(); encId = 1;
            } else if (encoder1.getCount() <= -2) {
                changed = true; speed -= rampValue;
                encoder1.setCount(0); rampMs = millis(); encId = 1;
            }
            if (speed <= 0)          { changed = true; speed = 0; }
            if (speed > speedlimit) { changed = true; speed = speedlimit; }
            if (changed) { 
                lv_slider_set_value(ui_homespeedslider, speed, LV_ANIM_OFF);
                //updateMXbutton=true;
            }
        } else if (lv_slider_get_value(ui_homespeedslider) != speed) {
            speed = lv_slider_get_value(ui_homespeedslider);
            //updateMXbutton=true;
        }
        homeMotionValueChanged = homeMotionValueChanged || changed || (lv_slider_get_value(ui_homespeedslider) != speed);
        char speed_v[7]; dtostrf(speed, 6, 0, speed_v);
        lv_label_set_text(ui_homespeedvalue, speed_v);

        // Encoder 2 — Depth
        if (lv_slider_is_dragged(ui_homedepthslider) == false) {
            changed = false;
            const float prevDepth = depth;
            const float prevStroke = stroke;
            if (encoder2.getCount() >= 2) {
                changed = true; depth += rampValue;
                if (dynamicStroke) stroke += rampValue;
                encoder2.setCount(0); rampMs = millis(); encId = 2;
            } else if (encoder2.getCount() <= -2) {
                changed = true; depth -= rampValue;
                if (dynamicStroke) {
                    stroke -= rampValue;
                    if (stroke >= depth) stroke = depth;
                }
                encoder2.setCount(0); rampMs = millis(); encId = 2;
            }
            if (depth <= 0)            { changed = true; depth = 0; stroke = 0; }   //here is the error
            if (depth > maxdepthinmm) { changed = true; depth = maxdepthinmm; }
            if (stroke > depth)         { changed = true; stroke = depth; }
            if (changed && (depth != prevDepth || stroke != prevStroke)) {
                LogDebug("Possible error 1");
                lv_slider_set_value(ui_homedepthslider, depth, LV_ANIM_OFF);
            }
        } else if (lv_slider_get_value(ui_homedepthslider) != depth) {
            depth = lv_slider_get_value(ui_homedepthslider);
        }
        homeMotionValueChanged = homeMotionValueChanged || changed || (lv_slider_get_value(ui_homedepthslider) != depth);
        char depth_v[7]; dtostrf(depth, 6, 0, depth_v);
        lv_label_set_text(ui_homedepthvalue, depth_v);

        // Encoder 3 — Stroke
        if (lv_slider_is_dragged(ui_homestrokeslider) == false) {
            changed = false;
            const float prevDepth = depth;
            const float prevStroke = stroke;
            bool invertStroke = ui_strokeinvert && lv_obj_has_state(ui_strokeinvert, LV_STATE_CHECKED);
            if (encoder3.getCount() >= 2) {
                changed = true; stroke += invertStroke ? -rampValue : rampValue;
                encoder3.setCount(0); rampMs = millis(); encId = 3;
            } else if (encoder3.getCount() <= -2) {  
                changed = true; stroke += invertStroke ? rampValue : -rampValue; 
                encoder3.setCount(0); rampMs = millis(); encId = 3;
            }
            if (stroke <= 0)            { changed = true; stroke = 0; }
            if (stroke > maxdepthinmm) { changed = true; stroke = maxdepthinmm; }
            if (stroke > depth)         { changed = true; stroke = depth; }
            if (invertStroke) {
                lv_bar_set_start_value(ui_homestrokeslider, depth - stroke, LV_ANIM_OFF);
                lv_slider_set_value(ui_homestrokeslider, depth, LV_ANIM_OFF);
            }
            else {
                lv_bar_set_start_value(ui_homestrokeslider, 0, LV_ANIM_OFF);
                lv_slider_set_value(ui_homestrokeslider, stroke, LV_ANIM_OFF);
            }

            if (changed && (depth != prevDepth || stroke != prevStroke)) {
                LogDebug("Possible error 3");
            }
        } else if (lv_slider_get_left_value(ui_homestrokeslider) != depth - stroke) {
            stroke = depth - lv_slider_get_left_value(ui_homestrokeslider);
        } else if (lv_slider_get_value(ui_homestrokeslider) != depth) {
            depth = lv_slider_get_value(ui_homestrokeslider);
        }
        homeMotionValueChanged = homeMotionValueChanged || changed || (lv_slider_get_left_value(ui_homestrokeslider) != depth - stroke) || (lv_slider_get_value(ui_homestrokeslider) != depth);
        char stroke_v[7]; dtostrf(stroke, 6, 0, stroke_v);
        lv_label_set_text(ui_homestrokevalue, stroke_v);

        // Encoder 4 — Sensation
        if (lv_slider_is_dragged(ui_homesensationslider) == false) {
            changed = false;
            lv_slider_set_value(ui_homesensationslider, sensation, LV_ANIM_OFF);
            if (encoder4.getCount() >= 2) {
                changed = true; sensation += 2;
                encoder4.setCount(0); rampMs = millis(); encId = 4;
            } else if (encoder4.getCount() <= -2) {
                changed = true; sensation -= 2;
                encoder4.setCount(0); rampMs = millis(); encId = 4;
            }
            if (sensation < 0)   { changed = true; sensation = 0; }
            if (sensation > 100) { changed = true; sensation = 100; }
            if (changed) { SendCommand(SENSATION, sensation, OSSM_ID); }
        } else if (lv_slider_get_value(ui_homesensationslider) != sensation) {
            sensation = lv_slider_get_value(ui_homesensationslider);
            SendCommand(SENSATION, sensation, OSSM_ID);
        }
        if (click2_long_waspressed) {
            lv_obj_send_event(ui_HomeButtonL, LV_EVENT_LONG_PRESSED, NULL);
        } else if (click2_double_waspressed) {
            lv_obj_send_event(ui_HomeButtonL, LV_EVENT_DOUBLE_CLICKED, NULL);
        } else if (click2_short_waspressed) {
            lv_obj_send_event(ui_HomeButtonL, LV_EVENT_CLICKED, NULL);
        } else if (mxclick_short_waspressed) {
            lv_obj_send_event(ui_HomeButtonM, LV_EVENT_CLICKED, NULL);
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
                flushMotionCommands(speed, depth, stroke, homeMotionValueChanged, true, (!wasMotionReady && isMotionReady));

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
        if (lv_obj_has_state(ui_lefty, LV_STATE_CHECKED) == 1) {
            touch_disabled = true;
        }
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
        const bool firstEntry = (s_prev_st_screens != ST_UI_STREAMING);
        streamingScreenHandle(firstEntry);
    }
    break;

    case ST_UI_ADDONS:
    {
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
        if (lv_obj_has_state(ui_lefty, LV_STATE_CHECKED) == 1) {
            touch_disabled = true;
        }
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
        if (lv_obj_has_state(ui_lefty, LV_STATE_CHECKED) == 1) {
            touch_disabled = true;
        }

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
        ButtonEvents events = {
            click2_short_waspressed,
            mxclick_short_waspressed,
            click3_short_waspressed
        };
        FistITHandleScreen(events);
    }
    break;

    case ST_UI_SETTINGS:
    {
        touch_disabled = false;
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
            //LogDebug("next");
            if (ui_g_settings) lv_group_focus_next(ui_g_settings);
            encoder4_enc = encoder4.getCount();
        } else if (encoder4.getCount() < encoder4_enc - 2) {
            if (ui_g_settings) lv_group_focus_prev(ui_g_settings);
            //LogDebug("Preview");
            encoder4_enc = encoder4.getCount();
        }
        if (click2_short_waspressed) {
            lv_obj_send_event(ui_SettingsButtonL, LV_EVENT_CLICKED, NULL);
        } else if (mxclick_short_waspressed) {
            lv_obj_send_event(ui_SettingsButtonM, LV_EVENT_CLICKED, NULL);
        } else if (click3_short_waspressed) {
            lv_obj_t *focused = ui_g_settings ? lv_group_get_focused(ui_g_settings) : NULL;
            if (focused) {
                bool isToggle = (focused == ui_vibrate || focused == ui_lefty ||
                                 focused == ui_strokeinvert || focused == ui_forceHome);
                if (isToggle) {
                    if (lv_obj_has_state(focused, LV_STATE_CHECKED)) {
                        lv_obj_clear_state(focused, LV_STATE_CHECKED);
                    } else {
                        lv_obj_add_state(focused, LV_STATE_CHECKED);
                    }
                    lv_obj_send_event(focused, LV_EVENT_VALUE_CHANGED, NULL);
                } else {
                    lv_obj_send_event(focused, LV_EVENT_SHORT_CLICKED, NULL);
                }
            }
        }
    }
    break;

    } // end switch(st_screens)

    // ---- Clear button flags ----
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
