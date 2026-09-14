# PR: Publish live rail position/speed telemetry as dedicated BLE characteristics

## Summary
`StrokeEngine` already computes a per-stroke target rail position, target
cruise speed, and the acceleration used to reach it for every pattern
(including random ones), but discards all of it. This PR wires up the
library's existing `registerTelemetryCallback()` (extending it with the
acceleration value it already computes but never forwarded) and exposes the
result as three new notify-only characteristics in the OSSM service, so BLE
remotes can finally see real rail motion instead of only the configured
min/max range.

Critically, position + speed + acceleration is enough for a remote client to
reconstruct the *exact* trapezoidal (or triangular, for short/fast strokes)
motion profile of each stroke using plain kinematics — without knowing
anything about which of the 10 patterns is running. Different patterns (e.g.
`RoboStroke`'s near-constant speed vs. `TeasingPounding`/`HalfnHalf`'s
asymmetric in/out acceleration) produce very different rail behavior, and
position+speed alone (the original version of this PR) only gives the two
endpoints of each stroke, not its shape.

## Problem statement
The BLE interface already reports the configured setpoints (speed, min/max
position, sensation, pattern) and the machine state name as individual
characteristics, plus a legacy JSON state characteristic carrying the same
values.

None of these describe the *live* rail position — `minPosition`/`maxPosition`
are the user-configured travel range, not where the rail currently is.
Remote-side features that want to react to actual motion (e.g. driving an
external device in sync with the stroke, motion-based safety cutoffs, richer
status displays) have no way to get this today.

`lib/StrokeEngine/src/StrokeEngine.cpp` already solves half of this problem
internally: every time a new stroke's trapezoidal move is applied to the
stepper (`_applyMotionProfile()`), it calls a registered telemetry callback
with `(position_mm, speed_mms, acceleration, clipping)` — the *exact* target
position, target cruise speed and acceleration of that stroke, straight from
the engine, not a guess. This is called once per stroke (not continuously),
for every pattern including `RandomStroke`. However,
`registerTelemetryCallback()` is never called anywhere in `src/`, so this
data is currently discarded.

## Proposed change
Wire up the existing (unused) telemetry callback and publish its output as
three new notify-only characteristics, following the same pattern as the
existing engine characteristics (`SENSAT_UUID`, `SENPAT_UUID`, `SENPTL_UUID`).

### 1. `src/services/communication/nimble.h`
```cpp
#define SENPTL_UUID "4f53534d-456e-6769-6e65-5061744c7374"
#define STRPOS_UUID "4f53534d-456e-6769-6e65-537472506f73"
#define STRSPD_UUID "4f53534d-456e-6769-6e65-537472537064"
#define STRACC_UUID "4f53534d-456e-6769-6e65-537472416363"
```

### 2. `src/ossm/stroke_engine/stroke_engine.cpp`
Register the callback and convert its mm-based units into the same
percent-of-travel/percent-of-max units already used by
`settings.minPosition/maxPosition`:

```cpp
    // Conversion factors for the StrokeEngine telemetry callback, which
    // reports the target position/speed/acceleration of each stroke in mm.
    // Set once per task start, alongside properties above.
    static float s_physicalTravelMm = 0.0f;
    static float s_maxSpeedMms = 0.0f;
    static float s_maxAccelMms2 = 0.0f;

    static void onStrokeTelemetry(float positionMm, float speedMms, float accelMms2, bool clipping) {
        if (s_physicalTravelMm > 0.0f) {
            liveTelemetry.positionPercent = constrain(100.0f * positionMm / s_physicalTravelMm, 0.0f, 100.0f);
        }
        if (s_maxSpeedMms > 0.0f) {
            liveTelemetry.speedPercent = constrain(100.0f * speedMms / s_maxSpeedMms, 0.0f, 100.0f);
        }
        if (s_maxAccelMms2 > 0.0f) {
            liveTelemetry.accelPercent = constrain(100.0f * accelMms2 / s_maxAccelMms2, 0.0f, 100.0f);
        }
        liveTelemetry.clipping = clipping;
        liveTelemetry.updatedAtMs = millis();
    }

    static void startStrokeEngineTask(void *pvParameters) {
        SettingPercents lastSetting = settings;
        properties = machineProperties{ ... };

        Stroker.begin(&properties, stepper);
        s_physicalTravelMm = abs(properties.physicalTravel);
        s_maxSpeedMms = properties.maxSpeed;
        s_maxAccelMms2 = properties.maxAcceleration;
        Stroker.registerTelemetryCallback(onStrokeTelemetry);
        // ... rest unchanged
    }
```

### 3. `src/services/communication/nimble.cpp`
Create the three characteristics in `initNimble()` (read + notify, no write,
same shape as the existing state characteristic) and publish them in
`nimbleLoop()` on the periodic notify gate rather than the state-change gate,
because the values change with every stroke. Reuses the existing
`notifyValue()` helper, which skips no-op notifications.

