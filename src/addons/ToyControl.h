#pragma once

// ToyControl.h — dynamic on-device toy control screen (Phase 7).
// Renders a slider per feature of the selected connected toy, built from the
// toy's feature list, RADR-style.

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
extern const int TOYCONTROL_ID;
#ifdef __cplusplus
}
#endif

#include "ButtonEvents.h"

typedef struct _lv_obj_t lv_obj_t;

#ifdef __cplusplus
void ToyControlPrepareScreen();
lv_obj_t* ToyControlGetScreen();
bool ToyControlOwnsActiveScreen();
void ToyControlHandleScreen(const ButtonEvents& events);
void ToyControlSetAddonEnabled(bool enabled);
bool ToyControlIsAddonEnabled();
#else
void ToyControlPrepareScreen();
lv_obj_t* ToyControlGetScreen();
bool ToyControlOwnsActiveScreen();
void ToyControlHandleScreen(const struct ButtonEvents* events);
void ToyControlSetAddonEnabled(bool enabled);
bool ToyControlIsAddonEnabled();
#endif
