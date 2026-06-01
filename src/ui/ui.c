

#include "ui.h"
#include "ui_helpers.h"
#include "main.h"
#include "language.h"
#include "../display/styles.h"
#include "../display/colors.h"
#include "../addons/addonsStreaming.h"
#include "../screens/ScreenHandler.h"

///////////////////// VARIABLES ////////////////////
lv_obj_t * ui_Start;
lv_obj_t * ui_Logo;
lv_obj_t * ui_StartButtonL;
lv_obj_t * ui_StartButtonLText;
lv_obj_t * ui_StartButtonM;
lv_obj_t * ui_StartButtonMText;
lv_obj_t * ui_StartButtonR;
lv_obj_t * ui_StartButtonRText;
lv_obj_t * ui_LOVE_Logo;
lv_obj_t * ui_OrtlofLogo;
lv_obj_t * ui_Welcome;
lv_obj_t * ui_Batt;
lv_obj_t * ui_BattValue;
lv_obj_t * ui_Battery;
lv_obj_t * ui_Home;
lv_obj_t * ui_Logo2;
lv_obj_t * ui_HomeButtonL;
lv_obj_t * ui_HomeButtonLText;
lv_obj_t * ui_HomeButtonM;
lv_obj_t * ui_HomeButtonMText;
lv_obj_t * ui_HomeButtonR;
lv_obj_t * ui_HomeButtonRText;
lv_obj_t * ui_SpeedL;
lv_obj_t * ui_homespeedslider;
lv_obj_t * ui_homespeedvalue;
lv_obj_t * ui_DepthL;
lv_obj_t * ui_homedepthslider;
lv_obj_t * ui_homedepthvalue;
lv_obj_t * ui_StrokeL;
lv_obj_t * ui_homestrokeslider;
lv_obj_t * ui_homestrokevalue;
lv_obj_t * ui_SensationL;
lv_obj_t * ui_homesensationslider;
lv_obj_t * ui_Batt2;
lv_obj_t * ui_BattValue2;
lv_obj_t * ui_Battery2;
lv_obj_t * ui_HomePatternLabel1;
lv_obj_t * ui_HomePatternLabel;
lv_obj_t * ui_connect;
lv_obj_t * ui_Batt3;
lv_obj_t * ui_BattValue3;
lv_obj_t * ui_Battery3;

// New Menu screen
lv_obj_t * ui_Menu = NULL;
lv_obj_t * ui_LogoMenu = NULL;
lv_obj_t * ui_MenuButtonTL = NULL;
lv_obj_t * ui_MenuButtonTLText = NULL;
lv_obj_t * ui_MenuButtonTR = NULL;
lv_obj_t * ui_MenuButtonTRText = NULL;
lv_obj_t * ui_MenuButtonML = NULL;
lv_obj_t * ui_MenuButtonMLText = NULL;
lv_obj_t * ui_MenuButtonMR = NULL;
lv_obj_t * ui_MenuButtonMRText = NULL;
lv_obj_t * ui_MenuButtonL = NULL;
lv_obj_t * ui_MenuButtonLText = NULL;
lv_obj_t * ui_MenuButtonM = NULL;
lv_obj_t * ui_MenuButtonMText = NULL;
lv_obj_t * ui_MenuButtonR = NULL;
lv_obj_t * ui_MenuButtonRText = NULL;
lv_group_t * ui_g_menu = NULL;

// Stroke screen (body created in strokeMode.cpp; screen object declared here)
lv_obj_t * ui_Stroke = NULL;

// Colors screen (body created in colors.cpp; screen object declared here)

// Placeholder objects â€” set to NULL until streaming/addons screens are implemented
lv_obj_t * ui_Streaming             = NULL;
lv_obj_t * ui_streamingspeedslider  = NULL;
lv_obj_t * ui_streamingdepthslider  = NULL;
lv_obj_t * ui_streamingstrokeslider = NULL;
lv_obj_t * ui_streamingsensationslider = NULL;
lv_obj_t * ui_StreamingButtonL      = NULL;
lv_obj_t * ui_StreamingButtonR      = NULL;
lv_obj_t * ui_LogoStreaming         = NULL;
lv_obj_t * ui_Addons                = NULL;
lv_obj_t * ui_AddonsButtonL        = NULL;
lv_obj_t * ui_AddonsButtonM        = NULL;
lv_obj_t * ui_AddonsButtonR        = NULL;
lv_obj_t * ui_AddonsItem0           = NULL;
lv_obj_t * ui_AddonsItem1           = NULL;
lv_obj_t * ui_AddonsItem2           = NULL;
lv_obj_t * ui_LogoAddons            = NULL;
lv_obj_t * ui_brightness_slider     = NULL;
lv_obj_t * ui_Pattern;
lv_obj_t * ui_Logo5;
lv_obj_t * ui_PatternButtonL;
lv_obj_t * ui_PatternButtonLText;
lv_obj_t * ui_PatternButtonM;
lv_obj_t * ui_PatternButtonMText;
lv_obj_t * ui_PatternButtonR;
lv_obj_t * ui_PatternButtonRText;
lv_obj_t * ui_Batt5;
lv_obj_t * ui_BattValue5;
lv_obj_t * ui_Battery5;
lv_obj_t * ui_Label4;
lv_obj_t * ui_PatternS;
lv_obj_t * ui_PatternBand0;
lv_obj_t * ui_PatternBand1;
lv_obj_t * ui_PatternBand2;
lv_obj_t * ui_Torqe;
lv_obj_t * ui_Logo4;
lv_obj_t * ui_TorqeButtonL;
lv_obj_t * ui_TorqeButtonLText;
lv_obj_t * ui_TorqeButtonM;
lv_obj_t * ui_TorqeButtonMText;
lv_obj_t * ui_TorqeButtonR;
lv_obj_t * ui_TorqeButtonRText;
lv_obj_t * ui_outtroqelabel;
lv_obj_t * ui_outtroqevalue;
lv_obj_t * ui_outtroqeslider;
lv_obj_t * ui_Low1;
lv_obj_t * ui_High1;
lv_obj_t * ui_introqelabel;
lv_obj_t * ui_introqevalue;
lv_obj_t * ui_introqeslider;
lv_obj_t * ui_Low2;
lv_obj_t * ui_High;
lv_obj_t * ui_Batt4;
lv_obj_t * ui_BattValue4;
lv_obj_t * ui_Battery4;
lv_obj_t * ui_EJECTSettings;
lv_obj_t * ui_Logo6;
lv_obj_t * ui_EJECTButtonL;
lv_obj_t * ui_EJECTButtonLText;
lv_obj_t * ui_EJECTButtonM;
lv_obj_t * ui_EJECTButtonMText;
lv_obj_t * ui_EJECTButtonR;
lv_obj_t * ui_EJECTButtonRText;
lv_obj_t * ui_Batt6;
lv_obj_t * ui_BattValue6;
lv_obj_t * ui_Battery6;
lv_obj_t * ui_Settings;
lv_obj_t * ui_Logo1;
lv_obj_t * ui_SettingsButtonL;
lv_obj_t * ui_SettingsButtonLText;
lv_obj_t * ui_SettingsButtonM;
lv_obj_t * ui_SettingsButtonMText;
lv_obj_t * ui_SettingsButtonR;
lv_obj_t * ui_SettingsButtonRText;
lv_obj_t * ui_brightness_icon;
lv_obj_t * ui_Batt1;
lv_obj_t * ui_BattValue1;
lv_obj_t * ui_Battery1;
lv_obj_t * ui_ejectaddon;
lv_obj_t * ui_strokeinvert;
lv_obj_t * ui_forceHome;
lv_obj_t * ui_vibrate;
lv_obj_t * ui_lefty;
lv_group_t * ui_g_settings;


///////////////////// TEST LVGL SETTINGS ////////////////////
#if LV_COLOR_DEPTH != 16
    #error "LV_COLOR_DEPTH should be 16bit to match SquareLine Studio's settings"
#endif
#if LV_COLOR_16_SWAP !=0
    #error "#error LV_COLOR_16_SWAP should be 0 to match SquareLine Studio's settings"
#endif

