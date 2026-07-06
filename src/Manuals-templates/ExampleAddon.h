#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
extern const int ExampleAddon_ID;
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
void ExampleAddonHandleScreen(const struct ButtonEvents *events);
}

void ExampleAddonPrepareScreen();
lv_obj_t *ExampleAddonGetScreen();
void ExampleAddonHandleScreen(const ButtonEvents &events);
#else
void ExampleAddonPrepareScreen();
lv_obj_t *ExampleAddonGetScreen();
void ExampleAddonHandleScreen(const struct ButtonEvents *events);
#endif

bool ExampleAddonHandleIncomingEspNowFrame(const uint8_t *mac,
                                           int target,
                                           int sender,
                                           int command,
                                           float value,
                                           bool heartbeat);

bool ExampleAddonSendCommand(int command, float value);
void ExampleAddonToggle();
bool ExampleAddonIsPaired();
void ExampleAddonSetAddonEnabled(bool enabled);
lv_obj_t *ExampleAddonGetBatteryTitleLabel();
lv_obj_t *ExampleAddonGetBatteryValueLabel();
