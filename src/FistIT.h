#pragma once

#include <stdint.h>

struct ButtonEvents;
typedef struct _lv_obj_t lv_obj_t;

void FistITPrepareScreen();
lv_obj_t *FistITGetScreen();
void FistITHandleScreen(const ButtonEvents &events);

bool FistITHandleIncomingEspNowFrame(const uint8_t *mac,
                                     int target,
                                     int sender,
                                     int command,
                                     float value,
                                     bool heartbeat);

bool FistITSendCommand(int command, float value);
bool FistITIsPaired();
void FistITSetAddonEnabled(bool enabled);
lv_obj_t *FistITGetBatteryTitleLabel();
lv_obj_t *FistITGetBatteryValueLabel();
