#ifdef __cplusplus
extern "C" {
#endif
extern int g_brightness_value;
#ifdef __cplusplus
}
#endif
/// SquareLine LVGL GENERATED FILE
// EDITOR VERSION: SquareLine Studio 1.0.5
// LVGL VERSION: 8.2
// PROJECT: OSSM-White

#include "ui.h"
#include "ui_helpers.h"
#include "main.h"
#include "language.h"
#include "colors.h"
#include "styles.h"
#include "esp_nowCommunication.h"


// Ensure C linkage for the generated UI symbols so they match the
// `extern` declarations in ui.h when compiled as C++.
#ifdef __cplusplus
extern "C" {
#endif

///////////////////// VARIABLES ////////////////////
lv_obj_t * ui_Start;
lv_obj_t * ui_strokeinvert;
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
lv_obj_t * ui_brightness_slider;
lv_obj_t * ui_Batt3;
lv_obj_t * ui_BattValue3;
lv_obj_t * ui_Battery3;
lv_obj_t * ui_EJECTSettingButton;
lv_obj_t * ui_EJECTSettingButtonText;
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
lv_obj_t * ui_Batt4;
lv_obj_t * ui_BattValue4;
lv_obj_t * ui_Battery4;
lv_obj_t * ui_Logo4;
lv_obj_t * ui_Torqe;
lv_obj_t * ui_TorqeButtonL;
lv_obj_t * ui_TorqeButtonLText;
lv_obj_t * ui_TorqeButtonM;
lv_obj_t * ui_TorqeButtonMText;
lv_obj_t * ui_TorqeButtonR;
lv_obj_t * ui_TorqeButtonRText;
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
lv_obj_t * ui_darkmode;
lv_obj_t * ui_vibrate;
lv_obj_t * ui_lefty;
//lv_group_t * ui_g_menue;
lv_group_t * ui_g_settings;

// UI_MENU screen objects
lv_obj_t * ui_Menu;
lv_obj_t * ui_LogoMenu;
lv_obj_t * ui_MenuButtonTL;
lv_obj_t * ui_MenuButtonTLText;
lv_obj_t * ui_MenuButtonTR;
lv_obj_t * ui_MenuButtonTRText;
lv_obj_t * ui_MenuButtonML;
lv_obj_t * ui_MenuButtonMLText;
lv_obj_t * ui_MenuButtonMR;
lv_obj_t * ui_MenuButtonMRText;
lv_obj_t * ui_MenuButtonBL;
lv_obj_t * ui_MenuButtonBLText;
lv_obj_t * ui_MenuButtonBR;
lv_obj_t * ui_MenuButtonBRText;
lv_obj_t * ui_Batt7;
lv_obj_t * ui_BattValue7;
lv_obj_t * ui_Battery7;
lv_group_t * ui_g_menu;
lv_obj_t * ui_MenuButtonL;
lv_obj_t * ui_MenuButtonLText;
lv_obj_t * ui_MenuButtonM;
lv_obj_t * ui_MenuButtonMText;
lv_obj_t * ui_MenuButtonR;
lv_obj_t * ui_MenuButtonRText;

// UI_STREAMING screen objects
lv_obj_t * ui_Streaming;
lv_obj_t * ui_LogoStreaming;
lv_obj_t * ui_StreamingButtonL;
lv_obj_t * ui_StreamingButtonLText;
lv_obj_t * ui_StreamingButtonM;
lv_obj_t * ui_StreamingButtonMText;
lv_obj_t * ui_StreamingButtonR;
lv_obj_t * ui_StreamingButtonRText;
lv_obj_t * ui_StreamingSpeedL;
lv_obj_t * ui_streamingspeedslider;
lv_obj_t * ui_streamingspeedvalue;
lv_obj_t * ui_StreamingDepthL;
lv_obj_t * ui_streamingdepthslider;
lv_obj_t * ui_streamingdepthvalue;
lv_obj_t * ui_StreamingStrokeL;
lv_obj_t * ui_streamingstrokeslider;
lv_obj_t * ui_streamingstrokevalue;
lv_obj_t * ui_Batt8;
lv_obj_t * ui_BattValue8;
lv_obj_t * ui_Battery8;
lv_obj_t * ui_Addons;
lv_obj_t * ui_LogoAddons;
lv_obj_t * ui_AddonsHint;
lv_obj_t * ui_AddonsItem0;
lv_obj_t * ui_AddonsItem0Text;
lv_obj_t * ui_AddonsItem1;
lv_obj_t * ui_AddonsItem1Text;
lv_obj_t * ui_AddonsButtonL;
lv_obj_t * ui_AddonsButtonLText;
lv_obj_t * ui_AddonsButtonM;
lv_obj_t * ui_AddonsButtonMText;
lv_obj_t * ui_AddonsButtonR;
lv_obj_t * ui_AddonsButtonRText;
lv_group_t * ui_g_addons;
lv_obj_t * ui_AddonsItem2 = NULL;
lv_obj_t * ui_AddonsItem2Text = NULL;



///////////////////// TEST LVGL SETTINGS ////////////////////
#if LV_COLOR_DEPTH != 16
    #error "LV_COLOR_DEPTH should be 16bit to match SquareLine Studio's settings"
#endif
#include "main.h"

#if LV_COLOR_16_SWAP !=0
    #error "#error LV_COLOR_16_SWAP should be 0 to match SquareLine Studio's settings"
#endif

///////////////////// ANIMATIONS ////////////////////

///////////////////// FUNCTIONS ////////////////////
static void ui_event_Start(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_SCREEN_LOADED) {
        // Right button is always Force ESP
        lv_label_set_text(ui_StartButtonRText, "Force ESP");
        screenmachine(e);
    }
}
static void ui_event_StartButtonL(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_SHORT_CLICKED) {
        connectbutton(e);
    }
}
static void ui_event_StartButtonM(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_SHORT_CLICKED) {
        _ui_screen_change(ui_Settings, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    }
}
static void ui_event_StartButtonR(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_SHORT_CLICKED) {
        // Force ESP-only mode (one-way): disable BLE attempts and wait for ESP-NOW pairing
        g_force_esp_only = true;
        OssmBleSetMode_c(0);
        Ossm_paired = false;
        Ossm_paired = false;
        lv_label_set_text(ui_StartButtonRText, "Force ESP");

        // Wait for OSSM to pair via ESP-NOW (heartbeats are sent by existing code)
        // Timeout: 15s, heartbeat interval 500ms
        EspNowWaitForPairingOrTimeout(15000, 500);

        if (Ossm_paired) {
            // Paired successfully - go to Home
            lv_label_set_text(ui_Welcome, T_ESPCONNECTED);
            _ui_screen_change(ui_Home, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
        } else {
            // Not paired - show failed message and stay on Start
            lv_label_set_text(ui_Welcome, T_FAILED);
        }
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
    if(event == LV_EVENT_SHORT_CLICKED) {
        homebuttonLevent(e);
    } else if(event == LV_EVENT_LONG_PRESSED) {
        homebuttonLlongEvent(e);
    }

}
static void ui_event_HomeButtonM(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_SHORT_CLICKED) {
        homebuttonmevent(e);
    } else if(event == LV_EVENT_LONG_PRESSED) {
        homebuttonMlongEvent(e);
    }
}
static void ui_event_HomeButtonR(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_SHORT_CLICKED) {
        _ui_screen_change(ui_Pattern, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    } else if(event == LV_EVENT_LONG_PRESSED){
        homebuttonRlongEvent(e);
    }
}

/*
static void ui_evenT_MENU(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_SCREEN_LOADED) {
        screenmachine(e);
    }
}
static void ui_evenT_MENUButtonL(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_SHORT_CLICKED) {
        _ui_screen_change(ui_Settings, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    }
}
static void ui_evenT_MENUButtonM(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_SHORT_CLICKED) {
        _ui_screen_change(ui_Home, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    }
}
static void ui_evenT_MENUButtonR(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_SHORT_CLICKED) {
        xtoysMenuButtonRToggle();
    }
}
*/

static void ui_event_Brightness_sliderChange(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_SHORT_CLICKED) {
        brightness_slider_event_cb(e);
    }
}

static void settings_checkbox_event_cb(lv_event_t * e) {
    lv_obj_t * cb = lv_event_get_target(e);
    bool checked = lv_obj_has_state(cb, LV_STATE_CHECKED);
    // Optionally update persistent settings here, or just toggle
    // Example: if(cb == ui_vibrate) { settings.vibrate = checked; }
    // Add similar lines for other checkboxes if needed
}

