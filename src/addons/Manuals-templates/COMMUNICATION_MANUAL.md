# M5 Remote Communication Manual

## 1) Purpose and Scope

This manual documents the communication design used by the M5 Remote firmware in the `src/communication` folder, and the closely related runtime monitoring in `src/screens/ScreenHandler.cpp`.

It explains:
- How ESP-NOW and BLE are architected and selected.
- Which functions exist and what each one does.
- How the OSSM state machine is read and monitored.
- How commands are routed and sent.
- Which guards and safety mechanisms are implemented.

Audience: beginner to advanced developers.

---

## 2) High-Level Design

The firmware supports two transport layers for OSSM control:

1. ESP-NOW (fast, direct Wi-Fi MAC communication)
2. BLE (NimBLE GATT client to OSSM BLE service)

A central dispatcher (`CommManager`) decides which transport is active and routes commands.

### Core design rule
- OSSM commands: prefer BLE when connected, otherwise ESP-NOW.
- Addon commands (Eject/Fist-IT): always via ESP-NOW.

This rule is implemented in `SendCommand()` in `src/communication/CommManager.cpp`.

---

## 3) Startup Sequence

From `src/main.cpp`:

1. `espNowInit()` starts ESP-NOW stack, registers callbacks, starts heartbeat task.
2. `EjectSetAddonEnabled(true)` and `FistITSetAddonEnabled(true)` enable addon modules.
3. `commInit()` initializes communication mode management (but does not eagerly start BLE scans).
4. UI and screen systems start.
5. Main loop calls `handleScreens()` each cycle.

Important: BLE is intentionally lazy-initialized (only when needed), to reduce coexistence contention during early ESP-NOW pairing.

---

## 4) Files and Responsibilities

### 4.1 `src/communication/CommManager.h/.cpp`
Main transport coordinator.

- Tracks current mode:
  - `COMM_MODE_NONE`
  - `COMM_MODE_ESPNOW`
  - `COMM_MODE_BLE`
- Provides connection button behavior (`connectbutton`) used by Start screen.
- Provides unified command API (`SendCommand`) used by screens and addons.
- Enables/disables BLE polling based on active mode.
- Suspends/resumes ESP-NOW heartbeat task when BLE mode is entered/exited.

### 4.2 `src/communication/BleComm.h/.cpp`
BLE transport implementation and OSSM state tracking.

- NimBLE client connection management.
- GATT characteristic read/write.
- Outgoing command queue with deduplication and throttling.
- Poll task to read OSSM state at fixed intervals.
- State parser and machine-mode cache.
- Motion safety gate: ensure OSSM is ready (StrokeEngine/Streaming) before motion commands.

### 4.3 `src/communication/EspNowComm.h/.cpp`
ESP-NOW transport implementation.

- ESP-NOW initialization and callback registration.
- Pairing and heartbeat behavior.
- Incoming frame validation and routing (OSSM vs addon).
- Outgoing command send for OSSM/addons.
- Optional multi-channel sweep utility.

### 4.4 `src/communication/esp_nowCommunication.h`
Compatibility header defining command constants and fallback macro values.

### 4.5 `src/communication/EjectComm.*` and `src/communication/FistComm.*`
Helper modules for addon frame/peer handling.

Current codebase status:
- Present and implemented.
- Not actively used by `EspNowComm.cpp` in the current transport path (addon routing is done directly via addon handlers).

---

## 5) IDs, Commands, and Packet Model

## 5.1 Device IDs
From `src/config/config_ids.h` and communication headers:

- `OSSM_ID = 1`
- `M5_ID = 99`
- `CUM_ID = 2`
- `FIST_ID = 3`

## 5.2 Command IDs
Defined in `src/communication/EspNowComm.h` (and mirrored in compatibility header):

