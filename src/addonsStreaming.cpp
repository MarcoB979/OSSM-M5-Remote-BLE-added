#include "addonsStreaming.h"

#include "ui/ui.h"
#include "ui/ui_helpers.h"
#include "language.h"
#include "styles.h"
#include "main.h"
#include "config_ids.h"
#include "buttonhandlers/ButtonHandlers.h"
#include "communication/EspNowComm.h"

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

static lv_obj_t *s_addons_btn_l_text = nullptr;
static lv_obj_t *s_addons_btn_m_text = nullptr;
static lv_obj_t *s_addons_btn_r_text = nullptr;
static lv_obj_t *s_addons_item0_text = nullptr;
static lv_obj_t *s_addons_item1_text = nullptr;
static lv_obj_t *s_addons_item2_text = nullptr;
static int s_addons_selected = 0;

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

static void refresh_addons_labels() {
    if (!s_addons_item0_text || !s_addons_item1_text || !s_addons_item2_text) return;

    lv_label_set_text(s_addons_item0_text, (s_addons_selected == 0) ? "> Eject" : "  Eject");
    lv_label_set_text(s_addons_item1_text, (s_addons_selected == 1) ? "> Fist-IT" : "  Fist-IT");
    lv_label_set_text(s_addons_item2_text, (s_addons_selected == 2) ? "> Streaming" : "  Streaming");
}

void addonsSyncSelectionVisual(void) {
    if (!ui_g_addons) return;

    lv_obj_t *target = ui_AddonsItem0;
    if (s_addons_selected == 1) target = ui_AddonsItem1;
    if (s_addons_selected == 2) target = ui_AddonsItem2;
    if (target) lv_group_focus_obj(target);

    refresh_addons_labels();
}

void addonsMoveSelection(int delta) {
    if (delta == 0) return;
    s_addons_selected += delta;
    while (s_addons_selected < 0) s_addons_selected += 3;
    while (s_addons_selected >= 3) s_addons_selected -= 3;
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
    if (ui_EJECTButtonLText) lv_label_set_text(ui_EJECTButtonLText, s_eject_on ? T_PAUSE : T_CUM);
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
    s_fist_on = !s_fist_on;
    SendCommand(s_fist_on ? ON : OFF, 0.0f, FIST_ID);
    fistUpdateValues();
}

void addonsFistToggle(void) {
    fistToggle();
}

static void ejectToggle() {
    s_eject_on = !s_eject_on;
    SendCommand(s_eject_on ? ON : OFF, 0.0f, CUM);
    ejectUpdateValues();
}

static void event_streaming_screen(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_SCREEN_LOADED) {
        screenmachine(e);
    }
}

static void event_streaming_btn_l(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_SHORT_CLICKED) {
        _ui_screen_change(ui_Menu, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    }
}

static void event_streaming_btn_m(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_SHORT_CLICKED) {
        _ui_screen_change(ui_Home, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    }
}

static void event_streaming_btn_r(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_SHORT_CLICKED) {
        _ui_screen_change(ui_Addons, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    }
}

static void event_addons_screen(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_SCREEN_LOADED) {
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
    if (s_addons_selected == 0) {
        _ui_screen_change(ui_EJECTSettings, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    } else if (s_addons_selected == 1) {
        _ui_screen_change(ui_FistIT, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    } else {
        _ui_screen_change(ui_Streaming, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    }
}

static void event_addons_open(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_SHORT_CLICKED) {
        addonsActivateSelection();
    }
}

static void event_addons_item0(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_SHORT_CLICKED) {
        s_addons_selected = 0;
        addonsActivateSelection();
    }
}

static void event_addons_item1(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_SHORT_CLICKED) {
        s_addons_selected = 1;
        addonsActivateSelection();
    }
}

static void event_addons_item2(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_SHORT_CLICKED) {
        s_addons_selected = 2;
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
    if (s_eject_ui_ready || !ui_EJECTSettings) return;

    if (ui_EJECTButtonLText) {
        lv_label_set_text(ui_EJECTButtonLText, T_CUM);
    }

    lv_obj_t *tmp = nullptr;
    createSliderRow(ui_EJECTSettings, &tmp, &s_eject_speed_slider, &s_eject_speed_val, T_CUM_SPEED, -60, 0, 100, 0);
    createSliderRow(ui_EJECTSettings, &tmp, &s_eject_time_slider, &s_eject_time_val, T_CUM_TIME, -25, 0, 61, 1);
    createSliderRow(ui_EJECTSettings, &tmp, &s_eject_size_slider, &s_eject_size_val, T_CUM_Volume, 10, 0, 100, 2);
    createSliderRow(ui_EJECTSettings, &tmp, &s_eject_accel_slider, &s_eject_accel_val, T_CUM_Accel, 45, 0, 100, 3);

    if (ui_EJECTButtonL) {
        lv_obj_add_event_cb(ui_EJECTButtonL, event_eject_btn_l, LV_EVENT_SHORT_CLICKED, nullptr);
    }

    s_eject_ui_ready = true;
    ejectUpdateValues();
}

