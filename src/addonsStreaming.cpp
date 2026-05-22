#include "ui/ui.h"
#include "ui/ui_helpers.h"
#include "language.h"
#include "styles.h"

// Extra globals exposed through ui.h
lv_obj_t *ui_StreamingButtonM = nullptr;
lv_obj_t *ui_Batt8 = nullptr;
lv_obj_t *ui_BattValue8 = nullptr;
lv_obj_t *ui_Battery8 = nullptr;
lv_group_t *ui_g_addons = nullptr;

static lv_obj_t *s_streaming_btn_l_text = nullptr;
static lv_obj_t *s_streaming_btn_m_text = nullptr;
static lv_obj_t *s_streaming_btn_r_text = nullptr;
static lv_obj_t *s_streaming_speed_val = nullptr;
static lv_obj_t *s_streaming_depth_val = nullptr;
static lv_obj_t *s_streaming_stroke_val = nullptr;

static lv_obj_t *s_addons_btn_l_text = nullptr;
static lv_obj_t *s_addons_btn_m_text = nullptr;
static lv_obj_t *s_addons_btn_r_text = nullptr;
static int s_addons_selected = 0;

static void refresh_addons_labels() {
    if (!ui_AddonsItem0 || !ui_AddonsItem1 || !ui_AddonsItem2) return;

    lv_label_set_text(ui_AddonsItem0, (s_addons_selected == 0) ? "> Eject" : "  Eject");
    lv_label_set_text(ui_AddonsItem1, (s_addons_selected == 1) ? "> Fist-IT" : "  Fist-IT");
    lv_label_set_text(ui_AddonsItem2, (s_addons_selected == 2) ? "> Streaming" : "  Streaming");
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
        refresh_addons_labels();
        screenmachine(e);
    }
}

static void event_addons_btn_l(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_SHORT_CLICKED) {
        _ui_screen_change(ui_Menu, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    }
}

static void event_addons_btn_m(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_SHORT_CLICKED) {
        s_addons_selected = (s_addons_selected + 1) % 3;
        refresh_addons_labels();
    }
}

