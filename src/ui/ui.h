// SquareLine LVGL GENERATED FILE
// EDITOR VERSION: SquareLine Studio 1.0.5
// LVGL VERSION: 8.2
// PROJECT: OSSM-White

#ifndef OSSM_WHITE_UI_H
#define OSSM_WHITE_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#if __has_include("lvgl.h")
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

extern lv_obj_t * ui_Start;
extern lv_obj_t * ui_Logo;
extern lv_obj_t * ui_StartButtonL;
extern lv_obj_t * ui_StartButtonLText;
extern lv_obj_t * ui_StartButtonM;
extern lv_obj_t * ui_StartButtonMText;
extern lv_obj_t * ui_StartButtonR;
extern lv_obj_t * ui_StartButtonRText;
extern lv_obj_t * ui_LOVE_Logo;
extern lv_obj_t * ui_OrtlofLogo;
extern lv_obj_t * ui_Welcome;
extern lv_obj_t * ui_Batt;
extern lv_obj_t * ui_BattValue;
extern lv_obj_t * ui_Battery;
extern lv_obj_t * ui_Home;
extern lv_obj_t * ui_Logo2;
extern lv_obj_t * ui_HomeButtonL;
extern lv_obj_t * ui_HomeButtonLText;
extern lv_obj_t * ui_HomeButtonM;
extern lv_obj_t * ui_HomeButtonMText;
extern lv_obj_t * ui_HomeButtonR;
extern lv_obj_t * ui_HomeButtonRText;
extern lv_obj_t * ui_SpeedL;
extern lv_obj_t * ui_homespeedslider;
extern lv_obj_t * ui_homespeedvalue;
extern lv_obj_t * ui_DepthL;
extern lv_obj_t * ui_homedepthslider;
extern lv_obj_t * ui_homedepthvalue;
extern lv_obj_t * ui_StrokeL;
extern lv_obj_t * ui_homestrokeslider;
extern lv_obj_t * ui_homestrokevalue;
extern lv_obj_t * ui_SensationL;
extern lv_obj_t * ui_homesensationslider;
extern lv_obj_t * ui_Batt2;
extern lv_obj_t * ui_BattValue2;
extern lv_obj_t * ui_Battery2;
extern lv_obj_t * ui_HomePatternLabel1;
extern lv_obj_t * ui_HomePatternLabel;
extern lv_obj_t * ui_connect;
/*extern lv_obj_t * ui_Menue;
extern lv_obj_t * ui_Logo3;
extern lv_obj_t * ui_MenueButtonL;
extern lv_obj_t * ui_MenueButtonLText;
extern lv_obj_t * ui_MenueButtonM;
extern lv_obj_t * ui_MenueButtonMText;
extern lv_obj_t * ui_MenueButtonR;
extern lv_obj_t * ui_MenueButtonRText;
*/
extern lv_obj_t * ui_brightness_slider;
extern lv_obj_t * ui_Batt3;
extern lv_obj_t * ui_BattValue3;
extern lv_obj_t * ui_Battery3;
extern lv_obj_t * ui_SDepthI;
extern lv_obj_t * ui_SDepthIL;
extern lv_obj_t * ui_SDepthF;
extern lv_obj_t * ui_SDepthFL;
extern lv_obj_t * ui_SPattern;
extern lv_obj_t * ui_SPatternL;
extern lv_obj_t * ui_STorqe;
extern lv_obj_t * ui_STorqeL;
extern lv_obj_t * ui_EJECTSettingButton;
extern lv_obj_t * ui_EJECTSettingButtonText;
extern lv_obj_t * ui_Pattern;
extern lv_obj_t * ui_Logo5;
extern lv_obj_t * ui_PatternButtonL;
extern lv_obj_t * ui_PatternButtonLText;
extern lv_obj_t * ui_PatternButtonM;
extern lv_obj_t * ui_PatternButtonMText;
extern lv_obj_t * ui_PatternButtonR;
extern lv_obj_t * ui_PatternButtonRText;
extern lv_obj_t * ui_Batt5;
extern lv_obj_t * ui_BattValue5;
extern lv_obj_t * ui_Battery5;
extern lv_obj_t * ui_Label4;
extern lv_obj_t * ui_PatternS;
extern lv_obj_t * ui_Torqe;
extern lv_obj_t * ui_Logo4;
extern lv_obj_t * ui_TorqeButtonL;
extern lv_obj_t * ui_TorqeButtonLText;
extern lv_obj_t * ui_TorqeButtonM;
extern lv_obj_t * ui_TorqeButtonMText;
extern lv_obj_t * ui_TorqeButtonR;
extern lv_obj_t * ui_TorqeButtonRText;
extern lv_obj_t * ui_outtroqelabel;
extern lv_obj_t * ui_outtroqevalue;
extern lv_obj_t * ui_outtroqeslider;
extern lv_obj_t * ui_Low1;
extern lv_obj_t * ui_High1;
extern lv_obj_t * ui_introqelabel;
extern lv_obj_t * ui_introqevalue;
extern lv_obj_t * ui_introqeslider;
extern lv_obj_t * ui_Low2;
extern lv_obj_t * ui_High;
extern lv_obj_t * ui_Batt4;
extern lv_obj_t * ui_BattValue4;
extern lv_obj_t * ui_Battery4;
extern lv_obj_t * ui_EJECTSettings;
extern lv_obj_t * ui_Logo6;
extern lv_obj_t * ui_EJECTButtonL;
extern lv_obj_t * ui_EJECTButtonLText;
extern lv_obj_t * ui_EJECTButtonM;
extern lv_obj_t * ui_EJECTButtonMText;
extern lv_obj_t * ui_EJECTButtonR;
extern lv_obj_t * ui_EJECTButtonRText;
extern lv_obj_t * ui_Batt6;
extern lv_obj_t * ui_BattValue6;
extern lv_obj_t * ui_Battery6;
extern lv_obj_t * ui_Settings;
extern lv_obj_t * ui_Logo1;
extern lv_obj_t * ui_SettingsButtonL;
extern lv_obj_t * ui_SettingsButtonLText;
extern lv_obj_t * ui_SettingsButtonM;
extern lv_obj_t * ui_SettingsButtonMText;
extern lv_obj_t * ui_SettingsButtonR;
extern lv_obj_t * ui_SettingsButtonRText;
extern lv_obj_t * ui_brightness_icon;
extern lv_obj_t * ui_Batt1;
extern lv_obj_t * ui_BattValue1;
extern lv_obj_t * ui_Battery1;
extern lv_obj_t * ui_ejectaddon;
extern lv_obj_t * ui_strokeinvert;
extern lv_obj_t * ui_darkmode;
extern lv_obj_t * ui_vibrate;
extern lv_obj_t * ui_lefty;

