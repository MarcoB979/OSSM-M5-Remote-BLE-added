#pragma GCC optimize ("Ofast")
#include <M5Unified.h>
#include <ESP32Encoder.h>
#include <esp_now.h>
#include <WiFi.h>
#include <PatternMath.h>
#include "OneButton.h"          //For Button Debounce and Longpress
#include "config.h"
#include <Arduino.h>
#include <Wire.h>
#include <lvgl.h>
#include <SPI.h>
#include "ui/ui.h"
#include "main.h"
#include "Preferences.h"      //EEPROM replacement function
#include "OssmBLE.h"
#include "language.h"
#include <esp_timer.h>

constexpr int32_t HOR_RES=320;
constexpr int32_t VER_RES=240;

///////////////////////////////////////////
////
////  To Debug or not to Debug
////
///////////////////////////////////////////

// Uncomment the following line if you wish to print DEBUG info
//#define DEBUG 

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

// Screens 

#define ST_UI_START 0
#define ST_UI_HOME 1

#define ST_UI_MENUE 10
#define ST_UI_PATTERN 11
#define ST_UI_Torqe 12
#define ST_UI_EJECTSETTINGS 13

#define ST_UI_SETTINGS 20

int st_screens = ST_UI_START;



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

// EEPROM replacement function using Non-volatie memory (NVS)
Preferences m5prf; //initiate an instance of the Preferences library

bool eject_status = false;
bool dark_mode = false;
bool vibrate_mode = true;
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
long torqe_f_enc = 0;
long torqe_r_enc = 0;
long cum_t_enc = 0;
long cum_si_enc =0;
long cum_s_enc = 0;
long cum_a_enc = 0;
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
float unpause_speed = 0.0;

unsigned long nowMs;
unsigned long rampMs = 0;
bool rampEnabled = true;
int rampValue = 1;
int rampTime = 0; //75;
int maxRamp = 1; //8;
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
bool OSSM_On = false;

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
  OssmBleReadyState readyState = OssmBleUpdateReadyState(forceRefresh);
  if (readyState == OssmBleReadyState::Ready) {
    lv_label_set_text(ui_connect, "BLE");
    lv_scr_load_anim(ui_Home, LV_SCR_LOAD_ANIM_FADE_ON,20,0,false);
    return;
  }

  if (readyState == OssmBleReadyState::WaitingForStrokeEngine) {
    lv_label_set_text(ui_Welcome, T_HOMING);
    lv_label_set_text(ui_connect, T_HOMING);
  }
}

// Makes vibration motor go Brrrrr
void vibrate(int vbr_Intensity = 200, int vbr_Length = 100){
    if(lv_obj_has_state(ui_vibrate, LV_STATE_CHECKED) == 1){
      M5.Power.setVibration(vbr_Intensity);
      vTaskDelay(pdMS_TO_TICKS(vbr_Length));
      M5.Power.setVibration(0);
    }
}

void mxclick();
bool mxclick_short_waspressed = false;
void click2();
bool click2_short_waspressed = false;
void click3();
bool click3_short_waspressed = false;
void c3long();
bool click3_long_waspressed = false;
void c3double();
bool click3_double_waspressed = false;