static void createFistScreen() {
    if (ui_FistIT) return;

    ui_FistIT = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_FistIT, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(ui_FistIT, event_fist_screen, LV_EVENT_ALL, NULL);

    lv_obj_t *title = lv_label_create(ui_FistIT);
    lv_obj_set_align(title, LV_ALIGN_TOP_MID);
    lv_obj_set_y(title, 10);
    lv_label_set_text(title, "Fist-IT");
    lv_obj_add_style(title, &style_title_bar, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *tmp = nullptr;
    createSliderRow(ui_FistIT, &tmp, &s_fist_speed_slider, &s_fist_speed_val, T_SPEED, -60, 0, 100, 0);
    createSliderRow(ui_FistIT, &tmp, &s_fist_rot_slider, &s_fist_rot_val, T_ROTATION, -25, 0, 360, 1);
    createSliderRow(ui_FistIT, &tmp, &s_fist_pause_slider, &s_fist_pause_val, T_PAUSE, 10, 0, 100, 2);
    createSliderRow(ui_FistIT, &tmp, &s_fist_accel_slider, &s_fist_accel_val, T_ACCEL, 45, 0, 100, 3);

    s_fist_btn_l = lv_btn_create(ui_FistIT);
    lv_obj_set_size(s_fist_btn_l, 100, 30);
    lv_obj_set_align(s_fist_btn_l, LV_ALIGN_BOTTOM_LEFT);
    lv_obj_set_x(s_fist_btn_l, 8);
    lv_obj_set_y(s_fist_btn_l, -8);
    applyButtonStylesLocal(s_fist_btn_l, &style_button_l, &style_button_l_focused);
    lv_obj_add_event_cb(s_fist_btn_l, event_fist_btn_l, LV_EVENT_SHORT_CLICKED, nullptr);
    s_fist_btn_l_text = lv_label_create(s_fist_btn_l);
    lv_obj_center(s_fist_btn_l_text);

    s_fist_btn_m = lv_btn_create(ui_FistIT);
    lv_obj_set_size(s_fist_btn_m, 100, 30);
    lv_obj_set_align(s_fist_btn_m, LV_ALIGN_BOTTOM_MID);
    lv_obj_set_y(s_fist_btn_m, -8);
    applyButtonStylesLocal(s_fist_btn_m, &style_button_m, &style_button_m_focused);
    lv_obj_add_event_cb(s_fist_btn_m, event_fist_btn_m, LV_EVENT_SHORT_CLICKED, nullptr);
    lv_obj_t *fist_btn_m_text = lv_label_create(s_fist_btn_m);
    lv_obj_center(fist_btn_m_text);
    lv_label_set_text(fist_btn_m_text, T_HOME);

    s_fist_btn_r = lv_btn_create(ui_FistIT);
    lv_obj_set_size(s_fist_btn_r, 100, 30);
    lv_obj_set_align(s_fist_btn_r, LV_ALIGN_BOTTOM_RIGHT);
    lv_obj_set_x(s_fist_btn_r, -8);
    lv_obj_set_y(s_fist_btn_r, -8);
    applyButtonStylesLocal(s_fist_btn_r, &style_button_r, &style_button_r_focused);
    lv_obj_add_event_cb(s_fist_btn_r, event_fist_btn_r, LV_EVENT_SHORT_CLICKED, nullptr);
    lv_obj_t *fist_btn_r_text = lv_label_create(s_fist_btn_r);
    lv_obj_center(fist_btn_r_text);
    lv_label_set_text(fist_btn_r_text, T_MENU);

    fistUpdateValues();
}

void addonsHandleEjectScreen(void) {
    ensureEjectUiInitialized();
    if (!s_eject_ui_ready) return;

    int d = detentsFromEncoder(encoder1, &s_eject_enc1);
    if (d != 0) {
        s_eject_speed += d;
        if (s_eject_speed < 0.0f) s_eject_speed = 0.0f;
        if (s_eject_speed > 100.0f) s_eject_speed = 100.0f;
        SendCommand(CUMSPEED, s_eject_speed / 10.0f, CUM);
    }

    d = detentsFromEncoder(encoder2, &s_eject_enc2);
    if (d != 0) {
        s_eject_time += d;
        if (s_eject_time < 0.0f) s_eject_time = 0.0f;
        if (s_eject_time > 61.0f) s_eject_time = 61.0f;
        SendCommand(CUMTIME, s_eject_time, CUM);
    }

    d = detentsFromEncoder(encoder3, &s_eject_enc3);
    if (d != 0) {
        s_eject_size += d;
        if (s_eject_size < 0.0f) s_eject_size = 0.0f;
        if (s_eject_size > 100.0f) s_eject_size = 100.0f;
        SendCommand(CUMSIZE, s_eject_size, CUM);
    }

    d = detentsFromEncoder(encoder4, &s_eject_enc4);
    if (d != 0) {
        s_eject_accel += d;
        if (s_eject_accel < 0.0f) s_eject_accel = 0.0f;
        if (s_eject_accel > 100.0f) s_eject_accel = 100.0f;
        SendCommand(CUMACCEL, s_eject_accel / 10.0f, CUM);
    }

    if (s_eject_speed_slider) {
        if (!lv_slider_is_dragged(s_eject_speed_slider)) {
            lv_slider_set_value(s_eject_speed_slider, (int)s_eject_speed, LV_ANIM_OFF);
        } else {
            s_eject_speed = (float)lv_slider_get_value(s_eject_speed_slider);
            SendCommand(CUMSPEED, s_eject_speed / 10.0f, CUM);
        }
    }
    if (s_eject_time_slider) {
        if (!lv_slider_is_dragged(s_eject_time_slider)) {
            lv_slider_set_value(s_eject_time_slider, (int)s_eject_time, LV_ANIM_OFF);
        } else {
            s_eject_time = (float)lv_slider_get_value(s_eject_time_slider);
            SendCommand(CUMTIME, s_eject_time, CUM);
        }
    }
    if (s_eject_size_slider) {
        if (!lv_slider_is_dragged(s_eject_size_slider)) {
            lv_slider_set_value(s_eject_size_slider, (int)s_eject_size, LV_ANIM_OFF);
        } else {
            s_eject_size = (float)lv_slider_get_value(s_eject_size_slider);
            SendCommand(CUMSIZE, s_eject_size, CUM);
        }
    }
    if (s_eject_accel_slider) {
        if (!lv_slider_is_dragged(s_eject_accel_slider)) {
            lv_slider_set_value(s_eject_accel_slider, (int)s_eject_accel, LV_ANIM_OFF);
        } else {
            s_eject_accel = (float)lv_slider_get_value(s_eject_accel_slider);
            SendCommand(CUMACCEL, s_eject_accel / 10.0f, CUM);
        }
    }

    ejectUpdateValues();
}

void addonsHandleFistScreen(void) {
    createFistScreen();
    if (!ui_FistIT) return;

    int d = detentsFromEncoder(encoder1, &s_fist_enc1);
    if (d != 0) {
        s_fist_speed += d;
        if (s_fist_speed < 0.0f) s_fist_speed = 0.0f;
        if (s_fist_speed > 100.0f) s_fist_speed = 100.0f;
        SendCommand(FIST_SPEED, s_fist_speed, FIST_ID);
    }

    d = detentsFromEncoder(encoder2, &s_fist_enc2);
    if (d != 0) {
        s_fist_rot += d;
        if (s_fist_rot < 0.0f) s_fist_rot = 0.0f;
        if (s_fist_rot > 360.0f) s_fist_rot = 360.0f;
        SendCommand(FIST_ROTATION, s_fist_rot, FIST_ID);
    }

    d = detentsFromEncoder(encoder3, &s_fist_enc3);
    if (d != 0) {
        s_fist_pause += d;
        if (s_fist_pause < 0.0f) s_fist_pause = 0.0f;
        if (s_fist_pause > 100.0f) s_fist_pause = 100.0f;
        SendCommand(FIST_PAUSE, s_fist_pause, FIST_ID);
    }

    d = detentsFromEncoder(encoder4, &s_fist_enc4);
    if (d != 0) {
        s_fist_accel += d;
        if (s_fist_accel < 0.0f) s_fist_accel = 0.0f;
        if (s_fist_accel > 100.0f) s_fist_accel = 100.0f;
        SendCommand(FIST_ACCEL, s_fist_accel, FIST_ID);
    }

    if (s_fist_speed_slider) {
        if (!lv_slider_is_dragged(s_fist_speed_slider)) {
            lv_slider_set_value(s_fist_speed_slider, (int)s_fist_speed, LV_ANIM_OFF);
        } else {
            s_fist_speed = (float)lv_slider_get_value(s_fist_speed_slider);
            SendCommand(FIST_SPEED, s_fist_speed, FIST_ID);
        }
    }
    if (s_fist_rot_slider) {
        if (!lv_slider_is_dragged(s_fist_rot_slider)) {
            lv_slider_set_value(s_fist_rot_slider, (int)s_fist_rot, LV_ANIM_OFF);
        } else {
            s_fist_rot = (float)lv_slider_get_value(s_fist_rot_slider);
            SendCommand(FIST_ROTATION, s_fist_rot, FIST_ID);
        }
    }
    if (s_fist_pause_slider) {
        if (!lv_slider_is_dragged(s_fist_pause_slider)) {
            lv_slider_set_value(s_fist_pause_slider, (int)s_fist_pause, LV_ANIM_OFF);
        } else {
            s_fist_pause = (float)lv_slider_get_value(s_fist_pause_slider);
            SendCommand(FIST_PAUSE, s_fist_pause, FIST_ID);
        }
    }
    if (s_fist_accel_slider) {
        if (!lv_slider_is_dragged(s_fist_accel_slider)) {
            lv_slider_set_value(s_fist_accel_slider, (int)s_fist_accel, LV_ANIM_OFF);
        } else {
            s_fist_accel = (float)lv_slider_get_value(s_fist_accel_slider);
            SendCommand(FIST_ACCEL, s_fist_accel, FIST_ID);
        }
    }

    fistUpdateValues();
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
    lv_obj_set_y(speed_l, -45);
    lv_label_set_text(speed_l, T_SPEED);

    ui_streamingspeedslider = lv_slider_create(ui_Streaming);
    lv_slider_set_range(ui_streamingspeedslider, 0, 100);
    lv_obj_set_size(ui_streamingspeedslider, 160, 18);
    lv_obj_set_align(ui_streamingspeedslider, LV_ALIGN_RIGHT_MID);
    lv_obj_set_x(ui_streamingspeedslider, -16);
    lv_obj_set_y(ui_streamingspeedslider, -45);
    lv_obj_add_style(ui_streamingspeedslider, &style_slider_track[0], LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_streamingspeedslider, &style_slider_indicator[0], LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_streamingspeedslider, &style_slider_indicator[0], LV_PART_KNOB | LV_STATE_DEFAULT);

    s_streaming_speed_val = lv_label_create(ui_Streaming);
    lv_obj_set_align(s_streaming_speed_val, LV_ALIGN_RIGHT_MID);
    lv_obj_set_x(s_streaming_speed_val, -2);
    lv_obj_set_y(s_streaming_speed_val, -45);
    lv_label_set_text(s_streaming_speed_val, "0");

    lv_obj_t *depth_l = lv_label_create(ui_Streaming);
    lv_obj_set_align(depth_l, LV_ALIGN_LEFT_MID);
    lv_obj_set_x(depth_l, 12);
    lv_obj_set_y(depth_l, -5);
    lv_label_set_text(depth_l, T_DEPTH);

    ui_streamingdepthslider = lv_slider_create(ui_Streaming);
    lv_slider_set_range(ui_streamingdepthslider, 0, 100);
    lv_obj_set_size(ui_streamingdepthslider, 160, 18);
    lv_obj_set_align(ui_streamingdepthslider, LV_ALIGN_RIGHT_MID);
    lv_obj_set_x(ui_streamingdepthslider, -16);
    lv_obj_set_y(ui_streamingdepthslider, -5);
    lv_obj_add_style(ui_streamingdepthslider, &style_slider_track[1], LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_streamingdepthslider, &style_slider_indicator[1], LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_streamingdepthslider, &style_slider_indicator[1], LV_PART_KNOB | LV_STATE_DEFAULT);

    s_streaming_depth_val = lv_label_create(ui_Streaming);
    lv_obj_set_align(s_streaming_depth_val, LV_ALIGN_RIGHT_MID);
    lv_obj_set_x(s_streaming_depth_val, -2);
    lv_obj_set_y(s_streaming_depth_val, -5);
    lv_label_set_text(s_streaming_depth_val, "0");

    lv_obj_t *stroke_l = lv_label_create(ui_Streaming);
    lv_obj_set_align(stroke_l, LV_ALIGN_LEFT_MID);
    lv_obj_set_x(stroke_l, 12);
    lv_obj_set_y(stroke_l, 35);
    lv_label_set_text(stroke_l, T_STROKE);

    ui_streamingstrokeslider = lv_slider_create(ui_Streaming);
    lv_slider_set_range(ui_streamingstrokeslider, 0, 100);
    lv_obj_set_size(ui_streamingstrokeslider, 160, 18);
    lv_obj_set_align(ui_streamingstrokeslider, LV_ALIGN_RIGHT_MID);
    lv_obj_set_x(ui_streamingstrokeslider, -16);
    lv_obj_set_y(ui_streamingstrokeslider, 35);
    lv_obj_add_style(ui_streamingstrokeslider, &style_slider_track[2], LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_streamingstrokeslider, &style_slider_indicator[2], LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_streamingstrokeslider, &style_slider_indicator[2], LV_PART_KNOB | LV_STATE_DEFAULT);

    s_streaming_stroke_val = lv_label_create(ui_Streaming);
    lv_obj_set_align(s_streaming_stroke_val, LV_ALIGN_RIGHT_MID);
    lv_obj_set_x(s_streaming_stroke_val, -2);
    lv_obj_set_y(s_streaming_stroke_val, 35);
    lv_label_set_text(s_streaming_stroke_val, "0");

    ui_Batt8 = lv_label_create(ui_Streaming);
    lv_obj_set_align(ui_Batt8, LV_ALIGN_TOP_RIGHT);
    lv_obj_set_x(ui_Batt8, -6);
    lv_obj_set_y(ui_Batt8, 8);
    lv_label_set_text(ui_Batt8, T_BATT);

    ui_BattValue8 = lv_label_create(ui_Batt8);
    lv_obj_set_align(ui_BattValue8, LV_ALIGN_RIGHT_MID);
    lv_obj_set_x(ui_BattValue8, 0);
    lv_obj_set_y(ui_BattValue8, -8);
    lv_label_set_text(ui_BattValue8, T_BLANK);

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
    lv_label_set_text(s_streaming_btn_l_text, T_BACK);

    ui_StreamingButtonM = lv_btn_create(ui_Streaming);
    lv_obj_set_size(ui_StreamingButtonM, 100, 30);
    lv_obj_set_align(ui_StreamingButtonM, LV_ALIGN_BOTTOM_MID);
    lv_obj_set_y(ui_StreamingButtonM, -8);
    applyButtonStylesLocal(ui_StreamingButtonM, &style_button_m, &style_button_m_focused);
    lv_obj_add_event_cb(ui_StreamingButtonM, event_streaming_btn_m, LV_EVENT_SHORT_CLICKED, NULL);

    s_streaming_btn_m_text = lv_label_create(ui_StreamingButtonM);
    lv_obj_center(s_streaming_btn_m_text);
    lv_label_set_text(s_streaming_btn_m_text, T_HOME);

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
    lv_label_set_text(s_addons_btn_l_text, T_BACK);

    ui_AddonsButtonM = lv_btn_create(ui_Addons);
    lv_obj_set_size(ui_AddonsButtonM, 100, 30);
    lv_obj_set_align(ui_AddonsButtonM, LV_ALIGN_BOTTOM_MID);
    lv_obj_set_y(ui_AddonsButtonM, -8);
    applyButtonStylesLocal(ui_AddonsButtonM, &style_button_m, &style_button_m_focused);
    lv_obj_add_event_cb(ui_AddonsButtonM, event_addons_home, LV_EVENT_SHORT_CLICKED, NULL);

    s_addons_btn_m_text = lv_label_create(ui_AddonsButtonM);
    lv_obj_center(s_addons_btn_m_text);
    lv_label_set_text(s_addons_btn_m_text, T_HOME);

    ui_AddonsButtonR = lv_btn_create(ui_Addons);
    lv_obj_set_size(ui_AddonsButtonR, 100, 30);
    lv_obj_set_align(ui_AddonsButtonR, LV_ALIGN_BOTTOM_RIGHT);
    lv_obj_set_x(ui_AddonsButtonR, -8);
    lv_obj_set_y(ui_AddonsButtonR, -8);
    applyButtonStylesLocal(ui_AddonsButtonR, &style_button_r, &style_button_r_focused);
    lv_obj_add_event_cb(ui_AddonsButtonR, event_addons_open, LV_EVENT_SHORT_CLICKED, NULL);

    s_addons_btn_r_text = lv_label_create(ui_AddonsButtonR);
    lv_obj_center(s_addons_btn_r_text);
    lv_label_set_text(s_addons_btn_r_text, T_SELECT);

    ui_g_addons = lv_group_create();
    lv_group_add_obj(ui_g_addons, ui_AddonsItem0);
    lv_group_add_obj(ui_g_addons, ui_AddonsItem1);
    lv_group_add_obj(ui_g_addons, ui_AddonsItem2);

    addonsSyncSelectionVisual();
}
