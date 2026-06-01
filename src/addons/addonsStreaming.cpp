#include "addons/addonsStreaming.h"

#include "ui/ui.h"
#include "ui/ui_helpers.h"
#include "language.h"
#include "display/styles.h"
#include "main.h"
#include "config/config_ids.h"
#include "buttonhandlers/ButtonHandlers.h"
#include "addons/Eject.h"
#include "addons/FistIT.h"
#include "communication/EspNowComm.h"
#include "communication/BleComm.h"
#include <Preferences.h>

// Extra globals exposed through ui.h
lv_obj_t *ui_StreamingButtonM = nullptr;
lv_obj_t *ui_Batt8 = nullptr;
lv_obj_t *ui_BattValue8 = nullptr;
lv_obj_t *ui_Battery8 = nullptr;
lv_group_t *ui_g_addons = nullptr;
lv_obj_t *ui_FistIT = nullptr;

static lv_obj_t *s_streaming_btn_l_text = nullptr;
static lv_obj_t *s_streaming_btn_m_text = nullptr;
static lv_obj_t *s_streaming_btn_r_text = nullptr;
static lv_obj_t *s_streaming_speed_val = nullptr;
static lv_obj_t *s_streaming_depth_val = nullptr;
static lv_obj_t *s_streaming_stroke_val = nullptr;
static lv_obj_t *s_streaming_sensation_val = nullptr;
static bool s_streaming_paused = false;

static lv_obj_t *s_addons_btn_l_text = nullptr;
static lv_obj_t *s_addons_btn_m_text = nullptr;
static lv_obj_t *s_addons_btn_r_text = nullptr;
static lv_obj_t *s_addons_item0_text = nullptr;
static lv_obj_t *s_addons_item1_text = nullptr;
static lv_obj_t *s_addons_item2_text = nullptr;
static int s_addons_selected = 0;

// ── Addon registry ───────────────────────────────────────────────────────────
// Each entry: display name, enabled flag, and activation callback.
// New addons can be appended here; the carousel handles any count >= 1.
struct AddonDef {
    const char* name;
    bool        enabled;
    void        (*activate)();
};


//for new addons: add a line here with the code what screen to activate when the addon is selected in the menu, and a default enabled/disabled state. The screen activation code should be a function that prepares the screen (e.g. updates slider values from current addon state) and then calls _ui_screen_change() to switch to it.
static void activateEject()     { _ui_screen_change(ui_EJECTSettings,  LV_SCR_LOAD_ANIM_FADE_ON, 20, 0); }
static void activateFistIT()    { FistITPrepareScreen(); _ui_screen_change(FistITGetScreen(), LV_SCR_LOAD_ANIM_FADE_ON, 20, 0); }
static void activateStreaming() { _ui_screen_change(ui_Streaming,       LV_SCR_LOAD_ANIM_FADE_ON, 20, 0); }

//new addons should be defined here with their screen activation function, and the function should be implemented above. The screen they activate should also be added to ui.h and ui.c, and implemented in a new .cpp file in the addons folder. See Eject and FistIT for examples.
//new addons should also be added to the language file with T_ADDONS_NAME, and to the ui with a button in the addons carousel and an event handler that calls the appropriate activate function above. See ui.c for examples.
static AddonDef s_addon_defs[] = {
    { "Eject",     true, activateEject     },
    { "Fist-IT",   true, activateFistIT    },
    { "Streaming", true, activateStreaming },
};
//increment NUM_ADDONS at the top of this file if you add more entries here
static const int NUM_ADDONS = 3;
static const int EJECT_ADDON_INDEX = 0;
static const int FISTIT_ADDON_INDEX = 1;

static bool s_addons_manage_mode = false;  // true = visibility-management mode
static int  s_addons_offset      = 0;      // index of first visible item in carousel
static int  s_enabled_indices[3];          // subset of s_addon_defs[] that are enabled
static int  s_enabled_count      = 0;

static lv_obj_t *s_fist_btn_l = nullptr;
static lv_obj_t *s_fist_btn_m = nullptr;
static lv_obj_t *s_fist_btn_r = nullptr;
static lv_obj_t *s_fist_btn_l_text = nullptr;
static lv_obj_t *s_fist_speed_slider = nullptr;
static lv_obj_t *s_fist_rot_slider = nullptr;
static lv_obj_t *s_fist_pause_slider = nullptr;
static lv_obj_t *s_fist_accel_slider = nullptr;
static lv_obj_t *s_fist_speed_val = nullptr;
static lv_obj_t *s_fist_rot_val = nullptr;
static lv_obj_t *s_fist_pause_val = nullptr;
static lv_obj_t *s_fist_accel_val = nullptr;
static bool s_fist_on = false;
static float s_fist_speed = 0.0f;
static float s_fist_rot = 0.0f;
static float s_fist_pause = 0.0f;
static float s_fist_accel = 0.0f;

static lv_obj_t *s_eject_speed_slider = nullptr;
static lv_obj_t *s_eject_time_slider = nullptr;
static lv_obj_t *s_eject_size_slider = nullptr;
static lv_obj_t *s_eject_accel_slider = nullptr;
static lv_obj_t *s_eject_speed_val = nullptr;
static lv_obj_t *s_eject_time_val = nullptr;
static lv_obj_t *s_eject_size_val = nullptr;
static lv_obj_t *s_eject_accel_val = nullptr;
static bool s_eject_ui_ready = false;
static bool s_eject_on = false;
static float s_eject_speed = 0.0f;
static float s_eject_time = 0.0f;
static float s_eject_size = 0.0f;
static float s_eject_accel = 0.0f;

