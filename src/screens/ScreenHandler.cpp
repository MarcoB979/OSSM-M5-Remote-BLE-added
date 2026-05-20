#include "ScreenHandler.h"
#include <M5Unified.h>
#include <lvgl.h>
#include <Preferences.h>
#include "../ui/ui.h"
#include "../main.h"
#include "../debug.h"
#include "../config_pins.h"
#include "../config_ids.h"
#include "../PatternMath.h"
#include "../buttonhandlers/ButtonHandlers.h"
#include "../communication/EspNowComm.h"

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
long encoder4_enc = 0;

int  pattern = 2;
char patternstr[20];

bool dynamicStroke  = false;
bool eject_status   = false;
bool vibrate_mode   = true;
bool touch_home     = false;
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

// -------------------------------------------------------
// screenInit() — load NVS settings and apply to UI
// Call after ui_init() and buttonInit()
// -------------------------------------------------------
void screenInit() {
    Preferences prefs;
    prefs.begin("m5-ctnr", false);
    eject_status = prefs.getBool("ejectAddon", false);
    dark_mode    = prefs.getBool("Darkmode",   true);
    vibrate_mode = prefs.getBool("Vibrate",    true);
    touch_home   = prefs.getBool("Lefty",      false);
    prefs.end();

    if (eject_status) {
        lv_obj_add_state(ui_ejectaddon,           LV_STATE_CHECKED);
        lv_obj_clear_state(ui_EJECTSettingButton, LV_STATE_DISABLED);
        lv_obj_clear_state(ui_HomeButtonL,        LV_STATE_DISABLED);
    }
    if (dark_mode)    { lv_obj_add_state(ui_darkmode, LV_STATE_CHECKED); }
    if (vibrate_mode) { lv_obj_add_state(ui_vibrate,  LV_STATE_CHECKED); }
    if (touch_home)   { lv_obj_add_state(ui_lefty,    LV_STATE_CHECKED); }

    lv_roller_set_selected(ui_PatternS, 2, LV_ANIM_OFF);
    lv_roller_get_selected_str(ui_PatternS, patternstr, 0);
    lv_label_set_text(ui_HomePatternLabel, patternstr);
}

// -------------------------------------------------------
// Screen event callbacks (registered via ui.c / ui.h)
// -------------------------------------------------------

void screenmachine(lv_event_t * e) {
    if (lv_scr_act() == ui_Start) {
        st_screens = ST_UI_START;
    } else if (lv_scr_act() == ui_Home) {
        st_screens = ST_UI_HOME;
        speed = lv_slider_get_value(ui_homespeedslider);
        LogDebug(speedenc);
        LogDebug(speed);
        lv_slider_set_range(ui_homedepthslider, 0, maxdepthinmm);
        lv_slider_set_range(ui_homestrokeslider, 0, maxdepthinmm);
    } else if (lv_scr_act() == ui_Menue) {
        st_screens = ST_UI_MENUE;
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

    if (lv_obj_has_state(ui_ejectaddon, LV_STATE_CHECKED) == 1) {
        prefs.putBool("ejectAddon", true);
    } else {
        prefs.putBool("ejectAddon", false);
    }

    bool theme_Change_Previous = prefs.getBool("Darkmode", true);
    bool theme_Change_New      = false;

    if (lv_obj_has_state(ui_darkmode, LV_STATE_CHECKED) == 1) {
        theme_Change_New = true;
        prefs.putBool("Darkmode", true);
    } else {
        theme_Change_New = false;
        prefs.putBool("Darkmode", false);
    }

    prefs.end();
    delay(100);

    if (theme_Change_Previous != theme_Change_New) {
        vibrate(225, 75);
        ESP.restart();
    } else {
        vibrate(225, 75);
    }
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
            if (encoder3.getCount() >= 2) {
                changed = true; stroke -= rampValue;
                encoder3.setCount(0); rampMs = millis(); encId = 3;
            } else if (encoder3.getCount() <= -2) {
                changed = true; stroke += rampValue;
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
            SendCommand(DEPTH, depth, OSSM_ID);
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

    case ST_UI_MENUE:
    {
        if (lv_obj_has_state(ui_lefty, LV_STATE_CHECKED) == 1) {
            touch_disabled = true;
        }
        if (encoder4.getCount() > encoder4_enc + 2) {
            LogDebug("next");
            lv_group_focus_next(ui_g_menue);
            encoder4_enc = encoder4.getCount();
        } else if (encoder4.getCount() < encoder4_enc - 2) {
            lv_group_focus_prev(ui_g_menue);
            LogDebug("Preview");
            encoder4_enc = encoder4.getCount();
        }
        if (click2_short_waspressed) {
            lv_obj_send_event(ui_MenueButtonL, LV_EVENT_CLICKED, NULL);
        } else if (mxclick_short_waspressed) {
            lv_obj_send_event(ui_MenueButtonM, LV_EVENT_CLICKED, NULL);
        } else if (click3_short_waspressed) {
            lv_obj_send_event(lv_group_get_focused(ui_g_menue), LV_EVENT_CLICKED, NULL);
        } else if (click3_long_waspressed) {
            SendCommand(REBOOT, 0, OSSM_ID);
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
        if (lv_obj_has_state(ui_lefty, LV_STATE_CHECKED) == 1) {
            touch_disabled = true;
        }
        if (click2_short_waspressed) {
            lv_obj_send_event(ui_EJECTButtonL, LV_EVENT_CLICKED, NULL);
        } else if (mxclick_short_waspressed) {
            lv_obj_send_event(ui_EJECTButtonM, LV_EVENT_CLICKED, NULL);
        }
        // click3_short: intentionally empty
    }
    break;

    case ST_UI_SETTINGS:
    {
        touch_disabled = false;
        if (encoder4.getCount() > encoder4_enc + 2) {
            LogDebug("next");
            lv_group_focus_next(ui_g_settings);
            encoder4_enc = encoder4.getCount();
        } else if (encoder4.getCount() < encoder4_enc - 2) {
            lv_group_focus_prev(ui_g_settings);
            LogDebug("Preview");
            encoder4_enc = encoder4.getCount();
        }
        if (click2_short_waspressed) {
            lv_obj_send_event(ui_MenueButtonL, LV_EVENT_CLICKED, NULL);
        } else if (mxclick_short_waspressed) {
            lv_obj_send_event(ui_MenueButtonM, LV_EVENT_CLICKED, NULL);
        } else if (click3_short_waspressed) {
            lv_obj_send_event(ui_EJECTButtonR, LV_EVENT_CLICKED, NULL);
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
