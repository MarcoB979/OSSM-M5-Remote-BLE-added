#include "ScreenHandler.h"
#include <M5Unified.h>
#include <lvgl.h>
#include <Preferences.h>
#include "../ui/ui.h"
#include "../ui/ui_helpers.h"
#include "../main.h"
#include "../debug.h"
#include "../config_pins.h"
#include "../config_ids.h"
#include "../PatternMath.h"
#include "../buttonhandlers/ButtonHandlers.h"
#include "../communication/EspNowComm.h"
#include "../communication/CommManager.h"
#include "../communication/BleComm.h"
#include "../colors.h"
#include "../styles.h"
#include "../strokeMode.h"
#include "../addonsStreaming.h"
#include "../icons.h"

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

static unsigned long nowMs      = 0;
static int           rampMs     = 0;
static bool          rampEnabled = true;
static int           rampValue  = 1;
static int           rampTime   = 75;
static int           maxRamp    = 8;
static int           encId      = 0;
static int           activeEncId = 0;

static constexpr int EJECT_ICON_W = 20;
static constexpr int EJECT_ICON_H = 21;
static constexpr int FIST_ICON_W = 18;
static constexpr int FIST_ICON_H = 23;

static const char* const EJECT_ICON_MASK[EJECT_ICON_H] = {
  "....................",
  "...#.....###.....#..",
  "..###..#####....###.",
  ".####..######..####.",
  "..###...####...###..",
  "....#.....#.....#...",
  "....................",
  "........#####.......",
  "......##..#..##.....",
  ".....##.......##....",
  ".....##.......##....",
  "......###...###.....",
  "......##.....#.#....",
  "......##.....#.#....",
  "......##.....#.#....",
  "......##.....#.#....",
  "......##.....#.#....",
  "......##.....#.#....",
  "......##########....",
  "........#####.......",
  "....................",
};