- Core OSSM: `CONN`, `SPEED`, `DEPTH`, `STROKE`, `SENSATION`, `PATTERN`, `TORQE_F`, `TORQE_R`, `OFF`, `ON`, `SETUP_D_I`, `SETUP_D_I_F`, `REBOOT`, `CONNECT`, `HEARTBEAT`
- Eject addon: `CUMSPEED`, `CUMTIME`, `CUMSIZE`, `CUMACCEL`
- Fist addon: `FIST_SPEED`, `FIST_ROTATION`, `FIST_PAUSE`, `FIST_ACCEL`

## 5.3 ESP-NOW packet struct
`struct_message` fields:

- Motion/state floats: `esp_speed`, `esp_depth`, `esp_stroke`, `esp_sensation`, `esp_pattern`
- Flags: `esp_rstate`, `esp_connected`, `esp_heartbeat`
- Command payload: `esp_command`, `esp_value`
- Routing: `esp_target`, `esp_sender`

Backward compatibility:
- `OnDataRecv` supports legacy OSSM packet length without `esp_sender` by zero-initializing and copying only received bytes.

---

## 6) Connection Flow (User presses Connect)

Function: `connectbutton(lv_event_t*)` in `src/communication/CommManager.cpp`

Flow:

1. If already connected (BLE or ESP-NOW mode), return immediately.
2. Show UI text: searching ESP-NOW.
3. Tier 1: fast ESP-NOW pairing (`tryEspNowFastConnect`):
   - Up to 5 iterations: send pairing heartbeat, wait 500 ms, check paired.
   - If paired: set mode ESP-NOW, update UI, load menu screen.
4. Tier 2: BLE fallback:
   - Show BLE search text.
   - `bleCommInit()` and `bleCommTryConnect()`.
   - If connected: set mode BLE, update UI, load menu screen.
5. If both fail:
   - Current active behavior: show "Connection failed... try again".

Notes:
- UI refresh inside callback uses `lv_refr_now(NULL)` (safe immediate refresh in LVGL callback context).
- A more advanced sweep block exists but is currently commented out in active connect path.

---

## 7) Transport Selection and Command Dispatch

Function: `SendCommand(int Command, float Value, int Target)` in `src/communication/CommManager.cpp`

Decision order:

1. If target is Eject (`CUM` or `CUM_ID`): route to `EjectSendCommand`.
2. If target is Fist (`FIST_ID`): route to `FistITSendCommand`.
3. Otherwise (OSSM):
   - If BLE connected: use `bleCommSendAppCommand`.
   - Else if ESP-NOW paired: use `espNowSendCommand`.
   - Else try to recover mode from actual connectivity (`commGetMode` + checks).

Why this is important:
- Addon communication continues over ESP-NOW even when OSSM control uses BLE.
- Prevents addon command path from being blocked by BLE OSSM mode.

---

## 8) BLE Design Details

## 8.1 BLE service/characteristics
In `BleComm.cpp`:

- Service UUID: `522b443a-4f53-534d-0001-420badbabe69`
- Command characteristic: `...-1000-...`
- Speed knob characteristic: `...-1010-...`
- State characteristic: `...-2000-...`

## 8.2 BLE connection (`bleCommTryConnect`)

Main steps:

1. Ensure BLE runtime is initialized (`bleCommInit`).
2. Use connection mutex to prevent parallel scan/connect collisions.
3. Enforce connect cooldown (`BLE_CONNECT_COOLDOWN_MS = 1200`).
4. Scan (active) for name match (`OSSM`, `OSSM-REMOTE`) or service UUID match.
5. Create/connect NimBLE client.
6. Discover required characteristics.
7. Configure speed knob characteristic to "false" (non-independent mode).
8. Subscribe to state notifications.
9. Read state once to seed state cache.

Connection success criterion:
- Returns true if initial state read succeeds or connection is otherwise valid.

## 8.3 BLE runtime tasks

### Poll task (`blePollTask`)
- Runs every `BLE_STATE_POLL_MS = 20` ms.
- If BLE mode enabled and connected, reads state characteristic (`bleReadStateOnce`).
- Maintains read success/fail counters.
- Optional serial diagnostics controlled by `showBlePollSerial`.

