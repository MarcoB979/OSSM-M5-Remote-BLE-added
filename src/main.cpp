#pragma GCC optimize ("Ofast")
#include <M5Unified.h>
#include <ESP32Encoder.h>
#include <esp_now.h>
#include <WiFi.h>
#include <PatternMath.h>
#include "OneButton.h"          //For Button Debounce and Longpress
#include "config.h"
#include <Arduino.h>
#include <cstring>
#include <Wire.h>
#include <lvgl.h>
#include <SPI.h>
#include "ui/ui.h"
#include "ui/ui_helpers.h"
#include "main.h"
#include "Preferences.h"      //EEPROM replacement function
#include "OssmBLE.h"

#include "language.h"
#include <esp_timer.h>
#include <esp_sleep.h>
#include "Xtoys.h"

constexpr int32_t HOR_RES=320;
constexpr int32_t VER_RES=240;

///////////////////////////////////////////
////
////  To Debug or not to Debug
////
///////////////////////////////////////////

// Uncomment the following line if you wish to print DEBUG info
#define DEBUG 

#ifdef DEBUG
#define LogDebug(...) Serial.println(__VA_ARGS__)
#define LogDebugFormatted(...) Serial.printf(__VA_ARGS__)
#else
#define LogDebug(...) ((void)0)
#define LogDebugFormatted(...) ((void)0)
#endif

// Uncomment the following line if you wish to print priority DEBUG info
#define DEBUGPRIO 

#ifdef DEBUGPRIO
#define LogDebugPrio(...) Serial.println(__VA_ARGS__)
#define LogDebugPrioFormatted(...) Serial.printf(__VA_ARGS__)
#else
#define LogDebugPrio(...) ((void)0)
#define LogDebugPrioFormatted(...) ((void)0)
#endif

// Set to 1 to enable Home MX filtering, 0 to disable for newer boards.
#define FilterMX 1

#define OFF 0
#define ON 1


#define ST_UI_START 0
#define ST_UI_HOME 1
#define ST_UI_MENUE 10
//duplicate removed
#define ST_UI_PATTERN 11
#define ST_UI_Torqe 12
#define ST_UI_EJECTSETTINGS 13
#define ST_UI_SETTINGS 20
#define ST_UI_MENU 21
#define ST_UI_STREAMING 22
#define ST_UI_ADDONS 23

// EEPROM replacement function using Non-volatie memory (NVS)
Preferences m5prf; //initiate an instance of the Preferences library


// Global variable for UI to access initial brightness
int g_brightness_value = 180;
int st_screens = ST_UI_START;
// --- Brightness slider event callback ---

// Menü States

#define CONNECT 0
#define HOME 1
#define MENUE 2
#define MENUE2 3
#define TORQE 4
#define PATTERN_MENUE 5
#define PATTERN_MENUE2 6
#define PATTERN_MENUE3 7
#define CUM_MENUE 20

int menuestatus = CONNECT;

bool eject_status = false;
bool dark_mode = false;
bool vibrate_mode = true;
bool strokeinvert_mode = false;
bool touch_home = false;
bool touch_disabled = false;

// Command States
#undef OFF
#undef ON
#undef CONNECT
#define CONN 0
#define SPEED 1
#define DEPTH 2
#define STROKE 3
#define SENSATION 4
#define PATTERN 5
#define TORQE_F 6
#define TORQE_R 7
#define OFF 10
#define ON  11
#define SETUP_D_I 12
#define SETUP_D_I_F 13
#define REBOOT 14

#define CUMSPEED 20
#define CUMTIME 21
#define CUMSIZE   22
#define CUMACCEL  23

#define CONNECT 88
#define HEARTBEAT 99

int displaywidth;
int displayheight;
int progheight = 30;
int distheight = 10;
int S1Pos;
int S2Pos;
int S3Pos;
int S4Pos;
bool rstate = false;
int pattern = 2;
char patternstr[20];
bool onoff = false;


long speedenc = 0;
long depthenc = 0;
long strokeenc = 0;
long sensationenc = 0;
long streamingSpeedEnc = 0;
long streamingDepthEnc = 0;
long streamingStrokeEnc = 0;
long torqe_f_enc = 0;
long torqe_r_enc = 0;
long cum_t_enc = 0;
long cum_si_enc =0;
long cum_s_enc = 0;
long cum_a_enc = 0;
long encoder3_enc = 0;
long encoder4_enc = 0;

float maxdepthinmm = 400.0;
float speedlimit = 600;
int speedscale = -5;

float speed = 0.0;
float depth = 0.0;
float stroke = 0.0;
float sensation = 0.0;
float torqe_f = 100.0;
float torqe_r = -180.0;
float cum_time = 0.0;
float cum_speed = 0.0;
float cum_size = 0.0;
float cum_accel = 0.0;

// Speed to use when unpausing (stored while paused)

// Speed to use when unpausing (stored while paused)
float unpause_speed = 0.0;

// --- Screen Saver Variables ---
unsigned long last_activity_ms = 0;
int screensaver_timeout_ms = 2 * 60 * 1000; // 2 minutes, configurable
int screensaver_dim_brightness = 15; // Value for dimmed screen
int screensaver_prev_brightness = 180; // Default, will be set at startup
bool screensaver_active = false;
unsigned long deep_sleep_timeout_ms = 10 * 60 * 1000; // 10 minutes
static uint32_t ble_home_go_strokeengine_ms = 0;
static uint32_t ossm_state_monitor_hold_until_ms = 0;

unsigned long nowMs;
unsigned long rampMs = 0;
bool rampEnabled = true;
int rampValue = 2;
int rampTime = 75; //75;
int maxRamp = 6; //8;
int encId = 0;
int activeEncId = 0;

bool dynamicStroke = false;

ESP32Encoder encoder1;
ESP32Encoder encoder2;
ESP32Encoder encoder3;
ESP32Encoder encoder4;

// Variable to store if sending data was successful
String success;

float out_esp_speed;
float out_esp_depth;
float out_esp_stroke;
float out_esp_sensation;
float out_esp_pattern;
bool out_esp_rstate;
bool out_esp_connected;
int out_esp_command;
float out_esp_value;
int out_esp_target;

float incoming_esp_speed;
float incoming_esp_depth;
float incoming_esp_stroke;
float incoming_esp_sensation;
float incoming_esp_pattern;
bool incoming_esp_rstate;
bool incoming_esp_connected;
bool incoming_esp_heartbeat;
int incoming_esp_target;

typedef struct struct_message {
  float esp_speed;
  float esp_depth;
  float esp_stroke;
  float esp_sensation;
  float esp_pattern;
  bool esp_rstate;
  bool esp_connected;
  bool esp_heartbeat;
  int esp_command;
  float esp_value;
  int esp_target;
} struct_message;

bool Ossm_paired = false;
bool waiting_for_limits = false; // true after pairing until OSSM reports max depth/speed

struct_message outgoingcontrol;
struct_message incomingcontrol;

esp_now_peer_info_t peerInfo;
uint8_t OSSM_Address[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // Broadcast to all ESP32s, upon connection gets updated to the actual address

#define HEARTBEAT_INTERVAL pdMS_TO_TICKS(5000) // 5 seconds

// Bool

bool EJECT_On = false;

#define state_OFF 0
#define state_ON 1
#define state_PAUSE 2
#define state_STOP 3
#define state_MENU 4
#define state_HOMING 5
#define state_STREAMING 6
#define state_SIMPLE_PENETRATION 7
#define state_STROKE_ENGINE 8
#define state_ERROR 9

#define state_FALSE state_OFF
#define state_TRUE state_ON

int OSSM_State = state_FALSE;

static inline bool isRunningUiState(int state)
{
  return state == state_TRUE || state == state_STREAMING;
}

#define APPLY_HOME_MX_START_STOP_UI() do { \
  if (ui_HomeButtonM != nullptr && ui_HomeButtonMText != nullptr) { \
    if (isRunningUiState(OSSM_State)) { \
      lv_obj_set_style_bg_color(ui_HomeButtonM, lv_color_hex(0xB3261E), LV_PART_MAIN | LV_STATE_DEFAULT); \
      lv_label_set_text(ui_HomeButtonMText, OssmBleIsMode() ? T_PAUSE : T_STOP); \
    } else { \
      lv_obj_set_style_bg_color(ui_HomeButtonM, lv_color_hex(0x228B22), LV_PART_MAIN | LV_STATE_DEFAULT); \
      lv_label_set_text(ui_HomeButtonMText, T_START); \
    } \
  } \
} while (0)

#define APPLY_STREAMING_MX_START_STOP_UI() do { \
  if (ui_StreamingButtonM != nullptr && ui_StreamingButtonMText != nullptr) { \
    if (isRunningUiState(OSSM_State)) { \
      lv_obj_set_style_bg_color(ui_StreamingButtonM, lv_color_hex(0xB3261E), LV_PART_MAIN | LV_STATE_DEFAULT); \
      lv_label_set_text(ui_StreamingButtonMText, OssmBleIsMode() ? T_PAUSE : T_STOP); \
    } else { \
      lv_obj_set_style_bg_color(ui_StreamingButtonM, lv_color_hex(0x228B22), LV_PART_MAIN | LV_STATE_DEFAULT); \
      lv_label_set_text(ui_StreamingButtonMText, T_START); \
    } \
  } \
} while (0)

#define APPLY_HOME_AND_STREAMING_MX_START_STOP_UI() do { \
  APPLY_HOME_MX_START_STOP_UI(); \
  APPLY_STREAMING_MX_START_STOP_UI(); \
} while (0)

// Tasks:

TaskHandle_t eRemote_t  = nullptr;  // Esp Now Remote Task

void espNowRemoteTask(void *pvParameters); // Handels the EspNow Remote
bool connectbtn(); //Handels Connectbtn
int64_t touchmenue();
bool SendCommand(int Command, float Value, int Target);

static void sendPairingHeartbeat()
{
  outgoingcontrol.esp_command = HEARTBEAT;
  outgoingcontrol.esp_heartbeat = true;
  outgoingcontrol.esp_target = OSSM_ID;
  esp_now_send(OSSM_Address, (uint8_t *) &outgoingcontrol, sizeof(outgoingcontrol));
}

