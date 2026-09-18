// ============================================================================
// language.h - runtime-selectable user-facing text for the M5 Remote.
//
// Screens use the T_XXX macros, which resolve through languageGet() to the
// active language. All languages are compiled in and switched at runtime,
// so a Settings entry can change language without reflashing (a restart to
// re-render labels is fine).
//
// Structure:
//   languages/defaultlanguage.h - master key list + English (the fallback)
//   languages/xx.h              - extra language (same keys, same order,
//                                NULL = keep English)
//   language.cpp                - tables + lookup + NVS persistence
//
// ADDING A LANGUAGE: see languages/defaultlanguage.h.
// ============================================================================
#pragma once

#include <stdint.h>
#include "languages/defaultlanguage.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum LanguageId {
    LANGUAGE_EN = 0,
    // LANGUAGE_DE,    // add new languages here + table in language.cpp
    LANGUAGE_COUNT
} LanguageId;

#define LANG_ENUM(key, text) Tk_##key,
typedef enum TextId {
    LANG_STRINGS(LANG_ENUM)
    Tk_COUNT
} TextId;
#undef LANG_ENUM

// Runtime language selection (all tables compiled in).
const char* languageGet(TextId id);        // active language, English fallback
LanguageId  languageGetCurrent(void);
void        languageSet(LanguageId lang);  // switch in memory
void        languageInit(void);            // load persisted choice from NVS
void        languageStore(LanguageId lang);// persist choice to NVS

#ifdef __cplusplus
}
#endif

// ---------------------------------------------------------------------------
// T_XXX convenience macros - call sites use these and never change.
// ---------------------------------------------------------------------------