### TX task (`bleTxTask`)
- Waits on semaphore when commands are queued.
- Pops command queue items.
- Reconnects if needed.
- Realtime commands are rate-limited to ~30 ms minimum spacing.
- For confirm-required commands, waits for response or state update confirmation.

## 8.4 State parsing and cache

State is tracked in:
- `g_machineMode` (`MachineMode` enum)
- `g_machineStateName` (raw state name)
- `g_lastStateUpdateMs`
- `g_confirmedState` (parsed fields)

State freshness:
- `hasFreshState()` checks if last update age <= `STATE_FRESH_TIMEOUT_MS` (1200 ms).

Mode parsing:
- Names starting with `menu`, `homing`, `strokeEngine`, `simplePenetration`, `streaming` map to enum values.

Side effects from state updates:
- `OSSM_On` is derived from parsed speed (> 0.5 means on).
- BLE control ranges are normalized to percent scale (`maxdepthinmm=100`, `speedlimit=100`).

## 8.5 Motion-command gate (`ensureStrokeEngineReady`)

Before motion commands, BLE path verifies OSSM is ready.

Behavior:

1. Ensure BLE connection.
2. If already in `StrokeEngine`, ready.
3. If state is stale, poll up to 2 s for fresh state.
4. If setting `ble_force_homeing` is enabled and mode is not menu/homing:
   - send `go:menu`
   - wait up to 2 s to reach menu/homing.
5. If currently homing, wait for completion (`waitForStrokeEngineReady`, up to 6 s).
6. Send `go:strokeEngine` (retry once on transient fail).
7. Wait up to 10 s for confirmed `StrokeEngine`.

This is the main safety/consistency gate for BLE motion control.

## 8.6 BLE app command mapping (`bleCommSendAppCommand`)

Important mappings:

- `SETUP_D_I` -> `go:simplePenetration`
- `SETUP_D_I_F` -> `go:strokeEngine`
- `CONN` -> reconnect attempt
- `SPEED`/`DEPTH`/`STROKE`/`SENSATION`/`PATTERN` -> `set:*:*`
- `OFF` -> `set:speed:0`
- `ON` -> `set:speed:<resume or requested>`

Special handling:

- Sensation mapping from -100..100 to BLE 0..100 percent.
- Realtime dedupe in queue: only latest pending command per realtime type is kept.
- `STROKE` transition handling:
  - If stroke goes to 0 from >0: stores current speed and sends speed 0.
  - If stroke rises from 0 and stored resume speed >0: queues stroke set then speed resume.
- In streaming mode, speed commands are not swallowed by local OSSM_On logic.

---

## 9) ESP-NOW Design Details

## 9.1 Initialization (`espNowInit`)

- Sets Wi-Fi station mode.
- Sets Wi-Fi channel explicitly to `ESP_NOW_CHANNEL` (default 1).
- Initializes ESP-NOW.
- Registers send and receive callbacks.
- Registers initial peer (broadcast address in `OSSM_Address` default state).
- Starts background heartbeat task (`espNowRemoteTask`) pinned to core 0.

## 9.2 Heartbeat and pairing

Background task behavior:
- If paired: send heartbeat to current OSSM address every 5 s.
- If not paired: call `espNowKickPairing()` to send broadcast-style pairing heartbeat.

Fast pairing helper used by connect button:
- `tryEspNowFastConnect()` runs a short loop calling `espNowKickPairing()` + delay.

## 9.3 Receive callback (`OnDataRecv`)

Processing order:

1. Reject packets shorter than legacy minimum.
2. Parse packet with legacy compatibility.
3. Route addon-origin frames:
   - sender `EJECT_ID` -> `EjectHandleIncomingEspNowFrame`
   - sender `FIST_ID` -> `FistITHandleIncomingEspNowFrame`
4. If not paired and target is M5 and message passes OSSM checks:
   - validate sender rules (explicit OSSM sender OR legacy sender with valid limits payload)
   - switch peer from broadcast to sender MAC
   - store speed/depth limits
   - ack back to OSSM
   - mark paired