```cpp
    // Live per-stroke telemetry (position/speed/acceleration of the last
    // commanded move), published on the periodic notify gate in nimbleLoop.
    NimBLECharacteristic* strokePos = ossmService->createCharacteristic(NimBLEUUID(STRPOS_UUID), NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    strokePos->setValue("0.0");
    NimBLECharacteristic* strokeSpd = ossmService->createCharacteristic(NimBLEUUID(STRSPD_UUID), NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    strokeSpd->setValue("0.0");
    NimBLECharacteristic* strokeAcc = ossmService->createCharacteristic(NimBLEUUID(STRACC_UUID), NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    strokeAcc->setValue("0.0");
```

```cpp
        // Stroke telemetry changes with every stroke and is deliberately not
        // part of the state fingerprint, so publish it on the periodic gate.
        notifyValue(STRPOS_UUID, String(liveTelemetry.positionPercent));
        notifyValue(STRSPD_UUID, String(liveTelemetry.speedPercent));
        notifyValue(STRACC_UUID, String(liveTelemetry.accelPercent));
```

The periodic gate is reduced from 1000 ms to 200 ms so the per-stroke values
arrive promptly. The legacy JSON state characteristic is left unchanged and
remains purely event-driven (published on state change only).

## Example values
While running `SimpleStroke`, a connected client observes the three
characteristics update once per stroke:

```text
STRPOS_UUID: 31.50   (oscillates between minPosition and maxPosition)
STRSPD_UUID: 18.20   (target cruise speed, % of max)
STRACC_UUID: 42.00   (acceleration used to reach it, % of max)
```

## Scope / files touched
- `src/services/communication/nimble.h` (three new UUIDs)
- `src/services/communication/nimble.cpp` (create + publish the three
  characteristics; reduce periodic gate 1000 ms -> 200 ms)
- `src/ossm/stroke_engine/stroke_engine.cpp` (register callback, convert units)

`lib/StrokeEngine` is untouched — the 4-argument telemetry callback
(`position, speed, acceleration, clipping`) and its call sites already
exist. `src/ossm/state/telemetry.h` / `telemetry.cpp` already define the
`liveTelemetry` struct and need no change. No changes to `getCurrentState()`,
no changes to the legacy JSON payload, no changes to command parsing or
`pattern.h` pattern logic.

## Relationship to the existing "200ms cadence" change request
This repo also has a separate earlier draft
(`OSSM_LIVE_POSITION_TELEMETRY_CHANGE_REQUEST.md` /
`OSSM_TELEMETRY_NIMBLE_PULL_REQUEST.md`) proposing a periodic state publish.
Because the live telemetry now has its own characteristics, the legacy JSON
state characteristic stays purely event-driven. The periodic gate in
`nimbleLoop()` is only reused to publish the three new telemetry
characteristics.

## Risks / side effects
- Three extra notify-only characteristics are negligible for the GATT table
  and live in the same OSSM service as the existing characteristics.
- `onStrokeTelemetry()` runs on the stroking task (high priority, core 1);
  it only writes a few floats/bool to a plain struct with no locking. This
  matches the lock-free read pattern already used for other `settings`
  fields consumed elsewhere, so no new concurrency primitives are
  introduced. If stricter thread-safety is desired, the existing
  `_patternMutex`-guarded call sites already serialize writes into
  `liveTelemetry` (both call sites happen from within the stroking task),
  so a torn read from another core is limited to individual float/bool
  fields and self-corrects on the next stroke.

## Validation plan
1. Build & flash to a dev board.
2. Connect a BLE client (e.g. M5 Remote) and subscribe to the three new
   characteristics.
3. Run each pattern (`SimpleStroke`, `TeasingPounding`, `RoboStroke`,
   `HalfnHalf`, `Deeper`, `StopNGo`, `Insist`, `ProgressiveStroke`,
   `RandomStroke`, `PointStroke`) at a few speed/sensation combinations.
4. Confirm `STRPOS` visibly changes stroke-to-stroke and roughly tracks
   expected motion (e.g. oscillates between `minPosition` and `maxPosition`
   for simple patterns).
5. Confirm no regressions in existing characteristics, the legacy JSON
   state payload, command handling, or BLE connection/disconnect behavior.

## Rollback plan
Revert this single commit; the added lines are additive and do not modify
any existing behavior besides the periodic-gate cadence and the presence of
three new characteristics.

## Suggested PR title
BLE: publish live per-stroke rail position/speed/acceleration characteristics

## Suggested PR description (short)
`StrokeEngine` already computes the target position/speed/acceleration of
every stroke via its (currently unused) `registerTelemetryCallback()` hook.
This PR registers that callback and publishes its output as three new
notify-only characteristics (`STRPOS_UUID`, `STRSPD_UUID`, `STRACC_UUID`),
giving remote clients real rail motion telemetry for the first time
(previously only the configured min/max range was visible, never the live
position).
