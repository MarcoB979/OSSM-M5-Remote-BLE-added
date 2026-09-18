#pragma once

#include <stdint.h>

#include "ButtonEvents.h"
typedef struct _lv_obj_t lv_obj_t;

#ifdef __cplusplus
extern "C" {
void CoyoteHandleScreen(const struct ButtonEvents *events);
}

void CoyotePrepareScreen();
lv_obj_t *CoyoteGetScreen();
void CoyoteHandleScreen(const ButtonEvents &events);
#else
void CoyotePrepareScreen();
lv_obj_t *CoyoteGetScreen();
void CoyoteHandleScreen(const struct ButtonEvents *events);
#endif

// Connection lifecycle, mirroring Eject/FistIT's addon API shape.
bool CoyoteIsPaired();
bool CoyoteIsOn();
bool CoyoteTryConnectNow();
bool CoyoteTryConnectBackground();
void CoyoteSetAddonEnabled(bool enabled);

// Runs the OSSM-telemetry -> frequency/intensity mapping and feeds the
// connected Coyote. Call every main loop iteration regardless of which
// screen is active (like a background service), throttled internally.
void CoyoteBackgroundTick();

lv_obj_t *CoyoteGetBatteryTitleLabel();
lv_obj_t *CoyoteGetBatteryValueLabel();
