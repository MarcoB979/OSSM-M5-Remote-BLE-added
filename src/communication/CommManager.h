#pragma once

#include <lvgl.h>

enum CommTransportMode {
  COMM_MODE_NONE = 0,
  COMM_MODE_ESPNOW,
  COMM_MODE_BLE,
};

void commInit();
CommTransportMode commGetMode();
bool commIsBleMode();
bool commIsEspNowMode();

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