// Home-only MX anti-ghost filter: ignore very fast repeated physical MX clicks.
static unsigned long mx_last_home_action_ms = 0;
static constexpr unsigned long MX_HOME_MIN_GAP_MS = 220;
static unsigned long mx_suppress_until_ms = 0;
static constexpr unsigned long MX_SUPPRESS_AFTER_ENCODER_MS = 300;
static constexpr unsigned long MX_MIN_RELEASE_STABLE_MS = 45; // Minimum time the MX must be stably released before accepting a new click, to filter out ghost clicks from contact bounce or partial presses. Set empirically based on measurements, and should be longer if the physical button is worn.
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

  if (mx_release_since_ms == 0 || (now - mx_release_since_ms) < MX_MIN_RELEASE_STABLE_MS) {
    LogDebugPrioFormatted("mx: ST_UI_HOME -> filtered unstable release (stable=%lu ms)\n",
      mx_release_since_ms == 0 ? 0UL : (now - mx_release_since_ms));
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

void my_touchpad_read(lv_indev_t * drv, lv_indev_data_t * data) {
  M5.update();
  auto count = M5.Touch.getCount();

  if(touch_disabled != true){
    if ( count == 0 ) {
      data->state = LV_INDEV_STATE_RELEASED;
    } else {
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

  if (target == ui_StartButtonL || target == ui_HomeButtonL || target == ui_MenueButtonL ||
      target == ui_PatternButtonL || target == ui_TorqeButtonL || target == ui_EJECTButtonL ||
      target == ui_SettingsButtonL) {
    buttonName = "left";
  } else if (target == ui_StartButtonM || target == ui_HomeButtonM || target == ui_MenueButtonM ||
             target == ui_PatternButtonM || target == ui_TorqeButtonM || target == ui_EJECTButtonM ||
             target == ui_SettingsButtonM) {
    buttonName = "mx";
  } else if (target == ui_StartButtonR || target == ui_HomeButtonR || target == ui_MenueButtonR ||
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
    ui_MenueButtonL, ui_MenueButtonM, ui_MenueButtonR,
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
    OSSM_On = false;
    }
    break;
    case ON:
    {
    OSSM_On = true;
    }
    break;
    }
}

//Sends Commands and Value to Remote device returns ture or false if sended
bool SendCommand(int Command, float Value, int Target){
  // Immediately update local paused/running state when sending ON/OFF
  if (Command == OFF) {
    OSSM_On = false;
    LogDebug("Local OSSM_On set to false (sent OFF)");
  } else if (Command == ON) {
    OSSM_On = true;
    LogDebug("Local OSSM_On set to true (sent ON)");
  }
  if(Target == OSSM_ID && OssmBleIsMode()){
    String response;
    bool ok = OssmBleSendAppCommand(
      Command,
      Value,
      speed,
      depth,
      stroke,
      OSSM_On,
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
          OssmBleBeginConnectFlow();
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

void savesettings(lv_event_t * e)
{

  m5prf.begin("m5-ctnr", false); //open NVS-storage container/session. False means that it's used it in read+write mode. Set true to open or create the namespace in read-only mode.

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

	if(lv_obj_has_state(ui_ejectaddon, LV_STATE_CHECKED) == 1){
    m5prf.putBool("ejectAddon", true);  
	}else if(lv_obj_has_state(ui_ejectaddon, LV_STATE_CHECKED) == 0){
    m5prf.putBool("ejectAddon", false);
	}

  //read darkmode saved setting to force reboot for theme change
  bool theme_Change_Previous = false;
  bool theme_Change_New = false;
  theme_Change_Previous = m5prf.getBool("Darkmode", true);

  if(lv_obj_has_state(ui_darkmode, LV_STATE_CHECKED) == 1){
    theme_Change_New = true;
    m5prf.putBool("Darkmode", true);
	}else if(lv_obj_has_state(ui_darkmode, LV_STATE_CHECKED) == 0){
    theme_Change_New = false;
    m5prf.putBool("Darkmode", false);
	}

  m5prf.end(); //close storage container/session.

  if(theme_Change_Previous != theme_Change_New){
    vibrate(225,75);
    ESP.restart(); //reboot is only required to change themes, you don't need to restart for settings to save with NVS.
  }else{
    vibrate(225,75);
  }
}

void screenmachine(lv_event_t * e)
{
  if (lv_scr_act() == ui_Start){
    st_screens = ST_UI_START;
  } else if (lv_scr_act() == ui_Home){
    st_screens = ST_UI_HOME;
    speed = lv_slider_get_value(ui_homespeedslider);
    LogDebug(speedenc);
    LogDebug(speed);
    // Clear home control encoders when entering Home to avoid carry-over from other screens.
    encoder1.setCount(0);
    encoder2.setCount(0);
    encoder3.setCount(0);
    // Clear encoder4 as well to avoid leftover torque/torque-r counts affecting Sensation.
    encoder4.setCount(0);

    if (OssmBleIsMode()) {
      lv_slider_set_range(ui_homespeedslider, 0, 100);
      lv_slider_set_range(ui_homedepthslider, 0, 100);
      lv_slider_set_range(ui_homestrokeslider, 0, 100);
    } else {
      lv_slider_set_range(ui_homespeedslider, 0, speedlimit);
      lv_slider_set_range(ui_homedepthslider, 0, maxdepthinmm);
      lv_slider_set_range(ui_homestrokeslider, 0, maxdepthinmm);
    }

            
  } else if (lv_scr_act() == ui_Menue){
    st_screens = ST_UI_MENUE;
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
  }
}

void homebuttonLevent(lv_event_t * e){
  //Pullout button
  lv_obj_clear_state(ui_HomeButtonL, LV_STATE_CHECKED);
  if (eject_status == false) {
    // No eject addon: button does a pullout manoeuvre
    // Ensure remote sees an immediate stop and paused state.
    if (OssmBleIsMode()) {
      depth = 0;
      stroke = 0;
      OssmBleExecutePulloutStop(speed, maxdepthinmm, speedlimit);
      OSSM_On = false;
      screenmachine(e);
      EJECT_On = true;
    } else {
      depth = 0;
      stroke = 0;
      SendCommand(SETUP_D_I, 0.0, OSSM_ID);
      SendCommand(DEPTH, depth, OSSM_ID);
      SendCommand(STROKE, stroke, OSSM_ID);
      OSSM_On = false;
      screenmachine(e);
      EJECT_On = true;
    }


  } 
  //FROM HERE EJECT CODE COULD BE INSERTED IF THE ADDON IS PRESENT, FOR NOW IT JUST TOGGLES THE EJECT STATE
  else {
    // Eject addon enabled: toggle eject/cumpump
    if (EJECT_On == true) {
      EJECT_On = false;
      //SendCommand(SETUP_D_I, 0.0, OSSM_ID);  // retract
    } else {
      EJECT_On = true;
      //SendCommand(SETUP_D_I_F, 0.0, OSSM_ID); // eject forward
    }
  }
}

void savepattern(lv_event_t * e){
  pattern = lv_roller_get_selected(ui_PatternS);
  lv_roller_get_selected_str(ui_PatternS,patternstr,0);
  lv_label_set_text(ui_HomePatternLabel,patternstr);
  LogDebug(pattern);
  float patterns = pattern;
  SendCommand(PATTERN, patterns, OSSM_ID);
}

static void homebuttonm_action(bool fromPhysicalMx = false)
{
  if (fromPhysicalMx && !filterMxPhysicalHomeClick()) {
    return;
  }

  // BLE mode: toggle paused state when on Home screen.
  if (OssmBleIsMode() && st_screens == ST_UI_HOME) {
    switch (OssmBleHandleHomeToggle(OSSM_On, speed)) {
      case OssmBleHomeToggleResult::Paused:
        OSSM_On = false;
        LogDebugFormatted("Sent BLE pause command with paused speed: %f\n", speed);
        break;
      case OssmBleHomeToggleResult::Resumed:
        OSSM_On = true;
        LogDebugFormatted("Sent BLE resume command (speed:%f)\n", OssmBleGetUnpauseSpeed());
        break;
      case OssmBleHomeToggleResult::Started:
        OSSM_On = true;
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
  if(OSSM_On == false){
    if (OssmBleIsMode()) {
      OssmBleGoToStrokeEngine();
    }
    SendCommand(ON, 0, OSSM_ID);
    if (OssmBleIsMode()) {
      OSSM_On = true;
    }
  } else if(OSSM_On == true){
    SendCommand(OFF, 0, OSSM_ID);
    if (OssmBleIsMode()) {
      OSSM_On = false;
    }
  }
}

void homebuttonmevent(lv_event_t * e){
  (void)e;
  LogDebugPrio("HomeButton (touch/LVGL)");
  homebuttonm_action(false);
}

void setupDepthInter(lv_event_t * e){
    SendCommand(SETUP_D_I, 0, OSSM_ID);
}

void setupdepthF(lv_event_t * e){
    SendCommand(SETUP_D_I_F, 0, OSSM_ID);
}

void setup(){
  Serial.begin(115200);
  vTaskDelay(pdMS_TO_TICKS(50));
  Serial.println("\n\nBooting...");
  auto cfg = M5.config();
  M5.begin(cfg);
  LogDebugFormatted("MX boot raw pin%d=%d\n", Button1.pin(), digitalRead(Button1.pin()));

  m5prf.begin("m5-ctnr", false); 
    // Loads these settings at boot
    eject_status = m5prf.getBool("ejectAddon", false); //boolean here is used if key does not exist yet
    dark_mode = m5prf.getBool("Darkmode", true);       // ^ (basically first boot defaults, saving settings surives a re-flash!)
    vibrate_mode = m5prf.getBool("Vibrate", true);
    touch_home= m5prf.getBool("Lefty", false);       // = touchcreen. There apears to be no actual lefthanded mode anywhere
  m5prf.end();

  M5.Power.setChargeCurrent(BATTERY_CHARGE_CURRENT);
  LogDebug("\n Starting");      // Start LogDebug

  WiFi.mode(WIFI_STA);
  LogDebug(WiFi.macAddress());

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
  Button1.setDebounceMs(50);
  Button2.attachClick(click2);
  Button3.attachClick(click3);
  Button3.attachLongPressStart(c3long);
  Button3.attachDoubleClick(c3double);

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
  ui_init();  
  register_event_debug_callbacks();

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

  LogDebug("Setup complete");
}

void loop()
{

     bool changed=false;
     const int BatteryLevel = M5.Power.getBatteryLevel();
     String BatteryValue = (String(BatteryLevel, DEC) + "%");
     const char *battVal = BatteryValue.c_str();
     lv_bar_set_value(ui_Battery, BatteryLevel, LV_ANIM_OFF);
     lv_label_set_text(ui_BattValue, battVal);
     lv_bar_set_value(ui_Battery1, BatteryLevel, LV_ANIM_OFF);
     lv_label_set_text(ui_BattValue1, battVal);
     lv_bar_set_value(ui_Battery2, BatteryLevel, LV_ANIM_OFF);
     lv_label_set_text(ui_BattValue2, battVal);
     lv_bar_set_value(ui_Battery3, BatteryLevel, LV_ANIM_OFF);
     lv_label_set_text(ui_BattValue3, battVal);
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

     switch(st_screens){
      
     case ST_UI_START: //Menu With logo after boot
      {
        if (OssmBleIsMode() && OssmBleIsWaitingForReady()) {
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

        if(click2_short_waspressed == true){
         lv_obj_send_event(ui_StartButtonL, LV_EVENT_CLICKED, NULL);
        } else if(mxclick_short_waspressed == true){
         LogDebug("mx: ST_UI_START -> sending StartButtonM click");
         lv_obj_send_event(ui_StartButtonM, LV_EVENT_CLICKED, NULL);
        } else if(click3_short_waspressed == true){
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

          // If using BLE transport, poll the device for updated limits and apply them.
          if (OssmBleIsMode()) {
            float bleMaxDepth = 0.0f;
            float bleMaxSpeed = 0.0f;
            if (OssmBlePollLimits(&bleMaxDepth, &bleMaxSpeed)) {
              if (bleMaxSpeed > 0.0f) {
                speedlimit = bleMaxSpeed;
                lv_slider_set_range(ui_homespeedslider, 0, speedlimit);
              }
              if (bleMaxDepth > 0.0f) {
                maxdepthinmm = bleMaxDepth;
                lv_slider_set_range(ui_homedepthslider, 0, maxdepthinmm);
                lv_slider_set_range(ui_homestrokeslider, 0, maxdepthinmm);
              }
            }

            OssmBleMachineState bleState;
            if (OssmBleGetCurrentState(&bleState, false)) {
              if (OssmBleIsHoming(bleState)) {
                OSSM_On = false;
              } else if (OssmBleIsReadyForStrokeEngine(bleState)) {
                OSSM_On = (bleState.speed > 0);
              }
            }
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
          lv_bar_set_start_value(ui_homestrokeslider, depth - stroke, LV_ANIM_OFF);
          lv_slider_set_value(ui_homestrokeslider, depth, LV_ANIM_OFF);


          long strokeCount = encoder3.getCount();
          int strokeDetents = (int)(strokeCount / 2);
          if (strokeDetents != 0) {
            changed = true;
            stroke -= getRampedDetentDelta(3, strokeDetents);   //make left turn increase Stroke value
            encoder3.setCount(strokeCount % 2);
            markEncoderActivityForMxFilter();
          }

          //Stoke min-max bounds
          if (stroke < 0){
            changed = true;
            stroke = 0;
          }
          const float strokeMax = OssmBleIsMode() ? 100.0f : maxdepthinmm;
          if (stroke > strokeMax){
            changed = true;
            stroke = strokeMax;
          }
          
          //send stroke
          if (changed) {
            SendCommand(STROKE, stroke, OSSM_ID);
          }

        } else if(lv_slider_get_left_value(ui_homestrokeslider) != depth - stroke){
            stroke = depth - lv_slider_get_left_value(ui_homestrokeslider);
            SendCommand(STROKE, stroke, OSSM_ID);
        } else if(lv_slider_get_value(ui_homestrokeslider) != depth){
            depth = lv_slider_get_value(ui_homestrokeslider);
            SendCommand(DEPTH, depth, OSSM_ID);
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

        if(click2_short_waspressed == true){
         lv_obj_send_event(ui_HomeButtonL, LV_EVENT_CLICKED, NULL);
        } else if(mxclick_short_waspressed == true){
         // Physical MX is handled directly only on Home, where the middle action
         // controls start/stop in ESP-NOW mode and pause/resume in BLE mode.
         // Reminder for later: if Eject and Fist-It addons are reinstated and
         // physical MX is expected to trigger addon-specific actions outside Home,
         // this routing/filtering decision may need to be expanded carefully.
         LogDebug("mx: ST_UI_HOME -> direct HomeButtonM action");
         homebuttonm_action(true);
        } else if(click3_short_waspressed == true){
         lv_obj_send_event(ui_HomeButtonR, LV_EVENT_CLICKED, NULL);
        } else if(click3_long_waspressed == true){
          sensation = 0;        //reset sensation to zero
          SendCommand(SENSATION, sensation, OSSM_ID);
        }else if(click3_double_waspressed == true){
          if (dynamicStroke == false){
            dynamicStroke = true;;            /// dynamicStroke = !dynamicStroke; crashes M5 for some reason
          }else{
            dynamicStroke = false;;
          }
          if (stroke >= depth){
            stroke = depth;
          }
        }
      }
      break;

      case ST_UI_MENUE:
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

        if(click2_short_waspressed == true){
         lv_obj_send_event(ui_MenueButtonL, LV_EVENT_CLICKED, NULL);
        } else if(mxclick_short_waspressed == true){
         LogDebug("mx: ST_UI_MENUE -> sending MenueButtonM click");
         lv_obj_send_event(ui_MenueButtonM, LV_EVENT_CLICKED, NULL);
        } else if(click3_short_waspressed == true){
         lv_obj_send_event(lv_group_get_focused(ui_g_menue), LV_EVENT_CLICKED, NULL);
        } else if(click3_long_waspressed == true){
         SendCommand(REBOOT, 0, OSSM_ID);
        } 
      }
      break;

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
         if(click2_short_waspressed == true){
         lv_obj_send_event(ui_PatternButtonL, LV_EVENT_CLICKED, NULL);
        } else if(mxclick_short_waspressed == true){
         LogDebug("mx: ST_UI_PATTERN -> sending PatternButtonM click");
         lv_obj_send_event(ui_PatternButtonM, LV_EVENT_CLICKED, NULL);
        } else if(click3_short_waspressed == true){
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

         if(click2_short_waspressed == true){
         lv_obj_send_event(ui_TorqeButtonL, LV_EVENT_CLICKED, NULL);
        } else if(mxclick_short_waspressed == true){
         LogDebug("mx: ST_UI_Torqe -> sending TorqeButtonM click");
         lv_obj_send_event(ui_TorqeButtonM, LV_EVENT_CLICKED, NULL);
        } else if(click3_short_waspressed == true){
         lv_obj_send_event(ui_TorqeButtonR, LV_EVENT_CLICKED, NULL);
        }
      }
      break;

      case ST_UI_EJECTSETTINGS:
      {
        if(lv_obj_has_state(ui_lefty, LV_STATE_CHECKED) == 1){
          touch_disabled = true;
        }
        
         if(click2_short_waspressed == true){
         lv_obj_send_event(ui_EJECTButtonL, LV_EVENT_CLICKED, NULL);
        } else if(mxclick_short_waspressed == true){
         LogDebug("mx: ST_UI_EJECTSETTINGS -> sending EJECTButtonM click");
         lv_obj_send_event(ui_EJECTButtonM, LV_EVENT_CLICKED, NULL);
        } else if(click3_short_waspressed == true){
         
        }
      }
      break;

      case ST_UI_SETTINGS: //Settings Menu
      {
        touch_disabled = false;

        if(encoder4.getCount() > encoder4_enc + 2){
          LogDebug("next");
          lv_group_focus_next(ui_g_settings);
          encoder4_enc = encoder4.getCount();
        } else if(encoder4.getCount() < encoder4_enc -2){
          lv_group_focus_prev(ui_g_settings);
          LogDebug("Preview");
          encoder4_enc = encoder4.getCount();
        }

        if(click2_short_waspressed == true){
         lv_obj_send_event(ui_MenueButtonL, LV_EVENT_CLICKED, NULL);
        } else if(mxclick_short_waspressed == true){
         LogDebug("mx: ST_UI_SETTINGS -> sending MenueButtonM click");
         lv_obj_send_event(ui_MenueButtonM, LV_EVENT_CLICKED, NULL);
        } else if(click3_short_waspressed == true){
         lv_obj_send_event(ui_EJECTButtonR, LV_EVENT_CLICKED, NULL);
        }
      }
      break;

     }
     mxclick_short_waspressed = false;
     click2_short_waspressed = false;
     click3_short_waspressed = false;
     click3_long_waspressed = false;
     click3_double_waspressed = false;

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

/*
void cumscreentask(void *pvParameters)
{
  for(;;)
  {
    M5.Lcd.setTextColor(FrontColor);
    if(encoder1.getCount() != cum_s_enc)
    {
    cum_s_enc = encoder1.getCount();
    cum_speed = map(constrain(cum_s_enc,0,Encoder_MAP),0,Encoder_MAP,1000,30000);
    M5.Lcd.fillRect(199,S1Pos,85,30,BgColor);
    M5.Lcd.setCursor(200,S1Pos+progheight-5);
    M5.Lcd.print(cum_speed);
    SendCommand(CUMSPEED, cum_speed, CUM);
    }

  if(encoder2.getCount() != cum_t_enc)
    {
    cum_t_enc = encoder2.getCount();
    cum_time = map(constrain(cum_t_enc,0,Encoder_MAP),0,Encoder_MAP,0,60);
    M5.Lcd.fillRect(199,S2Pos,85,30,BgColor);
    M5.Lcd.setCursor(200,S2Pos+progheight-5);
    M5.Lcd.print(cum_time);
    SendCommand(CUMTIME, cum_time, CUM);
    }

   if(encoder3.getCount() != cum_si_enc)
    {
    cum_si_enc = encoder3.getCount();
    cum_size = map(constrain(cum_si_enc,0,Encoder_MAP),0,Encoder_MAP,0,40);
    M5.Lcd.fillRect(199,S3Pos,85,30,BgColor);
    M5.Lcd.setCursor(200,S3Pos+progheight-5);
    M5.Lcd.print(cum_size);
    SendCommand(CUMSIZE, cum_size, CUM);
    }

   if(encoder4.getCount() != cum_a_enc)
    {
    cum_a_enc = encoder4.getCount();
    cum_accel = map(constrain(cum_a_enc,0,Encoder_MAP),0,Encoder_MAP,0,20);
    M5.Lcd.fillRect(199,S4Pos,85,30,BgColor);
    M5.Lcd.setCursor(200,S4Pos+progheight-5);
    M5.Lcd.print(cum_accel);
    SendCommand(CUMACCEL, cum_accel, CUM);
    }
  vTaskDelay(100);
  }
}
*/


void mxclick() {
  vibrate();
  mxclick_short_waspressed = true;
} 

void click2() {
  vibrate();
  click2_short_waspressed = true;
} // click2

void click3() {
  vibrate();
  click3_short_waspressed = true;
} // click3

void c3long() {
    vibrate();
  click3_long_waspressed = true;
}

void c3double() {
    vibrate();
  click3_double_waspressed = true;
}