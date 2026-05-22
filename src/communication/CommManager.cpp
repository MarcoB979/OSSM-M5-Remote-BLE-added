#include "CommManager.h"

#include "EspNowComm.h"
#include "BleComm.h"
#include "../config_ids.h"
#include "../debug.h"
#include "../main.h"
#include "../screens/ScreenHandler.h"
#include "../ui/ui.h"

namespace {

static CommTransportMode g_mode = COMM_MODE_NONE;

static void setMode(CommTransportMode mode) {
  g_mode = mode;
  bleCommSetEnabled(mode == COMM_MODE_BLE);
  if (mode == COMM_MODE_BLE) {
    speedlimit = 100.0f;
    maxdepthinmm = 100.0f;
  }
}

static bool tryEspNowFastConnect() {
  for (int i = 0; i < 6; ++i) {
    espNowKickPairing();
    delay(80);
    if (espNowIsPaired()) return true;
  }
  return false;
}

}  // namespace

void commInit() {
  setMode(COMM_MODE_NONE);
  bleCommInit();
}

CommTransportMode commGetMode() {
  if (g_mode == COMM_MODE_ESPNOW && !espNowIsPaired()) {
    setMode(COMM_MODE_NONE);
  }
  if (g_mode == COMM_MODE_BLE && !bleCommIsConnected()) {
    setMode(COMM_MODE_NONE);
  }
  return g_mode;
}

bool commIsBleMode() {
  return commGetMode() == COMM_MODE_BLE;
}

bool commIsEspNowMode() {
  return commGetMode() == COMM_MODE_ESPNOW;
}

void connectbutton(lv_event_t* e) {
  (void)e;

  if (commGetMode() == COMM_MODE_ESPNOW || commGetMode() == COMM_MODE_BLE) {
    return;
  }

  if (ui_connect) lv_label_set_text(ui_connect, "Connecting...");

  // Fast path: ESP-NOW first, because it is local and handshake is quick.
  if (tryEspNowFastConnect()) {
    setMode(COMM_MODE_ESPNOW);
    if (ui_connect) lv_label_set_text(ui_connect, "Connected (ESP-NOW)");
    return;
  }

  // Fallback: RADR-compatible BLE connection.
  if (bleCommTryConnect()) {
    setMode(COMM_MODE_BLE);
    if (ui_connect) lv_label_set_text(ui_connect, "Connected (BLE)");
    lv_scr_load_anim(ui_Home, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0, false);
    return;
  }

  if (ui_connect) lv_label_set_text(ui_connect, "No OSSM found");
}

bool SendCommand(int Command, float Value, int Target) {
  (void)Target;

  CommTransportMode mode = commGetMode();
  if (mode == COMM_MODE_NONE) {
    if (espNowIsPaired()) {
      setMode(COMM_MODE_ESPNOW);
      mode = COMM_MODE_ESPNOW;
    } else if (bleCommIsConnected()) {
      setMode(COMM_MODE_BLE);
      mode = COMM_MODE_BLE;
    } else {
      return false;
    }
  }

  if (mode == COMM_MODE_ESPNOW) {
    return espNowSendCommand(Command, Value, OSSM_ID);
  }

  if (mode == COMM_MODE_BLE) {
    return bleCommSendAppCommand(Command, Value, speed, depth, stroke, maxdepthinmm, speedlimit);
  }

  return false;
}