5. Security gate after pairing:
   - reject frames whose MAC does not match paired OSSM MAC
6. While paired and `!OssmBleIsMode()`:
   - update max speed/depth limits from telemetry if provided.

Important implementation note:
- `OssmBleIsMode()` is currently implemented as `commGetMode() == COMM_MODE_ESPNOW` in `EspNowComm.cpp`.
- So `!OssmBleIsMode()` means "not ESP-NOW mode" in current code, even though the function name suggests BLE.

## 9.4 OSSM message verification guard

Function: `isOssmMessage(const uint8_t* mac)`

Current effective checks:
- Before pairing: requires target to be `M5_ID`.
- After pairing: MAC must match stored `OSSM_Address`.

Note:
- Additional sender-ID checks exist as commented logic in this function.
- Current active enforcement is mainly MAC match (post-pair).

## 9.5 ESP-NOW send (`espNowSendCommand`)

- Fails early if OSSM is not paired.
- Routes addon targets to addon modules.
- For OSSM target:
  - fills packet fields and sends to `OSSM_Address`.
  - one immediate retry path exists after a short delay.

Return value behavior:
- First send success returns true.
- On first failure it retries, logs retry result, and returns false (current code does not return true on retry success).

## 9.6 Multi-channel sweep utility

Function: `espNowMultiChannelSweep()` exists as a last-resort pairing helper:
- scans channels in preferred order (1,6,11, then others)
- broadcasts heartbeats on each
- temporarily suspends background heartbeat task during sweep
- restores default configuration on failure

Current status in active UX:
- Sweep call in `connectbutton` is currently inside commented block, so normal connect flow does not automatically invoke it.

---

## 10) How State Machine Monitoring Works

There are two different "state machines" monitored at runtime:

1. OSSM machine mode/state (from BLE state JSON)
2. Remote UI screen state (`st_screens` in `ScreenHandler`)

## 10.1 OSSM machine monitoring (BLE)

In `BleComm.cpp`:
- Poll + notify callbacks continuously parse OSSM state.
- Parsed mode is cached as `MachineMode`.
- Freshness is tracked with timestamp guard.

Consumers:
- `bleCommIsHoming()` and `bleCommGetHomingDirection()` expose homing status/direction.
- `ScreenHandler` status strip displays BLE icon and homing arrows based on these APIs.

## 10.2 UI screen state monitoring

In `ScreenHandler.cpp`:
- `handleScreens()` runs every loop.
- It runs:
  - `checkBleDisconnectError()`
  - `updateStatusStrip()`
  - then per-screen input and command logic via switch on `st_screens`.

BLE disconnect guard (`checkBleDisconnectError`):
- Active only when not in ESP-NOW mode.
- Starts a 3-second grace timer on disconnect after a previously connected state.
- Attempts explicit reconnect once after grace window.
- If reconnect fails and not on start screen, shows blocking notification with actions:
  - Restart remote
   - Power off remote
- Prevents repeated notification spam during same disconnect episode.

---

## 11) Guards and Safety Features (Consolidated)

### Transport and coexistence
- BLE is lazy-started (not immediately at boot) to reduce ESP-NOW broadcast contention.
- Entering BLE mode suspends ESP-NOW heartbeat task; leaving BLE resumes it.

### BLE connection safety
- Connect mutex prevents overlapping scans/connects.
- Connect cooldown prevents repeated scan storms.
- Required characteristic checks prevent partial/invalid connection states.

### BLE command safety
- TX queue with semaphore decouples UI loop from transport timing.
- Realtime dedupe prevents queue backlog from stale slider values.
- Realtime rate limit (~30 ms) avoids overloading BLE path.
- Confirmation wait path reduces blind-fire command behavior.

### BLE motion safety
- Motion commands gated by `ensureStrokeEngineReady()`.
- Fresh-state requirement before mode decisions.
- Optional force-homing path when `ble_force_homeing` is enabled.
- Stroke zero/non-zero transitions preserve resume-speed intent safely.

