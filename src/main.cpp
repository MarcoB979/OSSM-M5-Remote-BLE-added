#pragma GCC optimize ("Ofast")
#include <M5Unified.h>
#include <ESP32Encoder.h>
#include <Arduino.h>
#include <Wire.h>
#include <lvgl.h>
#include <SPI.h>
#include <esp_timer.h>
#include "OneButton.h"    // must be before config.h which uses the OneButton type
#include "config.h"
#include "debug.h"
#include "main.h"
#include "ui/ui.h"
#include "buttonhandlers/ButtonHandlers.h"
#include "communication/EspNowComm.h"
#include "screens/ScreenHandler.h"

constexpr int32_t HOR_RES = 320;
constexpr int32_t VER_RES = 240;

// Shared state (defined here, declared extern in main.h)
bool dark_mode = false;

// Shared numeric limits (defined here, declared extern in main.h)
float maxdepthinmm = 400.0f;
float speedlimit   = 300.0f;

// ESP-NOW state (declared extern in communication/EspNowComm.h)
struct_message      outgoingcontrol;
struct_message      incomingcontrol;
esp_now_peer_info_t peerInfo;
uint8_t OSSM_Address[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
bool Ossm_paired = false;
bool OSSM_On     = false;
TaskHandle_t eRemote_t = nullptr;

// Encoder objects (declared extern in buttonhandlers/ButtonHandlers.h)
ESP32Encoder encoder1;
ESP32Encoder encoder2;
ESP32Encoder encoder3;
ESP32Encoder encoder4;

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
  lv_obj_t *label = reinterpret_cast<lv_obj_t *>(lv_event_get_user_data(e));

  switch (code)
  {
  case LV_EVENT_PRESSED:
    lv_label_set_text(label, "The last button event:\nLV_EVENT_PRESSED");
    break;
  case LV_EVENT_CLICKED:
    lv_label_set_text(label, "The last button event:\nLV_EVENT_CLICKED");
    break;
  case LV_EVENT_LONG_PRESSED:
    lv_label_set_text(label, "The last button event:\nLV_EVENT_LONG_PRESSED");
    break;
  case LV_EVENT_LONG_PRESSED_REPEAT:
    lv_label_set_text(label, "The last button event:\nLV_EVENT_LONG_PRESSED_REPEAT");
    break;
  default:
    break;
  }
}



void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&incomingcontrol, incomingData, sizeof(incomingcontrol));

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
      }
      else {
        LogDebug("Failed to add new peer");
      }
    }
    else {
      LogDebug("Failed to remove peer");
    }

    
    if(incomingcontrol.esp_speed > speedlimit){
      speedlimit = 300;
    } else (
    speedlimit = incomingcontrol.esp_speed);
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
      lv_label_set_text(ui_connect, "Connected");
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
  
  if(Ossm_paired == true){

    outgoingcontrol.esp_connected = true;
    outgoingcontrol.esp_command = Command;
    outgoingcontrol.esp_value = Value;
    outgoingcontrol.esp_target = Target;
    esp_err_t result = esp_now_send(OSSM_Address, (uint8_t *) &outgoingcontrol, sizeof(outgoingcontrol));
  
    if (result == ESP_OK) {
      return true;
    } else {
      delay(20);
      esp_now_send(OSSM_Address, (uint8_t *) &outgoingcontrol, sizeof(outgoingcontrol));
      return false;
    }
  }
  return false;
}

void connectbutton(lv_event_t * e)
{
    if(!Ossm_paired){
      outgoingcontrol.esp_command = HEARTBEAT;
      outgoingcontrol.esp_heartbeat = true;
      outgoingcontrol.esp_target = OSSM_ID;
      esp_err_t result = esp_now_send(OSSM_Address, (uint8_t *) &outgoingcontrol, sizeof(outgoingcontrol));
    }
}

void setup(){
  auto cfg = M5.config();
  M5.begin(cfg);

  M5.Power.setChargeCurrent(BATTERY_CHARGE_CURRENT);
  LogDebug("\n Starting");      // Start LogDebug

  WiFi.mode(WIFI_STA);
  LogDebug(WiFi.macAddress());

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
  }
  esp_now_register_send_cb(OnDataSent);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  memcpy(peerInfo.peer_addr, OSSM_Address, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
  }
  esp_now_register_recv_cb(OnDataRecv);
  LogDebug("\n esp now initialized");

  xTaskCreatePinnedToCore(espNowRemoteTask, "espNowRemoteTask", 3096, NULL, 5, &eRemote_t, 0);
  delay(200);
  LogDebug("\n esp_task created");
  
  M5.Display.setEpdMode(epd_mode_t::epd_fastest);
  if (M5.Display.width() < M5.Display.height()) {
    M5.Display.setRotation(M5.Display.getRotation() ^ 1);
  }
  LogDebug("\n display initialized");
  lv_init();
  lv_tick_set_cb(my_tick_function);
  LogDebug("\n lvgl initialized");

  display = lv_display_create(HOR_RES, VER_RES);
  lv_display_set_flush_cb(display, my_display_flush);
  // buf1 must be 4-byte aligned (LV_DRAW_BUF_ALIGN=4). lv_color_t is uint16_t,
  // so the linker may place it on a 2-byte boundary. Force alignment explicitly.
  static lv_color_t buf1[HOR_RES * 15] __attribute__((aligned(4)));
  LogDebug("\n display flush callback set");
  lv_display_set_buffers(display, buf1, nullptr, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);
  LogDebug("\n display buffers set");
  indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, my_touchpad_read);
  LogDebug("\n display and touchpad initialized");
  ui_init();
  LogDebug("\n ui initialized");

  buttonInit();
  screenInit();  // Load NVS settings and apply to UI

  LogDebug("\n End setup");
}

void loop()
{
  M5.update();
  lv_task_handler();
  Button1.tick();
  Button2.tick();
  Button3.tick();
  handleScreens();
  delay(5);
}

void espNowRemoteTask(void *pvParameters)
{
  for(;;){
    if(Ossm_paired){
      outgoingcontrol.esp_command   = HEARTBEAT;
      outgoingcontrol.esp_heartbeat = true;
      outgoingcontrol.esp_target    = OSSM_ID;
      esp_now_send(OSSM_Address, (uint8_t *) &outgoingcontrol, sizeof(outgoingcontrol));
    }
    vTaskDelay(HEARTBEAT_INTERVAL);
  }
}

// Screen event callbacks and handler moved to src/screens/ScreenHandler.cpp
// ESP-NOW functions will move to src/communication/EspNowComm.cpp in Step 2