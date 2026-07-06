# Pull Request Draft: OSSM BLE State Telemetry Refresh (200 ms)

## Repository
OSSM official firmware

## Branch Scope
NimBLE state publishing behavior in the communication loop.

## Why this change is needed
Remote clients can poll frequently, but state payload refresh on OSSM was effectively tied to state fingerprint changes. During position-only movement, fingerprint might not change, which can leave position telemetry stale until another setting changes.

This is a safety and control-quality issue for any remote logic that depends on timely position updates.

## What changed
File changed:
- src/services/communication/nimble.cpp

Behavior changed:
1. Publish gate interval reduced from 1000 ms to 200 ms.
2. State characteristic update and notify now run on each publish tick (state changed OR interval elapsed), not only when fingerprint changes.
3. State-change log line remains conditional, so debug noise does not increase from that line.

## Exact code delta
```cpp
// before
bool timeElapsed = (currentTime - lastMessageTime) > 1000;

if (!stateChanged && !timeElapsed) {
    vTaskDelay(1);
    continue;
}
lastMessageTime = currentTime;
lastState = fingerprint;

String currentState = ossm->getCurrentState();
if (stateChanged) {
    ESP_LOGD(NIMBLE_TAG, "State changed to: %s", currentState.c_str());
    pChr->setValue(currentState);
    pChr->notify();
}

// after
bool timeElapsed = (currentTime - lastMessageTime) > 200;

if (!stateChanged && !timeElapsed) {
    vTaskDelay(1);
    continue;
}
lastMessageTime = currentTime;
lastState = fingerprint;

String currentState = ossm->getCurrentState();
if (stateChanged) {
    ESP_LOGD(NIMBLE_TAG, "State changed to: %s", currentState.c_str());
}
pChr->setValue(currentState);
pChr->notify();
```

## Expected outcome
- Position telemetry is refreshed at least every 200 ms while connected.
- Position-only movement no longer depends on unrelated parameter changes to become visible remotely.

## Risk assessment
- Increased BLE notification traffic compared with the previous 1 s effective cadence.
- Slight increase in CPU and radio duty cycle.
- Potential throughput pressure on weak links if multiple characteristics are active.

## Validation checklist
- Build and flash firmware.
- Connect remote and observe continuous position updates during motion without changing speed/depth/pattern.
- Confirm BLE stability and no regressions in command handling.
- Confirm disconnect ramp-down behavior remains unchanged.

## Session update log
- 2026-07-05: Initial PR draft created for the NimBLE 200 ms telemetry refresh change.
- 2026-07-05: Documented final implemented behavior where notify is emitted on publish tick, not only on stateChanged.

## Suggested PR title
BLE: refresh state characteristic every 200 ms for live position telemetry

## Suggested PR body
This PR updates NimBLE state publishing in the official OSSM firmware to ensure position telemetry remains live during position-only motion. The state characteristic now refreshes every 200 ms (or earlier on state change), instead of effectively waiting for fingerprint changes before notifying. This improves safety and control reliability for remotes that depend on timely position updates.