### ESP-NOW safety
- Legacy packet size compatibility parsing avoids struct over-read assumptions.
- Pairing gate checks target and payload constraints before accepting legacy sender=0 path.
- Post-pair MAC verification blocks commands from other devices.
- Addon frame routing separated by sender IDs.

### Runtime safety UX
- BLE disconnect monitor with grace period + reconnect attempt.
- Critical fail notification with user recovery actions.

---

## 12) Public API Reference (What to call)

## 12.1 CommManager API

From `CommManager.h`:

- `void commInit();`
  - Initialize transport manager state.

- `CommTransportMode commGetMode();`
  - Returns active mode and normalizes stale mode if transport dropped.

- `bool commIsBleMode();`
- `bool commIsEspNowMode();`

- `void connectbutton(lv_event_t* e);`
  - Start-screen callback for connection workflow.

- `bool SendCommand(int Command, float Value, int Target);`
  - Unified transport-agnostic command function.

## 12.2 BLE API

From `BleComm.h`:

- `void bleCommInit();`
- `bool bleCommTryConnect();`
- `bool bleCommIsConnected();`
- `bool bleCommSendAppCommand(...);`
- `bool bleCommGoToMenu();`
- `bool bleCommGoToStreaming();`
- `bool bleCommGoToStrokeEngine();`
- `void bleCommSetEnabled(bool enabled);`
- `bool bleCommIsEnabled();`
- `bool bleCommIsHoming();`
- `int bleCommGetHomingDirection();`

## 12.3 ESP-NOW API

From `EspNowComm.h`:

- `void espNowInit();`
- `bool espNowSendCommand(int Command, float Value, int Target);`
- `void espNowKickPairing();`
- `bool espNowIsPaired();`
- `bool espNowIsEjectConnected();`
- `bool espNowIsFistConnected();`
- `bool espNowMultiChannelSweep();`
- Callback/task functions are exposed but primarily internal runtime hooks.

---

## 13) End-to-End Examples

## 13.1 User moves speed slider on Home screen

1. Screen code calls `SendCommand(SPEED, value, OSSM_ID)`.
2. If BLE connected:
   - command goes to `bleCommSendAppCommand`.
   - stroke-engine readiness is ensured.
   - `set:speed:<percent>` queued in BLE TX queue.
3. If BLE not connected but ESP-NOW paired:
   - packet sent via `espNowSendCommand`.

## 13.2 User enters Addons and starts Fist-IT addon

1. UI/action selects Fist target and calls `SendCommand(..., FIST_ID)`.
2. Dispatcher always routes to `FistITSendCommand` (ESP-NOW addon path).
3. OSSM BLE/ESP-NOW mode does not block addon send path.

## 13.3 BLE drops while user is on operational screen

1. `handleScreens()` calls `checkBleDisconnectError()` every loop.
2. 3-second grace timer starts.
3. One explicit reconnect attempt is made.
4. If still disconnected, blocking notification asks user to restart or power off.

---

## 14) Practical Maintenance Notes

1. If you add new OSSM commands:
   - Define command ID in `EspNowComm.h`.
   - Extend BLE mapping in `bleCommSendAppCommand`.
   - Ensure target routing in `SendCommand` still makes sense.

2. If you add new addons:
   - Keep addon traffic in addon modules / ESP-NOW path.
   - Do not force addon commands through BLE OSSM transport.

3. If you tune responsiveness:
   - BLE realtime queue behavior (dedupe/throttle) is primary control point.
   - Poll period and confirmation timeouts directly affect latency and confidence.

---

## 15) Summary

The communication subsystem is designed as a dual-transport system with strict routing:
- BLE is the preferred OSSM control channel when connected.
- ESP-NOW is fallback for OSSM and primary channel for addons.

Reliability is achieved through:
- explicit mode management,
- queue-based BLE transmission,
- continuous state polling with freshness checks,
- guarded motion gating,
- and runtime disconnect recovery UX.

This architecture is the reason commands remain consistent while supporting mixed OSSM/addon communication.
