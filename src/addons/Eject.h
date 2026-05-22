#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
extern const int EJECT_ID;
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

#ifdef __cplusplus
extern "C" {
void EjectHandleScreen(const struct ButtonEvents *events);
}

void EjectHandleScreen(const ButtonEvents &events);
#else
void EjectHandleScreen(const struct ButtonEvents *events);
#endif
bool EjectHandleIncomingEspNowFrame(const uint8_t *mac,
                                    int target,
                                    int sender,
                                    int command,
                                    float value,
                                    bool heartbeat);
bool EjectSendCommand(int command, float value);
bool EjectIsPaired();
void EjectSetAddonEnabled(bool enabled);
