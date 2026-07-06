# Change Request: Live BLE Position Telemetry Refresh (200 ms)

## Summary
This request updates BLE state publishing in official OSSM firmware so live rail position is refreshed at a fixed 200 ms interval, instead of only when state fingerprint changes.

## Problem Statement
Remote clients can poll every 20 ms, but they only receive newly updated position data when the OSSM state characteristic is refreshed.

Current behavior in NimBLE loop:
- A new state payload is created each loop.
- The state characteristic is only written and notified when fingerprint changes.
- Fingerprint excludes live position.

Result:
- Position-only movement may not produce fresh state payloads.
- Clients can observe stale position until another setting changes (speed, depth, pattern, etc.).
- This creates safety and control quality risks for position-dependent logic.

## Scope
Single-file behavioral change in official OSSM firmware:
- src/services/communication/nimble.cpp

No protocol UUID changes.
No payload schema changes.
No remote-side API changes required.

## Proposed Change
In nimbleLoop:
1. Reduce periodic telemetry gate from 1000 ms to 200 ms.
2. Publish state payload (setValue + notify) whenever publish gate opens (stateChanged OR timeElapsed).
3. Keep stateChanged logging as-is for diagnostics.

### Before
- timeElapsed threshold: 1000 ms
- setValue/notify only inside if (stateChanged)

### After
- timeElapsed threshold: 200 ms
- setValue/notify executed on each publish tick

## Exact Code Delta
File: src/services/communication/nimble.cpp

- bool timeElapsed = (currentTime - lastMessageTime) > 1000;
+ bool timeElapsed = (currentTime - lastMessageTime) > 200;

- if (stateChanged) {
-     ESP_LOGD(NIMBLE_TAG, "State changed to: %s", currentState.c_str());
-     pChr->setValue(currentState);
-     pChr->notify();
- }
+ if (stateChanged) {
+     ESP_LOGD(NIMBLE_TAG, "State changed to: %s", currentState.c_str());
+ }
+ pChr->setValue(currentState);
+ pChr->notify();

## Expected Benefits
- Position telemetry freshness improves significantly.
- Remote jog/control logic receives deterministic updates without requiring unrelated setting changes.
- Safer behavior for position-driven control and stop conditions.

## Risks and Side Effects
- Increased BLE notification traffic compared to 1 s cadence.
- Slightly higher CPU and radio activity.
- Potential congestion on weak BLE links if multiple clients are connected.

## Validation Plan
1. Build firmware for target environment(s).
2. Connect remote and log state payload position timestamps.
3. Move rail continuously without changing speed/depth/pattern.
4. Verify position values advance at roughly 200 ms cadence.
5. Confirm no regressions in command handling and disconnect ramp-down behavior.

## Rollback Plan
If BLE stability degrades:
1. Revert this single commit.
2. Restore previous 1000 ms and stateChanged-only notify behavior.
3. Re-run baseline connection and motion tests.

## Suggested PR Title
BLE: publish state at 200 ms cadence to keep position telemetry live

## Suggested PR Description (short)
This change updates NimBLE state publishing so the state characteristic is refreshed every 200 ms (or on state change), rather than only on fingerprint change. It resolves stale position telemetry during position-only motion and improves safety for remote position-dependent control logic.