// UI_MENU screen objects
extern lv_obj_t * ui_Menu;
extern lv_obj_t * ui_LogoMenu;
extern lv_obj_t * ui_MenuButtonTL;
extern lv_obj_t * ui_MenuButtonTLText;
extern lv_obj_t * ui_MenuButtonTR;
extern lv_obj_t * ui_MenuButtonTRText;
extern lv_obj_t * ui_MenuButtonML;
extern lv_obj_t * ui_MenuButtonMLText;
extern lv_obj_t * ui_MenuButtonMR;
extern lv_obj_t * ui_MenuButtonMRText;
extern lv_obj_t * ui_MenuButtonBL;
extern lv_obj_t * ui_MenuButtonBLText;
extern lv_obj_t * ui_MenuButtonBR;
extern lv_obj_t * ui_MenuButtonBRText;
extern lv_obj_t * ui_Batt7;
extern lv_obj_t * ui_BattValue7;
extern lv_obj_t * ui_Battery7;
extern lv_obj_t * ui_MenuButtonL;
extern lv_obj_t * ui_MenuButtonLText;
extern lv_obj_t * ui_MenuButtonM;
extern lv_obj_t * ui_MenuButtonMText;
extern lv_obj_t * ui_MenuButtonR;
extern lv_obj_t * ui_MenuButtonRText;

// UI_STREAMING screen objects
extern lv_obj_t * ui_Streaming;
extern lv_obj_t * ui_LogoStreaming;
extern lv_obj_t * ui_StreamingButtonL;
extern lv_obj_t * ui_StreamingButtonLText;
extern lv_obj_t * ui_StreamingButtonM;
extern lv_obj_t * ui_StreamingButtonMText;
extern lv_obj_t * ui_StreamingButtonR;
extern lv_obj_t * ui_StreamingButtonRText;
extern lv_obj_t * ui_StreamingSpeedL;
extern lv_obj_t * ui_streamingspeedslider;
extern lv_obj_t * ui_streamingspeedvalue;
extern lv_obj_t * ui_StreamingDepthL;
extern lv_obj_t * ui_streamingdepthslider;
extern lv_obj_t * ui_streamingdepthvalue;
extern lv_obj_t * ui_StreamingStrokeL;
extern lv_obj_t * ui_streamingstrokeslider;
extern lv_obj_t * ui_streamingstrokevalue;
extern lv_obj_t * ui_Batt8;
extern lv_obj_t * ui_BattValue8;
extern lv_obj_t * ui_Battery8;

// UI_ADDONS screen objects
extern lv_obj_t * ui_Addons;
extern lv_obj_t * ui_LogoAddons;
extern lv_obj_t * ui_AddonsHint;
extern lv_obj_t * ui_AddonsItem0;
extern lv_obj_t * ui_AddonsItem0Text;
extern lv_obj_t * ui_AddonsItem1;
extern lv_obj_t * ui_AddonsItem1Text;
extern lv_obj_t * ui_AddonsButtonL;
extern lv_obj_t * ui_AddonsButtonLText;
extern lv_obj_t * ui_AddonsButtonR;
extern lv_obj_t * ui_AddonsButtonRText;
extern lv_obj_t * ui_darkmode;
extern lv_obj_t * ui_vibrate;
extern lv_obj_t * ui_lefty;
extern lv_group_t * ui_g_menu;
extern lv_group_t * ui_g_menue;
extern lv_group_t * ui_g_settings;
extern lv_group_t * ui_g_addons;


void screenmachine(lv_event_t * e);
void connectbutton(lv_event_t * e);
void screenmachine(lv_event_t * e);
void homebuttonLevent(lv_event_t * e);
void homebuttonmevent(lv_event_t * e);
void streamingbuttonmevent(lv_event_t * e);
void screenmachine(lv_event_t * e);
void setupDepthInter(lv_event_t * e);
void setupdepthF(lv_event_t * e);
void screenmachine(lv_event_t * e);
void savepattern(lv_event_t * e);
void screenmachine(lv_event_t * e);
void screenmachine(lv_event_t * e);
void screenmachine(lv_event_t * e);
void savesettings(lv_event_t * e);
void brightness_slider_event_cb(lv_event_t * e);

LV_IMG_DECLARE(image50x50);    // assets\logo.svg

void ui_init(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
