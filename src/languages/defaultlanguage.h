// languages/defaultlanguage.h - default (English) UI strings.
//
// Master list: every (key, English) pair lives here exactly once. This
// single list drives the TextId enum and the fallback table, so key order
// can never drift.
//
// ADDING A LANGUAGE (e.g. German):
//   1. Create languages/de.h with a #define LANG_DE(X) list of the SAME
//      keys in the SAME order, using NULL for strings you haven't
//      translated yet (those fall back to English).
//   2. In language.cpp: #include it, build languageDe[] from LANG_DE,
//      and add it to languageTables[]. Extend LanguageId in language.h.
//   3. Add a Settings entry that calls languageSet(lang) + languageStore(lang).
#pragma once

#define LANG_STRINGS(X) \
    /* ---- App / connection status: ui.c, CommManager.cpp, ScreenHandler.cpp ---- */ \
    X(HEADER, "M5 Remote") \
    X(BLANK, "") \
    X(MENU, "Menu") \
    X(DEMO, "Demo") \
    X(CONNECT, "Connect") \
    X(AUTOCONNECTING, "Auto-connecting...") \
    X(SEARCHING_BLE, "Searching Bluetooth OSSM...") \
    X(FAILED, "Connection failed.. Wait for homeing\nto complete and retry") \
    X(HOMING, "Homing...") \
    X(BLECONNECTED, "Connected via Bluetooth") \
    X(CONNECTED_TO, "M5 connected to ") \
    X(ESPCONNECTED, "Connected via Wifi (ESP-NOW)") \
    X(SETTINGS, "Settings") \
    X(MOTD, "Welcome to OSSM M5 Remote") \
    X(BATT, "Battery") \
    /* ---- Home / motion buttons: ui.c, ScreenHandler.cpp, strokeMode.cpp ---- */ \
    X(HOMEL, "Pullout") \
    X(START, "Start/Stop") \
    X(RESUME, "Start") \
    X(PAUSE, "Pause") \
    X(STOP, "STOP") \
    X(CLOSE, "Close") \
    X(BACK, "Back") \
    X(FAIL, "Failed") \
    X(CUM, "Cum") \
    X(HOME, "Home") \
    X(SPEED, "Speed:") \
    X(DEPTH, "Depth:") \
    X(MM, " mm") \
    X(STROKE, "Stroke:") \
    X(SENSATION, "Sensation:") \
    X(SETUP_DEPTH_I, "Set Depth Interactively") \
    X(SETUP_DEPTH_F, "Set Depth Fancy") \
    X(SELECT_PATTERN, "Select Pattern") \
    X(PATTERN_Button, "Select Pattern \n Menu") \
    X(RESTART, "Restart") \
    X(TURN_OFF, "Turn off") \
    X(OUT_TORQE, "Outward Torque:") \
    X(IN_TORQE, "Inward Torque:") \
    X(EJECT, "cumpump") \
    X(SELECT, "Select") \
    X(OPEN, "Open") \
    X(SAVE, "Save") \
    X(LOW, "Low") \
    X(HIGH, "High") \
    /* ---- Settings toggles: ui.c, ScreenHandler.cpp, SettingsScreen.cpp ---- */ \
    X(HOME_FORCE, "Force re-Home") \
    X(VIBRATE, "Vibrate") \
    X(TOUCHSETTING, "Touch Disabled") \
    X(SAFESTARTSTOP, "Slow Start/Stop") \
    X(STROKEINVERT, "Stroke inverted") \
    X(VISUALSPEEDLOCK, "Natural speed behaviour") \
    X(STROKEDEPTHLINK, "Stroke affects depth") \
    X(SLEEP, "Sleep") \
    X(ESP_NOW, "ESP-NOW") \
    /* ---- Warnings: PowerManagement.cpp ---- */ \
    X(CHARGING_WARNING_TITLE, "Caution - Charging") \
    X(CHARGING_WARNING_TEXT, "When charging, stability issues can occur. \n\nIt is best to not use the M5 remote to control your toys now.") \
    /* ---- Pattern names: BleComm.cpp, ScreenHandler.cpp (multiple places) ---- */ \
    X(Patterns, "Pattern:") \
    X(SimpleStroke, "Simple Stroke") \
    X(TeasingPounding, "Teasing or Pounding") \
    X(RoboStroke, "Robo Stroke") \
    X(HalfnHalf, "Half 'n' Half") \
    X(Deeper, "Deeper") \
    X(StopNGo, "Stop'n'Go") \
    X(Insist, "Insist") \
    X(JackHammer, "Jack Hammer") \
    X(StrokeNibbler, "Stroke Nibbler") \
    X(Knot, "Knot") \
    /* ---- Pullout + Cum pump settings: ScreenHandler.cpp, addons ---- */ \
    X(PULLING_OUT, "Pullout active") \
    X(PULLING_OUT_TEXT, "A pullout has been activated. The OSSM will now safely retract and stop all movement.") \
    X(CUM_SPEED, "Speed") \
    X(CUM_TIME, "Shots") \
    X(CUM_Volume, "Volume") \
    X(CUM_Accel, "Force") \
    X(CUM_LOAD, "Load") \
    X(ROTATION, "Rotation") \
    X(ACCEL, "Accel") \
    /* ---- Menu buttons: ui.c ---- */ \
    X(HOMESCREEN, "Home screen") \
    X(HOMESCREEN_SUB, "(Stroke engine)") \
    X(STREAMING, "Streaming") \
    X(STREAMING_SUB, "mode") \
    X(STROKE_SCREEN, "Bator mode") \
    X(STROKE_SUB, "(stroker)") \
    X(ADDONS, "Addons") \
    X(PATTERN_GENERATOR, "Pattern Generator") \
    X(OVERRIDE, "Override") \
    X(STOP_STREAMING, "Stop streaming") \
    X(CONFIRM, "Confirm") \
    X(NEXT, "Next") \
    X(PREV, "Prev") \
    X(TOYS, "Toys") \
    X(NO_TOYS, "No toys connected") \
    X(SHUTDOWN, "Shutdown") \
    X(CANCEL, "Cancel/back") \
    X(DONE, "Ok-start home") \
    /* ---- Streaming notifications: addonsStreaming.cpp ---- */ \
    X(STREAMING_CAUTION_TITLE, "Connect your OSSM") \
    X(STREAMING_CAUTION_TEXT, "Connect your OSSM to the streaming service now.\nThe OSSM will then start a homing procedure\nMove away from the OSSM for your safety") \
    X(STREAMING_ACTIVE_TITLE, "OSSM in Streaming mode") \
    X(STREAMING_ACTIVE_TEXT, "External application(s) are now in control of OSSM movement.\nMake sure patterns/tools are safe before use!\nYou are responsible for your own safety using streaming mode!") \
    X(STREAMING_RUNNING_TITLE, "OSSM Streaming mode active") \
    X(STREAMING_RUNNING_TEXT, "The M5 is not in control anymore. You can shut down the M5 remote\nYou are responsible for your own safety using streaming mode!") \
    X(STREAMING_FAIL_TITLE, "Streaming failed") \
    X(STREAMING_FAIL_TEXT, "The OSSM could not enter streaming mode.") \
    /* ---- Screen titles: ScreenHandler.cpp, addons (multiple places) ---- */ \
    X(SCREEN_START, "Start") \
    X(SCREEN_STROKE_ENGINE, "OSSM Home") \
    X(SCREEN_MENU, "Menu") \
    X(SCREEN_STREAMING, "Streaming") \
    X(SCREEN_SETTINGS, "Settings") \
    X(SCREEN_ADDONS, "Addons") \
    X(SCREEN_COLORS, "Color Themes") \
    X(SCREEN_PATTERN, "Patterns") \
    X(SCREEN_TORQUE, "Torque") \
    X(SCREEN_EJECT, "EJECT Cum Pump") \
    /* ---- Addons screen: addons.cpp ---- */ \
    X(ADDONS_HINT, "Click addon to assign: Left -> Right -> Off") \
    X(ADDONS_SLOT_LEFT, "Left") \
    X(ADDONS_SLOT_RIGHT, "Right") \
    X(ADDONS_SLOT_OFF, "Off") \
    X(ENABLEDISABLE, "Show/Hide") \
    X(SHOWALL, "Show All") \
    /* ---- Coyote (estim) screen: Coyote.cpp ---- */ \
    X(COYOTE_SCREEN, "Coyote") \
    X(COYOTE_FREQUENCY, "Frequency") \
    X(COYOTE_INTENSITY, "Intensity") \
    X(COYOTE_SENSATION, "Sensation") \
    X(COYOTE_CHANNEL, "Channel(s)") \
    X(COYOTE_SPEED_SENS, "Speed Sens.") \
    X(COYOTE_ACCEL_SENS, "Accel Sens.") \
    X(PAIR, "Pair") \
    /* ---- AP-mode screen: AP-mode.cpp ---- */ \
    X(APMODE, "AP Mode") \
    X(FALLBACK, "Fallback") \
    X(PRESET, "Preset:") \
    X(MOD, "Mod for:") \
    X(MODIFIER, "Modifier") \
    X(PRESETS, "Presets") \
    X(WARNING, "warning label") \
    /* ---- AP-v2 screen: AP-v2.cpp ---- */ \
    X(APV2, "Advanced Penetration v2") \
    X(APV2_SPEED, "Speed") \
    X(APV2_TOP, "Top") \
    X(APV2_BOTTOM, "Bottom") \
    X(APV2_RISE, "Rise") \
    X(APV2_BASE, "Base") \
    X(APV2_VALUE, "Value") \
    X(APV2_MODS, "Mods") \
    /* ---- Disconnect warnings: ScreenHandler.cpp, PowerManagement.cpp ---- */ \
    X(BLE_COMM_ERROR_TITLE, "BLE COMMUNICATION ERROR") \
    X(BLE_COMM_ERROR_TEXT, "The BLE Communication with OSSM has failed.\nPlease restart or turn off your M5 remote.") \
    X(SHUTDOWN_SLEEP_TITLE, "Shutting down") \
    X(SHUTDOWN_SLEEP_TEXT, "The M5 remote is shutting down.\nPress cancel to abort.") \
    X(RECONNECT, "Re-connect") \
    /* ---- Settings screen: rail length / encoder ramp / speed behaviour ---- */ \
    X(SET_RAIL_LENGTH, "Set manual Rail length") \
    X(RAIL_LENGTH_PLAIN, "Set Rail length") \
    X(RAIL_LENGTH_VALUE, "Set Rail length : %.0f") \
    X(ENCODER_RAMP, "Encoder ramp : %s") \
    X(SPEED_BEHAVIOR, "Speed behaviour : %s") \
    X(LANGUAGE, "Language: %s") \
    X(WIFI_SETUP, "WiFi setup") \
    X(RAMP_NONE, "None") \
    X(RAMP_MEDIUM, "Medium") \
    X(RAMP_HIGH, "High") \
    X(RAMP_AGGRESSIVE, "Aggressive") \
    X(BEHAVIOR_STANDARD, "Standard") \
    X(BEHAVIOR_NATURAL, "Natural") \
    X(BEHAVIOR_TAMED, "Tamed") \
    /* ---- Rail length calibration workflow: SettingsScreen.cpp ---- */ \
    X(CALIBRATE_TITLE, "Moving to MAX") \
    X(CALIBRATE_NO_BLE, "BLE is not connected, so calibration cannot start.") \
    X(CALIBRATE_PROMPT, "The OSSM will now move to max depth and store this as a setting. Move away from your OSSM and press start") \
    X(CALIBRATE_CONFIRM_TITLE, "Confirm max depth") \
    X(CALIBRATE_CONFIRM_TEXT, "Confirm if your OSSM is indeed at the maximum position") \
    /* ---- Sleep / restart confirmation dialogs: PowerManagement.cpp ---- */ \
    X(SLEEP_CONFIRM_TITLE, "Enter Deep-Sleep") \
    X(SLEEP_CONFIRM_TEXT, "Are you sure you want to enter deep-sleep mode? This will stop all connections.") \
    X(RESTART_CONFIRM_TEXT, "Are you sure you want to perform a restart?") \
    /* ---- Generic dialog buttons + notification defaults ---- */ \
    X(YES, "Yes") \
    X(NO, "No") \
    X(OK, "OK") \
    X(CANCEL_SHORT, "Cancel") \
    X(RETRY, "Retry") \
    X(NOTIFICATION_TITLE, "Notification") \
    X(LEFT, "Left") \
    X(RIGHT, "Right") \
    /* ---- Addon screen titles: FistIT.cpp, AP-mode.cpp ---- */ \
    X(FISTIT_TITLE, "Fist-IT") \
    X(AP_TITLE, "AP") \
    /* ---- Color scheme names: ColorSchemes.cpp ---- */ \
    X(COLOR_DEEP_PURPLE, "Deep Purple") \
    X(COLOR_MIDNIGHT_NAVY, "Midnight Navy") \
    X(COLOR_ARMY_GREEN, "Army Green") \
    X(COLOR_STEEL_BLUE, "Steel Blue") \
    X(COLOR_AMBER_SUNSET, "Amber Sunset") \
    X(COLOR_RAINBOW, "Rainbow")