#define T_HEADER                   languageGet(Tk_HEADER)
#define T_BLANK                    languageGet(Tk_BLANK)
#define T_MENU                     languageGet(Tk_MENU)
#define T_DEMO                     languageGet(Tk_DEMO)
#define T_CON                     languageGet(Tk_CONNECT)
#define T_AUTOCONNECTING           languageGet(Tk_AUTOCONNECTING)
#define T_SEARCHING_BLE            languageGet(Tk_SEARCHING_BLE)
#define T_FAILED                   languageGet(Tk_FAILED)
#define T_HOMING                   languageGet(Tk_HOMING)
#define T_BLECONNECTED             languageGet(Tk_BLECONNECTED)
#define T_CONNECTED_TO             languageGet(Tk_CONNECTED_TO)
#define T_ESPCONNECTED             languageGet(Tk_ESPCONNECTED)
#define T_SETTINGS                 languageGet(Tk_SETTINGS)
#define T_MOTD                     languageGet(Tk_MOTD)
#define T_BATT                     languageGet(Tk_BATT)
#define T_HOMEL                    languageGet(Tk_HOMEL)
#define T_START                    languageGet(Tk_START)
#define T_RESUME                   languageGet(Tk_RESUME)
#define T_PAUSE                    languageGet(Tk_PAUSE)
#define T_STOP                     languageGet(Tk_STOP)
#define T_CLOSE                    languageGet(Tk_CLOSE)
#define T_BACK                     languageGet(Tk_BACK)
#define T_FAIL                     languageGet(Tk_FAIL)
#define T_CUM                      languageGet(Tk_CUM)
#define T_HOME                     languageGet(Tk_HOME)
#define T_SPEED                    languageGet(Tk_SPEED)
#define T_DEPTH                    languageGet(Tk_DEPTH)
#define T_MM                       languageGet(Tk_MM)
#define T_STROKE                   languageGet(Tk_STROKE)
#define T_SENSATION                languageGet(Tk_SENSATION)
#define T_SETUP_DEPTH_I            languageGet(Tk_SETUP_DEPTH_I)
#define T_SETUP_DEPTH_F            languageGet(Tk_SETUP_DEPTH_F)
#define T_SELECT_PATTERN           languageGet(Tk_SELECT_PATTERN)
#define T_PATTERN_Button           languageGet(Tk_PATTERN_Button)
#define T_RESTART                  languageGet(Tk_RESTART)
#define T_TURN_OFF                 languageGet(Tk_TURN_OFF)
#define T_OUT_TORQE                languageGet(Tk_OUT_TORQE)
#define T_IN_TORQE                 languageGet(Tk_IN_TORQE)
#define T_EJECT                    languageGet(Tk_EJECT)
#define T_SELECT                   languageGet(Tk_SELECT)
#define T_OPEN                     languageGet(Tk_OPEN)
#define T_SAVE                     languageGet(Tk_SAVE)
#define T_LOW                      languageGet(Tk_LOW)
#define T_HIGH                     languageGet(Tk_HIGH)
#define T_HOME_FORCE               languageGet(Tk_HOME_FORCE)
#define T_VIBRATE                  languageGet(Tk_VIBRATE)
#define T_TOUCHSETTING             languageGet(Tk_TOUCHSETTING)
#define T_SAFESTARTSTOP            languageGet(Tk_SAFESTARTSTOP)
#define T_STROKEINVERT             languageGet(Tk_STROKEINVERT)
#define T_VISUALSPEEDLOCK          languageGet(Tk_VISUALSPEEDLOCK)
#define T_STROKEDEPTHLINK          languageGet(Tk_STROKEDEPTHLINK)
#define T_SLEEP                    languageGet(Tk_SLEEP)
#define T_ESP_NOW                  languageGet(Tk_ESP_NOW)
#define T_CHARGING_WARNING_TITLE   languageGet(Tk_CHARGING_WARNING_TITLE)
#define T_CHARGING_WARNING_TEXT    languageGet(Tk_CHARGING_WARNING_TEXT)
#define T_Patterns                 languageGet(Tk_Patterns)
#define T_SimpleStroke             languageGet(Tk_SimpleStroke)
#define T_TeasingPounding          languageGet(Tk_TeasingPounding)
#define T_RoboStroke               languageGet(Tk_RoboStroke)
#define T_HalfnHalf                languageGet(Tk_HalfnHalf)
#define T_Deeper                   languageGet(Tk_Deeper)
#define T_StopNGo                  languageGet(Tk_StopNGo)
#define T_Insist                   languageGet(Tk_Insist)
#define T_JackHammer               languageGet(Tk_JackHammer)
#define T_StrokeNibbler            languageGet(Tk_StrokeNibbler)
#define T_Knot                     languageGet(Tk_Knot)
#define T_PULLING_OUT              languageGet(Tk_PULLING_OUT)
#define T_PULLING_OUT_TEXT         languageGet(Tk_PULLING_OUT_TEXT)
#define T_CUM_SPEED                languageGet(Tk_CUM_SPEED)
#define T_CUM_TIME                 languageGet(Tk_CUM_TIME)
#define T_CUM_Volume               languageGet(Tk_CUM_Volume)
#define T_CUM_Accel                languageGet(Tk_CUM_Accel)
#define T_CUM_LOAD                 languageGet(Tk_CUM_LOAD)
#define T_ROTATION                 languageGet(Tk_ROTATION)
#define T_ACCEL                    languageGet(Tk_ACCEL)
#define T_HOMESCREEN               languageGet(Tk_HOMESCREEN)
#define T_HOMESCREEN_SUB           languageGet(Tk_HOMESCREEN_SUB)
#define T_STREAMING                languageGet(Tk_STREAMING)
#define T_STREAMING_SUB            languageGet(Tk_STREAMING_SUB)
#define T_STROKE_SCREEN            languageGet(Tk_STROKE_SCREEN)
#define T_STROKE_SUB               languageGet(Tk_STROKE_SUB)
#define T_ADDONS                   languageGet(Tk_ADDONS)
#define T_PATTERN_GENERATOR        languageGet(Tk_PATTERN_GENERATOR)
#define T_OVERRIDE                 languageGet(Tk_OVERRIDE)
#define T_STOP_STREAMING           languageGet(Tk_STOP_STREAMING)
#define T_CONFIRM                  languageGet(Tk_CONFIRM)
#define T_NEXT                     languageGet(Tk_NEXT)
#define T_PREV                     languageGet(Tk_PREV)
#define T_TOYS                     languageGet(Tk_TOYS)
#define T_NO_TOYS                  languageGet(Tk_NO_TOYS)
#define T_SHUTDOWN                 languageGet(Tk_SHUTDOWN)
#define T_CANCEL                   languageGet(Tk_CANCEL)
#define T_DONE                     languageGet(Tk_DONE)
#define T_STREAMING_CAUTION_TITLE  languageGet(Tk_STREAMING_CAUTION_TITLE)
#define T_STREAMING_CAUTION_TEXT   languageGet(Tk_STREAMING_CAUTION_TEXT)
#define T_STREAMING_ACTIVE_TITLE   languageGet(Tk_STREAMING_ACTIVE_TITLE)
#define T_STREAMING_ACTIVE_TEXT    languageGet(Tk_STREAMING_ACTIVE_TEXT)
#define T_STREAMING_RUNNING_TITLE  languageGet(Tk_STREAMING_RUNNING_TITLE)
#define T_STREAMING_RUNNING_TEXT   languageGet(Tk_STREAMING_RUNNING_TEXT)
#define T_STREAMING_FAIL_TITLE     languageGet(Tk_STREAMING_FAIL_TITLE)
#define T_STREAMING_FAIL_TEXT      languageGet(Tk_STREAMING_FAIL_TEXT)
#define T_SCREEN_START             languageGet(Tk_SCREEN_START)
#define T_SCREEN_STROKE_ENGINE     languageGet(Tk_SCREEN_STROKE_ENGINE)
#define T_SCREEN_MENU              languageGet(Tk_SCREEN_MENU)
#define T_SCREEN_STREAMING         languageGet(Tk_SCREEN_STREAMING)
#define T_SCREEN_SETTINGS          languageGet(Tk_SCREEN_SETTINGS)
#define T_SCREEN_ADDONS            languageGet(Tk_SCREEN_ADDONS)
#define T_SCREEN_COLORS            languageGet(Tk_SCREEN_COLORS)
#define T_SCREEN_PATTERN           languageGet(Tk_SCREEN_PATTERN)
#define T_SCREEN_TORQUE            languageGet(Tk_SCREEN_TORQUE)
#define T_SCREEN_EJECT             languageGet(Tk_SCREEN_EJECT)
#define T_ADDONS_HINT              languageGet(Tk_ADDONS_HINT)
#define T_ADDONS_SLOT_LEFT         languageGet(Tk_ADDONS_SLOT_LEFT)
#define T_ADDONS_SLOT_RIGHT        languageGet(Tk_ADDONS_SLOT_RIGHT)
#define T_ADDONS_SLOT_OFF          languageGet(Tk_ADDONS_SLOT_OFF)
#define T_ENABLEDISABLE            languageGet(Tk_ENABLEDISABLE)
#define T_SHOWALL                  languageGet(Tk_SHOWALL)
#define T_COYOTE_SCREEN            languageGet(Tk_COYOTE_SCREEN)
#define T_COYOTE_FREQUENCY         languageGet(Tk_COYOTE_FREQUENCY)
#define T_COYOTE_INTENSITY         languageGet(Tk_COYOTE_INTENSITY)
#define T_COYOTE_SENSATION         languageGet(Tk_COYOTE_SENSATION)
#define T_COYOTE_CHANNEL           languageGet(Tk_COYOTE_CHANNEL)
#define T_COYOTE_SPEED_SENS        languageGet(Tk_COYOTE_SPEED_SENS)
#define T_COYOTE_ACCEL_SENS        languageGet(Tk_COYOTE_ACCEL_SENS)
#define T_PAIR                     languageGet(Tk_PAIR)
#define T_APMODE                   languageGet(Tk_APMODE)
#define T_FALLBACK                 languageGet(Tk_FALLBACK)
#define T_PRESET                   languageGet(Tk_PRESET)
#define T_MOD                      languageGet(Tk_MOD)
#define T_MODIFIER                 languageGet(Tk_MODIFIER)
#define T_PRESETS                  languageGet(Tk_PRESETS)
#define T_WARNING                  languageGet(Tk_WARNING)
#define T_APV2                     languageGet(Tk_APV2)
#define T_APV2_SPEED               languageGet(Tk_APV2_SPEED)
#define T_APV2_TOP                 languageGet(Tk_APV2_TOP)
#define T_APV2_BOTTOM              languageGet(Tk_APV2_BOTTOM)
#define T_APV2_RISE                languageGet(Tk_APV2_RISE)
#define T_APV2_BASE                languageGet(Tk_APV2_BASE)
#define T_APV2_VALUE               languageGet(Tk_APV2_VALUE)
#define T_APV2_MODS                languageGet(Tk_APV2_MODS)
#define T_BLE_COMM_ERROR_TITLE     languageGet(Tk_BLE_COMM_ERROR_TITLE)
#define T_BLE_COMM_ERROR_TEXT      languageGet(Tk_BLE_COMM_ERROR_TEXT)
#define T_SHUTDOWN_SLEEP_TITLE     languageGet(Tk_SHUTDOWN_SLEEP_TITLE)
#define T_SHUTDOWN_SLEEP_TEXT      languageGet(Tk_SHUTDOWN_SLEEP_TEXT)
#define T_RECONNECT                languageGet(Tk_RECONNECT)
#define T_SET_RAIL_LENGTH          languageGet(Tk_SET_RAIL_LENGTH)
#define T_RAIL_LENGTH_PLAIN        languageGet(Tk_RAIL_LENGTH_PLAIN)
#define T_RAIL_LENGTH_VALUE        languageGet(Tk_RAIL_LENGTH_VALUE)
#define T_ENCODER_RAMP             languageGet(Tk_ENCODER_RAMP)
#define T_SPEED_BEHAVIOR           languageGet(Tk_SPEED_BEHAVIOR)
#define T_LANGUAGE                 languageGet(Tk_LANGUAGE)
#define T_WIFI_SETUP               languageGet(Tk_WIFI_SETUP)
#define T_RAMP_NONE                languageGet(Tk_RAMP_NONE)
#define T_RAMP_MEDIUM              languageGet(Tk_RAMP_MEDIUM)
#define T_RAMP_HIGH                languageGet(Tk_RAMP_HIGH)
#define T_RAMP_AGGRESSIVE          languageGet(Tk_RAMP_AGGRESSIVE)
#define T_BEHAVIOR_STANDARD        languageGet(Tk_BEHAVIOR_STANDARD)
#define T_BEHAVIOR_NATURAL         languageGet(Tk_BEHAVIOR_NATURAL)
#define T_BEHAVIOR_TAMED           languageGet(Tk_BEHAVIOR_TAMED)
#define T_CALIBRATE_TITLE          languageGet(Tk_CALIBRATE_TITLE)
#define T_CALIBRATE_NO_BLE         languageGet(Tk_CALIBRATE_NO_BLE)
#define T_CALIBRATE_PROMPT         languageGet(Tk_CALIBRATE_PROMPT)
#define T_CALIBRATE_CONFIRM_TITLE  languageGet(Tk_CALIBRATE_CONFIRM_TITLE)
#define T_CALIBRATE_CONFIRM_TEXT   languageGet(Tk_CALIBRATE_CONFIRM_TEXT)
#define T_SLEEP_CONFIRM_TITLE      languageGet(Tk_SLEEP_CONFIRM_TITLE)
#define T_SLEEP_CONFIRM_TEXT       languageGet(Tk_SLEEP_CONFIRM_TEXT)
#define T_RESTART_CONFIRM_TEXT     languageGet(Tk_RESTART_CONFIRM_TEXT)
#define T_YES                      languageGet(Tk_YES)
#define T_NO                       languageGet(Tk_NO)
#define T_OK                       languageGet(Tk_OK)
#define T_CANCEL_SHORT             languageGet(Tk_CANCEL_SHORT)
#define T_RETRY                    languageGet(Tk_RETRY)
#define T_NOTIFICATION_TITLE       languageGet(Tk_NOTIFICATION_TITLE)
#define T_LEFT                     languageGet(Tk_LEFT)
#define T_RIGHT                    languageGet(Tk_RIGHT)
#define T_FISTIT_TITLE             languageGet(Tk_FISTIT_TITLE)
#define T_AP_TITLE                 languageGet(Tk_AP_TITLE)
#define T_COLOR_DEEP_PURPLE        languageGet(Tk_COLOR_DEEP_PURPLE)
#define T_COLOR_MIDNIGHT_NAVY      languageGet(Tk_COLOR_MIDNIGHT_NAVY)
#define T_COLOR_ARMY_GREEN         languageGet(Tk_COLOR_ARMY_GREEN)
#define T_COLOR_STEEL_BLUE         languageGet(Tk_COLOR_STEEL_BLUE)
#define T_COLOR_AMBER_SUNSET       languageGet(Tk_COLOR_AMBER_SUNSET)
#define T_COLOR_RAINBOW            languageGet(Tk_COLOR_RAINBOW)
