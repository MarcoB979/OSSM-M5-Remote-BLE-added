#pragma once
#include <esp_now.h>
#include <WiFi.h>
#include <lvgl.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// ---- Command states sent over ESP-NOW ----
#define CONN         0
#define SPEED        1
#define DEPTH        2
#define STROKE       3
#define SENSATION    4
#define PATTERN      5
#define TORQE_F      6
#define TORQE_R      7
#define OFF          10
#define ON           11
#define SETUP_D_I    12
#define SETUP_D_I_F  13
#define REBOOT       14
#define CUMSPEED     20
#define CUMTIME      21
#define CUMSIZE      22
#define CUMACCEL     23
#define CONNECT      88
#define HEARTBEAT    99

#define HEARTBEAT_INTERVAL  (5000 / portTICK_PERIOD_MS)

// ---- Shared data structure for ESP-NOW packets ----
typedef struct {
  float esp_speed;
  float esp_depth;
  float esp_stroke;
  float esp_sensation;
  float esp_pattern;
  bool  esp_rstate;
  bool  esp_connected;
  bool  esp_heartbeat;
  int   esp_command;
  float esp_value;
  int   esp_target;
} struct_message;

// ---- Variables (defined in main.cpp until Step 2 moves them to EspNowComm.cpp) ----
extern struct_message     outgoingcontrol;
extern struct_message     incomingcontrol;
extern esp_now_peer_info_t peerInfo;
extern uint8_t            OSSM_Address[6];
extern bool               Ossm_paired;
extern bool               OSSM_On;
extern TaskHandle_t       eRemote_t;

// ---- Function declarations ----
bool SendCommand(int Command, float Value, int Target);
void espNowRemoteTask(void *pvParameters);
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len);
// espNowInit() will be added in Step 2 to wrap the setup() init block