static const char* const FIST_ICON_MASK[FIST_ICON_H] = {
  "..........#####...",
  ".....##.##.##..#..",
  "..##...#...#...##.",
  "##..#...#...#..##.",
  "##...##..##..##.##",
  ".##.............##",
  ".##............##.",
  ".###.##........##.",
  "..#######......##.",
  ".###....##.....##.",
  "##.##....##...##..",
  "##..##....##..##..",
  "##...##....##.##..",
  "##....##....#.##..",
  "##.....##.....##..",
  "##......##....##..",
  "##.......##..###..",
  "##........######..",
  ".##.........###...",
  "..##........##....",
  "...##.......##....",
  "....#########.....",
  "..................",
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

static void updateStatusStrip() {
    static lv_obj_t* statusLabels[12] = { nullptr };
    static lv_obj_t* statusEjectIcons[12] = { nullptr };
    static lv_obj_t* statusFistIcons[12] = { nullptr };
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

        if (statusEjectIcons[i] == nullptr) {
            statusEjectIcons[i] = createStatusEjectIcon(statusScreens[i]);
        }
        if (statusFistIcons[i] == nullptr) {
            statusFistIcons[i] = createStatusFistIcon(statusScreens[i]);
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

    if (bleCommIsConnected()) appendToken(LV_SYMBOL_BLUETOOTH);
    if (espNowIsPaired()) appendToken(LV_SYMBOL_WIFI);
    if (bleCommIsHoming()) {
        int dir = bleCommGetHomingDirection();
        if (dir > 0) appendToken(LV_SYMBOL_UP);
        else if (dir < 0) appendToken(LV_SYMBOL_DOWN);
    }
    if (labelText[0] == '\0') {
        snprintf(labelText, sizeof(labelText), " ");
    }

    const bool ejectPaired = espNowIsEjectConnected();
    const bool fistPaired = espNowIsFistConnected();

    for (size_t i = 0; i < 12; ++i) {
        lv_obj_t* label = statusLabels[i];
        if (label == nullptr) continue;

        lv_obj_add_style(label, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_22, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(label, labelText);
        lv_obj_update_layout(label);

        int iconX = 10 + lv_obj_get_width(label) + 4;

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

        if (statusFistIcons[i] != nullptr) {
            lv_obj_set_align(statusFistIcons[i], LV_ALIGN_LEFT_MID);
            lv_obj_set_x(statusFistIcons[i], iconX);
            lv_obj_set_y(statusFistIcons[i], -102);
            if (fistPaired) {
                lv_obj_clear_flag(statusFistIcons[i], LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_add_flag(statusFistIcons[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
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

// -------------------------------------------------------
// screenInit() — load NVS settings and apply to UI
// Call after ui_init() and buttonInit()
// -------------------------------------------------------
void screenInit() {
    Preferences prefs;
    prefs.begin("m5-ctnr", false);
    eject_status = prefs.getBool("ejectAddon", false);
    vibrate_mode = prefs.getBool("Vibrate",    true);
    touch_home   = prefs.getBool("Lefty",      false);
    strokeinvert_mode = prefs.getBool("StrokeInvert", false);
    ble_force_homeing = prefs.getBool("BleForceHomeing", false);
    int brightness = prefs.getInt("Brightness", 180);
    if (brightness < 5) brightness = 5;
    if (brightness > 255) brightness = 255;
    prefs.end();

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
}

void brightness_slider_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED || !ui_brightness_slider) return;

    int val = lv_slider_get_value(ui_brightness_slider);
    if (val < 5) val = 5;
    if (val > 255) val = 255;

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
    if (lv_scr_act() == ui_Start) {
        st_screens = ST_UI_START;
    } else if (lv_scr_act() == ui_Home) {
        st_screens = ST_UI_HOME;
        syncHomeSliderRangesToLimits();
        speed = lv_slider_get_value(ui_homespeedslider);
        LogDebug(speedenc);
        LogDebug(speed);
    } else if (lv_scr_act() == ui_Menu) {
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

void ejectcreampie(lv_event_t * e) {
    if (EJECT_On == true) {
        lv_obj_clear_state(ui_HomeButtonL, LV_STATE_CHECKED);
        EJECT_On = false;
    } else {
        lv_obj_clear_state(ui_HomeButtonL, LV_STATE_CHECKED);
        depth  = 0;
        speed  = 0;
        stroke = 0;
        SendCommand(SETUP_D_I, 0.0, OSSM_ID);
        SendCommand(DEPTH, depth, OSSM_ID);
        screenmachine(e);
        EJECT_On = true;
    }
}

void savepattern(lv_event_t * e) {
    pattern = lv_roller_get_selected(ui_PatternS);
    lv_roller_get_selected_str(ui_PatternS, patternstr, 0);
    lv_label_set_text(ui_HomePatternLabel, patternstr);
    LogDebug(pattern);
    float patterns = pattern;
    SendCommand(PATTERN, patterns, OSSM_ID);
}

void homebuttonmevent(lv_event_t * e) {
    LogDebug("HomeButton");
    if (OSSM_On == false) {
        SendCommand(ON, 0.0, OSSM_ID);
    } else {
        SendCommand(OFF, 0.0, OSSM_ID);
    }
}

void setupDepthInter(lv_event_t * e) {
    SendCommand(SETUP_D_I, 0.0, OSSM_ID);
}

void setupdepthF(lv_event_t * e) {
    SendCommand(SETUP_D_I_F, 0.0, OSSM_ID);
}

// -------------------------------------------------------
// handleScreens() — battery update + state machine + flag clear
// Called every loop iteration from main loop()
// -------------------------------------------------------
void handleScreens() {
    updateStatusStrip();

    // ---- Battery display (all screens share the battery bars) ----
    const int    BatteryLevel = M5.Power.getBatteryLevel();
    String       BatteryValue = String(BatteryLevel, DEC) + "%";
    const char  *battVal      = BatteryValue.c_str();
    lv_bar_set_value(ui_Battery,  BatteryLevel, LV_ANIM_OFF);
    lv_label_set_text(ui_BattValue,  battVal);
    lv_bar_set_value(ui_Battery1, BatteryLevel, LV_ANIM_OFF);
    lv_label_set_text(ui_BattValue1, battVal);
    lv_bar_set_value(ui_Battery2, BatteryLevel, LV_ANIM_OFF);
    lv_label_set_text(ui_BattValue2, battVal);
    lv_bar_set_value(ui_Battery3, BatteryLevel, LV_ANIM_OFF);
    lv_label_set_text(ui_BattValue3, battVal);
    lv_bar_set_value(ui_Battery4, BatteryLevel, LV_ANIM_OFF);
    lv_label_set_text(ui_BattValue4, battVal);
    lv_bar_set_value(ui_Battery5, BatteryLevel, LV_ANIM_OFF);
    lv_label_set_text(ui_BattValue5, battVal);
    if (ui_Battery6) {
        lv_bar_set_value(ui_Battery6, BatteryLevel, LV_ANIM_OFF);
        lv_label_set_text(ui_BattValue6, battVal);
    }
    if (ui_Battery7) {
        lv_bar_set_value(ui_Battery7, BatteryLevel, LV_ANIM_OFF);
        lv_label_set_text(ui_BattValue7, battVal);
    }
    if (ui_Battery8) {
        lv_bar_set_value(ui_Battery8, BatteryLevel, LV_ANIM_OFF);
        lv_label_set_text(ui_BattValue8, battVal);
    }

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
    }
    break;

    case ST_UI_HOME:
    {
        if (lv_obj_has_state(ui_lefty, LV_STATE_CHECKED) == 1) {
            touch_disabled = true;
        }

        syncHomeSliderRangesToLimits();

        // Ramp helper
        nowMs = millis();
        if (nowMs - rampMs <= (unsigned long)rampTime && rampEnabled == true) {
            if (rampValue <= maxRamp && encId == activeEncId) {
                ++rampValue;
            }
        } else {
            rampValue   = 1;
            activeEncId = encId;
        }

        // Encoder 1 — Speed
        bool changed = false;
        if (lv_slider_is_dragged(ui_homespeedslider) == false) {
            changed = false;
            lv_slider_set_value(ui_homespeedslider, speed, LV_ANIM_OFF);
            if (encoder1.getCount() >= 2) {
                changed = true; speed += rampValue;
                encoder1.setCount(0); rampMs = millis(); encId = 1;
            } else if (encoder1.getCount() <= -2) {
                changed = true; speed -= rampValue;
                encoder1.setCount(0); rampMs = millis(); encId = 1;
            }
            if (speed < 0)          { changed = true; speed = 0; }
            if (speed > speedlimit) { changed = true; speed = speedlimit; }
            if (changed) { SendCommand(SPEED, speed, OSSM_ID); }
        } else if (lv_slider_get_value(ui_homespeedslider) != speed) {
            speed = lv_slider_get_value(ui_homespeedslider);
            SendCommand(SPEED, speed, OSSM_ID);
        }
        char speed_v[7]; dtostrf(speed, 6, 0, speed_v);
        lv_label_set_text(ui_homespeedvalue, speed_v);

        // Encoder 2 — Depth
        if (lv_slider_is_dragged(ui_homedepthslider) == false) {
            changed = false;
            lv_slider_set_value(ui_homedepthslider, depth, LV_ANIM_OFF);
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
            if (depth < 0)            { changed = true; depth = 0; }
            if (depth > maxdepthinmm) { changed = true; depth = maxdepthinmm; }
            if (changed) {
                SendCommand(DEPTH,  depth,  OSSM_ID);
                SendCommand(STROKE, stroke, OSSM_ID);
            }
        } else if (lv_slider_get_value(ui_homedepthslider) != depth) {
            depth = lv_slider_get_value(ui_homedepthslider);
            SendCommand(DEPTH,  depth,  OSSM_ID);
            SendCommand(STROKE, stroke, OSSM_ID);
        }
        char depth_v[7]; dtostrf(depth, 6, 0, depth_v);
        lv_label_set_text(ui_homedepthvalue, depth_v);

        // Encoder 3 — Stroke
        if (lv_slider_is_dragged(ui_homestrokeslider) == false) {
            changed = false;
            lv_bar_set_start_value(ui_homestrokeslider, depth - stroke, LV_ANIM_OFF);
            lv_slider_set_value(ui_homestrokeslider, depth, LV_ANIM_OFF);
            bool invertStroke = ui_strokeinvert && lv_obj_has_state(ui_strokeinvert, LV_STATE_CHECKED);
            if (encoder3.getCount() >= 2) {
                changed = true; stroke += invertStroke ? rampValue : -rampValue;
                encoder3.setCount(0); rampMs = millis(); encId = 3;
            } else if (encoder3.getCount() <= -2) {
                changed = true; stroke += invertStroke ? -rampValue : rampValue;
                encoder3.setCount(0); rampMs = millis(); encId = 3;
            }
            if (stroke < 0)            { changed = true; stroke = 0; }
            if (stroke > maxdepthinmm) { changed = true; stroke = maxdepthinmm; }
            if (changed) {
                SendCommand(STROKE, stroke, OSSM_ID);
                SendCommand(DEPTH,  depth,  OSSM_ID);
            }
        } else if (lv_slider_get_left_value(ui_homestrokeslider) != depth - stroke) {
            stroke = depth - lv_slider_get_left_value(ui_homestrokeslider);
            SendCommand(STROKE, stroke, OSSM_ID);
        } else if (lv_slider_get_value(ui_homestrokeslider) != depth) {
            depth = lv_slider_get_value(ui_homestrokeslider);
            SendCommand(DEPTH, depth, OSSM_ID);
            SendCommand(STROKE, stroke, OSSM_ID);
        }
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
            if (sensation < -100) { changed = true; sensation = -100; }
            if (sensation >  100) { changed = true; sensation =  100; }
            if (changed) { SendCommand(SENSATION, sensation, OSSM_ID); }
        } else if (lv_slider_get_value(ui_homesensationslider) != sensation) {
            sensation = lv_slider_get_value(ui_homesensationslider);
            SendCommand(SENSATION, sensation, OSSM_ID);
        }

        if (click2_short_waspressed) {
            lv_obj_send_event(ui_HomeButtonL, LV_EVENT_CLICKED, NULL);
        } else if (mxclick_short_waspressed) {
            lv_obj_send_event(ui_HomeButtonM, LV_EVENT_CLICKED, NULL);
        } else if (click3_short_waspressed) {
            lv_obj_send_event(ui_HomeButtonR, LV_EVENT_CLICKED, NULL);
        } else if (click3_long_waspressed) {
            sensation = 0;
            SendCommand(SENSATION, sensation, OSSM_ID);
        } else if (click3_double_waspressed) {
            // Original code used if/else instead of ! operator to avoid a reported M5 crash
            if (dynamicStroke == false) {
                dynamicStroke = true;
            } else {
                dynamicStroke = false;
            }
            if (stroke >= depth) stroke = depth;
        }
    }
    break;

    case ST_UI_MENU:
    {
        if (lv_obj_has_state(ui_lefty, LV_STATE_CHECKED) == 1) {
            touch_disabled = true;
        }
        if (encoder4.getCount() > encoder4_enc + 2) {
            lv_group_focus_next(ui_g_menu);
            encoder4_enc = encoder4.getCount();
        } else if (encoder4.getCount() < encoder4_enc - 2) {
            lv_group_focus_prev(ui_g_menu);
            encoder4_enc = encoder4.getCount();
        }
        if (click2_short_waspressed) {
            lv_obj_send_event(ui_MenuButtonL, LV_EVENT_SHORT_CLICKED, NULL);
        } else if (mxclick_short_waspressed) {
            lv_obj_send_event(ui_MenuButtonM, LV_EVENT_SHORT_CLICKED, NULL);
        } else if (click3_short_waspressed) {
            lv_obj_send_event(lv_group_get_focused(ui_g_menu), LV_EVENT_SHORT_CLICKED, NULL);
        } else if (click3_long_waspressed) {
            SendCommand(REBOOT, 0, OSSM_ID);
        }
    }
    break;

    case ST_UI_STROKE:
    {
        if (lv_obj_has_state(ui_lefty, LV_STATE_CHECKED) == 1) {
            touch_disabled = true;
        }
        bool changed = false;

        // Encoder 1 — Speed
        if (ui_StrokeSpeedSlider && lv_slider_is_dragged(ui_StrokeSpeedSlider) == false) {
            changed = false;
            lv_slider_set_value(ui_StrokeSpeedSlider, speed, LV_ANIM_OFF);
            if (encoder1.getCount() >= 2) {
                changed = true; speed += 1;
                encoder1.setCount(0);
            } else if (encoder1.getCount() <= -2) {
                changed = true; speed -= 1;
                encoder1.setCount(0);
            }
            if (speed < 0)          { changed = true; speed = 0; }
            if (speed > speedlimit) { changed = true; speed = speedlimit; }
            if (changed) {
                char sv[7]; dtostrf(speed, 6, 0, sv);
                if (ui_StrokeSpeedValue) lv_label_set_text(ui_StrokeSpeedValue, sv);
                SendCommand(SPEED, speed, OSSM_ID);
            }
        } else if (ui_StrokeSpeedSlider && lv_slider_get_value(ui_StrokeSpeedSlider) != (int)speed) {
            speed = lv_slider_get_value(ui_StrokeSpeedSlider);
            SendCommand(SPEED, speed, OSSM_ID);
        }

        // Encoder 2 — Stroke depth
        if (ui_StrokeStrokeSlider && lv_slider_is_dragged(ui_StrokeStrokeSlider) == false) {
            changed = false;
            lv_slider_set_value(ui_StrokeStrokeSlider, depth, LV_ANIM_OFF);
            if (encoder2.getCount() >= 2) {
                changed = true; depth += 1;
                encoder2.setCount(0);
            } else if (encoder2.getCount() <= -2) {
                changed = true; depth -= 1;
                encoder2.setCount(0);
            }
            if (depth < 0)            { changed = true; depth = 0; }
            if (depth > maxdepthinmm) { changed = true; depth = maxdepthinmm; }
            if (changed) {
                char dv[7]; dtostrf(depth, 6, 0, dv);
                if (ui_StrokeStrokeValue) lv_label_set_text(ui_StrokeStrokeValue, dv);
                SendCommand(DEPTH, depth, OSSM_ID);
            }
        } else if (ui_StrokeStrokeSlider && lv_slider_get_value(ui_StrokeStrokeSlider) != (int)depth) {
            depth = lv_slider_get_value(ui_StrokeStrokeSlider);
            SendCommand(DEPTH, depth, OSSM_ID);
        }

        // Encoder 4 — Sensation
        if (ui_StrokeSensationSlider && lv_slider_is_dragged(ui_StrokeSensationSlider) == false) {
            changed = false;
            lv_slider_set_value(ui_StrokeSensationSlider, sensation, LV_ANIM_OFF);
            if (encoder4.getCount() >= 2) {
                changed = true; sensation += 2;
                encoder4.setCount(0);
            } else if (encoder4.getCount() <= -2) {
                changed = true; sensation -= 2;
                encoder4.setCount(0);
            }
            if (sensation < -100) { changed = true; sensation = -100; }
            if (sensation >  100) { changed = true; sensation =  100; }
            if (changed) {
                char sv[7]; dtostrf(sensation, 6, 0, sv);
                if (ui_StrokeSensationValue) lv_label_set_text(ui_StrokeSensationValue, sv);
                SendCommand(SENSATION, sensation, OSSM_ID);
            }
        } else if (ui_StrokeSensationSlider && lv_slider_get_value(ui_StrokeSensationSlider) != (int)sensation) {
            sensation = lv_slider_get_value(ui_StrokeSensationSlider);
            SendCommand(SENSATION, sensation, OSSM_ID);
        }

        if (click2_short_waspressed) {
            _ui_screen_change(ui_Menu, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
        } else if (mxclick_short_waspressed) {
            homebuttonmevent(NULL);
            refreshStrokeStartStopUi();
        } else if (click3_short_waspressed) {
            _ui_screen_change(ui_Pattern, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
        }
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
        if (click2_short_waspressed) {
            lv_obj_send_event(ui_StreamingButtonL, LV_EVENT_SHORT_CLICKED, NULL);
        } else if (mxclick_short_waspressed) {
            lv_obj_send_event(ui_StreamingButtonM, LV_EVENT_SHORT_CLICKED, NULL);
        } else if (click3_short_waspressed) {
            lv_obj_send_event(ui_StreamingButtonR, LV_EVENT_SHORT_CLICKED, NULL);
        }
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
            addonsActivateSelection();
        }
    }
    break;


    case ST_UI_PATTERN:
    {
        if (lv_obj_has_state(ui_lefty, LV_STATE_CHECKED) == 1) {
            touch_disabled = true;
        }
        if (encoder4.getCount() > encoder4_enc + 2) {
            LogDebug("next");
            uint32_t t = LV_KEY_DOWN;
            lv_obj_send_event(ui_PatternS, LV_EVENT_KEY, &t);
            encoder4_enc = encoder4.getCount();
        } else if (encoder4.getCount() < encoder4_enc - 2) {
            uint32_t t = LV_KEY_UP;
            lv_obj_send_event(ui_PatternS, LV_EVENT_KEY, &t);
            LogDebug("Preview");
            encoder4_enc = encoder4.getCount();
        }
        if (click2_short_waspressed) {
            lv_obj_send_event(ui_PatternButtonL, LV_EVENT_CLICKED, NULL);
        } else if (mxclick_short_waspressed) {
            lv_obj_send_event(ui_PatternButtonM, LV_EVENT_CLICKED, NULL);
        } else if (click3_short_waspressed) {
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
        addonsHandleEjectScreen();
        if (lv_obj_has_state(ui_lefty, LV_STATE_CHECKED) == 1) {
            touch_disabled = true;
        }
        if (click2_short_waspressed) {
            lv_obj_send_event(ui_EJECTButtonL, LV_EVENT_CLICKED, NULL);
        } else if (mxclick_short_waspressed) {
            lv_obj_send_event(ui_EJECTButtonM, LV_EVENT_CLICKED, NULL);
        } else if (click3_short_waspressed) {
            lv_obj_send_event(ui_EJECTButtonR, LV_EVENT_CLICKED, NULL);
        }
    }
    break;

    case ST_UI_FISTIT:
    {
        addonsHandleFistScreen();
        if (click2_short_waspressed) {
            addonsFistToggle();
        } else if (mxclick_short_waspressed) {
            _ui_screen_change(ui_Home, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
        } else if (click3_short_waspressed) {
            _ui_screen_change(ui_Menu, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
        }
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
            LogDebug("next");
            if (ui_g_settings) lv_group_focus_next(ui_g_settings);
            encoder4_enc = encoder4.getCount();
        } else if (encoder4.getCount() < encoder4_enc - 2) {
            if (ui_g_settings) lv_group_focus_prev(ui_g_settings);
            LogDebug("Preview");
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
    click3_short_waspressed  = false;
    click3_long_waspressed   = false;
    click3_double_waspressed = false;
}