static long s_fist_enc1 = 0;
static long s_fist_enc2 = 0;
static long s_fist_enc3 = 0;
static long s_fist_enc4 = 0;
static long s_eject_enc1 = 0;
static long s_eject_enc2 = 0;
static long s_eject_enc3 = 0;
static long s_eject_enc4 = 0;

static int detentsFromEncoder(ESP32Encoder &enc, long *state) {
    if (!state) return 0;
    long count = enc.getCount();
    if (count >= *state + 2) {
        *state = count;
        return 1;
    }
    if (count <= *state - 2) {
        *state = count;
        return -1;
    }
    return 0;
}

static void setSliderLabelInt(lv_obj_t *label, float value) {
    if (!label) return;
    lv_label_set_text_fmt(label, "%d", (int)(value + 0.5f));
}

static void applyAddonRowStyle(lv_obj_t *row, int slot) {
    if (!row) return;
    int idx = slot;
    if (idx < 0) idx = 0;
    if (idx > 3) idx = 3;
    lv_obj_add_style(row, &style_slider_indicator[idx], LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(row, &style_button_m_focused, LV_PART_MAIN | LV_STATE_FOCUSED);
}

static void applyButtonStylesLocal(lv_obj_t *obj, lv_style_t *defaultStyle, lv_style_t *focusedStyle) {
    if (!obj || !defaultStyle) return;
    lv_obj_add_style(obj, defaultStyle, LV_PART_MAIN | LV_STATE_DEFAULT);
    if (focusedStyle) {
        lv_obj_add_style(obj, focusedStyle, LV_PART_MAIN | LV_STATE_FOCUSED);
    }
}
// ── NVS persistence ───────────────────────────────────────────────────────────
// Opens the "addons" namespace, writes defaults for any key not yet stored,
// and reads the current enabled state into s_addon_defs[].
static void loadAddonPrefs() {
    Preferences prefs;
    prefs.begin("addons", false);  // r/w so we can write first-run defaults in one pass
    for (int i = 0; i < NUM_ADDONS; i++) {
        char key[12];
        snprintf(key, sizeof(key), "addon_%d", i);
        if (!prefs.isKey(key)) prefs.putBool(key, true);  // first-run default = enabled
        s_addon_defs[i].enabled = prefs.getBool(key, true);
    }
    prefs.end();
}

static void saveAddonEnabled(int addonIdx) {
    if (addonIdx < 0 || addonIdx >= NUM_ADDONS) return;
    Preferences prefs;
    prefs.begin("addons", false);
    char key[12];
    snprintf(key, sizeof(key), "addon_%d", addonIdx);
    prefs.putBool(key, s_addon_defs[addonIdx].enabled);
    prefs.end();
}

static void buildEnabledList() {
    s_enabled_count = 0;
    for (int i = 0; i < NUM_ADDONS; i++) {
        if (s_addon_defs[i].enabled) s_enabled_indices[s_enabled_count++] = i;
    }
}

bool addonsIsFistITEnabled(void) {
    // Read persisted value directly so this works even before Addons screen loads.
    Preferences prefs;
    prefs.begin("addons", true);
    bool enabled = prefs.getBool("addon_1", s_addon_defs[FISTIT_ADDON_INDEX].enabled);
    prefs.end();
    return enabled;
}

bool addonsIsEjectEnabled(void) {
    // Read persisted value directly so this works even before Addons screen loads.
    Preferences prefs;
    prefs.begin("addons", true);
    bool enabled = prefs.getBool("addon_0", s_addon_defs[EJECT_ADDON_INDEX].enabled);
    prefs.end();
    return enabled;
}


// ── Manage-mode enter / exit ──────────────────────────────────────────────────
static void enterManageMode() {
    s_addons_manage_mode = true;
    s_addons_selected    = 0;
    s_addons_offset      = 0;
    if (s_addons_btn_r_text) lv_label_set_text(s_addons_btn_r_text, "Show All");
}

static void exitManageMode() {
    s_addons_manage_mode = false;
    s_addons_selected    = 0;
    s_addons_offset      = 0;
    if (s_addons_btn_r_text) lv_label_set_text(s_addons_btn_r_text, "Enable/Disable");
}
static void refresh_addons_labels() {
    lv_obj_t *rows[3]  = { ui_AddonsItem0, ui_AddonsItem1, ui_AddonsItem2 };
    lv_obj_t *texts[3] = { s_addons_item0_text, s_addons_item1_text, s_addons_item2_text };
    const int count = s_addons_manage_mode ? NUM_ADDONS : s_enabled_count;

    for (int row = 0; row < 3; row++) {
        if (!rows[row]) continue;
        int listPos = s_addons_offset + row;
        if (listPos < count) {
            lv_obj_remove_flag(rows[row], LV_OBJ_FLAG_HIDDEN);
            int ai  = s_addons_manage_mode ? listPos : s_enabled_indices[listPos];
            bool sel = (s_addons_selected == listPos);
            char buf[40];
            if (s_addons_manage_mode) {
                snprintf(buf, sizeof(buf), "%s%s (%s)",
                    sel ? "> " : "  ",
                    s_addon_defs[ai].name,
                    s_addon_defs[ai].enabled ? "show" : "hide");
            } else {
                snprintf(buf, sizeof(buf), "%s%s",
                    sel ? "> " : "  ",
                    s_addon_defs[ai].name);
            }
            if (texts[row]) lv_label_set_text(texts[row], buf);
        } else {
            lv_obj_add_flag(rows[row], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void addonsSyncSelectionVisual(void) {
    buildEnabledList();
    const int count = s_addons_manage_mode ? NUM_ADDONS : s_enabled_count;

    if (count == 0) {
        if (ui_AddonsItem0) lv_obj_add_flag(ui_AddonsItem0, LV_OBJ_FLAG_HIDDEN);
        if (ui_AddonsItem1) lv_obj_add_flag(ui_AddonsItem1, LV_OBJ_FLAG_HIDDEN);
        if (ui_AddonsItem2) lv_obj_add_flag(ui_AddonsItem2, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    // Clamp selected to valid range
    if (s_addons_selected < 0)      s_addons_selected = 0;
    if (s_addons_selected >= count) s_addons_selected = count - 1;

    // Scroll carousel so the selected item is always visible
    if (s_addons_selected < s_addons_offset)        s_addons_offset = s_addons_selected;
    if (s_addons_selected >= s_addons_offset + 3)   s_addons_offset = s_addons_selected - 2;
    if (s_addons_offset < 0) s_addons_offset = 0;

    // Focus the row button that corresponds to the selected item
    if (ui_g_addons) {
        int focusRow = s_addons_selected - s_addons_offset;
        lv_obj_t *focusTarget = nullptr;
        if      (focusRow == 0 && ui_AddonsItem0) focusTarget = ui_AddonsItem0;
        else if (focusRow == 1 && ui_AddonsItem1) focusTarget = ui_AddonsItem1;
        else if (focusRow == 2 && ui_AddonsItem2) focusTarget = ui_AddonsItem2;
        if (focusTarget) lv_group_focus_obj(focusTarget);
    }

    refresh_addons_labels();
}

void addonsMoveSelection(int delta) {
    if (delta == 0) return;
    const int count = s_addons_manage_mode ? NUM_ADDONS : s_enabled_count;
    if (count == 0) return;
    s_addons_selected += delta;
    while (s_addons_selected < 0)      s_addons_selected += count;
    while (s_addons_selected >= count) s_addons_selected -= count;
    addonsSyncSelectionVisual();
}

static void fistUpdateValues() {
    setSliderLabelInt(s_fist_speed_val, s_fist_speed);
    setSliderLabelInt(s_fist_rot_val, s_fist_rot);
    setSliderLabelInt(s_fist_pause_val, s_fist_pause);
    setSliderLabelInt(s_fist_accel_val, s_fist_accel);
    if (s_fist_btn_l_text) lv_label_set_text(s_fist_btn_l_text, s_fist_on ? T_PAUSE : T_START);
}

static void ejectUpdateValues() {
    setSliderLabelInt(s_eject_speed_val, s_eject_speed);
    setSliderLabelInt(s_eject_time_val, s_eject_time);
    setSliderLabelInt(s_eject_size_val, s_eject_size);
    setSliderLabelInt(s_eject_accel_val, s_eject_accel);
    if (ui_EJECTButtonMText) lv_label_set_text(ui_EJECTButtonMText, s_eject_on ? T_PAUSE : T_CUM);
}

static void createSliderRow(lv_obj_t *parent,
                            lv_obj_t **rowLabel,
                            lv_obj_t **rowSlider,
                            lv_obj_t **rowValue,
                            const char *labelText,
                            int y,
                            int minValue,
                            int maxValue,
                            int slot) {
    *rowLabel = lv_label_create(parent);
    lv_obj_set_width(*rowLabel, lv_pct(95));
    lv_obj_set_height(*rowLabel, LV_SIZE_CONTENT);
    lv_obj_set_x(*rowLabel, 0);
    lv_obj_set_y(*rowLabel, y);
    lv_obj_set_align(*rowLabel, LV_ALIGN_CENTER);
    lv_label_set_text(*rowLabel, labelText);
    lv_obj_set_style_text_font(*rowLabel, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(*rowLabel, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    *rowSlider = lv_slider_create(*rowLabel);
    lv_slider_set_range(*rowSlider, minValue, maxValue);
    lv_slider_set_value(*rowSlider, minValue, LV_ANIM_OFF);
    lv_obj_set_width(*rowSlider, 130);
    lv_obj_set_height(*rowSlider, 10);
    lv_obj_set_x(*rowSlider, -15);
    lv_obj_set_y(*rowSlider, 0);
    lv_obj_set_align(*rowSlider, LV_ALIGN_RIGHT_MID);
    lv_obj_add_style(*rowSlider, &style_slider_track[slot], LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(*rowSlider, &style_slider_indicator[slot], LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_add_style(*rowSlider, &style_slider_indicator[slot], LV_PART_KNOB | LV_STATE_DEFAULT);

    *rowValue = lv_label_create(*rowLabel);
    lv_obj_set_width(*rowValue, LV_SIZE_CONTENT);
    lv_obj_set_height(*rowValue, LV_SIZE_CONTENT);
    lv_obj_set_x(*rowValue, 85);
    lv_obj_set_y(*rowValue, 0);
    lv_obj_set_align(*rowValue, LV_ALIGN_LEFT_MID);
    lv_label_set_text(*rowValue, "0");
    lv_obj_add_style(*rowValue, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(*rowValue, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void fistToggle() {
    ButtonEvents events = { true, false, false };
    FistITHandleScreen(events);
}

static void ejectToggle() {
    ButtonEvents events = { true, false, false };
    EjectHandleScreen(events);
}

static lv_timer_t *s_streaming_cmd_timer = NULL;
static lv_timer_t *s_streaming_init_timer = NULL;
static bool s_streaming_init_completed = false;

enum StreamingInitPhase {
    STREAM_INIT_IDLE = 0,
    STREAM_INIT_SEND_DEPTH,
    STREAM_INIT_SEND_STROKE,
    STREAM_INIT_SEND_SENSATION,
    STREAM_INIT_RAMP_SPEED_SLOW,
    STREAM_INIT_RAMP_SPEED_FAST,
    STREAM_INIT_SEND_DEPTH_MAX,
    STREAM_INIT_SEND_STROKE_MAX,
    STREAM_INIT_SEND_STREAM_MOVE,
};

static StreamingInitPhase s_streaming_init_phase = STREAM_INIT_IDLE;
static int s_streaming_ramp_speed = 0;

static void stop_streaming_init_sequence() {
    if (s_streaming_init_timer) {
        lv_timer_delete(s_streaming_init_timer);
        s_streaming_init_timer = NULL;
    }
    s_streaming_init_phase = STREAM_INIT_IDLE;
    s_streaming_ramp_speed = 0;
}

static void streaming_init_sequence_cb(lv_timer_t *t) {
    (void)t;

    // Safety: if user already left the streaming screen, stop all pending sends.
    if (lv_scr_act() != ui_Streaming) {
        stop_streaming_init_sequence();
        return;
    }

    switch (s_streaming_init_phase) {
        case STREAM_INIT_SEND_DEPTH:
            SendCommand(DEPTH, 0.0f, OSSM_ID);
            s_streaming_init_phase = STREAM_INIT_SEND_STROKE;
            lv_timer_set_period(s_streaming_init_timer, 200);
            break;

        case STREAM_INIT_SEND_STROKE:
            SendCommand(STROKE, 0.0f, OSSM_ID);
            s_streaming_init_phase = STREAM_INIT_SEND_SENSATION;
            lv_timer_set_period(s_streaming_init_timer, 200);
            break;

        case STREAM_INIT_SEND_SENSATION:
            SendCommand(SENSATION, 50.0f, OSSM_ID);
            SendCommand(SPEED, 0.0f, OSSM_ID);
            s_streaming_ramp_speed = 0;
            s_streaming_init_phase = STREAM_INIT_RAMP_SPEED_FAST;
            lv_timer_set_period(s_streaming_init_timer, 500);
            break;

        case STREAM_INIT_RAMP_SPEED_FAST:
            if (s_streaming_ramp_speed < 25) {
                s_streaming_ramp_speed = 25;
            } else {
                s_streaming_ramp_speed += 25;
            }
            if (s_streaming_ramp_speed > 100) s_streaming_ramp_speed = 100;
            SendCommand(SPEED, (float)s_streaming_ramp_speed, OSSM_ID);
            if (s_streaming_ramp_speed >= 100) {
                s_streaming_init_phase = STREAM_INIT_SEND_DEPTH_MAX;
                lv_timer_set_period(s_streaming_init_timer, 200);
            }
            break;

        case STREAM_INIT_SEND_DEPTH_MAX:
            SendCommand(DEPTH, 100.0f, OSSM_ID);
            s_streaming_init_phase = STREAM_INIT_SEND_STROKE_MAX;
            lv_timer_set_period(s_streaming_init_timer, 200);
            break;

        case STREAM_INIT_SEND_STROKE_MAX:
            SendCommand(STROKE, 100.0f, OSSM_ID);
            s_streaming_init_phase = STREAM_INIT_SEND_STREAM_MOVE;
            lv_timer_set_period(s_streaming_init_timer, 500);
            break;

        case STREAM_INIT_SEND_STREAM_MOVE:
            // Move rail to max depth over 5s to finish init sequence.
            bleCommSendStreamCommand(100, 5000);
            s_streaming_init_completed = true;
            stop_streaming_init_sequence();
            break;

        case STREAM_INIT_IDLE:
        default:
            stop_streaming_init_sequence();
            break;
    }
}

static void start_streaming_init_sequence() {
    stop_streaming_init_sequence();
    s_streaming_init_completed = false;
    s_streaming_init_phase = STREAM_INIT_SEND_DEPTH;
    // Required: wait 500ms after go:streaming before sending values.
    s_streaming_init_timer = lv_timer_create(streaming_init_sequence_cb, 500, NULL);
}

static void streaming_goto_streaming_cb(lv_timer_t *t) {
    (void)t;
    bleCommGoToStreaming();
    start_streaming_init_sequence();
    s_streaming_cmd_timer = NULL;
}

void streamingBeginInitSequence() {
    s_streaming_init_completed = false;
    bleCommGoToMenu();
    stop_streaming_init_sequence();
    if (s_streaming_cmd_timer) {
        lv_timer_delete(s_streaming_cmd_timer);
        s_streaming_cmd_timer = NULL;
    }
    s_streaming_cmd_timer = lv_timer_create(streaming_goto_streaming_cb, 200, NULL);
    lv_timer_set_repeat_count(s_streaming_cmd_timer, 1);
}

void streamingCancelInitSequence() {
    if (s_streaming_cmd_timer) {
        lv_timer_delete(s_streaming_cmd_timer);
        s_streaming_cmd_timer = NULL;
    }
    stop_streaming_init_sequence();
    s_streaming_init_completed = false;
}

bool streamingConsumeInitCompleted() {
    const bool completed = s_streaming_init_completed;
    s_streaming_init_completed = false;
    return completed;
}

static void event_streaming_screen(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_SCREEN_LOADED) {
        screenmachine(e);
        streamingCancelInitSequence();
    }
    // When leaving the streaming screen, cancel any pending go:streaming timer and
    // put the OSSM back into menu mode.  This is essential because the OSSM ignores
    // "go:strokeEngine" from streaming mode, so we must exit streaming first.
    // Once in menu mode the home/stroke screen's bleCommGoToStrokeEngine() will work
    // correctly and the OSSM will re-home as expected.
    if (lv_event_get_code(e) == LV_EVENT_SCREEN_UNLOAD_START) {
        streamingCancelInitSequence();
        bleCommGoToMenu();
    }
}

void streamingUpdateValueLabels(float spd, float dep, float str, float sen) {
    char buf[8];
    if (s_streaming_speed_val)  { dtostrf(spd, 3, 0, buf); lv_label_set_text(s_streaming_speed_val,  buf); }
    if (s_streaming_depth_val)  { dtostrf(dep, 3, 0, buf); lv_label_set_text(s_streaming_depth_val,  buf); }
    if (s_streaming_stroke_val) { dtostrf(str, 3, 0, buf); lv_label_set_text(s_streaming_stroke_val, buf); }
    if (s_streaming_sensation_val) { dtostrf(sen, 3, 0, buf); lv_label_set_text(s_streaming_sensation_val, buf); }
}

bool streamingIsPaused() { return s_streaming_paused; }

void streamingResetPause() {
    s_streaming_paused = false;
    if (s_streaming_btn_m_text)
        lv_label_set_text(s_streaming_btn_m_text, T_STOP);
}

static void event_streaming_btn_l(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_SHORT_CLICKED) {
        _ui_screen_change(ui_Menu, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    }
}

static void event_streaming_btn_m(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_SHORT_CLICKED) {
        s_streaming_paused = !s_streaming_paused;
        if (s_streaming_btn_m_text)
            lv_label_set_text(s_streaming_btn_m_text, s_streaming_paused ? T_START : T_STOP);
    }
}

static void event_streaming_btn_r(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_SHORT_CLICKED) {
        _ui_screen_change(ui_Addons, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    }
}

static void event_addons_screen(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_SCREEN_LOADED) {
        loadAddonPrefs();
        buildEnabledList();
        s_addons_manage_mode = false;
        s_addons_selected    = 0;
        s_addons_offset      = 0;
        if (s_addons_btn_r_text) lv_label_set_text(s_addons_btn_r_text, "Enable/Disable");
        addonsSyncSelectionVisual();
        screenmachine(e);
    }
}

static void event_addons_back(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_SHORT_CLICKED) {
        _ui_screen_change(ui_Menu, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    }
}

static void event_addons_home(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_SHORT_CLICKED) {
        _ui_screen_change(ui_Home, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    }
}

void addonsActivateSelection(void) {
    if (s_addons_manage_mode) {
        // Toggle the selected addon's visibility and persist immediately
        if (s_addons_selected < 0 || s_addons_selected >= NUM_ADDONS) return;
        s_addon_defs[s_addons_selected].enabled = !s_addon_defs[s_addons_selected].enabled;
        saveAddonEnabled(s_addons_selected);
        buildEnabledList();
        addonsSyncSelectionVisual();
    } else {
        // Launch the selected enabled addon
        if (s_addons_selected < 0 || s_addons_selected >= s_enabled_count) return;
        int ai = s_enabled_indices[s_addons_selected];
        if (s_addon_defs[ai].activate) s_addon_defs[ai].activate();
    }
}

static void event_addons_open(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_SHORT_CLICKED) {
        addonsActivateSelection();
    }
}


static void event_addons_enable(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_SHORT_CLICKED) {
        if (s_addons_manage_mode) exitManageMode();
        else                      enterManageMode();
        addonsSyncSelectionVisual();
    }
}

static void event_addons_item0(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_SHORT_CLICKED) {
        s_addons_selected = s_addons_offset + 0;
        addonsActivateSelection();
    }
}

static void event_addons_item1(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_SHORT_CLICKED) {
        s_addons_selected = s_addons_offset + 1;
        addonsActivateSelection();
    }
}

static void event_addons_item2(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_SHORT_CLICKED) {
        s_addons_selected = s_addons_offset + 2;
        addonsActivateSelection();
    }
}

static void event_fist_screen(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_SCREEN_LOADED) {
        screenmachine(e);
    }
}

static void event_fist_btn_l(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_SHORT_CLICKED) fistToggle();
}

static void event_fist_btn_m(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_SHORT_CLICKED) {
        _ui_screen_change(ui_Home, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    }
}

static void event_fist_btn_r(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_SHORT_CLICKED) {
        _ui_screen_change(ui_Menu, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    }
}

static void event_eject_btn_l(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_SHORT_CLICKED) ejectToggle();
}

static void ensureEjectUiInitialized() {
    // Eject UI is now owned by addons/Eject.cpp.
    s_eject_ui_ready = true;
}

static void createFistScreen() {
    // Fist-IT UI is now owned by addons/FistIT.cpp.
    if (ui_FistIT == nullptr) {
        ui_FistIT = FistITGetScreen();
    }
}

void ui_Streaming_screen_init(void) {
    ui_Streaming = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Streaming, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(ui_Streaming, event_streaming_screen, LV_EVENT_ALL, NULL);

    ui_LogoStreaming = lv_label_create(ui_Streaming);
    lv_obj_set_align(ui_LogoStreaming, LV_ALIGN_TOP_MID);
    lv_obj_set_y(ui_LogoStreaming, 8);
    lv_label_set_text(ui_LogoStreaming, T_SCREEN_STREAMING);
    lv_obj_set_style_text_font(ui_LogoStreaming, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_LogoStreaming, &style_title_bar, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *speed_l = lv_label_create(ui_Streaming);
    lv_obj_set_align(speed_l, LV_ALIGN_LEFT_MID);
    lv_obj_set_x(speed_l, 12);
    lv_obj_set_y(speed_l, -55);
    lv_label_set_text(speed_l, T_SPEED);

    ui_streamingspeedslider = lv_slider_create(ui_Streaming);
    lv_slider_set_range(ui_streamingspeedslider, 0, 100);
    lv_obj_set_size(ui_streamingspeedslider, 160, 18);
    lv_obj_set_align(ui_streamingspeedslider, LV_ALIGN_RIGHT_MID);
    lv_obj_set_x(ui_streamingspeedslider, -16);
    lv_obj_set_y(ui_streamingspeedslider, -55);
    lv_obj_add_style(ui_streamingspeedslider, &style_slider_track[0], LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_streamingspeedslider, &style_slider_indicator[0], LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_streamingspeedslider, &style_slider_indicator[0], LV_PART_KNOB | LV_STATE_DEFAULT);

    s_streaming_speed_val = lv_label_create(ui_Streaming);
    lv_obj_set_width(s_streaming_speed_val, LV_SIZE_CONTENT);
    lv_obj_set_height(s_streaming_speed_val, LV_SIZE_CONTENT);
    lv_obj_set_align(s_streaming_speed_val, LV_ALIGN_LEFT_MID);
    lv_obj_set_x(s_streaming_speed_val, 90);
    lv_obj_set_y(s_streaming_speed_val, -55);
    lv_label_set_text(s_streaming_speed_val, "0");
    lv_obj_add_style(s_streaming_speed_val, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(s_streaming_speed_val, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *depth_l = lv_label_create(ui_Streaming);
    lv_obj_set_align(depth_l, LV_ALIGN_LEFT_MID);
    lv_obj_set_x(depth_l, 12);
    lv_obj_set_y(depth_l, -20);
    lv_label_set_text(depth_l, T_DEPTH);

    ui_streamingdepthslider = lv_slider_create(ui_Streaming);
    lv_slider_set_range(ui_streamingdepthslider, 0, 100);
    lv_obj_set_size(ui_streamingdepthslider, 160, 18);
    lv_obj_set_align(ui_streamingdepthslider, LV_ALIGN_RIGHT_MID);
    lv_obj_set_x(ui_streamingdepthslider, -16);
    lv_obj_set_y(ui_streamingdepthslider, -20);
    lv_obj_add_style(ui_streamingdepthslider, &style_slider_track[1], LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_streamingdepthslider, &style_slider_indicator[1], LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_streamingdepthslider, &style_slider_indicator[1], LV_PART_KNOB | LV_STATE_DEFAULT);

    s_streaming_depth_val = lv_label_create(ui_Streaming);
    lv_obj_set_width(s_streaming_depth_val, LV_SIZE_CONTENT);
    lv_obj_set_height(s_streaming_depth_val, LV_SIZE_CONTENT);
    lv_obj_set_align(s_streaming_depth_val, LV_ALIGN_LEFT_MID);
    lv_obj_set_x(s_streaming_depth_val, 90);
    lv_obj_set_y(s_streaming_depth_val, -20);
    lv_label_set_text(s_streaming_depth_val, "0");
    lv_obj_add_style(s_streaming_depth_val, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(s_streaming_depth_val, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *stroke_l = lv_label_create(ui_Streaming);
    lv_obj_set_align(stroke_l, LV_ALIGN_LEFT_MID);
    lv_obj_set_x(stroke_l, 12);
    lv_obj_set_y(stroke_l, 15);
    lv_label_set_text(stroke_l, T_STROKE);

    ui_streamingstrokeslider = lv_slider_create(ui_Streaming);
    lv_slider_set_range(ui_streamingstrokeslider, 0, 100);
    lv_obj_set_size(ui_streamingstrokeslider, 160, 18);
    lv_obj_set_align(ui_streamingstrokeslider, LV_ALIGN_RIGHT_MID);
    lv_obj_set_x(ui_streamingstrokeslider, -16);
    lv_obj_set_y(ui_streamingstrokeslider, 15);
    lv_obj_add_style(ui_streamingstrokeslider, &style_slider_track[2], LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_streamingstrokeslider, &style_slider_indicator[2], LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_streamingstrokeslider, &style_slider_indicator[2], LV_PART_KNOB | LV_STATE_DEFAULT);

    s_streaming_stroke_val = lv_label_create(ui_Streaming);
    lv_obj_set_width(s_streaming_stroke_val, LV_SIZE_CONTENT);
    lv_obj_set_height(s_streaming_stroke_val, LV_SIZE_CONTENT);
    lv_obj_set_align(s_streaming_stroke_val, LV_ALIGN_LEFT_MID);
    lv_obj_set_x(s_streaming_stroke_val, 90);
    lv_obj_set_y(s_streaming_stroke_val, 15);
    lv_label_set_text(s_streaming_stroke_val, "0");
    lv_obj_add_style(s_streaming_stroke_val, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(s_streaming_stroke_val, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *sensation_l = lv_label_create(ui_Streaming);
    lv_obj_set_align(sensation_l, LV_ALIGN_LEFT_MID);
    lv_obj_set_x(sensation_l, 12);
    lv_obj_set_y(sensation_l, 50);
    lv_label_set_text(sensation_l, T_SENSATION);

    ui_streamingsensationslider = lv_slider_create(ui_Streaming);
    lv_slider_set_range(ui_streamingsensationslider, 0, 100);
    lv_slider_set_value(ui_streamingsensationslider, 50, LV_ANIM_OFF);
    lv_obj_set_size(ui_streamingsensationslider, 160, 18);
    lv_obj_set_align(ui_streamingsensationslider, LV_ALIGN_RIGHT_MID);
    lv_obj_set_x(ui_streamingsensationslider, -16);
    lv_obj_set_y(ui_streamingsensationslider, 50);
    lv_obj_add_style(ui_streamingsensationslider, &style_slider_track[3], LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_streamingsensationslider, &style_slider_indicator[3], LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_streamingsensationslider, &style_slider_indicator[3], LV_PART_KNOB | LV_STATE_DEFAULT);

    s_streaming_sensation_val = lv_label_create(ui_Streaming);
    lv_obj_set_width(s_streaming_sensation_val, LV_SIZE_CONTENT);
    lv_obj_set_height(s_streaming_sensation_val, LV_SIZE_CONTENT);
    lv_obj_set_align(s_streaming_sensation_val, LV_ALIGN_LEFT_MID);
    lv_obj_set_x(s_streaming_sensation_val, 90);
    lv_obj_set_y(s_streaming_sensation_val, 50);
    lv_label_set_text(s_streaming_sensation_val, "50");
    lv_obj_add_style(s_streaming_sensation_val, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(s_streaming_sensation_val, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Batt8 = lv_label_create(ui_Streaming);
    lv_obj_set_width(ui_Batt8, 85);
    lv_obj_set_height(ui_Batt8, 30);
    lv_obj_set_align(ui_Batt8, LV_ALIGN_TOP_RIGHT);
    lv_obj_set_x(ui_Batt8, -6);
    lv_obj_set_y(ui_Batt8, 8);
    lv_label_set_text(ui_Batt8, T_BATT);

    ui_BattValue8 = lv_label_create(ui_Batt8);
    lv_obj_set_width(ui_BattValue8, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_BattValue8, LV_SIZE_CONTENT);
    lv_obj_set_align(ui_BattValue8, LV_ALIGN_RIGHT_MID);
    lv_obj_set_x(ui_BattValue8, 0);
    lv_obj_set_y(ui_BattValue8, -8);
    lv_label_set_text(ui_BattValue8, T_BLANK);
    lv_obj_add_style(ui_BattValue8, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Battery8 = lv_bar_create(ui_Batt8);
    lv_bar_set_range(ui_Battery8, 0, 100);
    lv_obj_set_size(ui_Battery8, 75, 8);
    lv_obj_set_align(ui_Battery8, LV_ALIGN_BOTTOM_MID);
    lv_obj_add_style(ui_Battery8, &style_battery_main, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_Battery8, &style_battery_indicator, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    ui_StreamingButtonL = lv_btn_create(ui_Streaming);
    lv_obj_set_size(ui_StreamingButtonL, 100, 30);
    lv_obj_set_align(ui_StreamingButtonL, LV_ALIGN_BOTTOM_LEFT);
    lv_obj_set_x(ui_StreamingButtonL, 8);
    lv_obj_set_y(ui_StreamingButtonL, -8);
    applyButtonStylesLocal(ui_StreamingButtonL, &style_button_l, &style_button_l_focused);
    lv_obj_add_event_cb(ui_StreamingButtonL, event_streaming_btn_l, LV_EVENT_SHORT_CLICKED, NULL);

    s_streaming_btn_l_text = lv_label_create(ui_StreamingButtonL);
    lv_obj_center(s_streaming_btn_l_text);
    lv_label_set_text(s_streaming_btn_l_text, T_MENU);

    ui_StreamingButtonM = lv_btn_create(ui_Streaming);
    lv_obj_set_size(ui_StreamingButtonM, 100, 30);
    lv_obj_set_align(ui_StreamingButtonM, LV_ALIGN_BOTTOM_MID);
    lv_obj_set_y(ui_StreamingButtonM, -8);
    applyButtonStylesLocal(ui_StreamingButtonM, &style_button_m, &style_button_m_focused);
    lv_obj_add_event_cb(ui_StreamingButtonM, event_streaming_btn_m, LV_EVENT_SHORT_CLICKED, NULL);

    s_streaming_btn_m_text = lv_label_create(ui_StreamingButtonM);
    lv_obj_center(s_streaming_btn_m_text);
    lv_label_set_text(s_streaming_btn_m_text, T_STOP);

    ui_StreamingButtonR = lv_btn_create(ui_Streaming);
    lv_obj_set_size(ui_StreamingButtonR, 100, 30);
    lv_obj_set_align(ui_StreamingButtonR, LV_ALIGN_BOTTOM_RIGHT);
    lv_obj_set_x(ui_StreamingButtonR, -8);
    lv_obj_set_y(ui_StreamingButtonR, -8);
    applyButtonStylesLocal(ui_StreamingButtonR, &style_button_r, &style_button_r_focused);
    lv_obj_add_event_cb(ui_StreamingButtonR, event_streaming_btn_r, LV_EVENT_SHORT_CLICKED, NULL);

    s_streaming_btn_r_text = lv_label_create(ui_StreamingButtonR);
    lv_obj_center(s_streaming_btn_r_text);
    lv_label_set_text(s_streaming_btn_r_text, T_ADDONS);
}

void ui_Addons_screen_init(void) {
    ui_Addons = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Addons, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(ui_Addons, event_addons_screen, LV_EVENT_ALL, NULL);

    ui_LogoAddons = lv_label_create(ui_Addons);
    lv_obj_set_align(ui_LogoAddons, LV_ALIGN_TOP_MID);
    lv_obj_set_y(ui_LogoAddons, 8);
    lv_label_set_text(ui_LogoAddons, T_ADDONS);
    lv_obj_set_style_text_font(ui_LogoAddons, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_LogoAddons, &style_title_bar, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_AddonsItem0 = lv_btn_create(ui_Addons);
    lv_obj_set_size(ui_AddonsItem0, 300, 34);
    lv_obj_set_align(ui_AddonsItem0, LV_ALIGN_TOP_MID);
    lv_obj_set_y(ui_AddonsItem0, 45);
    applyAddonRowStyle(ui_AddonsItem0, 0);
    lv_obj_add_event_cb(ui_AddonsItem0, event_addons_item0, LV_EVENT_SHORT_CLICKED, NULL);
    s_addons_item0_text = lv_label_create(ui_AddonsItem0);
    lv_obj_center(s_addons_item0_text);
    lv_obj_set_style_text_font(s_addons_item0_text, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_AddonsItem1 = lv_btn_create(ui_Addons);
    lv_obj_set_size(ui_AddonsItem1, 300, 34);
    lv_obj_set_align(ui_AddonsItem1, LV_ALIGN_TOP_MID);
    lv_obj_set_y(ui_AddonsItem1, 85);
    applyAddonRowStyle(ui_AddonsItem1, 1);
    lv_obj_add_event_cb(ui_AddonsItem1, event_addons_item1, LV_EVENT_SHORT_CLICKED, NULL);
    s_addons_item1_text = lv_label_create(ui_AddonsItem1);
    lv_obj_center(s_addons_item1_text);
    lv_obj_set_style_text_font(s_addons_item1_text, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_AddonsItem2 = lv_btn_create(ui_Addons);
    lv_obj_set_size(ui_AddonsItem2, 300, 34);
    lv_obj_set_align(ui_AddonsItem2, LV_ALIGN_TOP_MID);
    lv_obj_set_y(ui_AddonsItem2, 125);
    applyAddonRowStyle(ui_AddonsItem2, 2);
    lv_obj_add_event_cb(ui_AddonsItem2, event_addons_item2, LV_EVENT_SHORT_CLICKED, NULL);
    s_addons_item2_text = lv_label_create(ui_AddonsItem2);
    lv_obj_center(s_addons_item2_text);
    lv_obj_set_style_text_font(s_addons_item2_text, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_AddonsButtonL = lv_btn_create(ui_Addons);
    lv_obj_set_size(ui_AddonsButtonL, 100, 30);
    lv_obj_set_align(ui_AddonsButtonL, LV_ALIGN_BOTTOM_LEFT);
    lv_obj_set_x(ui_AddonsButtonL, 8);
    lv_obj_set_y(ui_AddonsButtonL, -8);
    applyButtonStylesLocal(ui_AddonsButtonL, &style_button_l, &style_button_l_focused);
    lv_obj_add_event_cb(ui_AddonsButtonL, event_addons_back, LV_EVENT_SHORT_CLICKED, NULL);

    s_addons_btn_l_text = lv_label_create(ui_AddonsButtonL);
    lv_obj_center(s_addons_btn_l_text);
    lv_label_set_text(s_addons_btn_l_text, T_MENU);

    ui_AddonsButtonM = lv_btn_create(ui_Addons);
    lv_obj_set_size(ui_AddonsButtonM, 100, 30);
    lv_obj_set_align(ui_AddonsButtonM, LV_ALIGN_BOTTOM_MID);
    lv_obj_set_y(ui_AddonsButtonM, -8);
    applyButtonStylesLocal(ui_AddonsButtonM, &style_button_m, &style_button_m_focused);
    lv_obj_add_event_cb(ui_AddonsButtonM, event_addons_open, LV_EVENT_SHORT_CLICKED, NULL);

    s_addons_btn_m_text = lv_label_create(ui_AddonsButtonM);
    lv_obj_center(s_addons_btn_m_text);
    lv_label_set_text(s_addons_btn_m_text, T_SELECT);

    ui_AddonsButtonR = lv_btn_create(ui_Addons);
    lv_obj_set_size(ui_AddonsButtonR, 100, 30);
    lv_obj_set_align(ui_AddonsButtonR, LV_ALIGN_BOTTOM_RIGHT);
    lv_obj_set_x(ui_AddonsButtonR, -8);
    lv_obj_set_y(ui_AddonsButtonR, -8);
    applyButtonStylesLocal(ui_AddonsButtonR, &style_button_r, &style_button_r_focused);
    lv_obj_add_event_cb(ui_AddonsButtonR, event_addons_enable, LV_EVENT_SHORT_CLICKED, NULL);

    s_addons_btn_r_text = lv_label_create(ui_AddonsButtonR);
    lv_obj_center(s_addons_btn_r_text);
    lv_label_set_text(s_addons_btn_r_text, "Enable/Disable");

    ui_g_addons = lv_group_create();
    lv_group_add_obj(ui_g_addons, ui_AddonsItem0);
    lv_group_add_obj(ui_g_addons, ui_AddonsItem1);
    lv_group_add_obj(ui_g_addons, ui_AddonsItem2);

    addonsSyncSelectionVisual();
}
