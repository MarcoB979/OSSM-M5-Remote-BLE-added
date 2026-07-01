#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
extern const int AP_ID;
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
void APModeHandleScreen(const struct ButtonEvents *events);
}

void APModePrepareScreen();
lv_obj_t *APModeGetScreen();
bool APModeOwnsActiveScreen();
void APModeHandleScreen(const ButtonEvents &events);
void APModeSetAddonEnabled(bool enabled);
bool APModeIsAddonEnabled();
lv_obj_t *APModeGetBatteryTitleLabel();
lv_obj_t *APModeGetBatteryValueLabel();
lv_obj_t *APModeGetBatteryBar();
#else
void APModePrepareScreen();
lv_obj_t *APModeGetScreen();
bool APModeOwnsActiveScreen();
void APModeHandleScreen(const struct ButtonEvents *events);
void APModeSetAddonEnabled(bool enabled);
bool APModeIsAddonEnabled();
lv_obj_t *APModeGetBatteryTitleLabel();
lv_obj_t *APModeGetBatteryValueLabel();
lv_obj_t *APModeGetBatteryBar();
#endif
