#pragma once

#include <stdint.h>

struct ButtonEvents;

void EjectHandleScreen(const ButtonEvents &events);
bool EjectHandleIncomingEspNowFrame(const uint8_t *mac,
                                    int target,
                                    int sender,
                                    int command,
                                    float value,
                                    bool heartbeat);
bool EjectSendCommand(int command, float value);
bool EjectIsPaired();
void EjectSetAddonEnabled(bool enabled);
