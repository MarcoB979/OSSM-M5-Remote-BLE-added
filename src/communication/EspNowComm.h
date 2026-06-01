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
#define FIST_SPEED   30
#define FIST_ROTATION 31
#define FIST_PAUSE   32
#define FIST_ACCEL   33
#define CONNECT      88
#define HEARTBEAT    99

#define CUM_ID       2
#define FIST_ID      3

//#define HEARTBEAT_INTERVAL  (5000 / portTICK_PERIOD_MS)

// ---- Shared data structure for ESP-NOW packets ----
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
  int esp_sender;
} struct_message;


// ---- Variables (defined in main.cpp until Step 2 moves them to EspNowComm.cpp) ----
extern struct_message     outgoingcontrol;
extern struct_message     incomingcontrol;
extern esp_now_peer_info_t peerInfo;
extern uint8_t            OSSM_Address[6];
extern bool               Ossm_paired;
extern bool               OSSM_On;
extern TaskHandle_t       eRemote_t;
extern int pattern;
extern float speedlimit;
extern float maxdepthinmm;

// ---- Function declarations ----
void espNowInit();
bool espNowSendCommand(int Command, float Value, int Target);
void espNowKickPairing();
bool espNowIsPaired();
bool espNowIsEjectConnected();
bool espNowIsFistConnected();
// Last-resort pairing: sweeps all 13 WiFi channels broadcasting heartbeats.
// Updates ui_connect with the current channel. Blocks until paired or all
// channels exhausted. Returns true if pairing succeeded.
// NOTE: if OSSM is found on a channel other than 1, addon communication
// (Eject / FistIT, which use channel 1) will not work until OSSM firmware
// is updated to explicitly use channel 1.
bool espNowMultiChannelSweep();
void espNowRemoteTask(void *pvParameters);
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len);
bool OssmBleIsMode();
// Implemented by communication/CommManager.cpp (transport dispatcher)
bool SendCommand(int Command, float Value, int Target);
