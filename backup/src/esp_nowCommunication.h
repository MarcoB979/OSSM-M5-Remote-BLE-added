#pragma once
#include <stdint.h>
#include "config.h"
#ifdef __cplusplus
extern "C" {
#endif

struct struct_message {
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
};

/*struct OSSMMessage {
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
};
*/


//extern bool Ossm_paired; // defined in main_helper.cpp
// Ossm_paired is declared in main.h (do not re-declare here)

// Public API for the OSSM-specific ESP-NOW addon integration.
// Implemented in esp_nowOSSM.cpp

// Returns true if the OSSM addon pairing is active
bool OSSMIsPaired();

// Enable or disable the OSSM addon handling (controls heartbeats and peer management)
void OSSMSetAddonEnabled(bool enabled);

// Send a command/value to the OSSM addon. Returns true on send success.
bool OSSMSendCommand(int command, float value, int Target);

// Handle an incoming ESPNOW frame for the addon. Called by higher-level recv path.
// Returns true if the frame was consumed by the addon.
bool OSSMHandleIncomingEspNowFrame(const uint8_t *mac, int Target, int sender, int command, float value, bool heartbeat);

#ifdef __cplusplus
}
#endif