static void waitForPairingOrTimeout(uint32_t timeoutMs, uint32_t heartbeatIntervalMs)
{
  uint32_t waitStartMs = millis();
  uint32_t lastHeartbeatMs = 0;

  while (!Ossm_paired && (millis() - waitStartMs) < timeoutMs) {
    if ((millis() - lastHeartbeatMs) >= heartbeatIntervalMs) {
      sendPairingHeartbeat();
      lastHeartbeatMs = millis();
    }

    lv_task_handler();
    M5.update();
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

static void syncBleConnectUi(bool forceRefresh)
{
  (void)forceRefresh;
  if (OssmBleConnected()) {
    lv_scr_load_anim(ui_Home, LV_SCR_LOAD_ANIM_FADE_ON,20,0,false);
    return;
  }
}

static const char* ossmStateToText(int state)
{
  switch (state) {
    case state_OFF: return "OFF";
    case state_ON: return "ON";
    case state_PAUSE: return "PAUSE";
    case state_STOP: return "STOP";
    case state_MENU: return "MENU";
    case state_HOMING: return "HOMING";
    case state_STREAMING: return "STREAM";
    case state_SIMPLE_PENETRATION: return "SIMPLE";
    case state_STROKE_ENGINE: return "STROKE";
    case state_ERROR: return "ERROR";
    default: return "UNKNOWN";
  }
}

static void updateHomeTopLeftStateLabel()
{
  if (ui_connect == nullptr) {
    return;
  }

  char labelText[32];
  if (OssmBleIsMode()) {
    if (OssmBleConnected()) {
      snprintf(labelText, sizeof(labelText), "%s", ossmStateToText(OSSM_State));
    } else {
      snprintf(labelText, sizeof(labelText), "%s", T_CONNECTING);
    }
  } else {
    snprintf(labelText, sizeof(labelText), "%s", ossmStateToText(OSSM_State));
  }

  lv_label_set_text(ui_connect, labelText);
}

static void monitorOssmState(bool forceBlePoll = false)
{
  static uint32_t lastBlePollMs = 0;
  const uint32_t now = millis();
  int nextState = OSSM_State;

  if (OssmBleIsMode()) {
    if (!forceBlePoll && now < ossm_state_monitor_hold_until_ms) {
      APPLY_HOME_AND_STREAMING_MX_START_STOP_UI();
      updateHomeTopLeftStateLabel();
      return;
    }

    // While OSSM is confirmed streaming, back off polling to 3 s.
    // Frequent blocking BLE reads compete with external stream frames (e.g.
    // X-Toys) on the same connection and cause motor stalls.
    const uint32_t pollInterval = (OSSM_State == state_STREAMING) ? 3000U : 300U;
    bool shouldPoll = forceBlePoll || ((now - lastBlePollMs) >= pollInterval);
    if (shouldPoll) {
      lastBlePollMs = now;
      OssmBleMachineState bleState;
      if (OssmBleGetCurrentState(&bleState, true)) {
        switch (bleState.mode) {
          case OssmBleMachineMode::HomingForward:
          case OssmBleMachineMode::HomingBackward:
            nextState = state_HOMING;
            break;
          case OssmBleMachineMode::Menu:
            nextState = state_MENU;
            break;
          case OssmBleMachineMode::Streaming:
            nextState = state_STREAMING;
            break;
          case OssmBleMachineMode::SimplePenetration:
            nextState = state_SIMPLE_PENETRATION;
            break;
          case OssmBleMachineMode::StrokeEngineActive:
            nextState = (bleState.speed > 0) ? state_ON : state_PAUSE;
            break;
          case OssmBleMachineMode::StrokeEngineIdle:
            nextState = (bleState.speed > 0) ? state_ON : state_PAUSE;
            break;
          default:
            break;
        }
      }
    }
  }

  if (st_screens == ST_UI_MENU && nextState != state_HOMING && nextState != state_ERROR) {
    nextState = state_MENU;
  }

  if (nextState != OSSM_State) {
    OSSM_State = nextState;
    LogDebugFormatted("OSSM_State -> %d\n", OSSM_State);
  }

  APPLY_HOME_AND_STREAMING_MX_START_STOP_UI();
  updateHomeTopLeftStateLabel();
}

// Makes vibration motor go Brrrrr
void vibrate(int vbr_Intensity = 200, int vbr_Length = 100){
    if(lv_obj_has_state(ui_vibrate, LV_STATE_CHECKED) == 1){
      M5.Power.setVibration(vbr_Intensity);
      vTaskDelay(pdMS_TO_TICKS(vbr_Length));
      M5.Power.setVibration(0);
    }
}

void brightness_slider_event_cb(lv_event_t * e)
{
  if(lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
    int val = lv_slider_get_value(ui_brightness_slider);
    M5.Display.setBrightness(val);
    m5prf.putInt("Brightness", val); // save to NVS
  }
}

// Forward declaration for mxclick
void mxclick();
void screensaver_check_activity();
bool mxclick_short_waspressed = false;
void mxlong();
bool mxclick_long_waspressed = false;
void mxdouble();
bool mxclick_double_waspressed = false;

void clickLeft();
bool clickLeft_short_waspressed = false;
void clickLeftDouble();
bool clickLeft_double_waspressed = false;
void clickRight();
bool clickRight_short_waspressed = false;
void clickRightLong();
bool clickRight_long_waspressed = false;
void clickRightDouble();
bool clickRight_double_waspressed = false;

// Keep this configurable for future non-blocking interaction modes.
static bool g_notification_blocks_inputs = true;
static bool g_streaming_entry_flow_pending = false;
static bool g_streaming_controls_locked = false;
static void streamingbuttonm_action(bool fromPhysicalMx);
static void streamingStartOnlyAction();
static int g_addon_slots[ADDON_DEFINITIONS_COUNT] = {ADDON_SLOT_NONE};
static bool g_addons_loaded_from_nvs = false;

static void refreshAddonsUi();
static void applyHomeDefaultsForModeChange();
static void updateScreenTitleLabels();

// Home-only MX anti-ghost filter: ignore very fast repeated physical MX clicks.
static unsigned long mx_last_home_action_ms = 0;
static constexpr unsigned long MX_HOME_MIN_GAP_MS = 220;
static unsigned long mx_suppress_until_ms = 0;
static constexpr unsigned long MX_SUPPRESS_AFTER_ENCODER_MS = 300;
static unsigned long mx_release_since_ms = 0;

static void updateMxReleaseStability()
{
  // Core2 mapping uses idle LOW, pressed HIGH for MX.
  bool mxReleased = (digitalRead(Button1.pin()) == LOW);
  if (mxReleased) {
    if (mx_release_since_ms == 0) {
      mx_release_since_ms = millis();
    }
  } else {
    mx_release_since_ms = 0;
  }
}

static volatile int g_notification_touch_result = NOTIFICATION_RESULT_NONE;

static void notification_left_button_cb(lv_event_t *e)
{
  (void)e;
  g_notification_touch_result = NOTIFICATION_RESULT_LEFT;
}

static void notification_right_button_cb(lv_event_t *e)
{
  (void)e;
  g_notification_touch_result = NOTIFICATION_RESULT_RIGHT;
}

int showNotification(const char *title,
                     const char *text,
                     uint32_t duration,
                     bool showLeftButton,
                     const char *leftButtonText,
                     bool showRightButton,
                     const char *rightButtonText,
                     bool showFullScreen)
{
  const bool hasButtons = showLeftButton || showRightButton;
  const bool prevTouchDisabled = touch_disabled;
  const bool shouldBlockTouch = g_notification_blocks_inputs && !hasButtons;
  const uint32_t startMs = millis();
  int result = NOTIFICATION_RESULT_NONE;
  g_notification_touch_result = NOTIFICATION_RESULT_NONE;

  if (shouldBlockTouch) {
    touch_disabled = true;
  }

  // Drain stale button states before opening the modal.
  mxclick_short_waspressed = false;
  mxclick_long_waspressed = false;
  mxclick_double_waspressed = false;
  clickLeft_short_waspressed = false;
  clickRight_short_waspressed = false;
  clickRight_long_waspressed = false;
  clickRight_double_waspressed = false;

  lv_obj_t *overlay = lv_obj_create(lv_layer_top());
  lv_obj_remove_style_all(overlay);
  lv_obj_set_size(overlay, HOR_RES, VER_RES);
  lv_obj_center(overlay);
  lv_obj_set_style_bg_opa(overlay, LV_OPA_50, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(overlay, lv_color_hex(0x5B0353), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_flag(overlay, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *panel = lv_obj_create(overlay);
  if (showFullScreen) {
    const int topOffset = 32;
    const int bottomPadding = 5;
    lv_obj_set_size(panel, 310, VER_RES - topOffset - bottomPadding);
    lv_obj_set_pos(panel, 5, topOffset);
  } else {
    lv_obj_set_size(panel, (HOR_RES * 90) / 100, (VER_RES * 75) / 100);
    lv_obj_align(panel, LV_ALIGN_CENTER, 0, 5);
  }
  lv_obj_set_style_radius(panel, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(panel, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(panel, lv_color_hex(0x83277B), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(panel, lv_color_hex(0xD691D0), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

  lv_obj_t *titleBar = lv_obj_create(panel);
  lv_obj_remove_style_all(titleBar);
  lv_obj_set_size(titleBar, lv_pct(100), 32);
  lv_obj_align(titleBar, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_set_style_bg_opa(titleBar, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(titleBar, lv_color_hex(0x5B0353), LV_PART_MAIN | LV_STATE_DEFAULT);

  lv_obj_t *titleLabel = lv_label_create(titleBar);
  lv_label_set_text(titleLabel, (title != nullptr && title[0] != '\0') ? title : "Notification");
  lv_obj_set_style_text_color(titleLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(titleLabel, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_center(titleLabel);

  lv_obj_t *bodyLabel = lv_label_create(panel);
  lv_obj_set_width(bodyLabel, lv_pct(90));
  lv_label_set_long_mode(bodyLabel, LV_LABEL_LONG_WRAP);
  lv_label_set_text(bodyLabel, (text != nullptr) ? text : "");
  lv_obj_set_style_text_color(bodyLabel, lv_color_hex(0x2E0A2B), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(bodyLabel, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_align(bodyLabel, LV_ALIGN_TOP_MID, 0, 48);

  if (hasButtons) {
    lv_obj_t *buttonRow = lv_obj_create(panel);
    lv_obj_remove_style_all(buttonRow);
    lv_obj_set_size(buttonRow, lv_pct(94), 44);
    lv_obj_align(buttonRow, LV_ALIGN_BOTTOM_MID, 0, -12);
    lv_obj_set_flex_flow(buttonRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(buttonRow, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    if (showLeftButton) {
      lv_obj_t *leftBtn = lv_btn_create(buttonRow);
      lv_obj_set_size(leftBtn, 120, 36);
      lv_obj_set_style_bg_color(leftBtn, lv_color_hex(0x83277B), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_bg_color(leftBtn, lv_color_hex(0x5B0353), LV_PART_MAIN | LV_STATE_PRESSED);
      lv_obj_set_style_border_width(leftBtn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

      lv_obj_t *leftLbl = lv_label_create(leftBtn);
      lv_label_set_text(leftLbl, (leftButtonText != nullptr && leftButtonText[0] != '\0') ? leftButtonText : "Left");
      lv_obj_set_style_text_color(leftLbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_center(leftLbl);
      lv_obj_add_event_cb(leftBtn, notification_left_button_cb, LV_EVENT_CLICKED, nullptr);
    }

    if (showRightButton) {
      lv_obj_t *rightBtn = lv_btn_create(buttonRow);
      lv_obj_set_size(rightBtn, 120, 36);
      lv_obj_set_style_bg_color(rightBtn, lv_color_hex(0x83277B), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_bg_color(rightBtn, lv_color_hex(0x5B0353), LV_PART_MAIN | LV_STATE_PRESSED);
      lv_obj_set_style_border_width(rightBtn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

      lv_obj_t *rightLbl = lv_label_create(rightBtn);
      lv_label_set_text(rightLbl, (rightButtonText != nullptr && rightButtonText[0] != '\0') ? rightButtonText : "Right");
      lv_obj_set_style_text_color(rightLbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_center(rightLbl);
      lv_obj_add_event_cb(rightBtn, notification_right_button_cb, LV_EVENT_CLICKED, nullptr);
    }
  }

  static uint32_t notif_last_state_log_ms = 0;

  while (true) {
    M5.update();
    lv_task_handler();
    updateMxReleaseStability();
    Button1.tick();
    Button2.tick();
    Button3.tick();

    // Periodic BLE state log inside modal (every 2 s) so streaming activity
    // is visible on serial even while notification blocks the main loop.
    if (OssmBleIsMode() && (millis() - notif_last_state_log_ms) >= 2000U) {
      notif_last_state_log_ms = millis();
      String stateText;
      OssmBleReadState(&stateText, true);
    }

    if (duration > 0 && (millis() - startMs) >= duration) {
      result = NOTIFICATION_RESULT_NONE;
      break;
    }

    if (hasButtons) {
      if (g_notification_touch_result != NOTIFICATION_RESULT_NONE) {
        result = g_notification_touch_result;
        break;
      }
      if (showLeftButton && clickLeft_short_waspressed) {
        result = NOTIFICATION_RESULT_LEFT;
        break;
      }
      if (showRightButton && clickRight_short_waspressed) {
        result = NOTIFICATION_RESULT_RIGHT;
        break;
      }
    }

    // Consume all activity so the current screen never receives latent button events.
    mxclick_short_waspressed = false;
    mxclick_long_waspressed = false;
    mxclick_double_waspressed = false;
    clickLeft_short_waspressed = false;
    clickRight_short_waspressed = false;
    clickRight_long_waspressed = false;
    clickRight_double_waspressed = false;

    vTaskDelay(pdMS_TO_TICKS(5));
  }

  lv_obj_del(overlay);

  mxclick_short_waspressed = false;
  mxclick_long_waspressed = false;
  mxclick_double_waspressed = false;
  clickLeft_short_waspressed = false;
  clickRight_short_waspressed = false;
  clickRight_long_waspressed = false;
  clickRight_double_waspressed = false;

  if (shouldBlockTouch) {
    touch_disabled = prevTouchDisabled;
  }

  return result;
}

extern "C" void requestStreamingEntryFlow(void)
{
  g_streaming_entry_flow_pending = true;
}

extern "C" void requestMenuEntryAction(void)
{
  if (OssmBleIsMode()) {
    OssmBleGoToMenu();
  }
}

static const char* addonSlotLabel(int slot)
{
  switch (slot) {
    case ADDON_SLOT_LEFT: return T_ADDONS_SLOT_LEFT;
    case ADDON_SLOT_RIGHT: return T_ADDONS_SLOT_RIGHT;
    default: return T_ADDONS_SLOT_OFF;
  }
}

static void loadAddonBindingsIfNeeded()
{
  if (g_addons_loaded_from_nvs) {
    return;
  }

  Preferences pref;
  pref.begin("m5-ctnr", true);
  for (size_t i = 0; i < ADDON_DEFINITIONS_COUNT; ++i) {
    char key[24];
    snprintf(key, sizeof(key), "AddonSlot%u", (unsigned)i);
    int slot = pref.getInt(key, ADDON_SLOT_NONE);
    if (slot != ADDON_SLOT_LEFT && slot != ADDON_SLOT_RIGHT) {
      slot = ADDON_SLOT_NONE;
    }
    g_addon_slots[i] = slot;
  }
  pref.end();

  // Enforce unique left/right bindings after loading potentially stale settings.
  int leftOwner = -1;
  int rightOwner = -1;
  for (size_t i = 0; i < ADDON_DEFINITIONS_COUNT; ++i) {
    if (g_addon_slots[i] == ADDON_SLOT_LEFT) {
      if (leftOwner >= 0) {
        g_addon_slots[i] = ADDON_SLOT_NONE;
      } else {
        leftOwner = (int)i;
      }
    } else if (g_addon_slots[i] == ADDON_SLOT_RIGHT) {
      if (rightOwner >= 0) {
        g_addon_slots[i] = ADDON_SLOT_NONE;
      } else {
        rightOwner = (int)i;
      }
    }
  }

  g_addons_loaded_from_nvs = true;
}

static void saveAddonBindings()
{
  Preferences pref;
  pref.begin("m5-ctnr", false);
  for (size_t i = 0; i < ADDON_DEFINITIONS_COUNT; ++i) {
    char key[24];
    snprintf(key, sizeof(key), "AddonSlot%u", (unsigned)i);
    pref.putInt(key, g_addon_slots[i]);
  }
  pref.end();
}

static void refreshAddonsUi()
{
  if (ui_AddonsItem0Text != nullptr && ADDON_DEFINITIONS_COUNT > 0) {
    char line[96];
    snprintf(line, sizeof(line), "%s  [%s]", ADDON_DEFINITIONS[0].displayName, addonSlotLabel(g_addon_slots[0]));
    lv_label_set_text(ui_AddonsItem0Text, line);
  }
  if (ui_AddonsItem1Text != nullptr && ADDON_DEFINITIONS_COUNT > 1) {
    char line[96];
    snprintf(line, sizeof(line), "%s  [%s]", ADDON_DEFINITIONS[1].displayName, addonSlotLabel(g_addon_slots[1]));
    lv_label_set_text(ui_AddonsItem1Text, line);
  }
}

static void cycleAddonSelection(size_t addonIndex)
{
  if (addonIndex >= ADDON_DEFINITIONS_COUNT) {
    return;
  }

  int current = g_addon_slots[addonIndex];
  int next = ADDON_SLOT_NONE;
  if (current == ADDON_SLOT_NONE) {
    next = ADDON_SLOT_LEFT;
  } else if (current == ADDON_SLOT_LEFT) {
    next = ADDON_SLOT_RIGHT;
  } else {
    next = ADDON_SLOT_NONE;
  }

  if (next == ADDON_SLOT_LEFT || next == ADDON_SLOT_RIGHT) {
    for (size_t i = 0; i < ADDON_DEFINITIONS_COUNT; ++i) {
      if (i != addonIndex && g_addon_slots[i] == next) {
        g_addon_slots[i] = ADDON_SLOT_NONE;
      }
    }
  }

  g_addon_slots[addonIndex] = next;
  saveAddonBindings();
}

extern "C" void addonsScreenLoaded(void)
{
  loadAddonBindingsIfNeeded();
  refreshAddonsUi();
}

extern "C" void addonsSelectIndex(int index)
{
  loadAddonBindingsIfNeeded();
  if (index < 0) {
    return;
  }
  cycleAddonSelection((size_t)index);
  refreshAddonsUi();
}

static void updateScreenTitleLabels()
{
  if (ui_Logo != nullptr) lv_label_set_text(ui_Logo, T_SCREEN_START);
  if (ui_Logo2 != nullptr) lv_label_set_text(ui_Logo2, T_SCREEN_STROKE_ENGINE);
//  if (ui_Logo3 != nullptr) lv_label_set_text(ui_Logo3, T_SCREEN_MENU);
  if (ui_Logo1 != nullptr) lv_label_set_text(ui_Logo1, T_SCREEN_SETTINGS);
  if (ui_Logo4 != nullptr) lv_label_set_text(ui_Logo4, T_SCREEN_TORQUE);
  if (ui_Logo5 != nullptr) lv_label_set_text(ui_Logo5, T_SCREEN_PATTERN);
  if (ui_Logo6 != nullptr) lv_label_set_text(ui_Logo6, T_SCREEN_EJECT);
  if (ui_LogoMenu != nullptr) lv_label_set_text(ui_LogoMenu, T_SCREEN_MENU);
  if (ui_LogoStreaming != nullptr) lv_label_set_text(ui_LogoStreaming, T_SCREEN_STREAMING);
  if (ui_LogoAddons != nullptr) lv_label_set_text(ui_LogoAddons, T_SCREEN_ADDONS);
}

static void applyHomeDefaultsForModeChange()
{
  speed = 0.0f;
  depth = 0.0f;
  stroke = 0.0f;
  sensation = 50.0f;

  if (ui_homespeedslider != nullptr) lv_slider_set_value(ui_homespeedslider, 0, LV_ANIM_OFF);
  if (ui_homedepthslider != nullptr) lv_slider_set_value(ui_homedepthslider, 0, LV_ANIM_OFF);
  if (ui_homestrokeslider != nullptr) lv_slider_set_value(ui_homestrokeslider, 0, LV_ANIM_OFF);
  if (ui_homesensationslider != nullptr) lv_slider_set_value(ui_homesensationslider, 50, LV_ANIM_OFF);
  if (ui_homespeedvalue != nullptr) lv_label_set_text(ui_homespeedvalue, "0");
  if (ui_homedepthvalue != nullptr) lv_label_set_text(ui_homedepthvalue, "0");
  if (ui_homestrokevalue != nullptr) lv_label_set_text(ui_homestrokevalue, "0");

  SendCommand(SPEED, 0, OSSM_ID);
  SendCommand(DEPTH, 0, OSSM_ID);
  SendCommand(STROKE, 0, OSSM_ID);
  SendCommand(SENSATION, 50, OSSM_ID);
}

extern "C" void streamingReturnToMenu(void)
{
  g_streaming_controls_locked = false;
  if (OssmBleIsMode()) {
    OssmBleGoToMenu();
  }
  _ui_screen_change(ui_Menu, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
}

static void prepareStreamingAtFullScale()
{
  speed = 100;
  depth = 100;
  stroke = 100;

  lv_slider_set_value(ui_streamingspeedslider, 100, LV_ANIM_OFF);
  lv_slider_set_value(ui_streamingdepthslider, 100, LV_ANIM_OFF);
  lv_slider_set_value(ui_streamingstrokeslider, 100, LV_ANIM_OFF);
  lv_label_set_text(ui_streamingspeedvalue, "100");
  lv_label_set_text(ui_streamingdepthvalue, "100");
  lv_label_set_text(ui_streamingstrokevalue, "100");

  SendCommand(SPEED, 100, OSSM_ID);
  SendCommand(DEPTH, 100, OSSM_ID);
  SendCommand(STROKE, 100, OSSM_ID);
}

void handleStreamingEntryFlow()
{
  g_streaming_entry_flow_pending = false;
  g_streaming_controls_locked = true;

  showNotification(T_STREAMING_CAUTION_TITLE,
                   T_STREAMING_CAUTION_TEXT,
                   5000,
                   false,
                   nullptr,
                   false,
                   nullptr,
                   true);

  if (OssmBleIsMode()) {
    if (!OssmBleGoToStreaming()) {
      showNotification(T_STREAMING_FAIL_TITLE,
                       T_STREAMING_FAIL_TEXT,
                       2500,
                       false,
                       nullptr,
                       false,
                       nullptr,
                       true);
      streamingReturnToMenu();
      return;
    }

    vTaskDelay(pdMS_TO_TICKS(200));

    // Set buffer to 0: OSSM acts on each stream:pos:time frame immediately,
    // with no pre-buffering delay. Required for low-latency external stream sources.
    OssmBleSetBuffer(0.0f);
  }

  prepareStreamingAtFullScale();

  while (true) {
    int result = showNotification(T_STREAMING_ACTIVE_TITLE,
                                  T_STREAMING_ACTIVE_TEXT,
                                  0,
                                  true,
                                  T_STOP_STREAMING,
                                  true,
                                  T_START,
                                  true);

    if (result == NOTIFICATION_RESULT_LEFT) {
      streamingReturnToMenu();
      return;
    }

    if (result == NOTIFICATION_RESULT_RIGHT) {
      streamingStartOnlyAction();
      continue;
    }
  }
}

static void markEncoderActivityForMxFilter()
{
  if (FilterMX == 1) {
    mx_suppress_until_ms = millis() + MX_SUPPRESS_AFTER_ENCODER_MS;
  }
}

static bool filterMxPhysicalHomeClick()
{
  if (FilterMX != 1) {
    return true;
  }

  unsigned long now = millis();
  unsigned long dt = now - mx_last_home_action_ms;

  if ((long)(now - mx_suppress_until_ms) < 0) {
    LogDebugPrioFormatted("mx: ST_UI_HOME -> filtered after encoder move (%ld ms left)\n", (long)(mx_suppress_until_ms - now));
    return false;
  }

  if (dt < MX_HOME_MIN_GAP_MS) {
    LogDebugPrioFormatted("mx: ST_UI_HOME -> filtered phantom click (gap=%lu ms)\n", dt);
    return false;
  }

  mx_last_home_action_ms = now;
  return true;
}

static int getRampedDetentDelta(int encoderId, int detents)
{
  if (detents == 0) return 0;
  if (!rampEnabled) {
    rampValue = 1;
    activeEncId = encoderId;
    rampMs = millis();
    return detents;
  }
  unsigned long now = millis();
  bool sameEncoder = (encoderId == activeEncId);
  bool withinRampWindow = ((now - rampMs) <= (unsigned long)rampTime);
  if (!sameEncoder || !withinRampWindow) {
    rampValue = 1;
  }
  int sign = (detents > 0) ? 1 : -1;
  int steps = abs(detents);
  int delta = 0;
  for (int i = 0; i < steps; ++i) {
    delta += sign * rampValue;
    if (rampValue < maxRamp) ++rampValue;
  }
  activeEncId = encoderId;
  rampMs = now;
  return delta;
}

lv_display_t *display;
lv_indev_t *indev;

static lv_draw_buf_t *draw_buf1;
static lv_draw_buf_t *draw_buf2;

// Display flushing
void my_display_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  lv_draw_sw_rgb565_swap(px_map, w*h);
  M5.Display.pushImageDMA<uint16_t>(area->x1, area->y1, w, h, (uint16_t *)px_map);
  lv_disp_flush_ready(disp);
}

uint32_t my_tick_function() {
  return (esp_timer_get_time() / 1000LL);
}

void screensaver_check_activity() {
  // Call this in every input handler (button, encoder, touch, etc.)
  last_activity_ms = millis();
  if (screensaver_active) {
    M5.Lcd.setBrightness(screensaver_prev_brightness);
    screensaver_active = false;
  }
}

static bool canEnterDeepSleep()
{
  // Allow deep sleep only when the OSSM motor is not actively moving.
  // PAUSE, STOP, and MENU are safe — the motor is stationary.
  // In BLE mode the monitor keeps OSSM_State at state_PAUSE (never state_OFF)
  // when the machine is idle, so we must allow those states too.
  switch (OSSM_State) {
    case state_OFF:
    case state_PAUSE:
    case state_STOP:
    case state_MENU:
      return true;
    default:
      return false;
  }
}

static void enterDeepSleep()
{
  gpio_num_t mxPin = static_cast<gpio_num_t>(Button1.pin());
  gpio_num_t leftPin = static_cast<gpio_num_t>(Button2.pin());
  gpio_num_t rightPin = static_cast<gpio_num_t>(Button3.pin());
  uint64_t wakeMask = (1ULL << mxPin) | (1ULL << leftPin) | (1ULL << rightPin);

  LogDebug("Entering deep sleep (wake on MX/left/right)");
  M5.Display.setBrightness(0);
  M5.Power.setVibration(0);

  // Buttons are active-high on this hardware mapping.
  esp_sleep_enable_ext1_wakeup(wakeMask, ESP_EXT1_WAKEUP_ANY_HIGH);
  delay(50);
  esp_deep_sleep_start();
}

void my_touchpad_read(lv_indev_t * drv, lv_indev_data_t * data) {
  M5.update();
  auto count = M5.Touch.getCount();

  if(touch_disabled != true){
    if ( count == 0 ) {
      data->state = LV_INDEV_STATE_RELEASED;
    } else {
      screensaver_check_activity();
      auto touch = M5.Touch.getDetail(0);
      data->state = LV_INDEV_STATE_PRESSED; 
      data->point.x = touch.x;
      data->point.y = touch.y;
    }
}
}

static void event_cb(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *target = reinterpret_cast<lv_obj_t *>(lv_event_get_target(e));
  lv_obj_t *label = reinterpret_cast<lv_obj_t *>(lv_event_get_user_data(e));
  const char *eventName = nullptr;
  const char *buttonName = "unknown";

  if (target == ui_StartButtonL || target == ui_HomeButtonL || target == ui_MenuButtonL ||
      target == ui_PatternButtonL || target == ui_TorqeButtonL || target == ui_EJECTButtonL ||
      target == ui_SettingsButtonL) {
    buttonName = "left";
  } else if (target == ui_StartButtonM || target == ui_HomeButtonM || target == ui_MenuButtonM ||
             target == ui_PatternButtonM || target == ui_TorqeButtonM || target == ui_EJECTButtonM ||
             target == ui_SettingsButtonM) {
    buttonName = "mx";
  } else if (target == ui_StartButtonR || target == ui_HomeButtonR || target == ui_MenuButtonR ||
             target == ui_PatternButtonR || target == ui_TorqeButtonR || target == ui_EJECTButtonR ||
             target == ui_SettingsButtonR) {
    buttonName = "right";
  }

  switch (code)
  {
  case LV_EVENT_PRESSED:
    eventName = "LV_EVENT_PRESSED";
    break;
  case LV_EVENT_RELEASED:
    eventName = "LV_EVENT_RELEASED";
    break;
  case LV_EVENT_SHORT_CLICKED:
    eventName = "LV_EVENT_SHORT_CLICKED";
    break;
  case LV_EVENT_CLICKED:
    eventName = "LV_EVENT_CLICKED";
    break;
  case LV_EVENT_LONG_PRESSED:
    eventName = "LV_EVENT_LONG_PRESSED";
    break;
  case LV_EVENT_LONG_PRESSED_REPEAT:
    eventName = "LV_EVENT_LONG_PRESSED_REPEAT";
    break;
  default:
    break;
  }

  if (eventName != nullptr) {
    if (label != nullptr) {
      lv_label_set_text_fmt(label, "The last button event:\n%s %s", eventName, buttonName);
    }
    LogDebugPrioFormatted("The last button event: %s %s\n", eventName, buttonName);
  }
}

static void register_event_debug_callbacks()
{
  lv_obj_t *debugButtons[] = {
    ui_StartButtonL, ui_StartButtonM, ui_StartButtonR,
    ui_HomeButtonL, ui_HomeButtonM, ui_HomeButtonR,

    ui_PatternButtonL, ui_PatternButtonM, ui_PatternButtonR,
    ui_TorqeButtonL, ui_TorqeButtonM, ui_TorqeButtonR,
    ui_EJECTButtonL, ui_EJECTButtonM, ui_EJECTButtonR,
    ui_SettingsButtonL, ui_SettingsButtonM, ui_SettingsButtonR,
  };

  for (lv_obj_t *obj : debugButtons) {
    if (obj != nullptr) {
      lv_obj_add_event_cb(obj, event_cb, LV_EVENT_ALL, nullptr);
    }
  }
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
//  LogDebug("Last Packet Send Status: ");
//  LogDebug(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
//  LogDebugFormatted("Speed: %f, Depth: %f, Stroke: %f, Command: %d, Value: %f, Target: %d\n",
//    outgoingcontrol.esp_speed, outgoingcontrol.esp_depth, outgoingcontrol.esp_stroke,  outgoingcontrol.esp_command, outgoingcontrol.esp_value, outgoingcontrol.esp_target);
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&incomingcontrol, incomingData, sizeof(incomingcontrol));
  LogDebug("Received ESP-NOW data");
  LogDebugPrioFormatted(
    "ESP-NOW rx: from=%02X:%02X:%02X:%02X:%02X:%02X len=%d target=%d cmd=%d hb=%d paired=%d bleMode=%d speed=%.1f depth=%.1f\n",
    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
    len,
    incomingcontrol.esp_target,
    incomingcontrol.esp_command,
    incomingcontrol.esp_heartbeat ? 1 : 0,
    Ossm_paired ? 1 : 0,
    OssmBleIsMode() ? 1 : 0,
    incomingcontrol.esp_speed,
    incomingcontrol.esp_depth);
//  LogDebugFormatted("Speed: %f, Depth: %f, Stroke: %f, Sensation: %f, Pattern: %f, RState: %d, Connected: %d, Heartbeat: %d, Command: %d, Value: %f, Target: %d\n",
//    incomingcontrol.esp_speed, incomingcontrol.esp_depth, incomingcontrol.esp_stroke, incomingcontrol.esp_sensation, incomingcontrol.esp_pattern, incomingcontrol.esp_rstate, incomingcontrol.esp_connected, incomingcontrol.esp_heartbeat, incomingcontrol.esp_command, incomingcontrol.esp_value, incomingcontrol.esp_target);


  
  if(incomingcontrol.esp_target == M5_ID && Ossm_paired == false){

    // Remove the existing peer (0xFF:0xFF:0xFF:0xFF:0xFF:0xFF)
    esp_err_t result = esp_now_del_peer(peerInfo.peer_addr);

    if (result == ESP_OK) {

      memcpy(OSSM_Address, mac, 6); //get the mac address of the sender
      
      // Add the new peer
      memcpy(peerInfo.peer_addr, OSSM_Address, 6);
      if (esp_now_add_peer(&peerInfo) == ESP_OK) {
        LogDebugFormatted("New peer added successfully, OSSM addresss : %02X:%02X:%02X:%02X:%02X:%02X\n", OSSM_Address[0], OSSM_Address[1], OSSM_Address[2], OSSM_Address[3], OSSM_Address[4], OSSM_Address[5]);
        Ossm_paired = true;
        lv_label_set_text(ui_Welcome, T_ESPCONNECTED);
      }
      else {
        LogDebug("Failed to add new peer");
      }
    }
    else {
      LogDebug("Failed to remove peer");
    }

    
    if (incomingcontrol.esp_speed > 600) {
      speedlimit = 600;
    } else {
      speedlimit = incomingcontrol.esp_speed;
    }
    LogDebug(speedlimit);
    maxdepthinmm = incomingcontrol.esp_depth;
    LogDebug(maxdepthinmm);
    pattern = incomingcontrol.esp_pattern;
    LogDebug(pattern);
    outgoingcontrol.esp_target = OSSM_ID;

    result = esp_now_send(OSSM_Address, (uint8_t *) &outgoingcontrol, sizeof(outgoingcontrol));
    LogDebug(result);

    if (result == ESP_OK) {
      Ossm_paired = true;
      // If the OSSM already reported max speed and depth in this packet,
      // go straight to Home. Otherwise remain on the welcome/start screen
      // and wait for post-homing updates.
      if (incomingcontrol.esp_speed > 0 && incomingcontrol.esp_depth > 0) {
        lv_label_set_text(ui_connect, "WIFI");
        lv_scr_load_anim(ui_Home, LV_SCR_LOAD_ANIM_FADE_ON,20,0,false);
      } else {
        waiting_for_limits = true;
        lv_label_set_text(ui_Welcome, T_HOMING);
      }
    }
  }

  // Update limits from post-homing OSSM packets.
  // The OSSM sends speed=0 before homing, then sends the real max speed once homing
  // is complete. Because Ossm_paired is already true by then, the block above is
  // skipped – so we catch those updates here instead.
  if (incomingcontrol.esp_target == M5_ID && Ossm_paired == true && !OssmBleIsMode()) {
    bool gotSpeed = false;
    bool gotDepth = false;
    if (incomingcontrol.esp_speed > 0) {
      // update speed limit and slider range
      speedlimit = incomingcontrol.esp_speed;
      lv_slider_set_range(ui_homespeedslider, 0, incomingcontrol.esp_speed);
      gotSpeed = true;
    }
    if (incomingcontrol.esp_depth > 0) {
      maxdepthinmm = incomingcontrol.esp_depth;
      lv_slider_set_range(ui_homedepthslider, 0, maxdepthinmm);
      lv_slider_set_range(ui_homestrokeslider, 0, maxdepthinmm);
      gotDepth = true;
    }

    // If we were waiting for limits after pairing, and now have both, load Home
    if (waiting_for_limits && gotSpeed && gotDepth) {
      waiting_for_limits = false;
      lv_label_set_text(ui_connect, "WIFI");
      lv_scr_load_anim(ui_Home, LV_SCR_LOAD_ANIM_FADE_ON,20,0,false);
    }
  }

  switch(incomingcontrol.esp_command)
    {
    case OFF: 
    {
    OSSM_State = state_FALSE;
    APPLY_HOME_AND_STREAMING_MX_START_STOP_UI();
    }
    break;
    case ON:
    {
    OSSM_State = state_TRUE;
    APPLY_HOME_AND_STREAMING_MX_START_STOP_UI();
    }
    break;
    }
}

//Sends Commands and Value to Remote device returns ture or false if sended
bool SendCommand(int Command, float Value, int Target){
  // Immediately update local paused/running state when sending ON/OFF
  if (Command == OFF) {
    OSSM_State = state_FALSE;
    APPLY_HOME_AND_STREAMING_MX_START_STOP_UI();
    LogDebug("Local OSSM_State set to false (sent OFF)");
  } else if (Command == ON) {
    OSSM_State = state_TRUE;
    APPLY_HOME_AND_STREAMING_MX_START_STOP_UI();
    LogDebug("Local OSSM_State set to true (sent ON)");
  }
  if(Target == OSSM_ID && OssmBleIsMode()){
    String response;
    bool ok = OssmBleSendAppCommand(
      Command,
      Value,
      speed,
      depth,
      stroke,
      isRunningUiState(OSSM_State),
      maxdepthinmm,
      speedlimit,
      &response);
    if (response.length() > 0) {
      LogDebug("BLE response:");
      LogDebug(response);
    }
    return ok;
  }

  if(Ossm_paired == true){

    outgoingcontrol.esp_connected = true;
    outgoingcontrol.esp_command = Command;
    outgoingcontrol.esp_value = Value;
    outgoingcontrol.esp_target = Target;
    esp_err_t result = esp_now_send(OSSM_Address, (uint8_t *) &outgoingcontrol, sizeof(outgoingcontrol));
  
    if (result == ESP_OK) {
      return true;
    } 
    else {
      vTaskDelay(pdMS_TO_TICKS(20));
      esp_now_send(OSSM_Address, (uint8_t *) &outgoingcontrol, sizeof(outgoingcontrol));
      return false;
    }
  }

  return false;
}

void connectbutton(lv_event_t * e)
{
    static constexpr uint32_t CONNECT_ATTEMPTS = 12;
    static constexpr uint32_t CONNECT_RETRY_WINDOW_MS = 5000UL;
    static constexpr uint32_t LONG_WAIT_WINDOW_MS = 60000UL;
    static constexpr uint32_t HEARTBEAT_INTERVAL_MS = 500UL;

    lv_label_set_text(ui_Welcome, T_CONNECTING);

    if(!Ossm_paired){
      OssmBleSetMode(false);

      // First connection attempt + one automatic retry after 10 seconds.
      for (uint32_t attempt = 0; attempt < CONNECT_ATTEMPTS && !Ossm_paired; ++attempt) {
        sendPairingHeartbeat();

        bool bleConnected = OssmBleTryConnect();
        if (bleConnected) {
          lv_label_set_text(ui_Welcome, T_BLECONNECTED);

          OssmBleSetMode(true);
          Ossm_paired = true;
          syncBleConnectUi(true);
          break;
        }

        // Retry once after 10 seconds total from attempt start.
        if (!Ossm_paired && attempt == 0) {
          waitForPairingOrTimeout(CONNECT_RETRY_WINDOW_MS, HEARTBEAT_INTERVAL_MS);
          if (!Ossm_paired) {
            lv_label_set_text(ui_Welcome, T_CONNECTING);
          }
        }
      }

      if (!Ossm_paired) {
        lv_label_set_text(ui_Welcome, T_LONG_WAIT);
        waitForPairingOrTimeout(LONG_WAIT_WINDOW_MS, HEARTBEAT_INTERVAL_MS);

        if (!Ossm_paired) {
          lv_label_set_text(ui_Welcome, T_FAILED);
        }
      }
      return;
    }

    if (OssmBleIsMode()) {
      syncBleConnectUi(true);
    }
}

extern "C" void xtoysMenuButtonRToggle(void)
{
  if (OssmBleIsMode()) {
    if (XToysIsActive()) {
      XToysDeactivate();
    } else {
      XToysActivate();
      if (XToysIsActive()) {
        lv_scr_load_anim(ui_Home, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0, false);
        st_screens = ST_UI_HOME;
      }
    }
  }
}


void savesettings(lv_event_t * e)
{
  //read darkmode saved setting to force reboot for theme change
  bool theme_Change_Previous = false;
  bool theme_Change_New = false;


  m5prf.begin("m5-ctnr", false); //open NVS-storage container/session. False means that it's used it in read+write mode. Set true to open or create the namespace in read-only mode.
  theme_Change_Previous = m5prf.getBool("Darkmode", true);

  if(lv_obj_has_state(ui_vibrate, LV_STATE_CHECKED) == 1){
    m5prf.putBool("Vibrate", true); //NSV-storage write true to key "Vibrate"
	}else if(lv_obj_has_state(ui_vibrate, LV_STATE_CHECKED) == 0){
    m5prf.putBool("Vibrate", false);
	}

  if(lv_obj_has_state(ui_lefty, LV_STATE_CHECKED) == 1){
    m5prf.putBool("Lefty", true); // ui_lefty in SL-Studio code is actually Touch-enable toggle
	}else if(lv_obj_has_state(ui_lefty, LV_STATE_CHECKED) == 0){
    m5prf.putBool("Lefty", false);
	}

  LogDebug("Saving StrokeInvert setting...");
  // Stroke Inverted persistent storage
  if(lv_obj_has_state(ui_strokeinvert, LV_STATE_CHECKED) == 1){
    m5prf.putBool("StrokeInvert", true);
    strokeinvert_mode = true;
  }else if(lv_obj_has_state(ui_strokeinvert, LV_STATE_CHECKED) == 0){
    m5prf.putBool("StrokeInvert", false);
    strokeinvert_mode = false;
  }

  if(lv_obj_has_state(ui_darkmode, LV_STATE_CHECKED) == 1){
    theme_Change_New = true;
    m5prf.putBool("Darkmode", true);
	}else if(lv_obj_has_state(ui_darkmode, LV_STATE_CHECKED) == 0){
    theme_Change_New = false;
    m5prf.putBool("Darkmode", false);
	}
  m5prf.putInt("Brightness", lv_slider_get_value(ui_brightness_slider)); // save brightness slider value to NVS
  
  m5prf.end(); //close storage container/session.
  LogDebugFormatted("Brightness value saved: %d", m5prf.getInt("Brightness", 180));

  if(theme_Change_Previous != theme_Change_New){
    vibrate(225,75);
    ESP.restart(); //reboot is only required to change themes, you don't need to restart for settings to save with NVS.
  }else{
    vibrate(225,75);
  }
 m5prf.begin("m5-ctnr", true); // Reopen in read-only mode to verify saved settings and log them.   
  LogDebug("Settings are saved, these are the values:");
  LogDebugFormatted("Vibrate: %s", m5prf.getBool("Vibrate", true) ? "true" : "false");
  LogDebugFormatted("Lefty/Touch: %s", m5prf.getBool("Lefty", true) ? "true" : "false");
  LogDebugFormatted("StrokeInvert: %s", m5prf.getBool("StrokeInvert", false) ? "true" : "false"); 
  LogDebugFormatted("Darkmode: %s", m5prf.getBool("Darkmode", false) ? "true" : "false");
  LogDebugFormatted("Brightness: %d", m5prf.getInt("Brightness", 180));
m5prf.end();
}

void screenmachine(lv_event_t * e)
{
  (void)e;
  const int previousScreen = st_screens;
  updateScreenTitleLabels();

  if (lv_scr_act() == ui_Start){
    st_screens = ST_UI_START;
  } else if (lv_scr_act() == ui_Home){
    st_screens = ST_UI_HOME;

    if (previousScreen == ST_UI_STREAMING) {
      applyHomeDefaultsForModeChange();
    }

    speed = lv_slider_get_value(ui_homespeedslider);
    LogDebug(speedenc);
    LogDebug(speed);
    // Clear home control encoders when entering Home to avoid carry-over from other screens.
    encoder1.setCount(0);
    encoder2.setCount(0);
    encoder3.setCount(0);
    // Clear encoder4 as well to avoid leftover torque/torque-r counts affecting Sensation.
    encoder4.setCount(0);
    if (!strokeinvert_mode) {
      lv_slider_set_mode(ui_homestrokeslider, LV_SLIDER_MODE_NORMAL);
    } else {  
      lv_slider_set_mode(ui_homestrokeslider, LV_SLIDER_MODE_RANGE);
    }

    if (OssmBleIsMode()) {
      lv_slider_set_range(ui_homespeedslider, 0, 100);
      lv_slider_set_range(ui_homedepthslider, 0, 100);
      lv_slider_set_range(ui_homestrokeslider, 0, 100);

      uint32_t nowMs = millis();
      if ((nowMs - ble_home_go_strokeengine_ms) > 1200U) {
        OssmBleGoToStrokeEngine();
        ble_home_go_strokeengine_ms = nowMs;
      }
    } else {
      lv_slider_set_range(ui_homespeedslider, 0, speedlimit);
      lv_slider_set_range(ui_homedepthslider, 0, maxdepthinmm);
      lv_slider_set_range(ui_homestrokeslider, 0, maxdepthinmm);
    }

    APPLY_HOME_AND_STREAMING_MX_START_STOP_UI();

            
  } else if (lv_scr_act() == ui_Pattern){
    st_screens = ST_UI_PATTERN;
    // Pattern screen only uses encoder4 (roller navigation).
    // Drain other encoder counters so turning those knobs here never leaks into Home values.
    encoder1.setCount(0);
    encoder2.setCount(0);
    encoder3.setCount(0);
    encoder4_enc = encoder4.getCount();
  } else if (lv_scr_act() == ui_Torqe){
    st_screens = ST_UI_Torqe;
    torqe_f = lv_slider_get_value(ui_outtroqeslider);
    torqe_f_enc = fscale(50, 200, 0, Encoder_MAP, torqe_f, 0);
    encoder1.setCount(torqe_f_enc);

    torqe_r = lv_slider_get_value(ui_introqeslider);
    torqe_r_enc = fscale(20, 200, 0, Encoder_MAP, torqe_r, 0);
    encoder4.setCount(torqe_r_enc);

  } else if (lv_scr_act() == ui_EJECTSettings){
    st_screens = ST_UI_EJECTSETTINGS;
  } else if (lv_scr_act() == ui_Settings){
    st_screens = ST_UI_SETTINGS;
  } else if (lv_scr_act() == ui_Menu){
    st_screens = ST_UI_MENU;
    encoder4_enc = encoder4.getCount();
    if (ui_g_menu != nullptr && ui_MenuButtonTL != nullptr) {
      lv_group_focus_obj(ui_MenuButtonTL);
    }
  } else if (lv_scr_act() == ui_Streaming){
    st_screens = ST_UI_STREAMING;
    encoder1.setCount(0);
    encoder2.setCount(0);
    encoder3.setCount(0);
  } else if (lv_scr_act() == ui_Addons){
    st_screens = ST_UI_ADDONS;
    encoder4_enc = encoder4.getCount();
    if (ui_g_addons != nullptr && ui_AddonsItem0 != nullptr) {
      lv_group_focus_obj(ui_AddonsItem0);
    }
  }

  monitorOssmState(true);
}

void homebuttonLevent(lv_event_t * e){
  // Pullout then return to Menu.
  lv_obj_clear_state(ui_HomeButtonL, LV_STATE_CHECKED);

  depth = 0;
  stroke = 0;
  if (ui_homedepthslider != nullptr) lv_slider_set_value(ui_homedepthslider, 0, LV_ANIM_OFF);
  if (ui_homestrokeslider != nullptr) lv_slider_set_value(ui_homestrokeslider, 0, LV_ANIM_OFF);

  if (OssmBleIsMode()) {
    OssmBleExecutePulloutStop(speed, maxdepthinmm, speedlimit);
    OssmBleGoToMenu();
  } else {
    SendCommand(SETUP_D_I, 0.0, OSSM_ID);
    SendCommand(DEPTH, 0.0, OSSM_ID);
    SendCommand(STROKE, 0.0, OSSM_ID);
    vTaskDelay(pdMS_TO_TICKS(300));
    SendCommand(SPEED, 0.0, OSSM_ID);
  }

  OSSM_State = state_FALSE;
  APPLY_HOME_AND_STREAMING_MX_START_STOP_UI();
  _ui_screen_change(ui_Menu, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
}
void savepattern(lv_event_t * e){
  pattern = lv_roller_get_selected(ui_PatternS);
  lv_roller_get_selected_str(ui_PatternS,patternstr,0);
  lv_label_set_text(ui_HomePatternLabel,patternstr);
  LogDebug(pattern);
  float patterns = pattern;
  SendCommand(PATTERN, patterns, OSSM_ID);
}

static void homebuttonm_action(bool fromPhysicalMx)
{
  (void)fromPhysicalMx;
  const bool isRunning = isRunningUiState(OSSM_State);

  // BLE mode: toggle paused state when on Home screen.
  if (OssmBleIsMode() && st_screens == ST_UI_HOME) {
    ossm_state_monitor_hold_until_ms = millis() + 1U;
    switch (OssmBleHandleHomeToggle(isRunning, speed)) {
      case OssmBleHomeToggleResult::Paused:
        OSSM_State = state_FALSE;
        APPLY_HOME_MX_START_STOP_UI();
        LogDebugFormatted("Sent BLE pause command with paused speed: %f\n", speed);
        break;
      case OssmBleHomeToggleResult::Resumed:
        OSSM_State = state_TRUE;
        APPLY_HOME_MX_START_STOP_UI();
        LogDebugFormatted("Sent BLE resume command (speed:%f)\n", OssmBleGetUnpauseSpeed());
        break;
      case OssmBleHomeToggleResult::Started:
        OSSM_State = state_TRUE;
        APPLY_HOME_MX_START_STOP_UI();
        LogDebugFormatted("Sent BLE start command (speed:%f)\n", OssmBleGetUnpauseSpeed());
        break;
      case OssmBleHomeToggleResult::BlockedNotReady:
        LogDebugPrio("BLE start blocked: OSSM is not ready for stroke engine");
        break;
      case OssmBleHomeToggleResult::Failed:
      default:
        LogDebugPrio("BLE home toggle failed");
        break;
    }
    return;
  }

  // Non-BLE (ESP-NOW) or other screens: original behavior
  if(OSSM_State == state_FALSE){
    if (OssmBleIsMode()) {
      OssmBleGoToStrokeEngine();
    }
    SendCommand(ON, 0, OSSM_ID);
    if (OssmBleIsMode()) {
      OSSM_State = state_TRUE;
      APPLY_HOME_MX_START_STOP_UI();
    }
  } else if(isRunning){
    SendCommand(OFF, 0, OSSM_ID);
    if (OssmBleIsMode()) {
      OSSM_State = state_FALSE;
      APPLY_HOME_MX_START_STOP_UI();
    }
  }
}

static void streamingbuttonm_action(bool fromPhysicalMx)
{
  (void)fromPhysicalMx;
  const bool isRunning = isRunningUiState(OSSM_State);

  if (OssmBleIsMode() && st_screens == ST_UI_STREAMING) {
    ossm_state_monitor_hold_until_ms = millis() + 1U;
    switch (OssmBleHandleStreamingToggle(isRunning, speed)) {
      case OssmBleHomeToggleResult::Paused:
        OSSM_State = state_FALSE;
        APPLY_STREAMING_MX_START_STOP_UI();
        LogDebugFormatted("Sent BLE streaming pause command with paused speed: %f\n", speed);
        break;
      case OssmBleHomeToggleResult::Resumed:
        OSSM_State = state_TRUE;
        APPLY_STREAMING_MX_START_STOP_UI();
        LogDebugFormatted("Sent BLE streaming resume command (speed:%f)\n", OssmBleGetUnpauseSpeed());
        LogDebugPrio("Streaming armed: waiting for external stream frames");
        break;
      case OssmBleHomeToggleResult::Started:
        OSSM_State = state_TRUE;
        APPLY_STREAMING_MX_START_STOP_UI();
        LogDebugFormatted("Sent BLE streaming start command (speed:%f)\n", OssmBleGetUnpauseSpeed());
        LogDebugPrio("Streaming armed: waiting for external stream frames");
        break;
      case OssmBleHomeToggleResult::BlockedNotReady:
        LogDebugPrio("BLE streaming start blocked: OSSM is not ready for streaming mode");
        break;
      case OssmBleHomeToggleResult::Failed:
      default:
        LogDebugPrio("BLE streaming toggle failed");
        break;
    }
    return;
  }

  if(OSSM_State == state_FALSE){
    SendCommand(ON, 0, OSSM_ID);
    if (OssmBleIsMode()) {
      OSSM_State = state_TRUE;
      APPLY_STREAMING_MX_START_STOP_UI();
    }
  } else if(isRunning){
    SendCommand(OFF, 0, OSSM_ID);
    if (OssmBleIsMode()) {
      OSSM_State = state_FALSE;
      APPLY_STREAMING_MX_START_STOP_UI();
    }
  }
}

static void streamingStartOnlyAction()
{
  const bool isRunning = isRunningUiState(OSSM_State);

  if (OssmBleIsMode() && st_screens == ST_UI_STREAMING) {
    if (isRunning) {
      LogDebugPrio("BLE streaming start ignored: already running");
      return;
    }

    ossm_state_monitor_hold_until_ms = millis() + 1U;
    switch (OssmBleHandleStreamingToggle(false, speed)) {
      case OssmBleHomeToggleResult::Resumed:
        OSSM_State = state_TRUE;
        APPLY_STREAMING_MX_START_STOP_UI();
        LogDebugFormatted("Sent BLE streaming resume command (speed:%f)\n", OssmBleGetUnpauseSpeed());
        LogDebugPrio("Streaming armed: waiting for external stream frames");
        break;
      case OssmBleHomeToggleResult::Started:
        OSSM_State = state_TRUE;
        APPLY_STREAMING_MX_START_STOP_UI();
        LogDebugFormatted("Sent BLE streaming start command (speed:%f)\n", OssmBleGetUnpauseSpeed());
        LogDebugPrio("Streaming armed: waiting for external stream frames");
        break;
      case OssmBleHomeToggleResult::Paused:
        LogDebugPrio("BLE streaming start-only action unexpectedly returned pause");
        break;
      case OssmBleHomeToggleResult::BlockedNotReady:
        LogDebugPrio("BLE streaming start blocked: OSSM is not ready for streaming mode");
        break;
      case OssmBleHomeToggleResult::Failed:
      default:
        LogDebugPrio("BLE streaming start-only action failed");
        break;
    }
    return;
  }

  if (!isRunning) {
    SendCommand(ON, 0, OSSM_ID);
    if (OssmBleIsMode()) {
      OSSM_State = state_TRUE;
      APPLY_STREAMING_MX_START_STOP_UI();
    }
  }
}

void homebuttonmevent(lv_event_t * e){
  (void)e;
  LogDebugPrio("HomeButton (touch/LVGL)");
  homebuttonm_action(false);
}

void streamingbuttonmevent(lv_event_t * e){
  (void)e;
  LogDebugPrio("StreamingButton (touch/LVGL)");
  streamingbuttonm_action(false);
}

void setupDepthInter(lv_event_t * e){
    SendCommand(SETUP_D_I, 0, OSSM_ID);
}

void setupdepthF(lv_event_t * e){
    SendCommand(SETUP_D_I_F, 0, OSSM_ID);
}

static bool triggerAddonForSlot(int slot)
{
  loadAddonBindingsIfNeeded();
  int addonIndex = -1;
  for (size_t i = 0; i < ADDON_DEFINITIONS_COUNT; ++i) {
    if (g_addon_slots[i] == slot) {
      addonIndex = (int)i;
      break;
    }
  }

  if (addonIndex < 0) {
    return false;
  }

  if (strcmp(ADDON_DEFINITIONS[addonIndex].id, "eject_cumpump") == 0) {
    _ui_screen_change(ui_EJECTSettings, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    return true;
  }

  if (strcmp(ADDON_DEFINITIONS[addonIndex].id, "fist_it") == 0) {
    _ui_screen_change(ui_Addons, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    return true;
  }

  return false;
}

void setup(){
  Serial.begin(115200);
  vTaskDelay(pdMS_TO_TICKS(50));
  Serial.println("\n\nBooting...");
  esp_sleep_wakeup_cause_t wakeCause = esp_sleep_get_wakeup_cause();
  auto cfg = M5.config();
  M5.begin(cfg);
  LogDebugFormatted("MX boot raw pin%d=%d\n", Button1.pin(), digitalRead(Button1.pin()));


  m5prf.begin("m5-ctnr", false); 
  // Loads these settings at boot
  LogDebug("Loading settings...");
  eject_status = m5prf.getBool("ejectAddon", false); //boolean here is used if key does not exist yet
  dark_mode = m5prf.getBool("Darkmode", true);       // ^ (basically first boot defaults, saving settings surives a re-flash!)
  vibrate_mode = m5prf.getBool("Vibrate", true);
  touch_home= m5prf.getBool("Lefty", false);       // = touchcreen. There apears to be no actual lefthanded mode anywhere
  strokeinvert_mode = m5prf.getBool("StrokeInvert");
  int brightness = m5prf.getInt("Brightness", -1);
  if (brightness < 0) {
    brightness = 180; // your default
    m5prf.putInt("Brightness", brightness); // save default to NVS
  }

  g_brightness_value = brightness;
  m5prf.end();
  // --- Apply brightness at startup ---
  M5.Lcd.setBrightness(brightness);
  LogDebugFormatted("Brightness applied: %d", brightness);

  // ...existing code...

  
  M5.Power.setChargeCurrent(BATTERY_CHARGE_CURRENT);
  LogDebug("\n Starting");      // Start LogDebug
  LogDebug("TEST ");
  
  WiFi.mode(WIFI_STA);
  LogDebug(WiFi.macAddress());
  LogDebug("TEST ");

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
  } else {
    Serial.println("esp_now_init ok");
  }
  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);

  // register peer
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  // register first peer  
  memcpy(peerInfo.peer_addr, OSSM_Address, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
  }
  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);

  xTaskCreatePinnedToCore(espNowRemoteTask,      /* Task function. */
                            "espNowRemoteTask",  /* name of task. */
                            3096,               /* Stack size of task */
                            NULL,               /* parameter of the task */
                            5,                  /* priority of the task */
                            &eRemote_t,         /* Task handle to keep track of created task */
                            0);                 /* pin task to core 0 */
  
  encoder1.attachHalfQuad(ENC_1_CLK, ENC_1_DT);
  encoder2.attachHalfQuad(ENC_2_CLK, ENC_2_DT);
  encoder3.attachHalfQuad(ENC_3_CLK, ENC_3_DT);
  encoder4.attachHalfQuad(ENC_4_CLK, ENC_4_DT);
  Button1.attachClick(mxclick);
  Button1.attachDoubleClick(mxdouble);
  Button1.attachLongPressStart(mxlong);
  Button1.setDebounceMs(80);
  Button1.setLongPressIntervalMs(400);
  Button2.attachClick(clickLeft);
  Button2.attachDoubleClick(clickLeftDouble);
  Button2.setDebounceMs(80);
  Button3.attachClick(clickRight);
  Button3.setDebounceMs(80);
  Button3.attachLongPressStart(clickRightLong);
  Button3.setLongPressIntervalMs(400);
  Button3.attachDoubleClick(clickRightDouble);
  
  // Initialize `disp_buf` display buffer with the buffer(s).
  // lv_draw_buf_init(&draw_buf, LV_HOR_RES_MAX, LV_VER_RES_MAX);
  M5.Display.setEpdMode(epd_mode_t::epd_fastest); // fastest but very-low quality.
  if (M5.Display.width() < M5.Display.height())
  { /// Landscape mode.
  M5.Display.setRotation(M5.Display.getRotation() ^ 1);
  }
  
  lv_init();
  lv_tick_set_cb(my_tick_function);

  display = lv_display_create(HOR_RES, VER_RES);
  lv_display_set_flush_cb(display, my_display_flush);

  // Force a strict alignment for LVGL draw buffer.
  static lv_color_t buf1[HOR_RES * 15] __attribute__((aligned(16)));
  lv_display_set_buffers(display, buf1, nullptr, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);

  indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, my_touchpad_read);

  LogDebug("Works till step 1");
  ui_init();  
  
  if (wakeCause == ESP_SLEEP_WAKEUP_EXT1 || wakeCause == ESP_SLEEP_WAKEUP_EXT0) {
    // After deep sleep wake, return to start screen so reconnect can happen cleanly.
    lv_scr_load(ui_Start);
  }
  register_event_debug_callbacks();
  APPLY_HOME_AND_STREAMING_MX_START_STOP_UI();
  XToysInit();

  // --- Restore stroke invert state from NVS and apply to UI (after ui_init) ---
  if (strokeinvert_mode) {
    lv_obj_add_state(ui_strokeinvert, LV_STATE_CHECKED);
  } else {
    lv_obj_clear_state(ui_strokeinvert, LV_STATE_CHECKED);
  }

  // Auto-connect once ui_Start has finished loading (first rendered frame).
  lv_obj_add_event_cb(ui_Start, [](lv_event_t *) { connectbutton(nullptr); },
                      LV_EVENT_SCREEN_LOADED, nullptr);

  lv_obj_clear_state(ui_HomeButtonL, LV_STATE_DISABLED);
  if(eject_status == true){
    lv_obj_add_state(ui_ejectaddon, LV_STATE_CHECKED);
    lv_obj_clear_state(ui_EJECTSettingButton, LV_STATE_DISABLED);
  }
  if(dark_mode == true){
    lv_obj_add_state(ui_darkmode, LV_STATE_CHECKED);
  }
  if(vibrate_mode == true){
    lv_obj_add_state(ui_vibrate, LV_STATE_CHECKED);
  }
  if(touch_home == true){
    lv_obj_add_state(ui_lefty, LV_STATE_CHECKED);
  }
  lv_roller_set_selected(ui_PatternS,2,LV_ANIM_OFF);
  lv_roller_get_selected_str(ui_PatternS,patternstr,0);
  lv_label_set_text(ui_HomePatternLabel,patternstr);
  last_activity_ms = millis();

  LogDebug("Setup complete");
}

void loop()
{

     bool changed=false;
     if(encoder1.getCount()+encoder2.getCount()+encoder3.getCount()+encoder4.getCount() != 0){
       screensaver_check_activity();
     }
     const int BatteryLevel = M5.Power.getBatteryLevel();
     String BatteryValue = (String(BatteryLevel, DEC) + "%");
     const char *battVal = BatteryValue.c_str();
    if (ui_Battery != nullptr) lv_bar_set_value(ui_Battery, BatteryLevel, LV_ANIM_OFF);
    if (ui_BattValue != nullptr) lv_label_set_text(ui_BattValue, battVal);
    if (ui_Battery1 != nullptr) lv_bar_set_value(ui_Battery1, BatteryLevel, LV_ANIM_OFF);
    if (ui_BattValue1 != nullptr) lv_label_set_text(ui_BattValue1, battVal);

     // --- Screen Saver Logic ---
     if (!screensaver_active && (millis() - last_activity_ms > (unsigned long)screensaver_timeout_ms)) {
       screensaver_prev_brightness = g_brightness_value;
       M5.Lcd.setBrightness(screensaver_dim_brightness);
       screensaver_active = true;
     }

     // --- Deep Sleep Logic ---
     if (millis() - last_activity_ms > deep_sleep_timeout_ms) {
       if (canEnterDeepSleep()) {
         enterDeepSleep();
       }
     }

     lv_bar_set_value(ui_Battery2, BatteryLevel, LV_ANIM_OFF);
     lv_label_set_text(ui_BattValue2, battVal);
//     lv_bar_set_value(ui_Battery3, BatteryLevel, LV_ANIM_OFF);
//     lv_label_set_text(ui_BattValue3, battVal);
     lv_bar_set_value(ui_Battery4, BatteryLevel, LV_ANIM_OFF);
     lv_label_set_text(ui_BattValue4, battVal);
     lv_bar_set_value(ui_Battery5, BatteryLevel, LV_ANIM_OFF);
     lv_label_set_text(ui_BattValue5, battVal);

     M5.update();
     lv_task_handler();
    updateMxReleaseStability();
     Button1.tick();
     Button2.tick();
     Button3.tick();

    XToysUpdate();

    monitorOssmState(false);

     switch(st_screens){
      
     case ST_UI_START: //Menu With logo after boot
      {
        if (OssmBleIsMode()) {
          static uint32_t lastBleHomingPollMs = 0;
          uint32_t nowPollMs = millis();
          if ((nowPollMs - lastBleHomingPollMs) >= 500UL) {
            lastBleHomingPollMs = nowPollMs;
            syncBleConnectUi(true);
          }
        }

        if(lv_obj_has_state(ui_lefty, LV_STATE_CHECKED) == 1){
          touch_disabled = true;
        }

        if(clickLeft_short_waspressed == true){
         lv_obj_send_event(ui_StartButtonL, LV_EVENT_CLICKED, NULL);
        } else if(mxclick_short_waspressed == true){
         LogDebug("mx: ST_UI_START -> sending StartButtonM click");
         lv_obj_send_event(ui_StartButtonM, LV_EVENT_CLICKED, NULL);
        } else if(clickRight_short_waspressed == true){
         lv_obj_send_event(ui_StartButtonR, LV_EVENT_CLICKED, NULL);
        }
      }
      break;

      case ST_UI_HOME: //Menu with OSSM control sliders
      {
        static int lastHomeTransportMode = -1;
        int currentTransportMode = OssmBleIsMode() ? 1 : 0;
        if (lastHomeTransportMode != currentTransportMode) {
          if (currentTransportMode == 1) {
            lv_slider_set_range(ui_homespeedslider, 0, 100);
            lv_slider_set_range(ui_homedepthslider, 0, 100);
            lv_slider_set_range(ui_homestrokeslider, 0, 100);
          } else {
            lv_slider_set_range(ui_homespeedslider, 0, speedlimit);
            lv_slider_set_range(ui_homedepthslider, 0, maxdepthinmm);
            lv_slider_set_range(ui_homestrokeslider, 0, maxdepthinmm);
          }

          if (speed > lv_slider_get_max_value(ui_homespeedslider)) {
            speed = lv_slider_get_max_value(ui_homespeedslider);
          }
          if (depth > lv_slider_get_max_value(ui_homedepthslider)) {
            depth = lv_slider_get_max_value(ui_homedepthslider);
          }
          if (stroke > lv_slider_get_max_value(ui_homestrokeslider)) {
            stroke = lv_slider_get_max_value(ui_homestrokeslider);
          }

          lastHomeTransportMode = currentTransportMode;
        }

        if(lv_obj_has_state(ui_lefty, LV_STATE_CHECKED) == 1){
          touch_disabled = true;
        }

        //
        // Encoder 1 Speed 
        //
        if(lv_slider_is_dragged(ui_homespeedslider) == false){ //if knob gets rotated
          changed = false;
          lv_slider_set_value(ui_homespeedslider, speed, LV_ANIM_OFF);

          long speedCount = encoder1.getCount();
          int speedDetents = (int)(speedCount / 2);
          if (speedDetents != 0) {
            changed = true;
            speed += getRampedDetentDelta(1, speedDetents);
            encoder1.setCount(speedCount % 2);
            markEncoderActivityForMxFilter();
          }

          //speed min-max bounds
          if (speed < 0){
            changed = true;
            speed = 0;
          }
          const float speedMax = OssmBleIsMode() ? 100.0f : speedlimit;
          if (speed > speedMax){
            changed = true;
            speed = speedMax;
          }
          
          //send speed
          if (changed) {
            SendCommand(SPEED, speed, OSSM_ID);
          }
        
        
        }else if (lv_slider_get_value(ui_homespeedslider) != speed){ //if slider moved
            speed = lv_slider_get_value(ui_homespeedslider);
            SendCommand(SPEED, speed, OSSM_ID);
        }
        char speed_v[12];
        if (OssmBleIsMode()) {
          snprintf(speed_v, sizeof(speed_v), "%d", (int)(speed + 0.5f));  //add %% after %d for % after number
        } else {
          dtostrf(speed, 6, 0, speed_v);
        }
        lv_label_set_text(ui_homespeedvalue, speed_v);

        //
        // Encoder 2 Depth 
        //
        if(lv_slider_is_dragged(ui_homedepthslider) == false){ //if knob gets rotated
          changed = false;
          lv_slider_set_value(ui_homedepthslider, depth, LV_ANIM_OFF);

          long depthCount = encoder2.getCount();
          int depthDetents = (int)(depthCount / 2);
          if (depthDetents != 0) {
            changed = true;
            int depthDelta = getRampedDetentDelta(2, depthDetents);
            depth += depthDelta;
            if (dynamicStroke == true) {
              stroke += depthDelta;
              if (stroke >= depth) stroke = depth;
            }
            encoder2.setCount(depthCount % 2);
            markEncoderActivityForMxFilter();
          }

          //depth min-max bounds
          if (depth < 0){
            changed = true;
            depth = 0;
          }
          const float depthMax = OssmBleIsMode() ? 100.0f : maxdepthinmm;
          if (depth > depthMax){
            changed = true;
            depth = depthMax;
          }
          
          //send depth
          if (changed) {
            SendCommand(DEPTH, depth, OSSM_ID);
          }
        }else if(lv_slider_get_value(ui_homedepthslider) != depth){
            depth = lv_slider_get_value(ui_homedepthslider);
            SendCommand(DEPTH, depth, OSSM_ID);
        }
        char depth_v[12];
        if (OssmBleIsMode()) {
          snprintf(depth_v, sizeof(depth_v), "%d", (int)(depth + 0.5f));
        } else {
          dtostrf(depth, 6, 0, depth_v);
        }
        lv_label_set_text(ui_homedepthvalue, depth_v);
        
        //
        // Encoder 3 Stroke 
        //
        if(lv_slider_is_dragged(ui_homestrokeslider) == false){ //if knob gets rotated
          changed = false;
          if (!strokeinvert_mode) {
            lv_slider_set_mode(ui_homestrokeslider, LV_SLIDER_MODE_NORMAL);
            lv_slider_set_value(ui_homestrokeslider, stroke, LV_ANIM_OFF);
          } else {  
            lv_slider_set_mode(ui_homestrokeslider, LV_SLIDER_MODE_RANGE);
            lv_slider_set_start_value(ui_homestrokeslider, depth - stroke, LV_ANIM_OFF);
            lv_slider_set_value(ui_homestrokeslider, depth, LV_ANIM_OFF);
          }

          long strokeCount = encoder3.getCount();
          int strokeDetents = (int)(strokeCount / 2);
          if (strokeDetents != 0) {
            changed = true;
            if (!strokeinvert_mode) {
              stroke += getRampedDetentDelta(3, strokeDetents); // CW increases stroke
            } else {
              stroke -= getRampedDetentDelta(3, strokeDetents); // CCW increases stroke
            }
            encoder3.setCount(strokeCount % 2);
            markEncoderActivityForMxFilter();
          }

          //Stroke min-max bounds
          if (stroke < 0){
            changed = true;
            stroke = 0;
          }
          const float strokeMax = OssmBleIsMode() ? 100.0f : maxdepthinmm;
          if (stroke > strokeMax){
            changed = true;
            stroke = strokeMax;
          }
          // Clamp stroke to depth
          if (stroke > depth) {
            changed = true;
            stroke = depth;
          }

          //send stroke
          if (changed) {
            SendCommand(STROKE, stroke, OSSM_ID);
          }

        } else {
          if (!strokeinvert_mode) {
            if(lv_slider_get_left_value(ui_homestrokeslider) != depth - stroke){
              stroke = depth - lv_slider_get_left_value(ui_homestrokeslider);
              // Clamp stroke to depth
              if (stroke > depth) stroke = depth;
              SendCommand(STROKE, stroke, OSSM_ID);
            } else if(lv_slider_get_value(ui_homestrokeslider) != depth){
              depth = lv_slider_get_value(ui_homestrokeslider);
              SendCommand(DEPTH, depth, OSSM_ID);
            }
          } else {
            if(lv_slider_get_value(ui_homestrokeslider) != depth){
              depth = lv_slider_get_value(ui_homestrokeslider);
              SendCommand(DEPTH, depth, OSSM_ID);
            } else if(lv_slider_get_left_value(ui_homestrokeslider) != depth - stroke){
              stroke = depth - lv_slider_get_left_value(ui_homestrokeslider);
              // Clamp stroke to depth
              if (stroke > depth) stroke = depth;
              SendCommand(STROKE, stroke, OSSM_ID);
            }
          }
        }

        char stroke_v[12];
        if (OssmBleIsMode()) {
          snprintf(stroke_v, sizeof(stroke_v), "%d", (int)(stroke + 0.5f));
        } else {
          dtostrf(stroke, 6, 0, stroke_v);
        }
        lv_label_set_text(ui_homestrokevalue, stroke_v);  //was lv_label_set_text(ui_homestrokevalue, stroke_v);

        //
        // Encoder4 Sensation
        //
        if(lv_slider_is_dragged(ui_homesensationslider) == false){
          changed = false;
          lv_slider_set_value(ui_homesensationslider, sensation, LV_ANIM_OFF);

          long sensationCount = encoder4.getCount();
          int sensationDetents = (int)(sensationCount / 2);
          if (sensationDetents != 0) {
            changed = true;
            sensation += 2.0f * getRampedDetentDelta(4, sensationDetents);
            encoder4.setCount(sensationCount % 2);
            markEncoderActivityForMxFilter();
          }

          //Stoke min-max bounds
          if (sensation < -100){
            changed = true;
            sensation = -100;
          }
          if (sensation > 100){
            changed = true;
            sensation = 100;
          }

          if (changed) {
            SendCommand(SENSATION, sensation, OSSM_ID);
          }          
        } else if(lv_slider_get_value(ui_homesensationslider) != sensation){
            sensation = lv_slider_get_value(ui_homesensationslider);
            SendCommand(SENSATION, sensation, OSSM_ID);
        }

        if(clickLeft_short_waspressed == true){
         lv_obj_send_event(ui_HomeButtonL, LV_EVENT_CLICKED, NULL);
         clickLeft_short_waspressed = false;
        } else if (clickLeft_double_waspressed == true) {
         triggerAddonForSlot(ADDON_SLOT_LEFT);
         clickLeft_double_waspressed = false;
        } else if(mxclick_short_waspressed == true){
         // Physical MX is handled directly only on Home, where the middle action
         // controls start/stop in ESP-NOW mode and pause/resume in BLE mode.
         // Reminder for later: if Eject and Fist-It addons are reinstated and
         // physical MX is expected to trigger addon-specific actions outside Home,
         // this routing/filtering decision may need to be expanded carefully.
         LogDebug("mx: ST_UI_HOME -> direct HomeButtonM action");
         homebuttonm_action(true);
         mxclick_short_waspressed = false;
        } else if(clickRight_short_waspressed == true){
         lv_obj_send_event(ui_HomeButtonR, LV_EVENT_CLICKED, NULL);
         clickRight_short_waspressed = false;
        } else if(clickRight_long_waspressed == true){
          _ui_screen_change(ui_Menu, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
          clickRight_long_waspressed = false;
        }else if(clickRight_double_waspressed == true){
          triggerAddonForSlot(ADDON_SLOT_RIGHT);
          clickRight_double_waspressed = false;
        }
      }
      break;

/*      case ST_UI_MENUE:
      {
        if(lv_obj_has_state(ui_lefty, LV_STATE_CHECKED) == 1){
          touch_disabled = true;
        }
        if(encoder4.getCount() > encoder4_enc + 2){
          LogDebug("next");
          lv_group_focus_next(ui_g_menue);
          encoder4_enc = encoder4.getCount();
        } else if(encoder4.getCount() < encoder4_enc -2){
          lv_group_focus_prev(ui_g_menue);
          LogDebug("Preview");
          encoder4_enc = encoder4.getCount();
        }

        if(clickLeft_short_waspressed == true){
         lv_obj_send_event(ui_MenueButtonL, LV_EVENT_CLICKED, NULL);
        } else if(mxclick_short_waspressed == true){
         LogDebug("mx: ST_UI_MENUE -> sending MenueButtonM click");
         lv_obj_send_event(ui_MenueButtonM, LV_EVENT_CLICKED, NULL);
        } else if(clickRight_short_waspressed == true){
         lv_obj_send_event(lv_group_get_focused(ui_g_menue), LV_EVENT_CLICKED, NULL);
        } else if(clickRight_long_waspressed == true){
         SendCommand(REBOOT, 0, OSSM_ID);
        } 
      }
      break;
*/
      case ST_UI_PATTERN:
      {
        if(lv_obj_has_state(ui_lefty, LV_STATE_CHECKED) == 1){
          touch_disabled = true;
        }
        if(encoder4.getCount() > encoder4_enc + 2){
          LogDebug("next");
          uint32_t t = LV_KEY_DOWN;
          lv_obj_send_event(ui_PatternS, LV_EVENT_KEY, &t);
          encoder4_enc = encoder4.getCount();
        } else if(encoder4.getCount() < encoder4_enc -2){
          uint32_t t = LV_KEY_UP;
          lv_obj_send_event(ui_PatternS, LV_EVENT_KEY, &t);
          LogDebug("Preview");
          encoder4_enc = encoder4.getCount();
        }
         if(clickLeft_short_waspressed == true){
         lv_obj_send_event(ui_PatternButtonL, LV_EVENT_CLICKED, NULL);
        } else if(mxclick_short_waspressed == true){
         LogDebug("mx: ST_UI_PATTERN -> sending PatternButtonM click");
         lv_obj_send_event(ui_PatternButtonM, LV_EVENT_CLICKED, NULL);
        } else if(clickRight_short_waspressed == true){
         lv_obj_send_event(ui_PatternButtonR, LV_EVENT_CLICKED, NULL);
        }
      }
      break;

      case ST_UI_Torqe:
      {
        if(lv_obj_has_state(ui_lefty, LV_STATE_CHECKED) == 1){
          touch_disabled = true;
        }
        // Encoder 1 Torqe Out
        if(lv_slider_is_dragged(ui_outtroqeslider) == false){
          if (encoder1.getCount() != torqe_f_enc){
            lv_slider_set_value(ui_outtroqeslider, torqe_f, LV_ANIM_OFF);
            if(encoder1.getCount() <= 0){
              encoder1.setCount(0);
            } else if (encoder1.getCount() >= Encoder_MAP){
              encoder1.setCount(Encoder_MAP);
            } 
            torqe_f_enc = encoder1.getCount();
            torqe_f = fscale(0, Encoder_MAP, 50, 200, torqe_f_enc, 0);
            SendCommand(TORQE_F, torqe_f, OSSM_ID);
          }
        } else if(lv_slider_get_value(ui_outtroqeslider) != torqe_f){
            torqe_f_enc = fscale(50, 200, 0, Encoder_MAP, torqe_f, 0);
            encoder1.setCount(torqe_f_enc);
            torqe_f = lv_slider_get_value(ui_outtroqeslider);
            SendCommand(TORQE_F, torqe_f, OSSM_ID);
        }
        char torqe_f_v[7];
        dtostrf((torqe_f*-1), 6, 0, torqe_f_v);
        lv_label_set_text(ui_outtroqevalue, torqe_f_v);

        // Encoder 4 Torqe IN
        if(lv_slider_is_dragged(ui_introqeslider) == false){
          if (encoder4.getCount() != torqe_r_enc){
            lv_slider_set_value(ui_introqeslider, torqe_r, LV_ANIM_OFF);
            if(encoder4.getCount() <= 0){
              encoder4.setCount(0);
            } else if (encoder4.getCount() >= Encoder_MAP){
              encoder4.setCount(Encoder_MAP);
            } 
            torqe_r_enc = encoder4.getCount();
            torqe_r = fscale(0, Encoder_MAP, 20, 200, torqe_r_enc, 0);
            SendCommand(TORQE_R, torqe_r, OSSM_ID);
          }
        } else if(lv_slider_get_value(ui_introqeslider) != torqe_r){
            torqe_r_enc = fscale(20, 200, 0, Encoder_MAP, torqe_r, 0);
            encoder4.setCount(torqe_r_enc);
            torqe_r = lv_slider_get_value(ui_introqeslider);
            SendCommand(TORQE_R, torqe_r, OSSM_ID);
        }
        char torqe_r_v[7];
        dtostrf(torqe_r, 6, 0, torqe_r_v);
        lv_label_set_text(ui_introqevalue, torqe_r_v);

         if(clickLeft_short_waspressed == true){
         lv_obj_send_event(ui_TorqeButtonL, LV_EVENT_CLICKED, NULL);
        } else if(mxclick_short_waspressed == true){
         LogDebug("mx: ST_UI_Torqe -> sending TorqeButtonM click");
         lv_obj_send_event(ui_TorqeButtonM, LV_EVENT_CLICKED, NULL);
        } else if(clickRight_short_waspressed == true){
         lv_obj_send_event(ui_TorqeButtonR, LV_EVENT_CLICKED, NULL);
        }
      }
      break;

      case ST_UI_EJECTSETTINGS:
      {
        if(lv_obj_has_state(ui_lefty, LV_STATE_CHECKED) == 1){
          touch_disabled = true;
        }

        if(clickLeft_short_waspressed == true){
         lv_obj_send_event(ui_EJECTButtonL, LV_EVENT_CLICKED, NULL);
        } else if(mxclick_short_waspressed == true){
         LogDebug("mx: ST_UI_EJECTSETTINGS -> sending EJECTButtonM click");
         lv_obj_send_event(ui_EJECTButtonM, LV_EVENT_CLICKED, NULL);
        } else if(clickRight_short_waspressed == true){
         lv_obj_send_event(ui_EJECTButtonR, LV_EVENT_CLICKED, NULL);
        }
      }
      break;

      case ST_UI_SETTINGS: //Settings Menu
      {
        touch_disabled = false;

        if (encoder3.getCount() > encoder3_enc + 2) {
          int val = lv_slider_get_value(ui_brightness_slider);
          int max = lv_slider_get_max_value(ui_brightness_slider);
          if (val < max) {
            int newval = (val + 5 <= max) ? val + 5 : max;
            lv_slider_set_value(ui_brightness_slider, newval, LV_ANIM_OFF);
            M5.Display.setBrightness(newval);
          }
          encoder3_enc = encoder3.getCount();
        } else if (encoder3.getCount() < encoder3_enc - 2) {
          int val = lv_slider_get_value(ui_brightness_slider);
          int min = lv_slider_get_min_value(ui_brightness_slider);
          if (val > min) {
            int newval = (val - 5 >= min) ? val - 5 : min;
            lv_slider_set_value(ui_brightness_slider, newval, LV_ANIM_OFF);
            M5.Display.setBrightness(newval);
          }
          encoder3_enc = encoder3.getCount();
        }

        if(encoder4.getCount() > encoder4_enc + 2){
          LogDebug("next setting");
          lv_group_focus_next(ui_g_settings);
          encoder4_enc = encoder4.getCount();
        } else if(encoder4.getCount() < encoder4_enc -2){
          lv_group_focus_prev(ui_g_settings);
          LogDebug("previous setting");
          encoder4_enc = encoder4.getCount();
        }

        if(encoder3.getCount() > encoder3_enc + 2){
          LogDebug("next setting");
          lv_group_focus_next(ui_g_settings);
          encoder3_enc = encoder3.getCount();
        } else if(encoder3.getCount() < encoder3_enc -2){
          lv_group_focus_prev(ui_g_settings);
          LogDebug("previous setting");
          encoder3_enc = encoder3.getCount();
        }

        if (mxclick_long_waspressed || mxclick_double_waspressed) {
          // If a long or double MX click is detected, handle as needed (currently: do nothing)
          // Clear all mxclick flags immediately to prevent phantom events
          mxclick_short_waspressed = false;
          mxclick_long_waspressed = false;
          mxclick_double_waspressed = false;
        } else if (clickRight_short_waspressed == true) {
          lv_obj_t *focused = lv_group_get_focused(ui_g_settings);
          if (focused) {
            // If focused object is a known checkbox, toggle its state manually
            if (focused == ui_vibrate || focused == ui_lefty || focused == ui_strokeinvert || focused == ui_darkmode) {
              bool checked = lv_obj_has_state(focused, LV_STATE_CHECKED);
              if (checked) {
                lv_obj_clear_state(focused, LV_STATE_CHECKED);
              } else {
                lv_obj_add_state(focused, LV_STATE_CHECKED);
              }
              // Call the event handler for value changed if needed
              lv_obj_send_event(focused, LV_EVENT_VALUE_CHANGED, NULL);
            } else {
              lv_obj_send_event(focused, LV_EVENT_CLICKED, NULL);
            }
          }
          // Clear all right click flags immediately after handling
          clickRight_short_waspressed = false;
          clickRight_long_waspressed = false;
          clickRight_double_waspressed = false;
        } else if (mxclick_short_waspressed) {
          LogDebug("mx: ST_UI_SETTINGS -> go to menu");
          lv_obj_send_event(ui_SettingsButtonM, LV_EVENT_CLICKED, NULL);
          mxclick_short_waspressed = false;
          mxclick_long_waspressed = false;
          mxclick_double_waspressed = false;
        } else if (clickLeft_short_waspressed == true) {
          lv_obj_send_event(ui_SettingsButtonL, LV_EVENT_CLICKED, NULL);
          clickLeft_short_waspressed = false;
        }
      }
      break;

      case ST_UI_MENU: // Menu screen
      {
        if(lv_obj_has_state(ui_lefty, LV_STATE_CHECKED) == 1){
          touch_disabled = true;
        }

        if(encoder4.getCount() > encoder4_enc + 2){
          lv_group_focus_next(ui_g_menu);
          encoder4_enc = encoder4.getCount();
        } else if(encoder4.getCount() < encoder4_enc - 2){
          lv_group_focus_prev(ui_g_menu);
          encoder4_enc = encoder4.getCount();
        }

        if(clickRight_short_waspressed == true){
          lv_obj_send_event(lv_group_get_focused(ui_g_menu), LV_EVENT_CLICKED, NULL);
          clickRight_short_waspressed = false;
        }
      }
      break;

      case ST_UI_ADDONS:
      {
        if(lv_obj_has_state(ui_lefty, LV_STATE_CHECKED) == 1){
          touch_disabled = true;
        }

        if(encoder4.getCount() > encoder4_enc + 2){
          lv_group_focus_next(ui_g_addons);
          encoder4_enc = encoder4.getCount();
        } else if(encoder4.getCount() < encoder4_enc - 2){
          lv_group_focus_prev(ui_g_addons);
          encoder4_enc = encoder4.getCount();
        }

        if(clickLeft_short_waspressed == true){
          _ui_screen_change(ui_Menu, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
          clickLeft_short_waspressed = false;
        } else if(clickRight_short_waspressed == true){
          lv_obj_send_event(lv_group_get_focused(ui_g_addons), LV_EVENT_CLICKED, NULL);
          clickRight_short_waspressed = false;
        }
      }
      break;

      case ST_UI_STREAMING: // Streaming screen
      {
        if (g_streaming_entry_flow_pending) {
          handleStreamingEntryFlow();
          if (lv_scr_act() != ui_Streaming) {
            break;
          }
        }

        if (g_streaming_controls_locked) {
          encoder1.setCount(0);
          encoder2.setCount(0);
          encoder3.setCount(0);
          break;
        }

        static int lastStreamingTransportMode = -1;
        int currentTransportMode = OssmBleIsMode() ? 1 : 0;
        if (lastStreamingTransportMode != currentTransportMode) {
          lv_slider_set_max_value(ui_streamingspeedslider, 100);
          lv_slider_set_max_value(ui_streamingdepthslider, 100);
          lv_slider_set_max_value(ui_streamingstrokeslider, 100);

          if (speed > lv_slider_get_max_value(ui_streamingspeedslider)) {
            speed = lv_slider_get_max_value(ui_streamingspeedslider);
          }
          if (depth > lv_slider_get_max_value(ui_streamingdepthslider)) {
            depth = lv_slider_get_max_value(ui_streamingdepthslider);
          }
          if (stroke > lv_slider_get_max_value(ui_streamingstrokeslider)) {
            stroke = lv_slider_get_max_value(ui_streamingstrokeslider);
          }

          lastStreamingTransportMode = currentTransportMode;
        }

        if(lv_obj_has_state(ui_lefty, LV_STATE_CHECKED) == 1){
          touch_disabled = true;
        }

        // Encoder 1 Speed
        if(lv_slider_is_dragged(ui_streamingspeedslider) == false){
          changed = false;
          lv_slider_set_value(ui_streamingspeedslider, speed, LV_ANIM_OFF);

          streamingSpeedEnc = encoder1.getCount();
          int speedDetents = (int)(streamingSpeedEnc / 2);
          if(speedDetents != 0){
            changed = true;
            speed += getRampedDetentDelta(1, speedDetents);
            encoder1.setCount(streamingSpeedEnc % 2);
            markEncoderActivityForMxFilter();
          }
          if(speed < 0){ changed = true; speed = 0; }
          const float streamSpeedMax = 100.0f;
          if(speed > streamSpeedMax){ changed = true; speed = streamSpeedMax; }
          if(changed){ SendCommand(SPEED, speed, OSSM_ID); }
        } else if(lv_slider_get_value(ui_streamingspeedslider) != speed){
          speed = lv_slider_get_value(ui_streamingspeedslider);
          SendCommand(SPEED, speed, OSSM_ID);
        }
        char speed_v[12];
        if(OssmBleIsMode()){
          snprintf(speed_v, sizeof(speed_v), "%d", (int)(speed + 0.5f));
        } else {
          dtostrf(speed, 6, 0, speed_v);
        }
        lv_label_set_text(ui_streamingspeedvalue, speed_v);

        // Encoder 2 Depth
        if(lv_slider_is_dragged(ui_streamingdepthslider) == false){
          changed = false;
          lv_slider_set_value(ui_streamingdepthslider, depth, LV_ANIM_OFF);

          streamingDepthEnc = encoder2.getCount();
          int depthDetents = (int)(streamingDepthEnc / 2);
          if(depthDetents != 0){
            changed = true;
            depth += getRampedDetentDelta(2, depthDetents);
            encoder2.setCount(streamingDepthEnc % 2);
            markEncoderActivityForMxFilter();
          }
          if(depth < 0){ changed = true; depth = 0; }
          const float streamDepthMax = 100.0f;
          if(depth > streamDepthMax){ changed = true; depth = streamDepthMax; }
          if(changed){ SendCommand(DEPTH, depth, OSSM_ID); }
        } else if(lv_slider_get_value(ui_streamingdepthslider) != depth){
          depth = lv_slider_get_value(ui_streamingdepthslider);
          SendCommand(DEPTH, depth, OSSM_ID);
        }
        char depth_v[12];
        if(OssmBleIsMode()){
          snprintf(depth_v, sizeof(depth_v), "%d", (int)(depth + 0.5f));
        } else {
          dtostrf(depth, 6, 0, depth_v);
        }
        lv_label_set_text(ui_streamingdepthvalue, depth_v);

        // Encoder 3 Stroke
        if(lv_slider_is_dragged(ui_streamingstrokeslider) == false){
          changed = false;
          lv_slider_set_value(ui_streamingstrokeslider, stroke, LV_ANIM_OFF);

          streamingStrokeEnc = encoder3.getCount();
          int strokeDetents = (int)(streamingStrokeEnc / 2);
          if(strokeDetents != 0){
            changed = true;
            stroke += getRampedDetentDelta(3, strokeDetents);
            encoder3.setCount(streamingStrokeEnc % 2);
            markEncoderActivityForMxFilter();
          }
          if(stroke < 0){ changed = true; stroke = 0; }
          const float streamStrokeMax = 100.0f;
          if(stroke > streamStrokeMax){ changed = true; stroke = streamStrokeMax; }
          if(stroke > depth){ changed = true; stroke = depth; }
          if(changed){ SendCommand(STROKE, stroke, OSSM_ID); }
        } else if(lv_slider_get_value(ui_streamingstrokeslider) != stroke){
          stroke = lv_slider_get_value(ui_streamingstrokeslider);
          SendCommand(STROKE, stroke, OSSM_ID);
        }
        char stroke_v[12];
        if(OssmBleIsMode()){
          snprintf(stroke_v, sizeof(stroke_v), "%d", (int)(stroke + 0.5f));
        } else {
          dtostrf(stroke, 6, 0, stroke_v);
        }
        lv_label_set_text(ui_streamingstrokevalue, stroke_v);

        if(mxclick_short_waspressed == true){
         LogDebug("mx: ST_UI_STREAMING -> direct StreamingButtonM action");
         streamingbuttonm_action(true);
         mxclick_short_waspressed = false;
        } else if(clickRight_short_waspressed == true){
         lv_obj_send_event(ui_StreamingButtonR, LV_EVENT_CLICKED, NULL);
         clickRight_short_waspressed = false;
        }
      }
      break;

     }
     mxclick_short_waspressed = false;
     clickLeft_short_waspressed = false;
    clickLeft_double_waspressed = false;
     clickRight_short_waspressed = false;
     clickRight_long_waspressed = false;
     clickRight_double_waspressed = false;

  vTaskDelay(pdMS_TO_TICKS(5));
}

void espNowRemoteTask(void *pvParameters)
{
  for(;;){
    if(Ossm_paired && !OssmBleIsMode()){
      outgoingcontrol.esp_command = HEARTBEAT;
      outgoingcontrol.esp_heartbeat = true;
      outgoingcontrol.esp_target = OSSM_ID;
      esp_now_send(OSSM_Address, (uint8_t *) &outgoingcontrol, sizeof(outgoingcontrol));
    }
    vTaskDelay(HEARTBEAT_INTERVAL);
  }
}


void mxclick() {
  vibrate();
  mxclick_short_waspressed = true;
  screensaver_check_activity(); // Reset screensaver timer on MX click
} 

void mxdouble() {
  vibrate();
  mxclick_double_waspressed = true;
  screensaver_check_activity(); // Reset screensaver timer on MX double click
} 

void mxlong() {
  vibrate();
  mxclick_long_waspressed = true;
  screensaver_check_activity(); // Reset screensaver timer on MX long click
} 

void clickLeft() {
  vibrate();
  clickLeft_short_waspressed = true;
  screensaver_check_activity(); // Reset screensaver timer on left click
} // clickLeft

void clickLeftDouble() {
  vibrate();
  clickLeft_double_waspressed = true;
  screensaver_check_activity();
}

void clickRight() {
  vibrate();
  clickRight_short_waspressed = true;
  screensaver_check_activity(); // Reset screensaver timer on right click
  LogDebug("clickRight_short_waspressed set to true");
} // clickRight

void clickRightLong() {
  vibrate();
  clickRight_long_waspressed = true;
  screensaver_check_activity(); // Reset screensaver timer on right long click
}

void clickRightDouble() {
    vibrate();
  clickRight_double_waspressed = true;
  screensaver_check_activity(); // Reset screensaver timer on right double click
}