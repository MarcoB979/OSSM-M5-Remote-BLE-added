#pragma once

#include <lvgl.h>

enum CommTransportMode {
  COMM_MODE_NONE = 0,
  COMM_MODE_BLE,
};

void commInit();
CommTransportMode commGetMode();
bool commIsBleMode();

// UI callback for Start->Connect button
#ifdef __cplusplus
extern "C" {
#endif
void connectbutton(lv_event_t* e);
#ifdef __cplusplus
}
#endif

// Unified command dispatcher used by all screens
bool SendCommand(int Command, float Value, int Target);

// Unified streaming move dispatcher (BLE only): stream:<position>:<durationMs>
bool SendStreamCommand(int position, int durationMs);


// ---- Command states  ----
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