static void event_addons_btn_r(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_SHORT_CLICKED) return;

    if (s_addons_selected == 0) {
        _ui_screen_change(ui_EJECTSettings, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    } else if (s_addons_selected == 1) {
        _ui_screen_change(ui_Home, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    } else {
        _ui_screen_change(ui_Streaming, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
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
    lv_obj_add_style(ui_StreamingButtonL, &style_button_l, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(ui_StreamingButtonL, event_streaming_btn_l, LV_EVENT_SHORT_CLICKED, NULL);

    s_streaming_btn_l_text = lv_label_create(ui_StreamingButtonL);
    lv_obj_center(s_streaming_btn_l_text);
    lv_label_set_text(s_streaming_btn_l_text, T_BACK);

    ui_StreamingButtonM = lv_btn_create(ui_Streaming);
    lv_obj_set_size(ui_StreamingButtonM, 100, 30);
    lv_obj_set_align(ui_StreamingButtonM, LV_ALIGN_BOTTOM_MID);
    lv_obj_set_y(ui_StreamingButtonM, -8);
    lv_obj_add_style(ui_StreamingButtonM, &style_button_m, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(ui_StreamingButtonM, event_streaming_btn_m, LV_EVENT_SHORT_CLICKED, NULL);

    s_streaming_btn_m_text = lv_label_create(ui_StreamingButtonM);
    lv_obj_center(s_streaming_btn_m_text);
    lv_label_set_text(s_streaming_btn_m_text, T_HOME);

    ui_StreamingButtonR = lv_btn_create(ui_Streaming);
    lv_obj_set_size(ui_StreamingButtonR, 100, 30);
    lv_obj_set_align(ui_StreamingButtonR, LV_ALIGN_BOTTOM_RIGHT);
    lv_obj_set_x(ui_StreamingButtonR, -8);
    lv_obj_set_y(ui_StreamingButtonR, -8);
    lv_obj_add_style(ui_StreamingButtonR, &style_button_r, LV_PART_MAIN | LV_STATE_DEFAULT);
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
    lv_obj_set_style_text_font(ui_LogoAddons, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_LogoAddons, &style_title_bar, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_AddonsItem0 = lv_label_create(ui_Addons);
    lv_obj_set_align(ui_AddonsItem0, LV_ALIGN_LEFT_MID);
    lv_obj_set_x(ui_AddonsItem0, 18);
    lv_obj_set_y(ui_AddonsItem0, -35);

    ui_AddonsItem1 = lv_label_create(ui_Addons);
    lv_obj_set_align(ui_AddonsItem1, LV_ALIGN_LEFT_MID);
    lv_obj_set_x(ui_AddonsItem1, 18);
    lv_obj_set_y(ui_AddonsItem1, -5);

    ui_AddonsItem2 = lv_label_create(ui_Addons);
    lv_obj_set_align(ui_AddonsItem2, LV_ALIGN_LEFT_MID);
    lv_obj_set_x(ui_AddonsItem2, 18);
    lv_obj_set_y(ui_AddonsItem2, 25);

    ui_AddonsButtonL = lv_btn_create(ui_Addons);
    lv_obj_set_size(ui_AddonsButtonL, 100, 30);
    lv_obj_set_align(ui_AddonsButtonL, LV_ALIGN_BOTTOM_LEFT);
    lv_obj_set_x(ui_AddonsButtonL, 8);
    lv_obj_set_y(ui_AddonsButtonL, -8);
    lv_obj_add_style(ui_AddonsButtonL, &style_button_l, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(ui_AddonsButtonL, event_addons_btn_l, LV_EVENT_SHORT_CLICKED, NULL);

    s_addons_btn_l_text = lv_label_create(ui_AddonsButtonL);
    lv_obj_center(s_addons_btn_l_text);
    lv_label_set_text(s_addons_btn_l_text, T_BACK);

    ui_AddonsButtonM = lv_btn_create(ui_Addons);
    lv_obj_set_size(ui_AddonsButtonM, 100, 30);
    lv_obj_set_align(ui_AddonsButtonM, LV_ALIGN_BOTTOM_MID);
    lv_obj_set_y(ui_AddonsButtonM, -8);
    lv_obj_add_style(ui_AddonsButtonM, &style_button_m, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(ui_AddonsButtonM, event_addons_btn_m, LV_EVENT_SHORT_CLICKED, NULL);

    s_addons_btn_m_text = lv_label_create(ui_AddonsButtonM);
    lv_obj_center(s_addons_btn_m_text);
    lv_label_set_text(s_addons_btn_m_text, T_SELECT);

    ui_AddonsButtonR = lv_btn_create(ui_Addons);
    lv_obj_set_size(ui_AddonsButtonR, 100, 30);
    lv_obj_set_align(ui_AddonsButtonR, LV_ALIGN_BOTTOM_RIGHT);
    lv_obj_set_x(ui_AddonsButtonR, -8);
    lv_obj_set_y(ui_AddonsButtonR, -8);
    lv_obj_add_style(ui_AddonsButtonR, &style_button_r, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(ui_AddonsButtonR, event_addons_btn_r, LV_EVENT_SHORT_CLICKED, NULL);

    s_addons_btn_r_text = lv_label_create(ui_AddonsButtonR);
    lv_obj_center(s_addons_btn_r_text);
    lv_label_set_text(s_addons_btn_r_text, T_SELECT);

    ui_g_addons = lv_group_create();
    lv_group_add_obj(ui_g_addons, ui_AddonsButtonL);
    lv_group_add_obj(ui_g_addons, ui_AddonsButtonM);
    lv_group_add_obj(ui_g_addons, ui_AddonsButtonR);

    refresh_addons_labels();
}