static void applyTitleBarStyle(lv_obj_t *obj)
{
    lv_obj_add_style(obj, &style_title_bar, LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void applyTextPrimaryStyle(lv_obj_t *obj)
{
    lv_obj_add_style(obj, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void applyButtonStyles(lv_obj_t *obj, lv_style_t *defaultStyle, lv_style_t *focusedStyle, lv_style_t *disabledStyle)
{
    lv_obj_add_style(obj, defaultStyle, LV_PART_MAIN | LV_STATE_DEFAULT);
    if (focusedStyle) {
        lv_obj_add_style(obj, focusedStyle, LV_PART_MAIN | LV_STATE_FOCUSED);
    }
    if (disabledStyle) {
        lv_obj_add_style(obj, disabledStyle, LV_PART_MAIN | LV_STATE_DISABLED);
    }
}

static void applySliderStyles(lv_obj_t *obj, int styleIndex)
{
    lv_obj_add_style(obj, &style_slider_track[styleIndex], LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(obj, &style_slider_indicator[styleIndex], LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_add_style(obj, &style_slider_indicator[styleIndex], LV_PART_KNOB | LV_STATE_DEFAULT);
}

static void applyBatteryStyles(lv_obj_t *obj)
{
    lv_obj_add_style(obj, &style_battery_main, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(obj, &style_battery_indicator, LV_PART_INDICATOR | LV_STATE_DEFAULT);
}

static void applyRollerStyles(lv_obj_t *obj)
{
    lv_obj_add_style(obj, &style_roller_main, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(obj, &style_roller, LV_PART_SELECTED | LV_STATE_DEFAULT);
}

static void applyCheckboxStyles(lv_obj_t *obj)
{
    lv_obj_add_style(obj, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(obj, &style_checkbox_indicator, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_add_style(obj, &style_button_m_focused, LV_PART_MAIN | LV_STATE_FOCUSED);
}

///////////////////// ANIMATIONS ////////////////////

///////////////////// FUNCTIONS ////////////////////
static bool s_start_auto_connected = false;
static void auto_connect_timer_cb(lv_timer_t *t) {
    (void)t;
    if (ui_StartButtonL) lv_obj_send_event(ui_StartButtonL, LV_EVENT_CLICKED, NULL);
}
static void ui_event_Start(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_SCREEN_LOADED) {
        screenmachine(e);
        if (!s_start_auto_connected) {
            s_start_auto_connected = true;
            lv_timer_t *t = lv_timer_create(auto_connect_timer_cb, 300, NULL);
            lv_timer_set_repeat_count(t, 1);
        }
    }
}
static void ui_event_StartButtonL(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_CLICKED) {
        connectbutton(e);
    }
}
static void ui_event_StartButtonM(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_CLICKED) {
        _ui_screen_change(ui_Settings, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    }
}
static void ui_event_StartButtonR(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_CLICKED) {
        _ui_screen_change(ui_Home, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    }
}
static void ui_event_Home(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_SCREEN_LOADED) {
        screenmachine(e);
    }
}
static void ui_event_HomeButtonL(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_LONG_PRESSED) {
        if (addonsIsEjectEnabled()) {
          _ui_screen_change(ui_ejectaddon, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
          ejectcreampie(e);
          screenmachine(e);
        }
    } else if(event == LV_EVENT_CLICKED){
        pullOut(e);
        screenmachine(e);
    }
}
static void ui_event_HomeButtonM(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_LONG_PRESSED) {
        emergencyStop(e);
        screenmachine(e);
    } else if(event == LV_EVENT_CLICKED) {
        homebuttonmevent(e);
    }
}
static void ui_event_HomeButtonR(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_CLICKED) {
        g_pattern_return_screen = ui_Home;
        _ui_screen_change(ui_Pattern, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    } else if(event == LV_EVENT_LONG_PRESSED){
        if (addonsIsFistITEnabled()) {
            _ui_screen_change(ui_FistIT, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
        }
    }
}

static void ui_event_Menu(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    if(event == LV_EVENT_SCREEN_LOADED) {
        screenmachine(e);
        lv_group_focus_obj(ui_MenuButtonTL);
    }
}
static void ui_event_MenuButtonTL(lv_event_t * e)
{
    if(lv_event_get_code(e) == LV_EVENT_SHORT_CLICKED)
        _ui_screen_change(ui_Home, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
}
static void ui_event_MenuButtonTR(lv_event_t * e)
{
    if(lv_event_get_code(e) == LV_EVENT_SHORT_CLICKED)
        _ui_screen_change(ui_Stroke, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
}
static void ui_event_MenuButtonML(lv_event_t * e)
{
    if(lv_event_get_code(e) == LV_EVENT_SHORT_CLICKED)
        _ui_screen_change(ui_Settings, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
}
static void ui_event_MenuButtonMR(lv_event_t * e)
{
    if(lv_event_get_code(e) == LV_EVENT_SHORT_CLICKED)
        _ui_screen_change(ui_Addons, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
}
static void ui_event_MenuButtonL(lv_event_t * e)
{
    if(lv_event_get_code(e) == LV_EVENT_SHORT_CLICKED)
        _ui_screen_change(ui_Colors, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0); //was ui_home
}
static void ui_event_MenuButtonM(lv_event_t * e)
{
    if(lv_event_get_code(e) == LV_EVENT_SHORT_CLICKED)
        lv_obj_send_event(lv_group_get_focused(ui_g_menu), LV_EVENT_SHORT_CLICKED, NULL);
}
static void ui_event_MenuButtonR(lv_event_t * e)
{
    if(lv_event_get_code(e) == LV_EVENT_SHORT_CLICKED)
        menuRestartAction();

}
static void ui_event_Pattern(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_SCREEN_LOADED) {
        screenmachine(e);
    }
}
static void ui_event_PatternButtonL(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_CLICKED) {
        _ui_screen_change(ui_Menu, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    }
}
static void ui_event_PatternButtonM(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_CLICKED) {
        _ui_screen_change(ui_Home, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    }
}
static void ui_event_PatternButtonR(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_CLICKED) {
        savepattern(e);
        
        lv_obj_t *dest = g_pattern_return_screen ? g_pattern_return_screen : ui_Home;
        _ui_screen_change(dest, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    }
}
static void ui_event_Torqe(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_SCREEN_LOADED) {
        screenmachine(e);
    }
}
static void ui_event_TorqeButtonL(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
}
static void ui_event_TorqeButtonM(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_CLICKED) {
        _ui_screen_change(ui_Home, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    }
}
static void ui_event_TorqeButtonR(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_CLICKED) {
        _ui_screen_change(ui_Menu, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    }
}
static void ui_event_EJECTSettings(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_SCREEN_LOADED) {
        screenmachine(e);
    }
}
static void ui_event_EJECTButtonL(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_CLICKED) {
        _ui_screen_change(ui_Home, LV_SCR_LOAD_ANIM_FADE_ON, 90, 0);
    }
}
static void ui_event_EJECTButtonM(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
}
static void ui_event_EJECTButtonR(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_CLICKED) {
        _ui_screen_change(ui_Menu, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    }
}
static void ui_event_Settings(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_SCREEN_LOADED) {
        screenmachine(e);
    }
}
static void ui_event_SettingsButtonL(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_CLICKED) {
        savesettings(e);
    }
}
static void ui_event_SettingsButtonM(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_CLICKED) {
        _ui_screen_change(ui_Home, LV_SCR_LOAD_ANIM_FADE_ON, 90, 0);
    }
}
static void ui_event_SettingsButtonR(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_CLICKED) {
        _ui_screen_change(ui_Menu, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    }
}
static void ui_event_ejectaddon(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
}

static void ui_event_Brightness_sliderChange(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        brightness_slider_event_cb(e);
    }
}

///////////////////// SCREENS ////////////////////
void ui_Start_screen_init(void)
{

    // ui_Start

    ui_Start = lv_obj_create(NULL);

    lv_obj_clear_flag(ui_Start, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(ui_Start, ui_event_Start, LV_EVENT_ALL, NULL);


    // ui_Logo

    ui_Logo = lv_label_create(ui_Start);

    lv_obj_set_width(ui_Logo, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_Logo, LV_SIZE_CONTENT);

    lv_obj_set_y(ui_Logo, -103);
    lv_obj_set_x(ui_Logo, lv_pct(0));

    lv_obj_set_align(ui_Logo, LV_ALIGN_CENTER);

    lv_label_set_text(ui_Logo, T_HEADER);

    lv_obj_set_style_text_font(ui_Logo, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    applyTitleBarStyle(ui_Logo);

    // ui_StartButtonL

    ui_StartButtonL = lv_btn_create(ui_Start);

    lv_obj_set_width(ui_StartButtonL, 100);
    lv_obj_set_height(ui_StartButtonL, 30);

    lv_obj_set_y(ui_StartButtonL, 100);
    lv_obj_set_x(ui_StartButtonL, lv_pct(-33));

    lv_obj_set_align(ui_StartButtonL, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_StartButtonL, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_StartButtonL, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(ui_StartButtonL, ui_event_StartButtonL, LV_EVENT_ALL, NULL);
    applyButtonStyles(ui_StartButtonL, &style_button_l, &style_button_l_focused, NULL);

    // ui_StartButtonLText

    ui_StartButtonLText = lv_label_create(ui_StartButtonL);

    lv_obj_set_width(ui_StartButtonLText, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_StartButtonLText, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_StartButtonLText, lv_pct(0));
    lv_obj_set_y(ui_StartButtonLText, lv_pct(0));

    lv_obj_set_align(ui_StartButtonLText, LV_ALIGN_CENTER);

    lv_label_set_text(ui_StartButtonLText, T_CONNECT);
    applyTextPrimaryStyle(ui_StartButtonLText);

    // ui_StartButtonM

    ui_StartButtonM = lv_btn_create(ui_Start);

    lv_obj_set_width(ui_StartButtonM, 100);
    lv_obj_set_height(ui_StartButtonM, 30);

    lv_obj_set_y(ui_StartButtonM, 100);
    lv_obj_set_x(ui_StartButtonM, lv_pct(0));

    lv_obj_set_align(ui_StartButtonM, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_StartButtonM, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_StartButtonM, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(ui_StartButtonM, ui_event_StartButtonM, LV_EVENT_ALL, NULL);
    applyButtonStyles(ui_StartButtonM, &style_button_m, &style_button_m_focused, NULL);

    // ui_StartButtonMText

    ui_StartButtonMText = lv_label_create(ui_StartButtonM);

    lv_obj_set_width(ui_StartButtonMText, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_StartButtonMText, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_StartButtonMText, lv_pct(0));
    lv_obj_set_y(ui_StartButtonMText, lv_pct(0));

    lv_obj_set_align(ui_StartButtonMText, LV_ALIGN_CENTER);

    lv_label_set_text(ui_StartButtonMText, T_SETTINGS);
    applyTextPrimaryStyle(ui_StartButtonMText);

    // ui_StartButtonR

    ui_StartButtonR = lv_btn_create(ui_Start);

    lv_obj_set_width(ui_StartButtonR, 100);
    lv_obj_set_height(ui_StartButtonR, 30);

    lv_obj_set_y(ui_StartButtonR, 100);
    lv_obj_set_x(ui_StartButtonR, lv_pct(33));

    lv_obj_set_align(ui_StartButtonR, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_StartButtonR, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_StartButtonR, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(ui_StartButtonR, ui_event_StartButtonR, LV_EVENT_ALL, NULL);
    applyButtonStyles(ui_StartButtonR, &style_button_r, &style_button_r_focused, NULL);

    // ui_StartButtonRText

    ui_StartButtonRText = lv_label_create(ui_StartButtonR);

    lv_obj_set_width(ui_StartButtonRText, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_StartButtonRText, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_StartButtonRText, lv_pct(0));
    lv_obj_set_y(ui_StartButtonRText, lv_pct(0));

    lv_obj_set_align(ui_StartButtonRText, LV_ALIGN_CENTER);

    lv_label_set_text(ui_StartButtonRText, T_DEMO);
    applyTextPrimaryStyle(ui_StartButtonRText);

    // ui_LOVE_Logo

    ui_LOVE_Logo = lv_img_create(ui_Start);
    lv_img_set_src(ui_LOVE_Logo, &image50x50);

    lv_obj_set_width(ui_LOVE_Logo, 100);    lv_obj_set_height(ui_LOVE_Logo, 100);

    lv_obj_set_x(ui_LOVE_Logo, 0);
    lv_obj_set_y(ui_LOVE_Logo, 10);

    lv_obj_set_align(ui_LOVE_Logo, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_LOVE_Logo, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_clear_flag(ui_LOVE_Logo, LV_OBJ_FLAG_SCROLLABLE);


    // ui_Welcome

    ui_Welcome = lv_label_create(ui_Start);

    lv_obj_set_width(ui_Welcome, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_Welcome, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_Welcome, 0);
    lv_obj_set_y(ui_Welcome, -70);

    lv_obj_set_align(ui_Welcome, LV_ALIGN_CENTER);

    lv_label_set_text(ui_Welcome, T_MOTD);

    // ui_Batt

    ui_Batt = lv_label_create(ui_Start);

    lv_obj_set_width(ui_Batt, 85);
    lv_obj_set_height(ui_Batt, 30);

    lv_obj_set_x(ui_Batt, 115);
    lv_obj_set_y(ui_Batt, -103);

    lv_obj_set_align(ui_Batt, LV_ALIGN_CENTER);

    lv_label_set_text(ui_Batt, T_BATT);

    // ui_BattValue

    ui_BattValue = lv_label_create(ui_Batt);

    lv_obj_set_width(ui_BattValue, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_BattValue, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_BattValue, 0);
    lv_obj_set_y(ui_BattValue, -7);

    lv_obj_set_align(ui_BattValue, LV_ALIGN_RIGHT_MID);

    lv_label_set_text(ui_BattValue, T_BLANK);

    // ui_Battery

    ui_Battery = lv_bar_create(ui_Batt);
    lv_bar_set_range(ui_Battery, 0, 100);

    lv_obj_set_width(ui_Battery, 80);
    lv_obj_set_height(ui_Battery, 10);

    lv_obj_set_x(ui_Battery, 0);
    lv_obj_set_y(ui_Battery, 10);

    lv_obj_set_align(ui_Battery, LV_ALIGN_CENTER);

    applyBatteryStyles(ui_Battery);

}
void ui_Home_screen_init(void)
{

    // ui_Home

    ui_Home = lv_obj_create(NULL);

    lv_obj_clear_flag(ui_Home, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(ui_Home, ui_event_Home, LV_EVENT_ALL, NULL);

    // ui_Logo2

    ui_Logo2 = lv_label_create(ui_Home);

    lv_obj_set_width(ui_Logo2, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_Logo2, LV_SIZE_CONTENT);

    lv_obj_set_y(ui_Logo2, -103);
    lv_obj_set_x(ui_Logo2, lv_pct(0));

    lv_obj_set_align(ui_Logo2, LV_ALIGN_CENTER);

    lv_label_set_text(ui_Logo2, T_HEADER);

    lv_obj_set_style_text_font(ui_Logo2, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    applyTitleBarStyle(ui_Logo2);

    // ui_HomeButtonL

    ui_HomeButtonL = lv_btn_create(ui_Home);

    lv_obj_set_width(ui_HomeButtonL, 100);
    lv_obj_set_height(ui_HomeButtonL, 30);

    lv_obj_set_y(ui_HomeButtonL, 100);
    lv_obj_set_x(ui_HomeButtonL, lv_pct(-33));

    lv_obj_set_align(ui_HomeButtonL, LV_ALIGN_CENTER);

    lv_obj_add_state(ui_HomeButtonL, LV_STATE_DISABLED);

    lv_obj_add_flag(ui_HomeButtonL, LV_OBJ_FLAG_CHECKABLE | LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_HomeButtonL, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(ui_HomeButtonL, ui_event_HomeButtonL, LV_EVENT_ALL, NULL);
    applyButtonStyles(ui_HomeButtonL, &style_button_l, &style_button_l_focused, &style_button_l_disabled);

    // ui_HomeButtonLText

    ui_HomeButtonLText = lv_label_create(ui_HomeButtonL);

    lv_obj_set_width(ui_HomeButtonLText, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_HomeButtonLText, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_HomeButtonLText, lv_pct(0));
    lv_obj_set_y(ui_HomeButtonLText, lv_pct(0));

    lv_obj_set_align(ui_HomeButtonLText, LV_ALIGN_CENTER);

    lv_label_set_text(ui_HomeButtonLText, T_CREAMPIE);
    applyTextPrimaryStyle(ui_HomeButtonLText);

    // ui_HomeButtonM

    ui_HomeButtonM = lv_btn_create(ui_Home);

    lv_obj_set_width(ui_HomeButtonM, 100);
    lv_obj_set_height(ui_HomeButtonM, 30);

    lv_obj_set_y(ui_HomeButtonM, 100);
    lv_obj_set_x(ui_HomeButtonM, lv_pct(0));

    lv_obj_set_align(ui_HomeButtonM, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_HomeButtonM, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_HomeButtonM, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(ui_HomeButtonM, ui_event_HomeButtonM, LV_EVENT_ALL, NULL);
    applyButtonStyles(ui_HomeButtonM, &style_button_m, &style_button_m_focused, NULL);

    // ui_HomeButtonMText

    ui_HomeButtonMText = lv_label_create(ui_HomeButtonM);

    lv_obj_set_width(ui_HomeButtonMText, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_HomeButtonMText, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_HomeButtonMText, lv_pct(0));
    lv_obj_set_y(ui_HomeButtonMText, lv_pct(0));

    lv_obj_set_align(ui_HomeButtonMText, LV_ALIGN_CENTER);

    lv_label_set_text(ui_HomeButtonMText, T_START);
    applyTextPrimaryStyle(ui_HomeButtonMText);

    // ui_HomeButtonR

    ui_HomeButtonR = lv_btn_create(ui_Home);

    lv_obj_set_width(ui_HomeButtonR, 100);
    lv_obj_set_height(ui_HomeButtonR, 30);

    lv_obj_set_y(ui_HomeButtonR, 100);
    lv_obj_set_x(ui_HomeButtonR, lv_pct(33));

    lv_obj_set_align(ui_HomeButtonR, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_HomeButtonR, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_HomeButtonR, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(ui_HomeButtonR, ui_event_HomeButtonR, LV_EVENT_ALL, NULL);
    applyButtonStyles(ui_HomeButtonR, &style_button_r, &style_button_r_focused, NULL);

    // ui_HomeButtonRText

    ui_HomeButtonRText = lv_label_create(ui_HomeButtonR);

    lv_obj_set_width(ui_HomeButtonRText, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_HomeButtonRText, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_HomeButtonRText, lv_pct(0));
    lv_obj_set_y(ui_HomeButtonRText, lv_pct(0));

    lv_obj_set_align(ui_HomeButtonRText, LV_ALIGN_CENTER);

    lv_label_set_text(ui_HomeButtonRText, T_PATTERN_Button);
    applyTextPrimaryStyle(ui_HomeButtonRText);
    lv_obj_set_style_text_font(ui_HomeButtonRText, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_SpeedL

    ui_SpeedL = lv_label_create(ui_Home);

    lv_obj_set_width(ui_SpeedL, lv_pct(95));
    lv_obj_set_height(ui_SpeedL, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_SpeedL, 0);
    lv_obj_set_y(ui_SpeedL, -60);

    lv_obj_set_align(ui_SpeedL, LV_ALIGN_CENTER);

    lv_label_set_text(ui_SpeedL, T_SPEED);

    lv_obj_set_style_text_font(ui_SpeedL, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_homespeedslider

    ui_homespeedslider = lv_slider_create(ui_SpeedL);
    lv_slider_set_range(ui_homespeedslider, 0, speedlimit);
    //lv_slider_set_mode(ui_homespeedslider, LV_SLIDER_MODE_SYMMETRICAL);

    lv_obj_set_width(ui_homespeedslider, 130);
    lv_obj_set_height(ui_homespeedslider, 10);

    lv_obj_set_x(ui_homespeedslider, -15);
    lv_obj_set_y(ui_homespeedslider, 0);

    lv_obj_set_align(ui_homespeedslider, LV_ALIGN_RIGHT_MID);

    applySliderStyles(ui_homespeedslider, 0);

    // ui_homespeedvalue

    ui_homespeedvalue = lv_label_create(ui_SpeedL);

    lv_obj_set_width(ui_homespeedvalue, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_homespeedvalue, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_homespeedvalue, 80);
    lv_obj_set_y(ui_homespeedvalue, 0);

    lv_obj_set_align(ui_homespeedvalue, LV_ALIGN_LEFT_MID);

    lv_label_set_text(ui_homespeedvalue, T_BLANK);

    // ui_DepthL

    ui_DepthL = lv_label_create(ui_Home);

    lv_obj_set_width(ui_DepthL, lv_pct(95));
    lv_obj_set_height(ui_DepthL, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_DepthL, 0);
    lv_obj_set_y(ui_DepthL, -25);

    lv_obj_set_align(ui_DepthL, LV_ALIGN_CENTER);

    lv_label_set_text(ui_DepthL, T_DEPTH);

    lv_obj_set_style_text_font(ui_DepthL, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_homedepthslider

    ui_homedepthslider = lv_slider_create(ui_DepthL);
    lv_slider_set_range(ui_homedepthslider, 0, maxdepthinmm);

    lv_obj_set_width(ui_homedepthslider, 130);
    lv_obj_set_height(ui_homedepthslider, 10);

    lv_obj_set_x(ui_homedepthslider, -15);
    lv_obj_set_y(ui_homedepthslider, 0);

    lv_obj_set_align(ui_homedepthslider, LV_ALIGN_RIGHT_MID);

    applySliderStyles(ui_homedepthslider, 1);

    // ui_homedepthvalue

    ui_homedepthvalue = lv_label_create(ui_DepthL);

    lv_obj_set_width(ui_homedepthvalue, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_homedepthvalue, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_homedepthvalue, 80);
    lv_obj_set_y(ui_homedepthvalue, 0);

    lv_obj_set_align(ui_homedepthvalue, LV_ALIGN_LEFT_MID);

    lv_label_set_text(ui_homedepthvalue, T_BLANK);

    // ui_StrokeL

    ui_StrokeL = lv_label_create(ui_Home);

    lv_obj_set_width(ui_StrokeL, lv_pct(95));
    lv_obj_set_height(ui_StrokeL, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_StrokeL, 0);
    lv_obj_set_y(ui_StrokeL, 10);

    lv_obj_set_align(ui_StrokeL, LV_ALIGN_CENTER);

    lv_label_set_text(ui_StrokeL, T_STROKE);

    lv_obj_set_style_text_font(ui_StrokeL, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_homestrokeslider

    ui_homestrokeslider = lv_slider_create(ui_StrokeL);
    lv_slider_set_range(ui_homestrokeslider, 0, maxdepthinmm);
    lv_bar_set_mode(ui_homestrokeslider, LV_BAR_MODE_RANGE);

    lv_obj_set_width(ui_homestrokeslider, 130);
    lv_obj_set_height(ui_homestrokeslider, 10);

    lv_obj_set_x(ui_homestrokeslider, -15);
    lv_obj_set_y(ui_homestrokeslider, 0);

    lv_obj_set_align(ui_homestrokeslider, LV_ALIGN_RIGHT_MID);

    applySliderStyles(ui_homestrokeslider, 2);

    // ui_homestrokevalue

    ui_homestrokevalue = lv_label_create(ui_StrokeL);

    lv_obj_set_width(ui_homestrokevalue, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_homestrokevalue, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_homestrokevalue, 80);
    lv_obj_set_y(ui_homestrokevalue, 0);

    lv_obj_set_align(ui_homestrokevalue, LV_ALIGN_LEFT_MID);

    lv_label_set_text(ui_homestrokevalue, T_BLANK);

    // ui_SensationL

    ui_SensationL = lv_label_create(ui_Home);

    lv_obj_set_width(ui_SensationL, lv_pct(95));
    lv_obj_set_height(ui_SensationL, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_SensationL, 0);
    lv_obj_set_y(ui_SensationL, 45);

    lv_obj_set_align(ui_SensationL, LV_ALIGN_CENTER);

    lv_label_set_text(ui_SensationL, T_SENSATION);

    lv_obj_set_style_text_font(ui_SensationL, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_homesensationslider

    ui_homesensationslider = lv_slider_create(ui_SensationL);
    lv_slider_set_range(ui_homesensationslider, -100, 100);
    lv_slider_set_mode(ui_homesensationslider, LV_SLIDER_MODE_SYMMETRICAL);
    lv_slider_set_value(ui_homesensationslider, 0, LV_ANIM_OFF);

    lv_obj_set_width(ui_homesensationslider, 170);
    lv_obj_set_height(ui_homesensationslider, 10);

    lv_obj_set_x(ui_homesensationslider, -15);
    lv_obj_set_y(ui_homesensationslider, 0);

    lv_obj_set_align(ui_homesensationslider, LV_ALIGN_RIGHT_MID);

    applySliderStyles(ui_homesensationslider, 3);

    // ui_Batt2

    ui_Batt2 = lv_label_create(ui_Home);

    lv_obj_set_width(ui_Batt2, 85);
    lv_obj_set_height(ui_Batt2, 30);

    lv_obj_set_x(ui_Batt2, 115);
    lv_obj_set_y(ui_Batt2, -103);

    lv_obj_set_align(ui_Batt2, LV_ALIGN_CENTER);

    lv_label_set_text(ui_Batt2, T_BATT);

    // ui_BattValue2

    ui_BattValue2 = lv_label_create(ui_Batt2);

    lv_obj_set_width(ui_BattValue2, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_BattValue2, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_BattValue2, 0);
    lv_obj_set_y(ui_BattValue2, -7);

    lv_obj_set_align(ui_BattValue2, LV_ALIGN_RIGHT_MID);

    lv_label_set_text(ui_BattValue2, T_BLANK);


    // ui_Battery2

    ui_Battery2 = lv_bar_create(ui_Batt2);
    lv_bar_set_range(ui_Battery2, 0, 100);

    lv_obj_set_width(ui_Battery2, 80);
    lv_obj_set_height(ui_Battery2, 10);

    lv_obj_set_x(ui_Battery2, 0);
    lv_obj_set_y(ui_Battery2, 10);

    lv_obj_set_align(ui_Battery2, LV_ALIGN_CENTER);

    applyBatteryStyles(ui_Battery2);

    // ui_HomePatternLabel1

    ui_HomePatternLabel1 = lv_label_create(ui_Home);

    lv_obj_set_width(ui_HomePatternLabel1, lv_pct(95));
    lv_obj_set_height(ui_HomePatternLabel1, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_HomePatternLabel1, 10);
    lv_obj_set_y(ui_HomePatternLabel1, 70);

    lv_obj_set_align(ui_HomePatternLabel1, LV_ALIGN_LEFT_MID);

    lv_label_set_text(ui_HomePatternLabel1, T_Patterns);

    lv_obj_set_style_text_font(ui_HomePatternLabel1, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_HomePatternLabel

    ui_HomePatternLabel = lv_label_create(ui_HomePatternLabel1);

    lv_obj_set_width(ui_HomePatternLabel, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_HomePatternLabel, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_HomePatternLabel, 120);
    lv_obj_set_y(ui_HomePatternLabel, 0);

    lv_obj_set_align(ui_HomePatternLabel, LV_ALIGN_LEFT_MID);

    lv_label_set_text(ui_HomePatternLabel, T_BLANK);

    // ui_connect

    ui_connect = lv_label_create(ui_Home);

    lv_obj_set_width(ui_connect, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_connect, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_connect, -113);
    lv_obj_set_y(ui_connect, -102);

    lv_obj_set_align(ui_connect, LV_ALIGN_CENTER);

    lv_label_set_text(ui_connect, T_BLANK);

}
void ui_Menu_screen_init(void)
{
    ui_Menu = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Menu, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(ui_Menu, ui_event_Menu, LV_EVENT_ALL, NULL);

    // Header
    ui_LogoMenu = lv_label_create(ui_Menu);
    lv_obj_set_width(ui_LogoMenu, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_LogoMenu, LV_SIZE_CONTENT);
    lv_obj_set_y(ui_LogoMenu, -103);
    lv_obj_set_align(ui_LogoMenu, LV_ALIGN_CENTER);
    lv_label_set_text(ui_LogoMenu, T_SCREEN_MENU);
    lv_obj_set_style_text_font(ui_LogoMenu, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_LogoMenu, &style_title_bar, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ---- Top-Left tile: Home ----
    ui_MenuButtonTL = lv_btn_create(ui_Menu);
    lv_obj_set_width(ui_MenuButtonTL, 140);
    lv_obj_set_height(ui_MenuButtonTL, 75);
    lv_obj_set_x(ui_MenuButtonTL, -75);
    lv_obj_set_y(ui_MenuButtonTL, -37);
    lv_obj_set_align(ui_MenuButtonTL, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_MenuButtonTL, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(ui_MenuButtonTL, &style_slider_indicator[0], LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(ui_MenuButtonTL, ui_event_MenuButtonTL, LV_EVENT_ALL, NULL);

    ui_MenuButtonTLText = lv_label_create(ui_MenuButtonTL);
    lv_obj_set_align(ui_MenuButtonTLText, LV_ALIGN_CENTER);
    lv_label_set_text(ui_MenuButtonTLText, T_HOMESCREEN);
    applyTextPrimaryStyle(ui_MenuButtonTLText);

    // ---- Top-Right tile: Bator Mode ----
    ui_MenuButtonTR = lv_btn_create(ui_Menu);
    lv_obj_set_width(ui_MenuButtonTR, 140);
    lv_obj_set_height(ui_MenuButtonTR, 75);
    lv_obj_set_x(ui_MenuButtonTR, 75);
    lv_obj_set_y(ui_MenuButtonTR, -37);
    lv_obj_set_align(ui_MenuButtonTR, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_MenuButtonTR, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(ui_MenuButtonTR, &style_slider_indicator[1], LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(ui_MenuButtonTR, ui_event_MenuButtonTR, LV_EVENT_ALL, NULL);

    ui_MenuButtonTRText = lv_label_create(ui_MenuButtonTR);
    lv_obj_set_align(ui_MenuButtonTRText, LV_ALIGN_CENTER);
    lv_label_set_text(ui_MenuButtonTRText, T_STROKE_SCREEN);
    applyTextPrimaryStyle(ui_MenuButtonTRText);

    // ---- Mid-Left tile: Settings ----
    ui_MenuButtonML = lv_btn_create(ui_Menu);
    lv_obj_set_width(ui_MenuButtonML, 140);
    lv_obj_set_height(ui_MenuButtonML, 75);
    lv_obj_set_x(ui_MenuButtonML, -75);
    lv_obj_set_y(ui_MenuButtonML, 43);
    lv_obj_set_align(ui_MenuButtonML, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_MenuButtonML, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(ui_MenuButtonML, &style_slider_indicator[2], LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(ui_MenuButtonML, ui_event_MenuButtonML, LV_EVENT_ALL, NULL);

    ui_MenuButtonMLText = lv_label_create(ui_MenuButtonML);
    lv_obj_set_align(ui_MenuButtonMLText, LV_ALIGN_CENTER);
    lv_label_set_text(ui_MenuButtonMLText, T_SETTINGS);
    applyTextPrimaryStyle(ui_MenuButtonMLText);

    // ---- Mid-Right tile: Color Schemes ----
    ui_MenuButtonMR = lv_btn_create(ui_Menu);
    lv_obj_set_width(ui_MenuButtonMR, 140);
    lv_obj_set_height(ui_MenuButtonMR, 75);
    lv_obj_set_x(ui_MenuButtonMR, 75);
    lv_obj_set_y(ui_MenuButtonMR, 43);
    lv_obj_set_align(ui_MenuButtonMR, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_MenuButtonMR, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(ui_MenuButtonMR, &style_slider_indicator[3], LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(ui_MenuButtonMR, ui_event_MenuButtonMR, LV_EVENT_ALL, NULL);

    ui_MenuButtonMRText = lv_label_create(ui_MenuButtonMR);
    lv_obj_set_align(ui_MenuButtonMRText, LV_ALIGN_CENTER);
    lv_label_set_text(ui_MenuButtonMRText, T_ADDONS);
    applyTextPrimaryStyle(ui_MenuButtonMRText);

    // ---- Bottom buttons ----
    ui_MenuButtonL = lv_btn_create(ui_Menu);
    lv_obj_set_width(ui_MenuButtonL, 100);
    lv_obj_set_height(ui_MenuButtonL, 30);
    lv_obj_set_x(ui_MenuButtonL, lv_pct(-33));
    lv_obj_set_y(ui_MenuButtonL, 103);
    lv_obj_set_align(ui_MenuButtonL, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_MenuButtonL, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(ui_MenuButtonL, &style_button_l, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(ui_MenuButtonL, ui_event_MenuButtonL, LV_EVENT_ALL, NULL);

    ui_MenuButtonLText = lv_label_create(ui_MenuButtonL);
    lv_obj_set_align(ui_MenuButtonLText, LV_ALIGN_CENTER);
    lv_label_set_text(ui_MenuButtonLText, T_SCREEN_COLORS);
    applyTextPrimaryStyle(ui_MenuButtonLText);

    ui_MenuButtonM = lv_btn_create(ui_Menu);
    lv_obj_set_width(ui_MenuButtonM, 100);
    lv_obj_set_height(ui_MenuButtonM, 30);
    lv_obj_set_x(ui_MenuButtonM, lv_pct(0));
    lv_obj_set_y(ui_MenuButtonM, 103);
    lv_obj_set_align(ui_MenuButtonM, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_MenuButtonM, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(ui_MenuButtonM, &style_button_m, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(ui_MenuButtonM, ui_event_MenuButtonM, LV_EVENT_ALL, NULL);

    ui_MenuButtonMText = lv_label_create(ui_MenuButtonM);
    lv_obj_set_align(ui_MenuButtonMText, LV_ALIGN_CENTER);
    lv_label_set_text(ui_MenuButtonMText, T_SELECT);
    applyTextPrimaryStyle(ui_MenuButtonMText);

    ui_MenuButtonR = lv_btn_create(ui_Menu);
    lv_obj_set_width(ui_MenuButtonR, 100);
    lv_obj_set_height(ui_MenuButtonR, 30);
    lv_obj_set_x(ui_MenuButtonR, lv_pct(33));
    lv_obj_set_y(ui_MenuButtonR, 103);
    lv_obj_set_align(ui_MenuButtonR, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_MenuButtonR, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(ui_MenuButtonR, &style_button_r, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(ui_MenuButtonR, ui_event_MenuButtonR, LV_EVENT_ALL, NULL);

    ui_MenuButtonRText = lv_label_create(ui_MenuButtonR);
    lv_obj_set_align(ui_MenuButtonRText, LV_ALIGN_CENTER);
    lv_label_set_text(ui_MenuButtonRText, T_RESTART);
    applyTextPrimaryStyle(ui_MenuButtonRText);

    // ---- Battery display ----
    ui_Batt3 = lv_label_create(ui_Menu);
    lv_obj_set_width(ui_Batt3, 85);
    lv_obj_set_height(ui_Batt3, 30);
    lv_obj_set_x(ui_Batt3, 115);
    lv_obj_set_y(ui_Batt3, -103);
    lv_obj_set_align(ui_Batt3, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Batt3, T_BATT);

    ui_BattValue3 = lv_label_create(ui_Batt3);
    lv_obj_set_width(ui_BattValue3, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_BattValue3, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_BattValue3, 0);
    lv_obj_set_y(ui_BattValue3, -7);
    lv_obj_set_align(ui_BattValue3, LV_ALIGN_RIGHT_MID);
    lv_label_set_text(ui_BattValue3, T_BLANK);

    ui_Battery3 = lv_bar_create(ui_Batt3);
    lv_bar_set_range(ui_Battery3, 0, 100);
    lv_obj_set_width(ui_Battery3, 80);
    lv_obj_set_height(ui_Battery3, 10);
    lv_obj_set_x(ui_Battery3, 0);
    lv_obj_set_y(ui_Battery3, 10);
    lv_obj_set_align(ui_Battery3, LV_ALIGN_CENTER);
    lv_obj_add_style(ui_Battery3, &style_battery_main,      LV_PART_MAIN      | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_Battery3, &style_battery_indicator, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    // ---- Focus group (tiles) ----
    ui_g_menu = lv_group_create();
    lv_group_set_wrap(ui_g_menu, true);
    lv_group_add_obj(ui_g_menu, ui_MenuButtonTL);
    lv_group_add_obj(ui_g_menu, ui_MenuButtonTR);
    lv_group_add_obj(ui_g_menu, ui_MenuButtonML);
    lv_group_add_obj(ui_g_menu, ui_MenuButtonMR);
}


void ui_Pattern_screen_init(void)
{

    // ui_Pattern

    ui_Pattern = lv_obj_create(NULL);

    lv_obj_clear_flag(ui_Pattern, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(ui_Pattern, ui_event_Pattern, LV_EVENT_ALL, NULL);

    // ui_Logo5

    ui_Logo5 = lv_label_create(ui_Pattern);

    lv_obj_set_width(ui_Logo5, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_Logo5, LV_SIZE_CONTENT);

    lv_obj_set_y(ui_Logo5, -103);
    lv_obj_set_x(ui_Logo5, lv_pct(0));

    lv_obj_set_align(ui_Logo5, LV_ALIGN_CENTER);

    lv_label_set_text(ui_Logo5, T_HEADER);

    lv_obj_set_style_text_font(ui_Logo5, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    applyTitleBarStyle(ui_Logo5);

    // ui_PatternButtonL

    ui_PatternButtonL = lv_btn_create(ui_Pattern);

    lv_obj_set_width(ui_PatternButtonL, 100);
    lv_obj_set_height(ui_PatternButtonL, 30);

    lv_obj_set_y(ui_PatternButtonL, 100);
    lv_obj_set_x(ui_PatternButtonL, lv_pct(-33));

    lv_obj_set_align(ui_PatternButtonL, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_PatternButtonL, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_PatternButtonL, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(ui_PatternButtonL, ui_event_PatternButtonL, LV_EVENT_ALL, NULL);
    applyButtonStyles(ui_PatternButtonL, &style_button_l, &style_button_l_focused, NULL);

    // ui_PatternButtonLText

    ui_PatternButtonLText = lv_label_create(ui_PatternButtonL);

    lv_obj_set_width(ui_PatternButtonLText, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_PatternButtonLText, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_PatternButtonLText, lv_pct(0));
    lv_obj_set_y(ui_PatternButtonLText, lv_pct(0));

    lv_obj_set_align(ui_PatternButtonLText, LV_ALIGN_CENTER);

    lv_label_set_text(ui_PatternButtonLText, T_MENU);
    applyTextPrimaryStyle(ui_PatternButtonLText);

    // ui_PatternButtonM

    ui_PatternButtonM = lv_btn_create(ui_Pattern);

    lv_obj_set_width(ui_PatternButtonM, 100);
    lv_obj_set_height(ui_PatternButtonM, 30);

    lv_obj_set_y(ui_PatternButtonM, 100);
    lv_obj_set_x(ui_PatternButtonM, lv_pct(0));

    lv_obj_set_align(ui_PatternButtonM, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_PatternButtonM, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_PatternButtonM, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(ui_PatternButtonM, ui_event_PatternButtonM, LV_EVENT_ALL, NULL);
    applyButtonStyles(ui_PatternButtonM, &style_button_m, &style_button_m_focused, NULL);

    // ui_PatternButtonMText

    ui_PatternButtonMText = lv_label_create(ui_PatternButtonM);

    lv_obj_set_width(ui_PatternButtonMText, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_PatternButtonMText, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_PatternButtonMText, lv_pct(0));
    lv_obj_set_y(ui_PatternButtonMText, lv_pct(0));

    lv_obj_set_align(ui_PatternButtonMText, LV_ALIGN_CENTER);

    lv_label_set_text(ui_PatternButtonMText, T_HOME);
    applyTextPrimaryStyle(ui_PatternButtonMText);

    // ui_PatternButtonR

    ui_PatternButtonR = lv_btn_create(ui_Pattern);

    lv_obj_set_width(ui_PatternButtonR, 100);
    lv_obj_set_height(ui_PatternButtonR, 30);

    lv_obj_set_y(ui_PatternButtonR, 100);
    lv_obj_set_x(ui_PatternButtonR, lv_pct(33));

    lv_obj_set_align(ui_PatternButtonR, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_PatternButtonR, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_PatternButtonR, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(ui_PatternButtonR, ui_event_PatternButtonR, LV_EVENT_ALL, NULL);
    applyButtonStyles(ui_PatternButtonR, &style_button_r, &style_button_r_focused, NULL);

    // ui_PatternButtonRText

    ui_PatternButtonRText = lv_label_create(ui_PatternButtonR);

    lv_obj_set_width(ui_PatternButtonRText, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_PatternButtonRText, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_PatternButtonRText, lv_pct(0));
    lv_obj_set_y(ui_PatternButtonRText, lv_pct(0));

    lv_obj_set_align(ui_PatternButtonRText, LV_ALIGN_CENTER);

    lv_label_set_text(ui_PatternButtonRText, T_SAVE);
    applyTextPrimaryStyle(ui_PatternButtonRText);

    // ui_Batt5

    ui_Batt5 = lv_label_create(ui_Pattern);

    lv_obj_set_width(ui_Batt5, 85);
    lv_obj_set_height(ui_Batt5, 30);

    lv_obj_set_x(ui_Batt5, 115);
    lv_obj_set_y(ui_Batt5, -103);

    lv_obj_set_align(ui_Batt5, LV_ALIGN_CENTER);

    lv_label_set_text(ui_Batt5, T_BATT);

    // ui_BattValue5

    ui_BattValue5 = lv_label_create(ui_Batt5);

    lv_obj_set_width(ui_BattValue5, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_BattValue5, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_BattValue5, 0);
    lv_obj_set_y(ui_BattValue5, -7);

    lv_obj_set_align(ui_BattValue5, LV_ALIGN_RIGHT_MID);

    lv_label_set_text(ui_BattValue5, T_BLANK);

    // ui_Battery5

    ui_Battery5 = lv_bar_create(ui_Batt5);
    lv_bar_set_range(ui_Battery5, 0, 100);

    lv_obj_set_width(ui_Battery5, 80);
    lv_obj_set_height(ui_Battery5, 10);

    lv_obj_set_x(ui_Battery5, 0);
    lv_obj_set_y(ui_Battery5, 10);

    lv_obj_set_align(ui_Battery5, LV_ALIGN_CENTER);

    applyBatteryStyles(ui_Battery5);

    // ui_Label4

    ui_Label4 = lv_label_create(ui_Pattern);

    lv_obj_set_width(ui_Label4, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_Label4, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_Label4, 0);
    lv_obj_set_y(ui_Label4, -60);

    lv_obj_set_align(ui_Label4, LV_ALIGN_CENTER);

    lv_label_set_text(ui_Label4, T_SELECT_PATTERN);

    lv_obj_set_style_text_font(ui_Label4, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

// ui_PatternBand0/1/2 — coloured row backgrounds (drawn before roller so roller sits on top)

    ui_PatternBand0 = lv_obj_create(ui_Pattern);
    lv_obj_set_width(ui_PatternBand0, lv_pct(95));
    lv_obj_set_height(ui_PatternBand0, 39);
    lv_obj_set_align(ui_PatternBand0, LV_ALIGN_CENTER);
    lv_obj_set_y(ui_PatternBand0, -24);
    lv_obj_clear_flag(ui_PatternBand0, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_style(ui_PatternBand0, &style_slider_indicator[0], LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_PatternBand1 = lv_obj_create(ui_Pattern);
    lv_obj_set_width(ui_PatternBand1, lv_pct(95));
    lv_obj_set_height(ui_PatternBand1, 39);
    lv_obj_set_align(ui_PatternBand1, LV_ALIGN_CENTER);
    lv_obj_set_y(ui_PatternBand1, 15);
    lv_obj_clear_flag(ui_PatternBand1, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_style(ui_PatternBand1, &style_slider_indicator[1], LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_PatternBand2 = lv_obj_create(ui_Pattern);
    lv_obj_set_width(ui_PatternBand2, lv_pct(95));
    lv_obj_set_height(ui_PatternBand2, 39);
    lv_obj_set_align(ui_PatternBand2, LV_ALIGN_CENTER);
    lv_obj_set_y(ui_PatternBand2, 54);
    lv_obj_clear_flag(ui_PatternBand2, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_style(ui_PatternBand2, &style_slider_indicator[2], LV_PART_MAIN | LV_STATE_DEFAULT);

// ui_PatternS

    ui_PatternS = lv_roller_create(ui_Pattern);
    lv_roller_set_options(ui_PatternS,
                          "SimpleStroke\nTeasingPounding\nRoboStroke\nHalfnHalf\nDeeper\nStopNGo\nInsist", //\nKnot"
                          LV_ROLLER_MODE_NORMAL);

    lv_obj_set_height(ui_PatternS, 119);
    lv_obj_set_width(ui_PatternS, lv_pct(95));

    lv_obj_set_x(ui_PatternS, 0);
    lv_obj_set_y(ui_PatternS, 15);

    lv_obj_set_align(ui_PatternS, LV_ALIGN_CENTER);

    lv_obj_add_style(ui_PatternS, &style_roller_main, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_PatternS, &style_roller, LV_PART_SELECTED | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_PatternS, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_PatternS, LV_OPA_TRANSP, LV_PART_SELECTED | LV_STATE_DEFAULT);

}
void ui_Torqe_screen_init(void)
{

    // ui_Torqe

    ui_Torqe = lv_obj_create(NULL);

    lv_obj_clear_flag(ui_Torqe, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(ui_Torqe, ui_event_Torqe, LV_EVENT_ALL, NULL);

    // ui_Logo4

    ui_Logo4 = lv_label_create(ui_Torqe);

    lv_obj_set_width(ui_Logo4, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_Logo4, LV_SIZE_CONTENT);

    lv_obj_set_y(ui_Logo4, -103);
    lv_obj_set_x(ui_Logo4, lv_pct(0));

    lv_obj_set_align(ui_Logo4, LV_ALIGN_CENTER);

    lv_label_set_text(ui_Logo4, T_HEADER);

    lv_obj_set_style_text_font(ui_Logo4, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    applyTitleBarStyle(ui_Logo4);

    // ui_TorqeButtonL

    ui_TorqeButtonL = lv_btn_create(ui_Torqe);

    lv_obj_set_width(ui_TorqeButtonL, 100);
    lv_obj_set_height(ui_TorqeButtonL, 30);

    lv_obj_set_y(ui_TorqeButtonL, 100);
    lv_obj_set_x(ui_TorqeButtonL, lv_pct(-33));

    lv_obj_set_align(ui_TorqeButtonL, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_TorqeButtonL, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_TorqeButtonL, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(ui_TorqeButtonL, ui_event_TorqeButtonL, LV_EVENT_ALL, NULL);
    applyButtonStyles(ui_TorqeButtonL, &style_button_l, &style_button_l_focused, NULL);

    // ui_TorqeButtonLText

    ui_TorqeButtonLText = lv_label_create(ui_TorqeButtonL);

    lv_obj_set_width(ui_TorqeButtonLText, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_TorqeButtonLText, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_TorqeButtonLText, lv_pct(0));
    lv_obj_set_y(ui_TorqeButtonLText, lv_pct(0));

    lv_obj_set_align(ui_TorqeButtonLText, LV_ALIGN_CENTER);

    lv_label_set_text(ui_TorqeButtonLText, T_BLANK);
    applyTextPrimaryStyle(ui_TorqeButtonLText);

    // ui_TorqeButtonM

    ui_TorqeButtonM = lv_btn_create(ui_Torqe);

    lv_obj_set_width(ui_TorqeButtonM, 100);
    lv_obj_set_height(ui_TorqeButtonM, 30);

    lv_obj_set_y(ui_TorqeButtonM, 100);
    lv_obj_set_x(ui_TorqeButtonM, lv_pct(0));

    lv_obj_set_align(ui_TorqeButtonM, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_TorqeButtonM, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_TorqeButtonM, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(ui_TorqeButtonM, ui_event_TorqeButtonM, LV_EVENT_ALL, NULL);
    applyButtonStyles(ui_TorqeButtonM, &style_button_m, &style_button_m_focused, NULL);

    // ui_TorqeButtonMText

    ui_TorqeButtonMText = lv_label_create(ui_TorqeButtonM);

    lv_obj_set_width(ui_TorqeButtonMText, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_TorqeButtonMText, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_TorqeButtonMText, lv_pct(0));
    lv_obj_set_y(ui_TorqeButtonMText, lv_pct(0));

    lv_obj_set_align(ui_TorqeButtonMText, LV_ALIGN_CENTER);

    lv_label_set_text(ui_TorqeButtonMText, T_HOME);
    applyTextPrimaryStyle(ui_TorqeButtonMText);

    // ui_TorqeButtonR

    ui_TorqeButtonR = lv_btn_create(ui_Torqe);

    lv_obj_set_width(ui_TorqeButtonR, 100);
    lv_obj_set_height(ui_TorqeButtonR, 30);

    lv_obj_set_y(ui_TorqeButtonR, 100);
    lv_obj_set_x(ui_TorqeButtonR, lv_pct(33));

    lv_obj_set_align(ui_TorqeButtonR, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_TorqeButtonR, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_TorqeButtonR, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(ui_TorqeButtonR, ui_event_TorqeButtonR, LV_EVENT_ALL, NULL);
    applyButtonStyles(ui_TorqeButtonR, &style_button_r, &style_button_r_focused, NULL);

    // ui_TorqeButtonRText

    ui_TorqeButtonRText = lv_label_create(ui_TorqeButtonR);

    lv_obj_set_width(ui_TorqeButtonRText, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_TorqeButtonRText, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_TorqeButtonRText, lv_pct(0));
    lv_obj_set_y(ui_TorqeButtonRText, lv_pct(0));

    lv_obj_set_align(ui_TorqeButtonRText, LV_ALIGN_CENTER);

    lv_label_set_text(ui_TorqeButtonRText, T_MENU);
    applyTextPrimaryStyle(ui_TorqeButtonRText);

    // ui_outtroqelabel

    ui_outtroqelabel = lv_label_create(ui_Torqe);

    lv_obj_set_height(ui_outtroqelabel, 60);
    lv_obj_set_width(ui_outtroqelabel, lv_pct(93));

    lv_obj_set_x(ui_outtroqelabel, 10);
    lv_obj_set_y(ui_outtroqelabel, -45);

    lv_obj_set_align(ui_outtroqelabel, LV_ALIGN_LEFT_MID);

    lv_label_set_text(ui_outtroqelabel, T_OUT_TORQE);

    lv_obj_set_style_text_font(ui_outtroqelabel, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_outtroqevalue

    ui_outtroqevalue = lv_label_create(ui_outtroqelabel);

    lv_obj_set_width(ui_outtroqevalue, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_outtroqevalue, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_outtroqevalue, 190);
    lv_obj_set_y(ui_outtroqevalue, -14);

    lv_obj_set_align(ui_outtroqevalue, LV_ALIGN_LEFT_MID);

    lv_label_set_text(ui_outtroqevalue, T_BLANK);

    lv_obj_set_style_text_font(ui_outtroqevalue, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_outtroqeslider

    ui_outtroqeslider = lv_slider_create(ui_outtroqelabel);
    lv_slider_set_range(ui_outtroqeslider, 50, 200);
    lv_slider_set_value(ui_outtroqeslider, 180, LV_ANIM_OFF);
    if(lv_slider_get_mode(ui_outtroqeslider) == LV_SLIDER_MODE_RANGE) lv_slider_set_left_value(ui_outtroqeslider, 0,
                                                                                                   LV_ANIM_OFF);

    lv_obj_set_width(ui_outtroqeslider, 195);
    lv_obj_set_height(ui_outtroqeslider, 10);

    lv_obj_set_x(ui_outtroqeslider, 50);
    lv_obj_set_y(ui_outtroqeslider, 15);

    lv_obj_set_align(ui_outtroqeslider, LV_ALIGN_LEFT_MID);

    applySliderStyles(ui_outtroqeslider, 0);

    // ui_Low1

    ui_Low1 = lv_label_create(ui_outtroqelabel);

    lv_obj_set_width(ui_Low1, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_Low1, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_Low1, 0);
    lv_obj_set_y(ui_Low1, 14);

    lv_obj_set_align(ui_Low1, LV_ALIGN_LEFT_MID);

    lv_label_set_text(ui_Low1, T_LOW);

    lv_obj_set_style_text_font(ui_Low1, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_High1

    ui_High1 = lv_label_create(ui_outtroqelabel);

    lv_obj_set_width(ui_High1, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_High1, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_High1, 0);
    lv_obj_set_y(ui_High1, 14);

    lv_obj_set_align(ui_High1, LV_ALIGN_RIGHT_MID);

    lv_label_set_text(ui_High1, T_HIGH);

    lv_obj_set_style_text_font(ui_High1, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_introqelabel

    ui_introqelabel = lv_label_create(ui_Torqe);

    lv_obj_set_height(ui_introqelabel, 60);
    lv_obj_set_width(ui_introqelabel, lv_pct(93));

    lv_obj_set_x(ui_introqelabel, 10);
    lv_obj_set_y(ui_introqelabel, 30);

    lv_obj_set_align(ui_introqelabel, LV_ALIGN_LEFT_MID);

    lv_label_set_text(ui_introqelabel, T_IN_TORQE);

    lv_obj_set_style_text_font(ui_introqelabel, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_introqevalue

    ui_introqevalue = lv_label_create(ui_introqelabel);

    lv_obj_set_width(ui_introqevalue, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_introqevalue, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_introqevalue, 190);
    lv_obj_set_y(ui_introqevalue, -14);

    lv_obj_set_align(ui_introqevalue, LV_ALIGN_LEFT_MID);

    lv_label_set_text(ui_introqevalue, T_BLANK);

    lv_obj_set_style_text_font(ui_introqevalue, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_introqeslider

    ui_introqeslider = lv_slider_create(ui_introqelabel);
    lv_slider_set_range(ui_introqeslider, 20, 200);
    lv_slider_set_value(ui_introqeslider, 100, LV_ANIM_OFF);
    if(lv_slider_get_mode(ui_introqeslider) == LV_SLIDER_MODE_RANGE) lv_slider_set_left_value(ui_introqeslider, 0,
                                                                                                  LV_ANIM_OFF);

    lv_obj_set_width(ui_introqeslider, 195);
    lv_obj_set_height(ui_introqeslider, 10);

    lv_obj_set_x(ui_introqeslider, 50);
    lv_obj_set_y(ui_introqeslider, 15);

    lv_obj_set_align(ui_introqeslider, LV_ALIGN_LEFT_MID);

    applySliderStyles(ui_introqeslider, 1);

    // ui_Low2

    ui_Low2 = lv_label_create(ui_introqelabel);

    lv_obj_set_width(ui_Low2, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_Low2, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_Low2, 0);
    lv_obj_set_y(ui_Low2, 14);

    lv_obj_set_align(ui_Low2, LV_ALIGN_LEFT_MID);

    lv_label_set_text(ui_Low2, T_LOW);

    lv_obj_set_style_text_font(ui_Low2, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_High

    ui_High = lv_label_create(ui_introqelabel);

    lv_obj_set_width(ui_High, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_High, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_High, 0);
    lv_obj_set_y(ui_High, 14);

    lv_obj_set_align(ui_High, LV_ALIGN_RIGHT_MID);

    lv_label_set_text(ui_High, T_HIGH);

    lv_obj_set_style_text_font(ui_High, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_Batt4

    ui_Batt4 = lv_label_create(ui_Torqe);

    lv_obj_set_width(ui_Batt4, 85);
    lv_obj_set_height(ui_Batt4, 30);

    lv_obj_set_x(ui_Batt4, 115);
    lv_obj_set_y(ui_Batt4, -103);

    lv_obj_set_align(ui_Batt4, LV_ALIGN_CENTER);

    lv_label_set_text(ui_Batt4, T_BATT);

    // ui_BattValue4

    ui_BattValue4 = lv_label_create(ui_Batt4);

    lv_obj_set_width(ui_BattValue4, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_BattValue4, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_BattValue4, 0);
    lv_obj_set_y(ui_BattValue4, -7);

    lv_obj_set_align(ui_BattValue4, LV_ALIGN_RIGHT_MID);

    lv_label_set_text(ui_BattValue4, T_BLANK);

    // ui_Battery4

    ui_Battery4 = lv_bar_create(ui_Batt4);
    lv_bar_set_range(ui_Battery4, 0, 100);

    lv_obj_set_width(ui_Battery4, 80);
    lv_obj_set_height(ui_Battery4, 10);

    lv_obj_set_x(ui_Battery4, 0);
    lv_obj_set_y(ui_Battery4, 10);

    lv_obj_set_align(ui_Battery4, LV_ALIGN_CENTER);

    applyBatteryStyles(ui_Battery4);

}
void ui_EJECTSettings_screen_init(void)
{

    // ui_EJECTSettings

    ui_EJECTSettings = lv_obj_create(NULL);

    lv_obj_clear_flag(ui_EJECTSettings, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(ui_EJECTSettings, ui_event_EJECTSettings, LV_EVENT_ALL, NULL);

    // ui_Logo6

    ui_Logo6 = lv_label_create(ui_EJECTSettings);

    lv_obj_set_width(ui_Logo6, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_Logo6, LV_SIZE_CONTENT);

    lv_obj_set_y(ui_Logo6, -103);
    lv_obj_set_x(ui_Logo6, lv_pct(0));

    lv_obj_set_align(ui_Logo6, LV_ALIGN_CENTER);

    lv_label_set_text(ui_Logo6, T_HEADER);

    lv_obj_set_style_text_font(ui_Logo6, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    applyTitleBarStyle(ui_Logo6);

    // ui_EJECTButtonL

    ui_EJECTButtonL = lv_btn_create(ui_EJECTSettings);

    lv_obj_set_width(ui_EJECTButtonL, 100);
    lv_obj_set_height(ui_EJECTButtonL, 30);

    lv_obj_set_y(ui_EJECTButtonL, 100);
    lv_obj_set_x(ui_EJECTButtonL, lv_pct(-33));

    lv_obj_set_align(ui_EJECTButtonL, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_EJECTButtonL, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_EJECTButtonL, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(ui_EJECTButtonL, ui_event_EJECTButtonL, LV_EVENT_ALL, NULL);
    applyButtonStyles(ui_EJECTButtonL, &style_button_l, &style_button_l_focused, NULL);

    // ui_EJECTButtonLText

    ui_EJECTButtonLText = lv_label_create(ui_EJECTButtonL);

    lv_obj_set_width(ui_EJECTButtonLText, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_EJECTButtonLText, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_EJECTButtonLText, lv_pct(0));
    lv_obj_set_y(ui_EJECTButtonLText, lv_pct(0));

    lv_obj_set_align(ui_EJECTButtonLText, LV_ALIGN_CENTER);

    lv_label_set_text(ui_EJECTButtonLText, "");
    applyTextPrimaryStyle(ui_EJECTButtonLText);

    // ui_EJECTButtonM

    ui_EJECTButtonM = lv_btn_create(ui_EJECTSettings);

    lv_obj_set_width(ui_EJECTButtonM, 100);
    lv_obj_set_height(ui_EJECTButtonM, 30);

    lv_obj_set_y(ui_EJECTButtonM, 100);
    lv_obj_set_x(ui_EJECTButtonM, lv_pct(0));

    lv_obj_set_align(ui_EJECTButtonM, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_EJECTButtonM, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_EJECTButtonM, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(ui_EJECTButtonM, ui_event_EJECTButtonM, LV_EVENT_ALL, NULL);
    applyButtonStyles(ui_EJECTButtonM, &style_button_m, &style_button_m_focused, NULL);

    // ui_EJECTButtonMText

    ui_EJECTButtonMText = lv_label_create(ui_EJECTButtonM);

    lv_obj_set_width(ui_EJECTButtonMText, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_EJECTButtonMText, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_EJECTButtonMText, lv_pct(0));
    lv_obj_set_y(ui_EJECTButtonMText, lv_pct(0));

    lv_obj_set_align(ui_EJECTButtonMText, LV_ALIGN_CENTER);

    lv_label_set_text(ui_EJECTButtonMText, T_HOME);
    applyTextPrimaryStyle(ui_EJECTButtonMText);

    // ui_EJECTButtonR

    ui_EJECTButtonR = lv_btn_create(ui_EJECTSettings);

    lv_obj_set_width(ui_EJECTButtonR, 100);
    lv_obj_set_height(ui_EJECTButtonR, 30);

    lv_obj_set_y(ui_EJECTButtonR, 100);
    lv_obj_set_x(ui_EJECTButtonR, lv_pct(33));

    lv_obj_set_align(ui_EJECTButtonR, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_EJECTButtonR, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_EJECTButtonR, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(ui_EJECTButtonR, ui_event_EJECTButtonR, LV_EVENT_ALL, NULL);
    applyButtonStyles(ui_EJECTButtonR, &style_button_r, &style_button_r_focused, NULL);

    // ui_EJECTButtonRText

    ui_EJECTButtonRText = lv_label_create(ui_EJECTButtonR);

    lv_obj_set_width(ui_EJECTButtonRText, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_EJECTButtonRText, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_EJECTButtonRText, lv_pct(0));
    lv_obj_set_y(ui_EJECTButtonRText, lv_pct(0));

    lv_obj_set_align(ui_EJECTButtonRText, LV_ALIGN_CENTER);

    lv_label_set_text(ui_EJECTButtonRText, T_MENU);
    applyTextPrimaryStyle(ui_EJECTButtonRText);

    // ui_Batt6

    ui_Batt6 = lv_label_create(ui_EJECTSettings);

    lv_obj_set_width(ui_Batt6, 85);
    lv_obj_set_height(ui_Batt6, 30);

    lv_obj_set_x(ui_Batt6, 115);
    lv_obj_set_y(ui_Batt6, -103);

    lv_obj_set_align(ui_Batt6, LV_ALIGN_CENTER);

    lv_label_set_text(ui_Batt6, T_BATT);

    // ui_BattValue6

    ui_BattValue6 = lv_label_create(ui_Batt6);

    lv_obj_set_width(ui_BattValue6, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_BattValue6, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_BattValue6, 0);
    lv_obj_set_y(ui_BattValue6, -7);

    lv_obj_set_align(ui_BattValue6, LV_ALIGN_RIGHT_MID);

    lv_label_set_text(ui_BattValue6, T_BLANK);


    // ui_Battery6

    ui_Battery6 = lv_bar_create(ui_Batt6);
    lv_bar_set_range(ui_Battery6, 0, 100);

    lv_obj_set_width(ui_Battery6, 80);
    lv_obj_set_height(ui_Battery6, 10);

    lv_obj_set_x(ui_Battery6, 0);
    lv_obj_set_y(ui_Battery6, 10);

    lv_obj_set_align(ui_Battery6, LV_ALIGN_CENTER);

    applyBatteryStyles(ui_Battery6);

}
void ui_Settings_screen_init(void)
{

    // ui_Settings

    ui_Settings = lv_obj_create(NULL);

    lv_obj_clear_flag(ui_Settings, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(ui_Settings, ui_event_Settings, LV_EVENT_ALL, NULL);

    // ui_Logo1

    ui_Logo1 = lv_label_create(ui_Settings);

    lv_obj_set_width(ui_Logo1, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_Logo1, LV_SIZE_CONTENT);

    lv_obj_set_y(ui_Logo1, -103);
    lv_obj_set_x(ui_Logo1, lv_pct(0));

    lv_obj_set_align(ui_Logo1, LV_ALIGN_CENTER);

    lv_label_set_text(ui_Logo1, T_HEADER);

    lv_obj_set_style_text_font(ui_Logo1, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    applyTitleBarStyle(ui_Logo1);

    // ui_SettingsButtonL

    ui_SettingsButtonL = lv_btn_create(ui_Settings);

    lv_obj_set_width(ui_SettingsButtonL, 100);
    lv_obj_set_height(ui_SettingsButtonL, 30);

    lv_obj_set_x(ui_SettingsButtonL, -102);
    lv_obj_set_y(ui_SettingsButtonL, 100);

    lv_obj_set_align(ui_SettingsButtonL, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_SettingsButtonL, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_SettingsButtonL, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(ui_SettingsButtonL, ui_event_SettingsButtonL, LV_EVENT_ALL, NULL);
    applyButtonStyles(ui_SettingsButtonL, &style_button_l, &style_button_l_focused, NULL);

    // ui_SettingsButtonLText

    ui_SettingsButtonLText = lv_label_create(ui_SettingsButtonL);

    lv_obj_set_width(ui_SettingsButtonLText, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_SettingsButtonLText, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_SettingsButtonLText, lv_pct(0));
    lv_obj_set_y(ui_SettingsButtonLText, lv_pct(0));

    lv_obj_set_align(ui_SettingsButtonLText, LV_ALIGN_CENTER);

    lv_label_set_text(ui_SettingsButtonLText, T_SAVE);
    applyTextPrimaryStyle(ui_SettingsButtonLText);

    // ui_SettingsButtonM

    ui_SettingsButtonM = lv_btn_create(ui_Settings);

    lv_obj_set_width(ui_SettingsButtonM, 100);
    lv_obj_set_height(ui_SettingsButtonM, 30);

    lv_obj_set_y(ui_SettingsButtonM, 100);
    lv_obj_set_x(ui_SettingsButtonM, lv_pct(0));

    lv_obj_set_align(ui_SettingsButtonM, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_SettingsButtonM, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_SettingsButtonM, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(ui_SettingsButtonM, ui_event_SettingsButtonM, LV_EVENT_ALL, NULL);
    applyButtonStyles(ui_SettingsButtonM, &style_button_m, &style_button_m_focused, NULL);

    // ui_SettingsButtonMText

    ui_SettingsButtonMText = lv_label_create(ui_SettingsButtonM);

    lv_obj_set_width(ui_SettingsButtonMText, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_SettingsButtonMText, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_SettingsButtonMText, lv_pct(0));
    lv_obj_set_y(ui_SettingsButtonMText, lv_pct(0));

    lv_obj_set_align(ui_SettingsButtonMText, LV_ALIGN_CENTER);

    lv_label_set_text(ui_SettingsButtonMText, T_HOME);
    applyTextPrimaryStyle(ui_SettingsButtonMText);

    // ui_SettingsButtonR

    ui_SettingsButtonR = lv_btn_create(ui_Settings);

    lv_obj_set_width(ui_SettingsButtonR, 100);
    lv_obj_set_height(ui_SettingsButtonR, 30);

    lv_obj_set_y(ui_SettingsButtonR, 100);
    lv_obj_set_x(ui_SettingsButtonR, lv_pct(33));

    lv_obj_set_align(ui_SettingsButtonR, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_SettingsButtonR, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_SettingsButtonR, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(ui_SettingsButtonR, ui_event_SettingsButtonR, LV_EVENT_ALL, NULL);
    applyButtonStyles(ui_SettingsButtonR, &style_button_r, &style_button_r_focused, NULL);

    // ui_SettingsButtonRText

    ui_SettingsButtonRText = lv_label_create(ui_SettingsButtonR);

    lv_obj_set_width(ui_SettingsButtonRText, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_SettingsButtonRText, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_SettingsButtonRText, lv_pct(0));
    lv_obj_set_y(ui_SettingsButtonRText, lv_pct(0));

    lv_obj_set_align(ui_SettingsButtonRText, LV_ALIGN_CENTER);

    lv_label_set_text(ui_SettingsButtonRText, T_MENU);
    applyTextPrimaryStyle(ui_SettingsButtonRText);

    // ui_Batt1

    ui_Batt1 = lv_label_create(ui_Settings);

    lv_obj_set_width(ui_Batt1, 85);
    lv_obj_set_height(ui_Batt1, 30);

    lv_obj_set_x(ui_Batt1, 115);
    lv_obj_set_y(ui_Batt1, -103);

    lv_obj_set_align(ui_Batt1, LV_ALIGN_CENTER);

    lv_label_set_text(ui_Batt1, T_BATT);

    // ui_BattValue1

    ui_BattValue1 = lv_label_create(ui_Batt1);

    lv_obj_set_width(ui_BattValue1, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_BattValue1, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_BattValue1, 0);
    lv_obj_set_y(ui_BattValue1, -7);

    lv_obj_set_align(ui_BattValue1, LV_ALIGN_RIGHT_MID);

    lv_label_set_text(ui_BattValue1, T_BLANK);

    // ui_Battery1

    ui_Battery1 = lv_bar_create(ui_Batt1);
    lv_bar_set_range(ui_Battery1, 0, 100);

    lv_obj_set_width(ui_Battery1, 80);
    lv_obj_set_height(ui_Battery1, 10);

    lv_obj_set_x(ui_Battery1, 0);
    lv_obj_set_y(ui_Battery1, 10);

    lv_obj_set_align(ui_Battery1, LV_ALIGN_CENTER);

    applyBatteryStyles(ui_Battery1);

    // ui_ejectaddon

    ui_ejectaddon = lv_checkbox_create(ui_Settings);
    lv_checkbox_set_text(ui_ejectaddon, T_EJECT);

    lv_obj_set_width(ui_ejectaddon, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_ejectaddon, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_ejectaddon, 10);
    lv_obj_set_y(ui_ejectaddon, -60);

    lv_obj_set_align(ui_ejectaddon, LV_ALIGN_LEFT_MID);

    lv_obj_add_flag(ui_ejectaddon, LV_OBJ_FLAG_SCROLL_ON_FOCUS);

    lv_obj_add_event_cb(ui_ejectaddon, ui_event_ejectaddon, LV_EVENT_ALL, NULL);

    lv_obj_set_style_text_font(ui_ejectaddon, &lv_font_montserrat_22, LV_PART_MAIN | LV_STATE_DEFAULT);
    applyCheckboxStyles(ui_ejectaddon);
    // Legacy checkbox kept for backward compatibility, but hidden in new settings UI.
    lv_obj_add_flag(ui_ejectaddon, LV_OBJ_FLAG_HIDDEN);

    // ui_vibrate

    ui_vibrate = lv_checkbox_create(ui_Settings);
    lv_checkbox_set_text(ui_vibrate, T_VIBRATE);

    lv_obj_set_width(ui_vibrate, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_vibrate, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_vibrate, 10);
    lv_obj_set_y(ui_vibrate, -60);

    lv_obj_set_align(ui_vibrate, LV_ALIGN_LEFT_MID);

    lv_obj_add_flag(ui_vibrate, LV_OBJ_FLAG_SCROLL_ON_FOCUS);

    lv_obj_set_style_text_font(ui_vibrate, &lv_font_montserrat_22, LV_PART_MAIN | LV_STATE_DEFAULT);
    applyCheckboxStyles(ui_vibrate);

    // ui_lefty

    ui_lefty = lv_checkbox_create(ui_Settings);
    lv_checkbox_set_text(ui_lefty, T_TOUCHSETTING);

    lv_obj_set_width(ui_lefty, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_lefty, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_lefty, 11);
    lv_obj_set_y(ui_lefty, -30);

    lv_obj_set_align(ui_lefty, LV_ALIGN_LEFT_MID);

    lv_obj_add_flag(ui_lefty, LV_OBJ_FLAG_SCROLL_ON_FOCUS);

    lv_obj_set_style_text_font(ui_lefty, &lv_font_montserrat_22, LV_PART_MAIN | LV_STATE_DEFAULT);
    applyCheckboxStyles(ui_lefty);

    // ui_strokeinvert

    ui_strokeinvert = lv_checkbox_create(ui_Settings);
    lv_checkbox_set_text(ui_strokeinvert, T_STROKEINVERT);

    lv_obj_set_width(ui_strokeinvert, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_strokeinvert, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_strokeinvert, 10);
    lv_obj_set_y(ui_strokeinvert, 0);

    lv_obj_set_align(ui_strokeinvert, LV_ALIGN_LEFT_MID);

    lv_obj_add_flag(ui_strokeinvert, LV_OBJ_FLAG_SCROLL_ON_FOCUS);

    lv_obj_set_style_text_font(ui_strokeinvert, &lv_font_montserrat_22, LV_PART_MAIN | LV_STATE_DEFAULT);
    applyCheckboxStyles(ui_strokeinvert);

    // ui_forceHome

    ui_forceHome = lv_checkbox_create(ui_Settings);
    lv_checkbox_set_text(ui_forceHome, T_HOME_FORCE);

    lv_obj_set_width(ui_forceHome, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_forceHome, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_forceHome, 10);
    lv_obj_set_y(ui_forceHome, 30);

    lv_obj_set_align(ui_forceHome, LV_ALIGN_LEFT_MID);

    lv_obj_add_flag(ui_forceHome, LV_OBJ_FLAG_SCROLL_ON_FOCUS);

    lv_obj_set_style_text_font(ui_forceHome, &lv_font_montserrat_22, LV_PART_MAIN | LV_STATE_DEFAULT);
    applyCheckboxStyles(ui_forceHome);

    // ui_brightness_icon

    ui_brightness_icon = lv_label_create(ui_Settings);

    lv_obj_set_width(ui_brightness_icon, 24);
    lv_obj_set_height(ui_brightness_icon, 24);

    lv_obj_set_x(ui_brightness_icon, 10);
    lv_obj_set_y(ui_brightness_icon, 65);

    lv_obj_set_align(ui_brightness_icon, LV_ALIGN_LEFT_MID);

    lv_label_set_text(ui_brightness_icon, LV_SYMBOL_IMAGE);
    lv_obj_set_style_text_font(ui_brightness_icon, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
    applyTextPrimaryStyle(ui_brightness_icon);

    // ui_brightness_slider

    ui_brightness_slider = lv_slider_create(ui_Settings);
    lv_slider_set_range(ui_brightness_slider, 5, 255);
    lv_slider_set_value(ui_brightness_slider, 180, LV_ANIM_OFF);

    lv_obj_set_width(ui_brightness_slider, 255);
    lv_obj_set_height(ui_brightness_slider, 18);

    lv_obj_set_x(ui_brightness_slider, 55);
    lv_obj_set_y(ui_brightness_slider, 65);

    lv_obj_set_align(ui_brightness_slider, LV_ALIGN_LEFT_MID);

    lv_obj_add_flag(ui_brightness_slider, LV_OBJ_FLAG_SCROLL_ON_FOCUS | LV_OBJ_FLAG_CLICK_FOCUSABLE);

    lv_obj_add_style(ui_brightness_slider, &style_slider_track[0], LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_brightness_slider, &style_slider_indicator[0], LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_brightness_slider, &style_slider_indicator[0], LV_PART_KNOB | LV_STATE_DEFAULT);

    lv_obj_add_event_cb(ui_brightness_slider, ui_event_Brightness_sliderChange, LV_EVENT_ALL, NULL);

    // Encoder focus group for settings screen
    if (ui_g_settings) {
        lv_group_del(ui_g_settings);
    }
    ui_g_settings = lv_group_create();
    lv_group_add_obj(ui_g_settings, ui_vibrate);
    lv_group_add_obj(ui_g_settings, ui_lefty);
    lv_group_add_obj(ui_g_settings, ui_strokeinvert);
    lv_group_add_obj(ui_g_settings, ui_forceHome);
    lv_group_focus_obj(ui_vibrate);

}

void ui_init(void)
{
    lv_disp_t * dispp = lv_disp_get_default();
    lv_theme_t * theme = lv_theme_default_init(dispp, lv_color_hex(getActivePrimaryColor()), lv_color_hex(getActiveSecondaryColor()), true, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);
    ui_Start_screen_init();
    ui_Home_screen_init();
    ui_Menu_screen_init();
    ui_Pattern_screen_init();
    ui_Torqe_screen_init();
    ui_EJECTSettings_screen_init();
    ui_Settings_screen_init();
    ui_Stroke_screen_init();
    ui_Streaming_screen_init();
    ui_Addons_screen_init();
    colors_ui_screen_init();
    lv_disp_load_scr(ui_Start);
}
