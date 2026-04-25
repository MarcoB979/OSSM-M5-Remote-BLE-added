#pragma once

#include <stdint.h>

struct ButtonEvents;

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
