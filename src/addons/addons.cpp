#include "addons.h"

#include <Arduino.h>

#include "AP-mode.h"
#include "AP-v2.h"
#include "Coyote.h"
#include "Eject.h"
#include "FistIT.h"
#include "ToyControl.h"
#include "addonsStreaming.h"
#include "../communication/BleComm.h"
#include "../communication/BleBackground.h"
#include "../screens/ScreenHandler.h"  // for last_activity_ms

void addonsInit() {
    // Ensure the "addons" NVS namespace + defaults exist before the read-only
    // addonsIsXxxEnabled() probes run, so they don't spam
    // "nvs_open failed: NOT_FOUND" after a flash erase.
    addonsLoadPrefs();
    // Match the original setup() order so addon start behaviour is unchanged.
    EjectSetAddonEnabled(addonsIsEjectEnabled());
    FistITSetAddonEnabled(addonsIsFistITEnabled());
    CoyoteSetAddonEnabled(addonsIsCoyoteEnabled());
    APModeSetAddonEnabled(true);
    APV2ModeSetAddonEnabled(true);
    ToyControlSetAddonEnabled(true);
}

void addonsTick() {
    // Coyote maps live OSSM rail telemetry to stim output and feeds the
    // connected device; it throttles itself internally. Add other addons'
    // background ticks here as they gain them.
    CoyoteBackgroundTick();
}

void addonsBackgroundConnect() {
    if (bleRadioIsSuspended()) return;  // do not re-init NimBLE while the portal is up
    static uint32_t s_next_probe_ms = 0;
    static uint8_t  s_probe_slot = 0;
    static constexpr uint32_t INPUT_QUIET_WINDOW_MS = 350U;

    const uint32_t nowMs = millis();
    // Only probe when recent input traffic has settled to reduce user-visible lag.
    if ((nowMs - last_activity_ms) < INPUT_QUIET_WINDOW_MS) {
        return;
    }

    if (s_next_probe_ms != 0 && (int32_t)(nowMs - s_next_probe_ms) < 0) {
        return;
    }
    s_next_probe_ms = nowMs + 5000U;  // probe at most every 5 s (each addon every ~15 s)

    // Stagger probes: at most one addon connect attempt per scheduler tick.
    // The blocking scan/connect runs in the background task, never on the UI loop.
    if (s_probe_slot == 0) {
        if (addonsIsEjectEnabled() && !EjectIsPaired()) {
            bleBackgroundRequest(BleBgJob::EjectProbe);
        }
    } else if (s_probe_slot == 1) {
        if (addonsIsFistITEnabled() && !FistITIsPaired()) {
            bleBackgroundRequest(BleBgJob::FistITProbe);
        }
    } else {
        if (addonsIsCoyoteEnabled() && !CoyoteIsPaired()) {
            bleBackgroundRequest(BleBgJob::CoyoteProbe);
        }
    }

    s_probe_slot = (uint8_t)((s_probe_slot + 1U) % 3U);
}