static void ui_event_SPattern(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_SHORT_CLICKED) {
        _ui_screen_change(ui_Pattern, LV_SCR_LOAD_ANIM_FADE_ON, 90, 0);
    }
}
static void ui_event_EJECTSettingButton(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_SHORT_CLICKED) {
        _ui_screen_change(ui_EJECTSettings, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    }
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
    if(event == LV_EVENT_SHORT_CLICKED) {
        _ui_screen_change(ui_Menu, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    }
}
static void ui_event_PatternButtonM(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_SHORT_CLICKED) {
        _ui_screen_change(ui_Home, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    }
}
static void ui_event_PatternButtonR(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_SHORT_CLICKED) {
        savepattern(e);
        _ui_screen_change(ui_Home, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
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
    if(event == LV_EVENT_SHORT_CLICKED) {
        _ui_screen_change(ui_Home, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    }
}
static void ui_event_TorqeButtonR(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_SHORT_CLICKED) {
        _ui_screen_change(ui_Menu, LV_SCR_LOAD_ANIM_FADE_ON, 90, 0);
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
}
static void ui_event_EJECTButtonM(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_SHORT_CLICKED) {
        _ui_screen_change(ui_Home, LV_SCR_LOAD_ANIM_FADE_ON, 90, 0);
    }
}
static void ui_event_EJECTButtonR(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_SHORT_CLICKED) {
        _ui_screen_change(ui_Menu, LV_SCR_LOAD_ANIM_FADE_ON, 90, 0);
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
    if(event == LV_EVENT_SHORT_CLICKED) {
        savesettings(e);
    }
}
static void ui_event_SettingsButtonM(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_SHORT_CLICKED) {
        _ui_screen_change(ui_Menu, LV_SCR_LOAD_ANIM_FADE_ON, 90, 0);
    }
}
static void ui_event_SettingsButtonR(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_SHORT_CLICKED) {
        _ui_screen_change(ui_Menu, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    }
}
static void ui_event_ejectaddon(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
}

// UI_MENU event handlers
static void ui_event_Menu(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_SCREEN_LOADED) {
        requestMenuEntryAction();
        screenmachine(e);
    }
}
static void ui_event_MenuButtonTL(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    if(event == LV_EVENT_SHORT_CLICKED) {
        _ui_screen_change(ui_Home, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    }
}
static void ui_event_MenuButtonTR(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    if(event == LV_EVENT_SHORT_CLICKED) {
        menuPrepareNonHomeAction();
        requestStreamingEntryFlow();
        _ui_screen_change(ui_Streaming, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    }
}
static void ui_event_MenuButtonML(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    if(event == LV_EVENT_SHORT_CLICKED) {
        menuPrepareNonHomeAction();
        _ui_screen_change(ui_Addons, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    }
}
static void ui_event_MenuButtonM_bottom(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    if(event == LV_EVENT_SHORT_CLICKED) {
        menuPrepareNonHomeAction();
        menuSleepAction();
    }
}

static void ui_event_MenuButtonL_restart(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    if(event == LV_EVENT_SHORT_CLICKED) {
        menuPrepareNonHomeAction();
        menuRestartAction();
    }
}

static void ui_event_MenuButtonR_bottom(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    if(event == LV_EVENT_SHORT_CLICKED) {
        lv_obj_t *focused = (ui_g_menu != NULL) ? lv_group_get_focused(ui_g_menu) : NULL;
        if (focused != NULL) {
            lv_obj_send_event(focused, LV_EVENT_SHORT_CLICKED, NULL);
        }
    }
}

static void ui_event_MenuButtonMR(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    if(event == LV_EVENT_SHORT_CLICKED) {
        menuPrepareNonHomeAction();
        requestStreamingEntryFlow();
        _ui_screen_change(ui_Streaming, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    }
}
static void ui_event_MenuButtonBL(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    if(event == LV_EVENT_SHORT_CLICKED) {
        menuPrepareNonHomeAction();
        _ui_screen_change(ui_Settings, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    }
}
static void ui_event_MenuButtonBR(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    if(event == LV_EVENT_SHORT_CLICKED) {
        _ui_screen_change(ui_Menu, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    }
}

// UI_STREAMING event handlers
static void ui_event_Streaming(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(event == LV_EVENT_SCREEN_LOADED) {
        screenmachine(e);
    }
}
static void ui_event_StreamingButtonL(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
}
static void ui_event_StreamingButtonM(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    if(event == LV_EVENT_SHORT_CLICKED) {
        streamingbuttonmevent(e);
    }
}
static void ui_event_StreamingButtonR(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    if(event == LV_EVENT_SHORT_CLICKED) {
        streamingReturnToMenu();
    }
}

static void ui_event_Addons(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    if(event == LV_EVENT_SCREEN_LOADED) {
        addonsScreenLoaded();
        screenmachine(e);
    }
}

static void ui_event_AddonsItem0(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    if(event == LV_EVENT_SHORT_CLICKED) {
        addonsSelectIndex(0);
    }
}

static void ui_event_AddonsItem1(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    if(event == LV_EVENT_SHORT_CLICKED) {
        addonsSelectIndex(1);
    }
}

static void ui_event_AddonsItem2(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    if(event == LV_EVENT_SHORT_CLICKED) {
        addonsSelectIndex(2);
    }
}

static void ui_event_AddonsButtonL(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    if(event == LV_EVENT_SHORT_CLICKED) {
        _ui_screen_change(ui_Menu, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    }
}

static void ui_event_AddonsButtonM(lv_event_t * e)
{
    // Middle button will be handled in screen.cpp to trigger the addon
    // This event handler is just for the button UI integration
}

static void ui_event_AddonsButtonR(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    if(event == LV_EVENT_SHORT_CLICKED) {
        lv_obj_t * focused = lv_group_get_focused(ui_g_addons);
        if (focused != NULL) {
            lv_obj_send_event(focused, LV_EVENT_SHORT_CLICKED, NULL);
        }
    }
}

// Custom draw handler for the Pattern roller: paint three horizontal bands
// using the three slider slot colors so the visible options show distinct
// accents (top=slider1, mid=slider2, bottom=slider3).
// Pattern bands are regular LVGL objects created beneath the roller
// so we avoid using low-level draw APIs that vary between LVGL versions.

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
    lv_obj_add_style(ui_Logo, &style_title_bar, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_Logo, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_StartButtonL

    ui_StartButtonL = lv_btn_create(ui_Start);

    lv_obj_set_width(ui_StartButtonL, 100);
    lv_obj_set_height(ui_StartButtonL, 30);

    lv_obj_set_y(ui_StartButtonL, 100);
    lv_obj_set_x(ui_StartButtonL, lv_pct(-33));

    lv_obj_set_align(ui_StartButtonL, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_StartButtonL, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_StartButtonL, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(ui_StartButtonL, ui_event_StartButtonL, LV_EVENT_SHORT_CLICKED, NULL);
    lv_obj_add_style(ui_StartButtonL, &style_button_l, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_StartButtonL, &style_button_l, LV_PART_MAIN | LV_STATE_FOCUSED);

    // ui_StartButtonLText

    ui_StartButtonLText = lv_label_create(ui_StartButtonL);

    lv_obj_set_width(ui_StartButtonLText, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_StartButtonLText, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_StartButtonLText, lv_pct(0));
    lv_obj_set_y(ui_StartButtonLText, lv_pct(0));

    lv_obj_set_align(ui_StartButtonLText, LV_ALIGN_CENTER);

    lv_label_set_text(ui_StartButtonLText, T_CONNECT);

    lv_obj_add_style(ui_StartButtonLText, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_StartButtonM

    ui_StartButtonM = lv_btn_create(ui_Start);

    lv_obj_set_width(ui_StartButtonM, 100);
    lv_obj_set_height(ui_StartButtonM, 30);

    lv_obj_set_y(ui_StartButtonM, 100);
    lv_obj_set_x(ui_StartButtonM, lv_pct(0));

    lv_obj_set_align(ui_StartButtonM, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_StartButtonM, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_StartButtonM, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(ui_StartButtonM, ui_event_StartButtonM, LV_EVENT_SHORT_CLICKED, NULL);
    lv_obj_add_style(ui_StartButtonM, &style_button_m, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_StartButtonM, &style_button_m, LV_PART_MAIN | LV_STATE_FOCUSED);

    // ui_StartButtonMText

    ui_StartButtonMText = lv_label_create(ui_StartButtonM);

    lv_obj_set_width(ui_StartButtonMText, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_StartButtonMText, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_StartButtonMText, lv_pct(0));
    lv_obj_set_y(ui_StartButtonMText, lv_pct(0));

    lv_obj_set_align(ui_StartButtonMText, LV_ALIGN_CENTER);

    lv_label_set_text(ui_StartButtonMText, T_SETTINGS);

    lv_obj_add_style(ui_StartButtonMText, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_StartButtonR

    ui_StartButtonR = lv_btn_create(ui_Start);

    lv_obj_set_width(ui_StartButtonR, 100);
    lv_obj_set_height(ui_StartButtonR, 30);

    lv_obj_set_y(ui_StartButtonR, 100);
    lv_obj_set_x(ui_StartButtonR, lv_pct(33));

    lv_obj_set_align(ui_StartButtonR, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_StartButtonR, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_StartButtonR, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(ui_StartButtonR, ui_event_StartButtonR, LV_EVENT_SHORT_CLICKED, NULL);
    lv_obj_add_style(ui_StartButtonR, &style_button_r, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_StartButtonR, &style_button_r, LV_PART_MAIN | LV_STATE_FOCUSED);

    // ui_StartButtonRText

    ui_StartButtonRText = lv_label_create(ui_StartButtonR);

    lv_obj_set_width(ui_StartButtonRText, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_StartButtonRText, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_StartButtonRText, lv_pct(0));
    lv_obj_set_y(ui_StartButtonRText, lv_pct(0));

    lv_obj_set_align(ui_StartButtonRText, LV_ALIGN_CENTER);

    if (g_force_esp_only) {
        lv_label_set_text(ui_StartButtonRText, "Force ESP");
    } else {
        lv_label_set_text(ui_StartButtonRText, T_DEMO);
    }

    lv_obj_add_style(ui_StartButtonRText, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

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
    lv_obj_add_style(ui_Batt, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_BattValue

    ui_BattValue = lv_label_create(ui_Batt);

    lv_obj_set_width(ui_BattValue, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_BattValue, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_BattValue, 0);
    lv_obj_set_y(ui_BattValue, -7);

    lv_obj_set_align(ui_BattValue, LV_ALIGN_RIGHT_MID);

    lv_label_set_text(ui_BattValue, T_BLANK);
    lv_obj_add_style(ui_BattValue, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_Battery

    ui_Battery = lv_bar_create(ui_Batt);
    lv_bar_set_range(ui_Battery, 0, 100);

    lv_obj_set_width(ui_Battery, 80);
    lv_obj_set_height(ui_Battery, 10);

    lv_obj_set_x(ui_Battery, 0);
    lv_obj_set_y(ui_Battery, 10);

    lv_obj_set_align(ui_Battery, LV_ALIGN_CENTER);

    lv_obj_add_style(ui_Battery, &style_battery_main, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_Battery, &style_battery_indicator, LV_PART_INDICATOR | LV_STATE_DEFAULT);

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
    lv_obj_add_style(ui_Logo2, &style_title_bar, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_Logo2, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_HomeButtonL

    ui_HomeButtonL = lv_btn_create(ui_Home);

    lv_obj_set_width(ui_HomeButtonL, 100);
    lv_obj_set_height(ui_HomeButtonL, 30);

    lv_obj_set_y(ui_HomeButtonL, 100);
    lv_obj_set_x(ui_HomeButtonL, lv_pct(-33));

    lv_obj_set_align(ui_HomeButtonL, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_HomeButtonL, LV_OBJ_FLAG_CHECKABLE | LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_HomeButtonL, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(ui_HomeButtonL, ui_event_HomeButtonL, LV_EVENT_SHORT_CLICKED, NULL);
    lv_obj_add_event_cb(ui_HomeButtonL, ui_event_HomeButtonL, LV_EVENT_LONG_PRESSED, NULL);
    lv_obj_add_style(ui_HomeButtonL, &style_button_l, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_HomeButtonL, &style_button_m, LV_PART_MAIN | LV_STATE_FOCUSED);

    // ui_HomeButtonLText

    ui_HomeButtonLText = lv_label_create(ui_HomeButtonL);

    lv_obj_set_width(ui_HomeButtonLText, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_HomeButtonLText, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_HomeButtonLText, lv_pct(0));
    lv_obj_set_y(ui_HomeButtonLText, lv_pct(0));

    lv_obj_set_align(ui_HomeButtonLText, LV_ALIGN_CENTER);

    lv_label_set_text(ui_HomeButtonLText, T_MENU);
    lv_obj_add_style(ui_HomeButtonLText, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_HomeButtonM

    ui_HomeButtonM = lv_btn_create(ui_Home);

    lv_obj_set_width(ui_HomeButtonM, 100);
    lv_obj_set_height(ui_HomeButtonM, 30);

    lv_obj_set_y(ui_HomeButtonM, 100);
    lv_obj_set_x(ui_HomeButtonM, lv_pct(0));

    lv_obj_set_align(ui_HomeButtonM, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_HomeButtonM, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_HomeButtonM, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(ui_HomeButtonM, ui_event_HomeButtonM, LV_EVENT_SHORT_CLICKED, NULL);
    lv_obj_add_event_cb(ui_HomeButtonM, ui_event_HomeButtonM, LV_EVENT_LONG_PRESSED, NULL);
    lv_obj_add_style(ui_HomeButtonM, &style_button_m, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_HomeButtonM, &style_button_m, LV_PART_MAIN | LV_STATE_FOCUSED);

    // ui_HomeButtonMText

    ui_HomeButtonMText = lv_label_create(ui_HomeButtonM);

    lv_obj_set_width(ui_HomeButtonMText, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_HomeButtonMText, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_HomeButtonMText, lv_pct(0));
    lv_obj_set_y(ui_HomeButtonMText, lv_pct(0));

    lv_obj_set_align(ui_HomeButtonMText, LV_ALIGN_CENTER);

    lv_label_set_text(ui_HomeButtonMText, T_START);
    lv_obj_add_style(ui_HomeButtonMText, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_HomeButtonR

    ui_HomeButtonR = lv_btn_create(ui_Home);

    lv_obj_set_width(ui_HomeButtonR, 100);
    lv_obj_set_height(ui_HomeButtonR, 30);

    lv_obj_set_y(ui_HomeButtonR, 100);
    lv_obj_set_x(ui_HomeButtonR, lv_pct(33));

    lv_obj_set_align(ui_HomeButtonR, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_HomeButtonR, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_HomeButtonR, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(ui_HomeButtonR, ui_event_HomeButtonR, LV_EVENT_SHORT_CLICKED, NULL);
    lv_obj_add_event_cb(ui_HomeButtonR, ui_event_HomeButtonR, LV_EVENT_LONG_PRESSED, NULL);
    lv_obj_add_style(ui_HomeButtonR, &style_button_m, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_HomeButtonR, &style_button_r, LV_PART_MAIN | LV_STATE_FOCUSED);

    // ui_HomeButtonRText

    ui_HomeButtonRText = lv_label_create(ui_HomeButtonR);

    lv_obj_set_width(ui_HomeButtonRText, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_HomeButtonRText, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_HomeButtonRText, lv_pct(0));
    lv_obj_set_y(ui_HomeButtonRText, lv_pct(0));

    lv_obj_set_align(ui_HomeButtonRText, LV_ALIGN_CENTER);

    lv_label_set_text(ui_HomeButtonRText, T_SCREEN_PATTERN);

    lv_obj_add_style(ui_HomeButtonRText, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

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

    lv_obj_add_style(ui_homespeedslider, &style_slider_track[0], LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_homespeedslider, &style_slider_indicator[0], LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_homespeedslider, &style_slider_indicator[0], LV_PART_KNOB | LV_STATE_DEFAULT);

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
    lv_slider_set_mode(ui_homedepthslider, LV_SLIDER_MODE_NORMAL);
    lv_slider_set_range(ui_homedepthslider, 0, maxdepthinmm);

    lv_obj_set_width(ui_homedepthslider, 130);
    lv_obj_set_height(ui_homedepthslider, 10);

    lv_obj_set_x(ui_homedepthslider, -15);
    lv_obj_set_y(ui_homedepthslider, 0);

    lv_obj_set_align(ui_homedepthslider, LV_ALIGN_RIGHT_MID);

    lv_obj_add_style(ui_homedepthslider, &style_slider_track[1], LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_homedepthslider, &style_slider_indicator[1], LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_homedepthslider, &style_slider_indicator[1], LV_PART_KNOB | LV_STATE_DEFAULT);

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
    lv_slider_set_mode(ui_homestrokeslider, LV_SLIDER_MODE_NORMAL);

    lv_obj_set_width(ui_homestrokeslider, 130);
    lv_obj_set_height(ui_homestrokeslider, 10);

    lv_obj_set_x(ui_homestrokeslider, -15);
    lv_obj_set_y(ui_homestrokeslider, 0);

    lv_obj_set_align(ui_homestrokeslider, LV_ALIGN_RIGHT_MID);

    lv_obj_add_style(ui_homestrokeslider, &style_slider_track[2], LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_homestrokeslider, &style_slider_indicator[2], LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_homestrokeslider, &style_slider_indicator[2], LV_PART_KNOB | LV_STATE_DEFAULT);

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

    lv_obj_set_width(ui_homesensationslider, 170);
    lv_obj_set_height(ui_homesensationslider, 10);

    lv_obj_set_x(ui_homesensationslider, -15);
    lv_obj_set_y(ui_homesensationslider, 0);

    lv_obj_set_align(ui_homesensationslider, LV_ALIGN_RIGHT_MID);

    lv_obj_add_style(ui_homesensationslider, &style_slider_track[3], LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_homesensationslider, &style_slider_indicator[3], LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_homesensationslider, &style_slider_indicator[3], LV_PART_KNOB | LV_STATE_DEFAULT);

    // ui_Batt2

    ui_Batt2 = lv_label_create(ui_Home);

    lv_obj_set_width(ui_Batt2, 85);
    lv_obj_set_height(ui_Batt2, 30);

    lv_obj_set_x(ui_Batt2, 115);
    lv_obj_set_y(ui_Batt2, -103);

    lv_obj_set_align(ui_Batt2, LV_ALIGN_CENTER);

    lv_label_set_text(ui_Batt2, T_BATT);
    lv_obj_add_style(ui_Batt2, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_BattValue2

    ui_BattValue2 = lv_label_create(ui_Batt2);

    lv_obj_set_width(ui_BattValue2, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_BattValue2, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_BattValue2, 0);
    lv_obj_set_y(ui_BattValue2, -7);

    lv_obj_set_align(ui_BattValue2, LV_ALIGN_RIGHT_MID);

    lv_label_set_text(ui_BattValue2, T_BLANK);
    lv_obj_add_style(ui_BattValue2, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);


    // ui_Battery2

    ui_Battery2 = lv_bar_create(ui_Batt2);
    lv_bar_set_range(ui_Battery2, 0, 100);

    lv_obj_set_width(ui_Battery2, 80);
    lv_obj_set_height(ui_Battery2, 10);

    lv_obj_set_x(ui_Battery2, 0);
    lv_obj_set_y(ui_Battery2, 10);

    lv_obj_set_align(ui_Battery2, LV_ALIGN_CENTER);

    lv_obj_add_style(ui_Battery2, &style_battery_main, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_Battery2, &style_battery_indicator, LV_PART_INDICATOR | LV_STATE_DEFAULT);

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

    lv_obj_set_x(ui_HomePatternLabel, 110);
    lv_obj_set_y(ui_HomePatternLabel, 0);

    lv_obj_set_align(ui_HomePatternLabel, LV_ALIGN_LEFT_MID);

    lv_label_set_text(ui_HomePatternLabel, T_BLANK);

    // ui_connect

    ui_connect = lv_label_create(ui_Home);

    lv_obj_set_width(ui_connect, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_connect, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_connect, 10);
    lv_obj_set_y(ui_connect, -102);

    lv_obj_set_align(ui_connect, LV_ALIGN_LEFT_MID);


    lv_label_set_text(ui_connect, T_BLANK);
    lv_obj_add_style(ui_connect, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

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
    lv_obj_add_style(ui_Logo5, &style_title_bar, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_Logo5, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_PatternButtonL

    ui_PatternButtonL = lv_btn_create(ui_Pattern);

    lv_obj_set_width(ui_PatternButtonL, 100);
    lv_obj_set_height(ui_PatternButtonL, 30);

    lv_obj_set_y(ui_PatternButtonL, 100);
    lv_obj_set_x(ui_PatternButtonL, lv_pct(-33));

    lv_obj_set_align(ui_PatternButtonL, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_PatternButtonL, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_PatternButtonL, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(ui_PatternButtonL, ui_event_PatternButtonL, LV_EVENT_SHORT_CLICKED, NULL);
    lv_obj_add_style(ui_PatternButtonL, &style_button_l, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_PatternButtonL, &style_button_l, LV_PART_MAIN | LV_STATE_FOCUSED);

    // ui_PatternButtonLText

    ui_PatternButtonLText = lv_label_create(ui_PatternButtonL);

    lv_obj_set_width(ui_PatternButtonLText, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_PatternButtonLText, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_PatternButtonLText, lv_pct(0));
    lv_obj_set_y(ui_PatternButtonLText, lv_pct(0));

    lv_obj_set_align(ui_PatternButtonLText, LV_ALIGN_CENTER);

    lv_label_set_text(ui_PatternButtonLText, T_MENU);

    lv_obj_add_style(ui_PatternButtonLText, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_PatternButtonM

    ui_PatternButtonM = lv_btn_create(ui_Pattern);

    lv_obj_set_width(ui_PatternButtonM, 100);
    lv_obj_set_height(ui_PatternButtonM, 30);

    lv_obj_set_y(ui_PatternButtonM, 100);
    lv_obj_set_x(ui_PatternButtonM, lv_pct(0));

    lv_obj_set_align(ui_PatternButtonM, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_PatternButtonM, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_PatternButtonM, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(ui_PatternButtonM, ui_event_PatternButtonM, LV_EVENT_SHORT_CLICKED, NULL);
    lv_obj_add_style(ui_PatternButtonM, &style_button_m, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_PatternButtonM, &style_button_m, LV_PART_MAIN | LV_STATE_FOCUSED);

    // ui_PatternButtonMText

    ui_PatternButtonMText = lv_label_create(ui_PatternButtonM);

    lv_obj_set_width(ui_PatternButtonMText, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_PatternButtonMText, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_PatternButtonMText, lv_pct(0));
    lv_obj_set_y(ui_PatternButtonMText, lv_pct(0));

    lv_obj_set_align(ui_PatternButtonMText, LV_ALIGN_CENTER);

    lv_label_set_text(ui_PatternButtonMText, T_HOME);

    lv_obj_add_style(ui_PatternButtonMText, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_PatternButtonR

    ui_PatternButtonR = lv_btn_create(ui_Pattern);

    lv_obj_set_width(ui_PatternButtonR, 100);
    lv_obj_set_height(ui_PatternButtonR, 30);

    lv_obj_set_y(ui_PatternButtonR, 100);
    lv_obj_set_x(ui_PatternButtonR, lv_pct(33));

    lv_obj_set_align(ui_PatternButtonR, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_PatternButtonR, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_PatternButtonR, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(ui_PatternButtonR, ui_event_PatternButtonR, LV_EVENT_SHORT_CLICKED, NULL);
    lv_obj_add_style(ui_PatternButtonR, &style_button_r, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_PatternButtonR, &style_button_r, LV_PART_MAIN | LV_STATE_FOCUSED);

    // ui_PatternButtonRText

    ui_PatternButtonRText = lv_label_create(ui_PatternButtonR);

    lv_obj_set_width(ui_PatternButtonRText, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_PatternButtonRText, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_PatternButtonRText, lv_pct(0));
    lv_obj_set_y(ui_PatternButtonRText, lv_pct(0));

    lv_obj_set_align(ui_PatternButtonRText, LV_ALIGN_CENTER);

    lv_label_set_text(ui_PatternButtonRText, T_SAVE);

    lv_obj_add_style(ui_PatternButtonRText, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_Batt5

    ui_Batt5 = lv_label_create(ui_Pattern);

    lv_obj_set_width(ui_Batt5, 85);
    lv_obj_set_height(ui_Batt5, 30);

    lv_obj_set_x(ui_Batt5, 115);
    lv_obj_set_y(ui_Batt5, -103);

    lv_obj_set_align(ui_Batt5, LV_ALIGN_CENTER);

    lv_label_set_text(ui_Batt5, T_BATT);
    lv_obj_add_style(ui_Batt5, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_BattValue5

    ui_BattValue5 = lv_label_create(ui_Batt5);

    lv_obj_set_width(ui_BattValue5, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_BattValue5, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_BattValue5, 0);
    lv_obj_set_y(ui_BattValue5, -7);

    lv_obj_set_align(ui_BattValue5, LV_ALIGN_RIGHT_MID);

    lv_label_set_text(ui_BattValue5, T_BLANK);
    lv_obj_add_style(ui_BattValue5, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_Battery5

    ui_Battery5 = lv_bar_create(ui_Batt5);
    lv_bar_set_range(ui_Battery5, 0, 100);

    lv_obj_set_width(ui_Battery5, 80);
    lv_obj_set_height(ui_Battery5, 10);

    lv_obj_set_x(ui_Battery5, 0);
    lv_obj_set_y(ui_Battery5, 10);

    lv_obj_set_align(ui_Battery5, LV_ALIGN_CENTER);

    lv_obj_add_style(ui_Battery5, &style_battery_main, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_Battery5, &style_battery_indicator, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    // ui_Label4

    ui_Label4 = lv_label_create(ui_Pattern);

    lv_obj_set_width(ui_Label4, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_Label4, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_Label4, 0);
    lv_obj_set_y(ui_Label4, -60);

    lv_obj_set_align(ui_Label4, LV_ALIGN_CENTER);

    lv_label_set_text(ui_Label4, T_SELECT_PATTERN);

    lv_obj_set_style_text_font(ui_Label4, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

// ui_PatternS

    // Create three background bands (top/mid/bottom) to show slider colors.
    // These are created first so the roller sits on top and remains interactive.
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

    ui_PatternS = lv_roller_create(ui_Pattern);
    lv_roller_set_options(ui_PatternS,
                          "SimpleStroke\nTeasingPounding\nRoboStroke\nHalfnHalf\nDeeper\nStopNGo\nInsist",
                          LV_ROLLER_MODE_NORMAL);

    lv_obj_set_height(ui_PatternS, 119);
    lv_obj_set_width(ui_PatternS, lv_pct(95));

    lv_obj_set_x(ui_PatternS, 0);
    lv_obj_set_y(ui_PatternS, 15);

    lv_obj_set_align(ui_PatternS, LV_ALIGN_CENTER);
    lv_obj_add_style(ui_PatternS, &style_roller, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_PatternS, &style_title_bar, LV_PART_SELECTED | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_PatternS, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui_PatternS, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui_PatternS, LV_TEXT_ALIGN_CENTER, LV_PART_SELECTED | LV_STATE_DEFAULT);
    /* roller uses bands underneath; no custom draw callback required */

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
    lv_obj_add_style(ui_Logo6, &style_title_bar, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_Logo6, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_EJECTButtonL

    ui_EJECTButtonL = lv_btn_create(ui_EJECTSettings);

    lv_obj_set_width(ui_EJECTButtonL, 100);
    lv_obj_set_height(ui_EJECTButtonL, 30);

    lv_obj_set_y(ui_EJECTButtonL, 100);
    lv_obj_set_x(ui_EJECTButtonL, lv_pct(-33));

    lv_obj_set_align(ui_EJECTButtonL, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_EJECTButtonL, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_EJECTButtonL, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(ui_EJECTButtonL, ui_event_EJECTButtonL, LV_EVENT_SHORT_CLICKED, NULL);
    lv_obj_add_style(ui_EJECTButtonL, &style_button_l, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_EJECTButtonL, &style_button_l, LV_PART_MAIN | LV_STATE_FOCUSED);

    // ui_EJECTButtonLText

    ui_EJECTButtonLText = lv_label_create(ui_EJECTButtonL);

    lv_obj_set_width(ui_EJECTButtonLText, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_EJECTButtonLText, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_EJECTButtonLText, lv_pct(0));
    lv_obj_set_y(ui_EJECTButtonLText, lv_pct(0));

    lv_obj_set_align(ui_EJECTButtonLText, LV_ALIGN_CENTER);

    lv_label_set_text(ui_EJECTButtonLText, "");

    lv_obj_add_style(ui_EJECTButtonLText, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_EJECTButtonM

    ui_EJECTButtonM = lv_btn_create(ui_EJECTSettings);

    lv_obj_set_width(ui_EJECTButtonM, 100);
    lv_obj_set_height(ui_EJECTButtonM, 30);

    lv_obj_set_y(ui_EJECTButtonM, 100);
    lv_obj_set_x(ui_EJECTButtonM, lv_pct(0));

    lv_obj_set_align(ui_EJECTButtonM, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_EJECTButtonM, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_EJECTButtonM, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(ui_EJECTButtonM, ui_event_EJECTButtonM, LV_EVENT_SHORT_CLICKED, NULL);
    lv_obj_add_style(ui_EJECTButtonM, &style_button_m, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_EJECTButtonM, &style_button_m, LV_PART_MAIN | LV_STATE_FOCUSED);

    // ui_EJECTButtonMText

    ui_EJECTButtonMText = lv_label_create(ui_EJECTButtonM);

    lv_obj_set_width(ui_EJECTButtonMText, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_EJECTButtonMText, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_EJECTButtonMText, lv_pct(0));
    lv_obj_set_y(ui_EJECTButtonMText, lv_pct(0));

    lv_obj_set_align(ui_EJECTButtonMText, LV_ALIGN_CENTER);

    lv_label_set_text(ui_EJECTButtonMText, T_HOME);

    lv_obj_add_style(ui_EJECTButtonMText, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_EJECTButtonR

    ui_EJECTButtonR = lv_btn_create(ui_EJECTSettings);

    lv_obj_set_width(ui_EJECTButtonR, 100);
    lv_obj_set_height(ui_EJECTButtonR, 30);

    lv_obj_set_y(ui_EJECTButtonR, 100);
    lv_obj_set_x(ui_EJECTButtonR, lv_pct(33));

    lv_obj_set_align(ui_EJECTButtonR, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_EJECTButtonR, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_EJECTButtonR, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(ui_EJECTButtonR, ui_event_EJECTButtonR, LV_EVENT_SHORT_CLICKED, NULL);
    lv_obj_add_style(ui_EJECTButtonR, &style_button_r, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_EJECTButtonR, &style_button_r, LV_PART_MAIN | LV_STATE_FOCUSED);

    // ui_EJECTButtonRText

    ui_EJECTButtonRText = lv_label_create(ui_EJECTButtonR);

    lv_obj_set_width(ui_EJECTButtonRText, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_EJECTButtonRText, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_EJECTButtonRText, lv_pct(0));
    lv_obj_set_y(ui_EJECTButtonRText, lv_pct(0));

    lv_obj_set_align(ui_EJECTButtonRText, LV_ALIGN_CENTER);

    lv_label_set_text(ui_EJECTButtonRText, T_MENU);

    lv_obj_add_style(ui_EJECTButtonRText, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_Batt6

    ui_Batt6 = lv_label_create(ui_EJECTSettings);

    lv_obj_set_width(ui_Batt6, 85);
    lv_obj_set_height(ui_Batt6, 30);

    lv_obj_set_x(ui_Batt6, 115);
    lv_obj_set_y(ui_Batt6, -103);

    lv_obj_set_align(ui_Batt6, LV_ALIGN_CENTER);

    lv_label_set_text(ui_Batt6, T_BATT);
    lv_obj_add_style(ui_Batt6, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_BattValue6

    ui_BattValue6 = lv_label_create(ui_Batt6);

    lv_obj_set_width(ui_BattValue6, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_BattValue6, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_BattValue6, 0);
    lv_obj_set_y(ui_BattValue6, -7);

    lv_obj_set_align(ui_BattValue6, LV_ALIGN_RIGHT_MID);

    lv_label_set_text(ui_BattValue6, T_BLANK);
    lv_obj_add_style(ui_BattValue6, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);


    // ui_Battery6

    ui_Battery6 = lv_bar_create(ui_Batt6);
    lv_bar_set_range(ui_Battery6, 0, 100);

    lv_obj_set_width(ui_Battery6, 80);
    lv_obj_set_height(ui_Battery6, 10);

    lv_obj_set_x(ui_Battery6, 0);
    lv_obj_set_y(ui_Battery6, 10);

    lv_obj_set_align(ui_Battery6, LV_ALIGN_CENTER);

    lv_obj_add_style(ui_Battery6, &style_battery_main, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_Battery6, &style_battery_indicator, LV_PART_INDICATOR | LV_STATE_DEFAULT);

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
    lv_obj_add_style(ui_Logo1, &style_title_bar, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_Logo1, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

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
    lv_obj_add_style(ui_SettingsButtonL, &style_button_l, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_SettingsButtonLText

    ui_SettingsButtonLText = lv_label_create(ui_SettingsButtonL);

    lv_obj_set_width(ui_SettingsButtonLText, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_SettingsButtonLText, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_SettingsButtonLText, lv_pct(0));
    lv_obj_set_y(ui_SettingsButtonLText, lv_pct(0));

    lv_obj_set_align(ui_SettingsButtonLText, LV_ALIGN_CENTER);

//CHECK    lv_obj_add_event_cb(ui_vibrate, settings_checkbox_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_label_set_text(ui_SettingsButtonLText, T_SAVE);

    lv_obj_add_style(ui_SettingsButtonLText, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_SettingsButtonM

    ui_SettingsButtonM = lv_btn_create(ui_Settings);

    lv_obj_set_width(ui_SettingsButtonM, 100);
    lv_obj_set_height(ui_SettingsButtonM, 30);

//CHECK    lv_obj_add_event_cb(ui_lefty, settings_checkbox_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_set_y(ui_SettingsButtonM, 100);
    lv_obj_set_x(ui_SettingsButtonM, lv_pct(0));

    lv_obj_set_align(ui_SettingsButtonM, LV_ALIGN_CENTER);

    lv_obj_add_flag(ui_SettingsButtonM, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_SettingsButtonM, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(ui_SettingsButtonM, ui_event_SettingsButtonM, LV_EVENT_ALL, NULL);
    lv_obj_add_style(ui_SettingsButtonM, &style_button_m, LV_PART_MAIN | LV_STATE_DEFAULT);

//CHECK    lv_obj_add_event_cb(ui_darkmode, settings_checkbox_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    // ui_SettingsButtonMText

    ui_SettingsButtonMText = lv_label_create(ui_SettingsButtonM);

    lv_obj_set_width(ui_SettingsButtonMText, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_SettingsButtonMText, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_SettingsButtonMText, lv_pct(0));
    lv_obj_set_y(ui_SettingsButtonMText, lv_pct(0));

    lv_obj_set_align(ui_SettingsButtonMText, LV_ALIGN_CENTER);

    // MX (middle) button now goes to menu
    lv_label_set_text(ui_SettingsButtonMText, T_MENU);

    lv_obj_add_style(ui_SettingsButtonMText, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

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
    lv_obj_add_style(ui_SettingsButtonR, &style_button_r, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_SettingsButtonRText

    ui_SettingsButtonRText = lv_label_create(ui_SettingsButtonR);

    lv_obj_set_width(ui_SettingsButtonRText, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_SettingsButtonRText, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_SettingsButtonRText, lv_pct(0));
    lv_obj_set_y(ui_SettingsButtonRText, lv_pct(0));

    lv_obj_set_align(ui_SettingsButtonRText, LV_ALIGN_CENTER);

    // Right button now selects/toggles
    lv_label_set_text(ui_SettingsButtonRText, T_SELECT);

    lv_obj_add_style(ui_SettingsButtonRText, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_Batt1

    ui_Batt1 = lv_label_create(ui_Settings);

    lv_obj_set_width(ui_Batt1, 85);
    lv_obj_set_height(ui_Batt1, 30);

    lv_obj_set_x(ui_Batt1, 115);
    lv_obj_set_y(ui_Batt1, -103);

    lv_obj_set_align(ui_Batt1, LV_ALIGN_CENTER);

    lv_label_set_text(ui_Batt1, T_BATT);
    lv_obj_add_style(ui_Batt1, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_BattValue1

    ui_BattValue1 = lv_label_create(ui_Batt1);

    lv_obj_set_width(ui_BattValue1, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_BattValue1, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_BattValue1, 0);
    lv_obj_set_y(ui_BattValue1, -7);

    lv_obj_set_align(ui_BattValue1, LV_ALIGN_RIGHT_MID);

    lv_label_set_text(ui_BattValue1, T_BLANK);
    lv_obj_add_style(ui_BattValue1, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_Battery1

    ui_Battery1 = lv_bar_create(ui_Batt1);
    lv_bar_set_range(ui_Battery1, 0, 100);

    lv_obj_set_width(ui_Battery1, 80);
    lv_obj_set_height(ui_Battery1, 10);

    lv_obj_set_x(ui_Battery1, 0);
    lv_obj_set_y(ui_Battery1, 10);

    lv_obj_set_align(ui_Battery1, LV_ALIGN_CENTER);

    lv_obj_add_style(ui_Battery1, &style_battery_main, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_Battery1, &style_battery_indicator, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    // ui_vibrate

    ui_vibrate = lv_checkbox_create(ui_Settings);
    lv_checkbox_set_text(ui_vibrate, T_VIBRATE);

    lv_obj_set_width(ui_vibrate, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_vibrate, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_vibrate, 10);
    lv_obj_set_y(ui_vibrate, -60);

    lv_obj_set_align(ui_vibrate, LV_ALIGN_LEFT_MID);

//    lv_obj_add_flag(ui_vibrate, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
//    lv_obj_set_style_bg_color(ui_vibrate, lv_color_hex(getActiveButtonMColor()), LV_PART_MAIN | LV_STATE_FOCUSED); // standard light purple when focused

//    lv_obj_add_event_cb(ui_vibrate, ui_event_vibrate, LV_EVENT_ALL, NULL);
    lv_obj_add_style(ui_vibrate, &style_option_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_vibrate, &style_slider_indicator[0], LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_vibrate, lv_color_hex(COLOR_SCHEMES[g_active_color_scheme].slider1), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_vibrate, 2, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_vibrate, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_vibrate, &style_button_m, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_text_font(ui_vibrate, &lv_font_montserrat_22, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_lefty (touch Disabled option)

    ui_lefty = lv_checkbox_create(ui_Settings);
    lv_checkbox_set_text(ui_lefty, T_TOUCHSETTING);

    lv_obj_set_width(ui_lefty, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_lefty, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_lefty, 10);
    lv_obj_set_y(ui_lefty, -30);

    lv_obj_set_align(ui_lefty, LV_ALIGN_LEFT_MID);

    lv_obj_add_flag(ui_lefty, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_add_style(ui_lefty, &style_option_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_lefty, &style_slider_indicator[1], LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_lefty, lv_color_hex(COLOR_SCHEMES[g_active_color_scheme].slider2), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_lefty, 2, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_lefty, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_lefty, &style_button_m, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_text_font(ui_lefty, &lv_font_montserrat_22, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_StrokeInvert

    ui_strokeinvert = lv_checkbox_create(ui_Settings);
    lv_checkbox_set_text(ui_strokeinvert, T_STROKEINVERT);

    lv_obj_set_width(ui_strokeinvert, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_strokeinvert, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_strokeinvert, 10);
    lv_obj_set_y(ui_strokeinvert, 0);

    lv_obj_set_align(ui_strokeinvert, LV_ALIGN_LEFT_MID);

    lv_obj_add_flag(ui_strokeinvert, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_add_style(ui_strokeinvert, &style_option_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_strokeinvert, &style_slider_indicator[2], LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_strokeinvert, lv_color_hex(COLOR_SCHEMES[g_active_color_scheme].slider3), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_strokeinvert, 2, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_strokeinvert, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_strokeinvert, &style_button_m, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_text_font(ui_strokeinvert, &lv_font_montserrat_22, LV_PART_MAIN | LV_STATE_DEFAULT);

    //lv_obj_set_style_border_color(ui_strokeinvert, lv_color_hex(getActiveButtonLColor()), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    //lv_obj_set_style_border_opa(ui_strokeinvert, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    // ui_darkmode

    ui_darkmode = lv_checkbox_create(ui_Settings);
    lv_checkbox_set_text(ui_darkmode, T_DARKM);

    lv_obj_set_width(ui_darkmode, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_darkmode, LV_SIZE_CONTENT);

    lv_obj_set_x(ui_darkmode, 10);
    lv_obj_set_y(ui_darkmode, 30);

    lv_obj_set_align(ui_darkmode, LV_ALIGN_LEFT_MID);

    lv_obj_add_flag(ui_darkmode, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_add_style(ui_darkmode, &style_option_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_darkmode, &style_slider_indicator[3], LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_darkmode, lv_color_hex(COLOR_SCHEMES[g_active_color_scheme].slider4), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_darkmode, 2, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_darkmode, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_darkmode, &style_button_m, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_text_font(ui_darkmode, &lv_font_montserrat_22, LV_PART_MAIN | LV_STATE_DEFAULT);

    //lv_obj_set_style_border_color(ui_darkmode, lv_color_hex(getActiveButtonLColor()), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    //lv_obj_set_style_border_opa(ui_darkmode, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    
    // Sun icon and brightness slider
    ui_brightness_icon = lv_label_create(ui_Settings);
    lv_label_set_text(ui_brightness_icon, LV_SYMBOL_IMAGE);
    lv_obj_set_width(ui_brightness_icon, 24);
    lv_obj_set_height(ui_brightness_icon, 24);
    lv_obj_set_x(ui_brightness_icon, 10);
    lv_obj_set_y(ui_brightness_icon, 65);
    lv_obj_set_align(ui_brightness_icon, LV_ALIGN_LEFT_MID);
    lv_obj_set_style_text_font(ui_brightness_icon, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_brightness_icon, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);
//    lv_obj_add_flag(ui_brightness_icon, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
//    lv_obj_set_style_bg_color(ui_brightness_icon, lv_color_hex(getActiveBackgroundColor()), LV_PART_MAIN | LV_STATE_DEFAULT);
//    lv_obj_set_style_bg_opa(ui_brightness_icon, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
//    lv_obj_set_style_bg_color(ui_brightness_icon, lv_color_hex(getActiveButtonMColor()), LV_PART_MAIN | LV_STATE_FOCUSED);

    
    ui_brightness_slider = lv_slider_create(ui_Settings);
    lv_slider_set_range(ui_brightness_slider, 5, 255);
    lv_slider_set_value(ui_brightness_slider, g_brightness_value, LV_ANIM_OFF);
    lv_obj_set_width(ui_brightness_slider, 255);
    lv_obj_set_height(ui_brightness_slider, 18);
    lv_obj_set_x(ui_brightness_slider, 55);
    lv_obj_set_y(ui_brightness_slider, 65);
    lv_obj_set_align(ui_brightness_slider, LV_ALIGN_LEFT_MID);
    lv_obj_add_flag(ui_brightness_slider, LV_OBJ_FLAG_SCROLL_ON_FOCUS | LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_obj_add_style(ui_brightness_slider, &style_slider_track[0], LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_brightness_slider, &style_slider_indicator[0], LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_brightness_slider, &style_slider_indicator[0], LV_PART_KNOB | LV_STATE_DEFAULT);
//    lv_obj_add_flag(ui_brightness_slider, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
//    lv_obj_set_style_bg_color(ui_brightness_slider, lv_color_hex(getActiveBackgroundColor()), LV_PART_MAIN | LV_STATE_DEFAULT);
//    lv_obj_set_style_bg_opa(ui_brightness_slider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
//    lv_obj_set_style_bg_color(ui_brightness_slider, lv_color_hex(getActiveButtonMColor()), LV_PART_MAIN | LV_STATE_FOCUSED);

    // Attach event for immediate brightness effect
    extern void brightness_slider_event_cb(lv_event_t * e);
    lv_obj_add_event_cb(ui_brightness_slider, brightness_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // Encoder 2 group for settings screen
    if (ui_g_settings) lv_group_del(ui_g_settings);
    ui_g_settings = lv_group_create();
    lv_group_add_obj(ui_g_settings, ui_vibrate);
    lv_group_add_obj(ui_g_settings, ui_lefty);
    lv_group_add_obj(ui_g_settings, ui_strokeinvert);
    lv_group_add_obj(ui_g_settings, ui_darkmode);
//    lv_group_add_obj(ui_g_settings, ui_brightness_slider);
    // TODO: Set encoder 2 to use ui_g_settings group here, e.g.:
    // lv_indev_set_group(encoder2_indev, ui_g_settings);
}
void ui_Menu_screen_init(void)
{
    // ui_Menu
    ui_Menu = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Menu, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(ui_Menu, ui_event_Menu, LV_EVENT_ALL, NULL);

    // ui_LogoMenu
    ui_LogoMenu = lv_label_create(ui_Menu);
    lv_obj_set_width(ui_LogoMenu, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_LogoMenu, LV_SIZE_CONTENT);
    lv_obj_set_y(ui_LogoMenu, -103);
    lv_obj_set_x(ui_LogoMenu, lv_pct(0));
    lv_obj_set_align(ui_LogoMenu, LV_ALIGN_CENTER);
    lv_label_set_text(ui_LogoMenu, T_HEADER);
    lv_obj_set_style_text_font(ui_LogoMenu, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_LogoMenu, &style_title_bar, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Top-left: Home screen
    ui_MenuButtonTL = lv_btn_create(ui_Menu);
    lv_obj_set_width(ui_MenuButtonTL, 150);
    lv_obj_set_height(ui_MenuButtonTL, 94);
    lv_obj_set_y(ui_MenuButtonTL, -25);
    lv_obj_set_x(ui_MenuButtonTL, -81);
    lv_obj_set_align(ui_MenuButtonTL, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_MenuButtonTL, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_MenuButtonTL, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(ui_MenuButtonTL, ui_event_MenuButtonTL, LV_EVENT_SHORT_CLICKED, NULL);
    lv_obj_add_style(ui_MenuButtonTL, &style_slider_indicator[0], LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_MenuButtonTL, &style_button_m, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_radius(ui_MenuButtonTL, 8, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_MenuButtonTLText = lv_label_create(ui_MenuButtonTL);
    lv_obj_set_width(ui_MenuButtonTLText, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_MenuButtonTLText, LV_SIZE_CONTENT);
    lv_obj_align(ui_MenuButtonTLText, LV_ALIGN_CENTER, 0, -10);
    lv_label_set_text(ui_MenuButtonTLText, T_HOMESCREEN);
    lv_obj_add_style(ui_MenuButtonTLText, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui_MenuButtonTLText, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_MenuButtonTLText, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t * ui_MenuButtonTLSubText = lv_label_create(ui_MenuButtonTL);
    lv_obj_set_width(ui_MenuButtonTLSubText, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_MenuButtonTLSubText, LV_SIZE_CONTENT);
    lv_obj_align(ui_MenuButtonTLSubText, LV_ALIGN_CENTER, 0, 12);
    lv_label_set_text(ui_MenuButtonTLSubText, T_HOMESCREEN_SUB);
    lv_obj_add_style(ui_MenuButtonTLSubText, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui_MenuButtonTLSubText, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_MenuButtonTLSubText, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Top-right: Streaming mode
    ui_MenuButtonTR = lv_btn_create(ui_Menu);
    lv_obj_set_width(ui_MenuButtonTR, 150);
    lv_obj_set_height(ui_MenuButtonTR, 94);
    lv_obj_set_y(ui_MenuButtonTR, -25);
    lv_obj_set_x(ui_MenuButtonTR, 81);
    lv_obj_set_align(ui_MenuButtonTR, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_MenuButtonTR, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_MenuButtonTR, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(ui_MenuButtonTR, ui_event_MenuButtonTR, LV_EVENT_SHORT_CLICKED, NULL);
    lv_obj_add_style(ui_MenuButtonTR, &style_slider_indicator[1], LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_MenuButtonTR, &style_button_m, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_radius(ui_MenuButtonTR, 8, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_MenuButtonTRText = lv_label_create(ui_MenuButtonTR);
    lv_obj_set_width(ui_MenuButtonTRText, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_MenuButtonTRText, LV_SIZE_CONTENT);
    lv_obj_set_align(ui_MenuButtonTRText, LV_ALIGN_CENTER);
    lv_label_set_text(ui_MenuButtonTRText, T_STREAMING "\n" T_STREAMING_SUB);
    lv_obj_add_style(ui_MenuButtonTRText, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui_MenuButtonTRText, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_MenuButtonTRText, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Bottom-left: Addons
    ui_MenuButtonML = lv_btn_create(ui_Menu);
    lv_obj_set_width(ui_MenuButtonML, 150);
    lv_obj_set_height(ui_MenuButtonML, 44);
    lv_obj_set_y(ui_MenuButtonML, 50);
    lv_obj_set_x(ui_MenuButtonML, -81);
    lv_obj_set_align(ui_MenuButtonML, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_MenuButtonML, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_MenuButtonML, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(ui_MenuButtonML, ui_event_MenuButtonML, LV_EVENT_SHORT_CLICKED, NULL);
    lv_obj_add_style(ui_MenuButtonML, &style_slider_indicator[2], LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_MenuButtonML, &style_button_m, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_radius(ui_MenuButtonML, 8, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_MenuButtonMLText = lv_label_create(ui_MenuButtonML);
    lv_obj_set_width(ui_MenuButtonMLText, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_MenuButtonMLText, LV_SIZE_CONTENT);
    lv_obj_set_align(ui_MenuButtonMLText, LV_ALIGN_CENTER);
    lv_label_set_text(ui_MenuButtonMLText, T_ADDONS);
    lv_obj_add_style(ui_MenuButtonMLText, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Middle-right: removed (X-Toys redundant with streaming mode)
    ui_MenuButtonMR = NULL;
    ui_MenuButtonMRText = NULL;

    // Bottom-right: Settings
    ui_MenuButtonBL = lv_btn_create(ui_Menu);
    lv_obj_set_width(ui_MenuButtonBL, 150);
    lv_obj_set_height(ui_MenuButtonBL, 44);
    lv_obj_set_y(ui_MenuButtonBL, 50);
    lv_obj_set_x(ui_MenuButtonBL, 81);
    lv_obj_set_align(ui_MenuButtonBL, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_MenuButtonBL, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_MenuButtonBL, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(ui_MenuButtonBL, ui_event_MenuButtonBL, LV_EVENT_SHORT_CLICKED, NULL);
    lv_obj_add_style(ui_MenuButtonBL, &style_slider_indicator[3], LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_MenuButtonBL, &style_button_m, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_radius(ui_MenuButtonBL, 8, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_MenuButtonBLText = lv_label_create(ui_MenuButtonBL);
    lv_obj_set_width(ui_MenuButtonBLText, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_MenuButtonBLText, LV_SIZE_CONTENT);
    lv_obj_set_align(ui_MenuButtonBLText, LV_ALIGN_CENTER);
    lv_label_set_text(ui_MenuButtonBLText, T_SETTINGS);
    lv_obj_add_style(ui_MenuButtonBLText, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Bottom-right: removed (Pattern Generator not implemented)
    ui_MenuButtonBR = NULL;
    ui_MenuButtonBRText = NULL;

    // Battery display at top
    ui_Batt7 = lv_label_create(ui_Menu);
    lv_obj_set_width(ui_Batt7, 85);
    lv_obj_set_height(ui_Batt7, 30);
    lv_obj_set_x(ui_Batt7, 115);
    lv_obj_set_y(ui_Batt7, -103);
    lv_obj_set_align(ui_Batt7, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Batt7, T_BATT);
    lv_obj_add_style(ui_Batt7, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_BattValue7 = lv_label_create(ui_Batt7);
    lv_obj_set_width(ui_BattValue7, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_BattValue7, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_BattValue7, 0);
    lv_obj_set_y(ui_BattValue7, -7);
    lv_obj_set_align(ui_BattValue7, LV_ALIGN_RIGHT_MID);
    lv_label_set_text(ui_BattValue7, T_BLANK);
    lv_obj_add_style(ui_BattValue7, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Battery7 = lv_bar_create(ui_Batt7);
    lv_bar_set_range(ui_Battery7, 0, 100);
    lv_obj_set_width(ui_Battery7, 80);
    lv_obj_set_height(ui_Battery7, 10);
    lv_obj_set_x(ui_Battery7, 0);
    lv_obj_set_y(ui_Battery7, 10);
    lv_obj_set_align(ui_Battery7, LV_ALIGN_CENTER);
    lv_obj_add_style(ui_Battery7, &style_battery_main, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_Battery7, &style_battery_indicator, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    // Bottom control buttons (left, middle, right)
    ui_MenuButtonL = lv_btn_create(ui_Menu);
    lv_obj_set_width(ui_MenuButtonL, 100);
    lv_obj_set_height(ui_MenuButtonL, 30);
    lv_obj_set_y(ui_MenuButtonL, 100);
    lv_obj_set_x(ui_MenuButtonL, lv_pct(-33));
    lv_obj_set_align(ui_MenuButtonL, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_MenuButtonL, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_MenuButtonL, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(ui_MenuButtonL, ui_event_MenuButtonL_restart, LV_EVENT_SHORT_CLICKED, NULL);
    lv_obj_add_style(ui_MenuButtonL, &style_button_l, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_MenuButtonL, &style_button_l, LV_PART_MAIN | LV_STATE_FOCUSED);

    ui_MenuButtonLText = lv_label_create(ui_MenuButtonL);
    lv_obj_set_width(ui_MenuButtonLText, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_MenuButtonLText, LV_SIZE_CONTENT);
    lv_obj_set_align(ui_MenuButtonLText, LV_ALIGN_CENTER);
    lv_label_set_text(ui_MenuButtonLText, T_RESTART);
    lv_obj_add_style(ui_MenuButtonLText, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_MenuButtonM = lv_btn_create(ui_Menu);
    lv_obj_set_width(ui_MenuButtonM, 100);
    lv_obj_set_height(ui_MenuButtonM, 30);
    lv_obj_set_y(ui_MenuButtonM, 100);
    lv_obj_set_x(ui_MenuButtonM, lv_pct(0));
    lv_obj_set_align(ui_MenuButtonM, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_MenuButtonM, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_MenuButtonM, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(ui_MenuButtonM, ui_event_MenuButtonM_bottom, LV_EVENT_SHORT_CLICKED, NULL);
    lv_obj_add_style(ui_MenuButtonM, &style_button_m, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_MenuButtonM, &style_button_m, LV_PART_MAIN | LV_STATE_FOCUSED);

    ui_MenuButtonMText = lv_label_create(ui_MenuButtonM);
    lv_obj_set_width(ui_MenuButtonMText, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_MenuButtonMText, LV_SIZE_CONTENT);
    lv_obj_set_align(ui_MenuButtonMText, LV_ALIGN_CENTER);
    lv_label_set_text(ui_MenuButtonMText, T_SLEEP);
    lv_obj_add_style(ui_MenuButtonMText, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_MenuButtonR = lv_btn_create(ui_Menu);
    lv_obj_set_width(ui_MenuButtonR, 100);
    lv_obj_set_height(ui_MenuButtonR, 30);
    lv_obj_set_y(ui_MenuButtonR, 100);
    lv_obj_set_x(ui_MenuButtonR, lv_pct(33));
    lv_obj_set_align(ui_MenuButtonR, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_MenuButtonR, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_MenuButtonR, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(ui_MenuButtonR, ui_event_MenuButtonR_bottom, LV_EVENT_SHORT_CLICKED, NULL);
    lv_obj_add_style(ui_MenuButtonR, &style_button_r, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_MenuButtonR, &style_button_r, LV_PART_MAIN | LV_STATE_FOCUSED);

    ui_MenuButtonRText = lv_label_create(ui_MenuButtonR);
    lv_obj_set_width(ui_MenuButtonRText, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_MenuButtonRText, LV_SIZE_CONTENT);
    lv_obj_set_align(ui_MenuButtonRText, LV_ALIGN_CENTER);
    lv_label_set_text(ui_MenuButtonRText, T_SELECT);
    lv_obj_add_style(ui_MenuButtonRText, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Encoder4 navigation group
    ui_g_menu = lv_group_create();
    lv_group_add_obj(ui_g_menu, ui_MenuButtonTL);
    lv_group_add_obj(ui_g_menu, ui_MenuButtonTR);
    lv_group_add_obj(ui_g_menu, ui_MenuButtonML);
    lv_group_add_obj(ui_g_menu, ui_MenuButtonBL);
}

void ui_Streaming_screen_init(void)
{
    // ui_Streaming - Copy of Home but without Sensation slider and Pattern label
    ui_Streaming = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Streaming, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(ui_Streaming, ui_event_Streaming, LV_EVENT_ALL, NULL);

    // ui_LogoStreaming
    ui_LogoStreaming = lv_label_create(ui_Streaming);
    lv_obj_set_width(ui_LogoStreaming, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_LogoStreaming, LV_SIZE_CONTENT);
    lv_obj_set_y(ui_LogoStreaming, -103);
    lv_obj_set_x(ui_LogoStreaming, lv_pct(0));
    lv_obj_set_align(ui_LogoStreaming, LV_ALIGN_CENTER);
    lv_label_set_text(ui_LogoStreaming, T_HEADER);
    lv_obj_set_style_text_font(ui_LogoStreaming, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_LogoStreaming, &style_title_bar, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_LogoStreaming, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_StreamingButtonL
    ui_StreamingButtonL = lv_btn_create(ui_Streaming);
    lv_obj_set_width(ui_StreamingButtonL, 100);
    lv_obj_set_height(ui_StreamingButtonL, 30);
    lv_obj_set_y(ui_StreamingButtonL, 100);
    lv_obj_set_x(ui_StreamingButtonL, lv_pct(-33));
    lv_obj_set_align(ui_StreamingButtonL, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_StreamingButtonL, LV_OBJ_FLAG_CHECKABLE | LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_StreamingButtonL, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(ui_StreamingButtonL, ui_event_StreamingButtonL, LV_EVENT_SHORT_CLICKED, NULL);
    lv_obj_add_style(ui_StreamingButtonL, &style_button_l, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_StreamingButtonL, &style_button_l, LV_PART_MAIN | LV_STATE_FOCUSED);

    ui_StreamingButtonLText = lv_label_create(ui_StreamingButtonL);
    lv_obj_set_width(ui_StreamingButtonLText, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_StreamingButtonLText, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_StreamingButtonLText, lv_pct(0));
    lv_obj_set_y(ui_StreamingButtonLText, lv_pct(0));
    lv_obj_set_align(ui_StreamingButtonLText, LV_ALIGN_CENTER);
    lv_label_set_text(ui_StreamingButtonLText, T_CREAMPIE);
    lv_obj_add_style(ui_StreamingButtonLText, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_StreamingButtonM
    ui_StreamingButtonM = lv_btn_create(ui_Streaming);
    lv_obj_set_width(ui_StreamingButtonM, 100);
    lv_obj_set_height(ui_StreamingButtonM, 30);
    lv_obj_set_y(ui_StreamingButtonM, 100);
    lv_obj_set_x(ui_StreamingButtonM, lv_pct(0));
    lv_obj_set_align(ui_StreamingButtonM, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_StreamingButtonM, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_StreamingButtonM, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(ui_StreamingButtonM, ui_event_StreamingButtonM, LV_EVENT_SHORT_CLICKED, NULL);
    lv_obj_add_event_cb(ui_StreamingButtonM, ui_event_StreamingButtonM, LV_EVENT_SHORT_CLICKED, NULL);
    lv_obj_add_style(ui_StreamingButtonM, &style_button_m, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_StreamingButtonM, &style_button_m, LV_PART_MAIN | LV_STATE_FOCUSED);

    ui_StreamingButtonMText = lv_label_create(ui_StreamingButtonM);
    lv_obj_set_width(ui_StreamingButtonMText, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_StreamingButtonMText, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_StreamingButtonMText, lv_pct(0));
    lv_obj_set_y(ui_StreamingButtonMText, lv_pct(0));
    lv_obj_set_align(ui_StreamingButtonMText, LV_ALIGN_CENTER);
    lv_label_set_text(ui_StreamingButtonMText, T_START);
    lv_obj_add_style(ui_StreamingButtonMText, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ui_StreamingButtonR - Menu button
    ui_StreamingButtonR = lv_btn_create(ui_Streaming);
    lv_obj_set_width(ui_StreamingButtonR, 100);
    lv_obj_set_height(ui_StreamingButtonR, 30);
    lv_obj_set_y(ui_StreamingButtonR, 100);
    lv_obj_set_x(ui_StreamingButtonR, lv_pct(33));
    lv_obj_set_align(ui_StreamingButtonR, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_StreamingButtonR, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_StreamingButtonR, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(ui_StreamingButtonR, ui_event_StreamingButtonR, LV_EVENT_SHORT_CLICKED, NULL);
    lv_obj_add_style(ui_StreamingButtonR, &style_button_r, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_StreamingButtonR, &style_button_r, LV_PART_MAIN | LV_STATE_FOCUSED);

    ui_StreamingButtonRText = lv_label_create(ui_StreamingButtonR);
    lv_obj_set_width(ui_StreamingButtonRText, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_StreamingButtonRText, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_StreamingButtonRText, lv_pct(0));
    lv_obj_set_y(ui_StreamingButtonRText, lv_pct(0));
    lv_obj_set_align(ui_StreamingButtonRText, LV_ALIGN_CENTER);
    lv_label_set_text(ui_StreamingButtonRText, T_MENU);
    lv_obj_add_style(ui_StreamingButtonRText, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_StreamingButtonRText, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Speed slider with taller positioning
    ui_StreamingSpeedL = lv_label_create(ui_Streaming);
    lv_obj_set_width(ui_StreamingSpeedL, lv_pct(95));
    lv_obj_set_height(ui_StreamingSpeedL, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_StreamingSpeedL, 0);
    lv_obj_set_y(ui_StreamingSpeedL, -50);
   lv_obj_set_align(ui_StreamingSpeedL, LV_ALIGN_CENTER);
    lv_label_set_text(ui_StreamingSpeedL, T_SPEED);
    lv_obj_set_style_text_font(ui_StreamingSpeedL, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_streamingspeedslider = lv_slider_create(ui_StreamingSpeedL);
    lv_slider_set_range(ui_streamingspeedslider, 0, 100);
    lv_obj_set_width(ui_streamingspeedslider, 130);
    lv_obj_set_height(ui_streamingspeedslider, 10);
    lv_obj_set_x(ui_streamingspeedslider, -15);
    lv_obj_set_y(ui_streamingspeedslider, 0);
    lv_obj_set_align(ui_streamingspeedslider, LV_ALIGN_RIGHT_MID);
    lv_obj_add_style(ui_streamingspeedslider, &style_slider_track[0], LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_streamingspeedslider, &style_slider_indicator[0], LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_streamingspeedslider, &style_slider_indicator[0], LV_PART_KNOB | LV_STATE_DEFAULT);

    ui_streamingspeedvalue = lv_label_create(ui_StreamingSpeedL);
    lv_obj_set_width(ui_streamingspeedvalue, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_streamingspeedvalue, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_streamingspeedvalue, 80);
    lv_obj_set_y(ui_streamingspeedvalue, 0);
    lv_obj_set_align(ui_streamingspeedvalue, LV_ALIGN_LEFT_MID);
    lv_label_set_text(ui_streamingspeedvalue, T_BLANK);

    // Depth slider
    ui_StreamingDepthL = lv_label_create(ui_Streaming);
    lv_obj_set_width(ui_StreamingDepthL, lv_pct(95));
    lv_obj_set_height(ui_StreamingDepthL, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_StreamingDepthL, 0);
    lv_obj_set_y(ui_StreamingDepthL, 0);
    lv_obj_set_align(ui_StreamingDepthL, LV_ALIGN_CENTER);
    lv_label_set_text(ui_StreamingDepthL, T_DEPTH);
    lv_obj_set_style_text_font(ui_StreamingDepthL, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_streamingdepthslider = lv_slider_create(ui_StreamingDepthL);
    lv_slider_set_mode(ui_streamingdepthslider, LV_SLIDER_MODE_NORMAL);
    lv_slider_set_range(ui_streamingdepthslider, 0, 100);
    lv_obj_set_width(ui_streamingdepthslider, 130);
    lv_obj_set_height(ui_streamingdepthslider, 10);
    lv_obj_set_x(ui_streamingdepthslider, -15);
    lv_obj_set_y(ui_streamingdepthslider, 0);
    lv_obj_set_align(ui_streamingdepthslider, LV_ALIGN_RIGHT_MID);
    lv_obj_add_style(ui_streamingdepthslider, &style_slider_track[1], LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_streamingdepthslider, &style_slider_indicator[1], LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_streamingdepthslider, &style_slider_indicator[1], LV_PART_KNOB | LV_STATE_DEFAULT);

    ui_streamingdepthvalue = lv_label_create(ui_StreamingDepthL);
    lv_obj_set_width(ui_streamingdepthvalue, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_streamingdepthvalue, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_streamingdepthvalue, 80);
    lv_obj_set_y(ui_streamingdepthvalue, 0);
    lv_obj_set_align(ui_streamingdepthvalue, LV_ALIGN_LEFT_MID);
    lv_label_set_text(ui_streamingdepthvalue, T_BLANK);

    // Stroke slider
    ui_StreamingStrokeL = lv_label_create(ui_Streaming);
    lv_obj_set_width(ui_StreamingStrokeL, lv_pct(95));
    lv_obj_set_height(ui_StreamingStrokeL, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_StreamingStrokeL, 0);
    lv_obj_set_y(ui_StreamingStrokeL, 50);
    lv_obj_set_align(ui_StreamingStrokeL, LV_ALIGN_CENTER);
    lv_label_set_text(ui_StreamingStrokeL, T_STROKE);
    lv_obj_set_style_text_font(ui_StreamingStrokeL, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_streamingstrokeslider = lv_slider_create(ui_StreamingStrokeL);
    lv_slider_set_range(ui_streamingstrokeslider, 0, 100);
    lv_slider_set_mode(ui_streamingstrokeslider, LV_SLIDER_MODE_NORMAL);
    lv_obj_set_width(ui_streamingstrokeslider, 130);
    lv_obj_set_height(ui_streamingstrokeslider, 10);
    lv_obj_set_x(ui_streamingstrokeslider, -15);
    lv_obj_set_y(ui_streamingstrokeslider, 0);
    lv_obj_set_align(ui_streamingstrokeslider, LV_ALIGN_RIGHT_MID);
    lv_obj_add_style(ui_streamingstrokeslider, &style_slider_track[2], LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_streamingstrokeslider, &style_slider_indicator[2], LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_streamingstrokeslider, &style_slider_indicator[2], LV_PART_KNOB | LV_STATE_DEFAULT);

    ui_streamingstrokevalue = lv_label_create(ui_StreamingStrokeL);
    lv_obj_set_width(ui_streamingstrokevalue, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_streamingstrokevalue, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_streamingstrokevalue, 80);
    lv_obj_set_y(ui_streamingstrokevalue, 0);
    lv_obj_set_align(ui_streamingstrokevalue, LV_ALIGN_LEFT_MID);
    lv_label_set_text(ui_streamingstrokevalue, T_BLANK);

    // Battery display
    ui_Batt8 = lv_label_create(ui_Streaming);
    lv_obj_set_width(ui_Batt8, 85);
    lv_obj_set_height(ui_Batt8, 30);
    lv_obj_set_x(ui_Batt8, 115);
    lv_obj_set_y(ui_Batt8, -103);
    lv_obj_set_align(ui_Batt8, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Batt8, T_BATT);
    lv_obj_add_style(ui_Batt8, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_BattValue8 = lv_label_create(ui_Batt8);
    lv_obj_set_width(ui_BattValue8, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_BattValue8, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_BattValue8, 0);
    lv_obj_set_y(ui_BattValue8, -7);
    lv_obj_set_align(ui_BattValue8, LV_ALIGN_RIGHT_MID);
    lv_label_set_text(ui_BattValue8, T_BLANK);
    lv_obj_add_style(ui_BattValue8, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Battery8 = lv_bar_create(ui_Batt8);
    lv_bar_set_range(ui_Battery8, 0, 100);
    lv_obj_set_width(ui_Battery8, 80);
    lv_obj_set_height(ui_Battery8, 10);
    lv_obj_set_x(ui_Battery8, 0);
    lv_obj_set_y(ui_Battery8, 10);
    lv_obj_set_align(ui_Battery8, LV_ALIGN_CENTER);
    lv_obj_add_style(ui_Battery8, &style_battery_main, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_Battery8, &style_battery_indicator, LV_PART_INDICATOR | LV_STATE_DEFAULT);
}

void ui_Addons_screen_init(void)
{
    ui_Addons = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Addons, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(ui_Addons, ui_event_Addons, LV_EVENT_ALL, NULL);
    lv_obj_add_style(ui_Addons, &style_option_bg, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_LogoAddons = lv_label_create(ui_Addons);
    lv_obj_set_width(ui_LogoAddons, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_LogoAddons, LV_SIZE_CONTENT);
    lv_obj_set_y(ui_LogoAddons, -103);
    lv_obj_set_x(ui_LogoAddons, lv_pct(0));
    lv_obj_set_align(ui_LogoAddons, LV_ALIGN_CENTER);
    lv_label_set_text(ui_LogoAddons, T_SCREEN_ADDONS);
    lv_obj_set_style_text_font(ui_LogoAddons, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_LogoAddons, &style_title_bar, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_LogoAddons, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_AddonsHint = lv_label_create(ui_Addons);
    lv_obj_set_width(ui_AddonsHint, lv_pct(95));
    lv_obj_set_height(ui_AddonsHint, LV_SIZE_CONTENT);
    lv_obj_set_y(ui_AddonsHint, -68);
    lv_obj_set_align(ui_AddonsHint, LV_ALIGN_CENTER);
    lv_label_set_text(ui_AddonsHint, T_ADDONS_HINT);
    lv_obj_set_style_text_font(ui_AddonsHint, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_AddonsHint, &style_text_secondary, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Three items: height 35px each, y offsets: -32, 10, 52 (gap 7px)
    ui_AddonsItem0 = lv_btn_create(ui_Addons);
    lv_obj_set_width(ui_AddonsItem0, 300);
    lv_obj_set_height(ui_AddonsItem0, 35);
    lv_obj_set_y(ui_AddonsItem0, -32);
    lv_obj_set_align(ui_AddonsItem0, LV_ALIGN_CENTER);
    lv_obj_add_event_cb(ui_AddonsItem0, ui_event_AddonsItem0, LV_EVENT_SHORT_CLICKED, NULL);
    lv_obj_add_style(ui_AddonsItem0, &style_slider_track[0], LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_AddonsItem0, &style_button_m, LV_PART_MAIN | LV_STATE_FOCUSED);

    ui_AddonsItem0Text = lv_label_create(ui_AddonsItem0);
    lv_obj_center(ui_AddonsItem0Text);
    lv_label_set_text(ui_AddonsItem0Text, "");
    lv_obj_add_style(ui_AddonsItem0Text, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_AddonsItem1 = lv_btn_create(ui_Addons);
    lv_obj_set_width(ui_AddonsItem1, 300);
    lv_obj_set_height(ui_AddonsItem1, 35);
    lv_obj_set_y(ui_AddonsItem1, 10);
    lv_obj_set_align(ui_AddonsItem1, LV_ALIGN_CENTER);
    lv_obj_add_event_cb(ui_AddonsItem1, ui_event_AddonsItem1, LV_EVENT_SHORT_CLICKED, NULL);
    lv_obj_add_style(ui_AddonsItem1, &style_slider_track[1], LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_AddonsItem1, &style_button_m, LV_PART_MAIN | LV_STATE_FOCUSED);

    ui_AddonsItem1Text = lv_label_create(ui_AddonsItem1);
    lv_obj_center(ui_AddonsItem1Text);
    lv_label_set_text(ui_AddonsItem1Text, "");
    lv_obj_add_style(ui_AddonsItem1Text, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_AddonsItem2 = lv_btn_create(ui_Addons);
    lv_obj_set_width(ui_AddonsItem2, 300);
    lv_obj_set_height(ui_AddonsItem2, 35);
    lv_obj_set_y(ui_AddonsItem2, 52);
    lv_obj_set_align(ui_AddonsItem2, LV_ALIGN_CENTER);
    lv_obj_add_event_cb(ui_AddonsItem2, ui_event_AddonsItem2, LV_EVENT_SHORT_CLICKED, NULL);
    lv_obj_add_style(ui_AddonsItem2, &style_slider_track[2], LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(ui_AddonsItem2, &style_button_m, LV_PART_MAIN | LV_STATE_FOCUSED);

    ui_AddonsItem2Text = lv_label_create(ui_AddonsItem2);
    lv_obj_center(ui_AddonsItem2Text);
    lv_label_set_text(ui_AddonsItem2Text, "");
    lv_obj_add_style(ui_AddonsItem2Text, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_AddonsButtonL = lv_btn_create(ui_Addons);
    lv_obj_set_width(ui_AddonsButtonL, 100);
    lv_obj_set_height(ui_AddonsButtonL, 30);
    lv_obj_set_y(ui_AddonsButtonL, 100);
    lv_obj_set_x(ui_AddonsButtonL, lv_pct(-33));
    lv_obj_set_align(ui_AddonsButtonL, LV_ALIGN_CENTER);
    lv_obj_add_event_cb(ui_AddonsButtonL, ui_event_AddonsButtonL, LV_EVENT_SHORT_CLICKED, NULL);
    lv_obj_add_style(ui_AddonsButtonL, &style_button_l, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_AddonsButtonLText = lv_label_create(ui_AddonsButtonL);
    lv_obj_center(ui_AddonsButtonLText);
    lv_label_set_text(ui_AddonsButtonLText, T_BACK);
    lv_obj_add_style(ui_AddonsButtonLText, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_AddonsButtonM = lv_btn_create(ui_Addons);
    lv_obj_set_width(ui_AddonsButtonM, 100);
    lv_obj_set_height(ui_AddonsButtonM, 30);
    lv_obj_set_y(ui_AddonsButtonM, 100);
    lv_obj_set_x(ui_AddonsButtonM, lv_pct(0));
    lv_obj_set_align(ui_AddonsButtonM, LV_ALIGN_CENTER);
    lv_obj_add_event_cb(ui_AddonsButtonM, ui_event_AddonsButtonM, LV_EVENT_SHORT_CLICKED, NULL);
    lv_obj_add_style(ui_AddonsButtonM, &style_button_m, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_AddonsButtonMText = lv_label_create(ui_AddonsButtonM);
    lv_obj_center(ui_AddonsButtonMText);
    lv_label_set_text(ui_AddonsButtonMText, T_OPEN);
    lv_obj_add_style(ui_AddonsButtonMText, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_AddonsButtonR = lv_btn_create(ui_Addons);
    lv_obj_set_width(ui_AddonsButtonR, 100);
    lv_obj_set_height(ui_AddonsButtonR, 30);
    lv_obj_set_y(ui_AddonsButtonR, 100);
    lv_obj_set_x(ui_AddonsButtonR, lv_pct(33));
    lv_obj_set_align(ui_AddonsButtonR, LV_ALIGN_CENTER);
    lv_obj_add_event_cb(ui_AddonsButtonR, ui_event_AddonsButtonR, LV_EVENT_SHORT_CLICKED, NULL);
    lv_obj_add_style(ui_AddonsButtonR, &style_button_r, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_AddonsButtonRText = lv_label_create(ui_AddonsButtonR);
    lv_obj_center(ui_AddonsButtonRText);
    lv_label_set_text(ui_AddonsButtonRText, T_SELECT);
    lv_obj_add_style(ui_AddonsButtonRText, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_g_addons = lv_group_create();
    lv_group_add_obj(ui_g_addons, ui_AddonsItem0);
    lv_group_add_obj(ui_g_addons, ui_AddonsItem1);
    lv_group_add_obj(ui_g_addons, ui_AddonsItem2);
}

void ui_init(void)
{
    lv_disp_t * dispp = lv_disp_get_default();
    lv_theme_t * theme = lv_theme_default_init(dispp, lv_color_hex(getActivePrimaryColor()), lv_color_hex(getActiveSecondaryColor()), dark_mode, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);

    ui_Start_screen_init();
    ui_Home_screen_init();
//    ui_Menue_screen_init();
    ui_Pattern_screen_init();
    ui_EJECTSettings_screen_init();
    ui_Settings_screen_init();
    ui_Menu_screen_init();
    ui_Streaming_screen_init();
    ui_Addons_screen_init();
    colors_ui_screen_init();
    lv_disp_load_scr(ui_Start);
}

#ifdef __cplusplus
} /* extern "C" */
#endif

