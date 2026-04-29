#pragma once

#include <stdint.h>

#ifdef __cplusplus
#include "OneButton.h"
#endif

#define OSSM 1
#define CUM 2
extern uint8_t CUM_Address[6];


#define OSSM_ID  1 //OSSM_ID Default can be changed with M5 Remote in the Future will be Saved in EPROOM
#define M5_ID 99 //M5_ID Default can be changed with M5 Remote in the Future will be Saved in EPROOM
#define BATTERY_CHARGE_CURRENT 390 // Charge current, must be one of AXP192::CHGCurrent

// ESP-NOW fixed channel for reliable pairing with OSSM (1..11)
#ifndef ESP_NOW_CHANNEL
#define ESP_NOW_CHANNEL 1
#endif

#ifdef ARDUINO_M5STACK_CORES3
//Pin Definitons Encoders S3:

#define ENC_1_CLK 5
#define ENC_1_DT 9

#define ENC_2_CLK 18
#define ENC_2_DT 17

#define ENC_3_CLK 1
#define ENC_3_DT 2

#define ENC_4_CLK 7
#define ENC_4_DT 6
#ifdef __cplusplus
extern OneButton Button1;
extern OneButton Button2;
extern OneButton Button3;
#endif
#else

#define ENC_1_CLK 25
#define ENC_1_DT 26

#define ENC_2_CLK 13
#define ENC_2_DT 14

#define ENC_3_CLK 33
#define ENC_3_DT 32

#define ENC_4_CLK 19
#define ENC_4_DT 27
#ifdef __cplusplus
extern OneButton Button1;
extern OneButton Button2;
extern OneButton Button3;
#endif
#endif

#define Encoder_MAP 144

#define LV_HOR_RES_MAX 320
#define LV_VER_RES_MAX 240

// Addons catalog used by the Addons UI and dynamic Home double-click bindings.
typedef struct AddonDefinition {
	const char* id;
	const char* displayName;
} AddonDefinition;

#define ADDON_SLOT_NONE  0
#define ADDON_SLOT_LEFT  1
#define ADDON_SLOT_RIGHT 2

static const AddonDefinition ADDON_DEFINITIONS[] = {
	{"eject_cumpump",  "Eject Cumpump"},
	{"fist_it",        "Fist-IT"},
	{"color_schemes",  "Color Schemes"},
	{"streaming_mode", "Streaming Mode"},
};

#define ADDON_DEFINITIONS_COUNT (sizeof(ADDON_DEFINITIONS) / sizeof(ADDON_DEFINITIONS[0]))

// Color 
#define TextColor  TFT_PURPLE
#define BgColor    TFT_WHITE
#define FrontColor TFT_PURPLE
#define HighlightColor TFT_BLACK

// Runtime power/screen policy defaults (edit these values here)
#define SCREENSAVER_TIMEOUT_MS_DEFAULT (2 * 60 * 1000)
#define SCREENSAVER_DIM_BRIGHTNESS_DEFAULT 15
#define DEEP_SLEEP_TIMEOUT_MS_DEFAULT (10UL * 60UL * 1000UL)
// Set to 0 to disable automatic idle deep-sleep. Manual sleep from Menu remains available.
#define AUTO_IDLE_DEEP_SLEEP_ENABLED 0

// Minimum time to keep Start screen visible before auto-switching to Home.
#define START_SCREEN_MIN_DISPLAY_MS 3000UL

#ifdef __cplusplus
// Runtime-configurable debug flags (defined in config_debug.cpp)
extern bool ShowBLECommandResponses; // print BLE command responses
extern bool ShowBLEDebugCommands;   // print concise BLE send/parse debug
#endif


