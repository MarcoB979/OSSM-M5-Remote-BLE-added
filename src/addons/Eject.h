#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
extern const int EJECT_ID;
void EjectUiScreenCreate(void);
#ifdef __cplusplus
}
#endif

#ifndef ADDON_BUTTON_EVENTS_DEFINED
#define ADDON_BUTTON_EVENTS_DEFINED
struct ButtonEvents {
    bool leftShort;
    bool mxShort;
    bool rightShort;
};
#endif
typedef struct _lv_obj_t lv_obj_t;

#ifdef __cplusplus
extern "C" {
void EjectHandleScreen(const struct ButtonEvents *events);
}

void EjectPrepareScreen();
lv_obj_t *EjectGetScreen();
void EjectHandleScreen(const ButtonEvents &events);
#else
void EjectPrepareScreen();
lv_obj_t *EjectGetScreen();
void EjectHandleScreen(const struct ButtonEvents *events);
#endif

bool EjectHandleIncomingEspNowFrame(const uint8_t *mac,
                                     int target,
                                     int sender,
                                     int command,
                                     float value,
                                     bool heartbeat);

bool EjectSendCommand(int command, float value);
bool EjectTryConnectNow();
void EjectToggle();        // Toggle on/off (uses internal state)
bool EjectIsPaired();
void EjectSetAddonEnabled(bool enabled);
const uint8_t* EjectGetTxAddress();
bool EjectEnsureTxPeer();
lv_obj_t *EjectGetBatteryTitleLabel();
lv_obj_t *EjectGetBatteryValueLabel();
