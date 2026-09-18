#pragma once

// AP-v2.h — "Advanced Penetration v2" addon.
//
// A second, simpler advanced-penetration controller modelled after the
// RADR/OSSM "simple penetration" screen (rail graph, direct encoders, menu +
// pause buttons), but using the M5's four rotary encoders. It talks to the
// same OSSM advanced-penetration module as AP-mode (via the ADVANCED_CONTROL
// characteristic), so the core only ever touches it through this header.

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
extern const int APV2_ID;
#ifdef __cplusplus
}
#endif

#include "ButtonEvents.h"

typedef struct _lv_obj_t lv_obj_t;

#ifdef __cplusplus
extern "C" {
void APV2ModeHandleScreen(const struct ButtonEvents *events);
}

void APV2ModePrepareScreen();
lv_obj_t *APV2ModeGetScreen();
bool APV2ModeOwnsActiveScreen();
void APV2ModeHandleScreen(const ButtonEvents &events);
void APV2ModeSetAddonEnabled(bool enabled);
bool APV2ModeIsAddonEnabled();
lv_obj_t *APV2ModeGetBatteryTitleLabel();
lv_obj_t *APV2ModeGetBatteryValueLabel();
lv_obj_t *APV2ModeGetBatteryBar();
#else
void APV2ModePrepareScreen();
lv_obj_t *APV2ModeGetScreen();
bool APV2ModeOwnsActiveScreen();
void APV2ModeHandleScreen(const struct ButtonEvents *events);
void APV2ModeSetAddonEnabled(bool enabled);
bool APV2ModeIsAddonEnabled();
lv_obj_t *APV2ModeGetBatteryTitleLabel();
lv_obj_t *APV2ModeGetBatteryValueLabel();
lv_obj_t *APV2ModeGetBatteryBar();
#endif
