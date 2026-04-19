#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
extern const int FIST_ID;
#ifdef __cplusplus
}
#endif

struct ButtonEvents;
typedef struct _lv_obj_t lv_obj_t;

#ifdef __cplusplus
extern "C" {
void FistITHandleScreen(const struct ButtonEvents *events);
}

void FistITPrepareScreen();
lv_obj_t *FistITGetScreen();
void FistITHandleScreen(const ButtonEvents &events);
#else
void FistITPrepareScreen();
lv_obj_t *FistITGetScreen();
void FistITHandleScreen(const struct ButtonEvents *events);
#endif

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
