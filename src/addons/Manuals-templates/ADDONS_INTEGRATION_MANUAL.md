# Addons Integration Manual (M5 Remote)

## 1) Purpose

This manual explains how addons are implemented in the M5 Remote firmware and gives an exact, practical checklist for integrating a new addon.

It is based on the current code in:
- `src/addons`
- `src/communication`
- `src/screens`
- `src/ui`
- `src/main.cpp`

---

## 2) Addon architecture in the current firmware

The project currently has 3 addons/features in the Addons screen:
1. Eject
2. Fist-IT
3. Streaming

They are integrated in slightly different ways:

## 2.1 Eject pattern (screen owned by generated UI)
- Main file: `src/addons/Eject.cpp`
- Uses existing screen object: `ui_EJECTSettings` from `src/ui/ui.c`
- Runtime handling is delegated from state machine (`ST_UI_EJECTSETTINGS`) to `EjectHandleScreen(events)`.
- Communication is ESP-NOW via `EjectSendCommand`.

## 2.2 Fist-IT pattern (screen owned by addon module)
- Main file: `src/addons/FistIT.cpp`
- Creates its own LVGL screen in addon file (`createScreenIfNeeded` + `FistITGetScreen`).
- Addons launcher calls `FistITPrepareScreen()` then `_ui_screen_change(FistITGetScreen(), ...)`.
- Runtime handling is delegated from state machine (`ST_UI_FISTIT`) to `FistITHandleScreen(events)`.
- Communication is ESP-NOW via `FistITSendCommand`.

## 2.3 Streaming pattern (implemented in addons hub module)
- Main file: `src/addons/addonsStreaming.cpp`
- Uses dedicated streaming screen + event handlers in that module.
- Commands to OSSM go through unified `SendCommand` API.

---

## 3) Core addon interfaces you should implement

For a new addon, follow the Eject/Fist-IT API shape.

## 3.1 Header contract (recommended)
Use a header similar to `src/addons/Eject.h` or `src/addons/FistIT.h`:

1. C-linkage addon ID export
- `extern const int YOUR_ADDON_ID;`

2. Shared button event struct (or include existing one)
- `struct ButtonEvents { bool leftShort; bool mxShort; bool rightShort; };`

3. Runtime screen handler
- C wrapper: `void YourAddonHandleScreen(const struct ButtonEvents *events);`
- C++ overload: `void YourAddonHandleScreen(const ButtonEvents &events);`

4. Communication hooks
- `bool YourAddonHandleIncomingEspNowFrame(const uint8_t *mac, int target, int sender, int command, float value, bool heartbeat);`
- `bool YourAddonSendCommand(int command, float value);`
- `bool YourAddonIsPaired();`
- `void YourAddonSetAddonEnabled(bool enabled);`

5. If addon owns its own screen
- `void YourAddonPrepareScreen();`
- `lv_obj_t *YourAddonGetScreen();`

---

## 4) Communication model for addons (important)

Addon traffic is ESP-NOW in current architecture.

Design rule from `src/communication/CommManager.cpp`:
- Addon targets route directly to addon send functions.
- OSSM control may use BLE or ESP-NOW, but addons stay on ESP-NOW path.

Why this matters:
- Addon communication can coexist even while OSSM control transport changes.

---

## 5) Step-by-step integration checklist for a new addon

## Step 1: Choose IDs and command IDs

Update command and target constants in `src/communication/EspNowComm.h`:
- Add command IDs for your addon (similar to `FIST_SPEED`, `FIST_ROTATION`, etc.).
- Add target ID constant (similar to `FIST_ID`).

Optional compatibility mirror:
- If needed by legacy include paths, mirror constants in `src/communication/esp_nowCommunication.h`.

Guideline:
- Keep command IDs grouped in a dedicated range.
- Keep target/device ID unique and stable.

## Step 2: Create addon module files

Create:
- `src/addons/YourAddon.h`
- `src/addons/YourAddon.cpp`

Implement at least:
1. Pairing state and peer address storage.
2. `YourAddonHandleIncomingEspNowFrame` for inbound frames.
3. `YourAddonSendCommand` for outbound frames.
4. `YourAddonHandleScreen` for per-loop UI/encoder/button behavior.
5. `YourAddonSetAddonEnabled` and `YourAddonIsPaired`.

If self-owned screen:
- Build screen lazily (`createScreenIfNeeded` style).
- Return pointer via `YourAddonGetScreen`.

## Step 3: Wire incoming ESP-NOW frame routing

In `src/communication/EspNowComm.cpp`, update `OnDataRecv(...)`:

1. Detect sender ID for your addon.
2. Call `YourAddonHandleIncomingEspNowFrame(...)`.
3. If handled, return early.

This is the same pattern used for Eject/Fist-IT.

## Step 4: Wire outgoing command routing

Update both dispatch points:

1. `src/communication/CommManager.cpp` -> `SendCommand(...)`
- Add target check for your addon ID.
- Route to `YourAddonSendCommand(...)`.

2. `src/communication/EspNowComm.cpp` -> `espNowSendCommand(...)`
- Add target check for your addon ID.
- Route to `YourAddonSendCommand(...)`.

Reason for both:
- `SendCommand` is the main app dispatcher.
- `espNowSendCommand` also has direct target routing logic.

## Step 5: Enable addon at startup

In `src/main.cpp` setup:
- Include your addon header.
- Call `YourAddonSetAddonEnabled(true);` after communication init pattern used for existing addons.

## Step 6: Integrate in Addons launcher/menu

Primary integration file: `src/addons/addonsStreaming.cpp`

1. Add include for your addon header.
2. Add activation function:
- Example pattern: prepare screen then `_ui_screen_change(...)`.
3. Add entry in `s_addon_defs[]`.
4. Update `NUM_ADDONS`.

Critical current constraint:
- `s_enabled_indices` is currently declared as fixed size 3.
- If you increase `NUM_ADDONS`, you must also increase this array capacity (or refactor to dynamic/container-based storage).

If you do not update this, addon visibility indexing can overflow or behave incorrectly.

## Step 7: Add screen-state handling (if addon has dedicated runtime state)

If addon has its own screen and runtime logic (like Fist-IT):

1. Add state ID in `src/screens/ScreenHandler.h`.
2. In `screenmachine(...)` (`src/screens/ScreenHandler.cpp`), map active screen pointer to your state.
3. In `handleScreens()` switch, add `case ST_UI_YOURADDON:` and call `YourAddonHandleScreen(events)`.

If addon reuses existing screen state (like Eject using `ST_UI_EJECTSETTINGS`), this can be unnecessary.

## Step 8: Add UI declarations if needed

If screen objects need global access:

1. Declare externs in `src/ui/ui.h`.
2. Define globals in `src/ui/ui.c` or addon module (depending on ownership model).

Ownership recommendation:
- If addon screen is fully self-contained, keep object creation in addon file and only expose minimal accessors (`GetScreen`, maybe battery labels).

## Step 9: Language/text integration

Add any new labels/macros in `src/language.h`.

Examples:
- Screen title
- Button texts
- Slider labels
- Status/notification strings

## Step 10: Status strip integration (optional)

If you want a new pairing icon in status strip:

1. Add connectivity query API (similar to `espNowIsEjectConnected`, `espNowIsFistConnected`).
2. Extend icon drawing and placement logic in `src/screens/ScreenHandler.cpp` (`updateStatusStrip`).

---

## 6) Two recommended implementation patterns

## Pattern A: Existing generated screen
Use this when you already have a SquareLine screen or want generated LVGL layout.

Pros:
- Fast integration.
- Easy to theme with existing style flow.

Cons:
- More coupling with `ui.c` globals.

## Pattern B: Self-owned addon screen (recommended for new complex addons)
Use this when addon has custom behavior and should be modular.

Pros:
- Better encapsulation.
- Easier maintenance and testing.

Cons:
- Slightly more initial code.

Fist-IT is a good example of Pattern B.

---

## 7) Minimal file-change matrix for adding one new addon

Expected files to touch:

1. `src/addons/YourAddon.h`
2. `src/addons/YourAddon.cpp`
3. `src/communication/EspNowComm.h`
4. `src/communication/EspNowComm.cpp`
5. `src/communication/CommManager.cpp`
6. `src/addons/addonsStreaming.cpp`
7. `src/main.cpp`
8. `src/language.h`

Conditionally:

9. `src/screens/ScreenHandler.h` (new state constant)
10. `src/screens/ScreenHandler.cpp` (state mapping + runtime case)
11. `src/ui/ui.h` / `src/ui/ui.c` (if you expose global UI pointers)
12. `src/communication/esp_nowCommunication.h` (legacy compatibility constants)

---

## 8) Guard and safety requirements for new addons

When implementing new addon communication, keep these guards:

1. Verify sender/target in incoming frame handler.
2. Ignore broadcast/all-zero MAC as paired address.
3. Ensure peer exists before send (add peer if needed).
4. Maintain timeout-based pairing freshness (`lastSeen` style).
5. Return `false` safely when addon disabled/not paired.
6. Keep UI handlers non-blocking in normal loop path.

For UI responsiveness:
- Avoid heavy full-theme re-apply every loop.
- Apply theme on screen prepare/load events, not every frame.

---

## 9) Validation checklist before merging

1. Build passes for core target.
2. Addon appears in Addons list and can be toggled show/hide.
3. Addon enable state persists in NVS through reboot.
4. Selecting addon opens correct screen.
5. Physical button events work immediately on first load.
6. Encoder + touch slider interactions are both functional.
7. Incoming ESP-NOW frames are routed to addon handler.
8. Outgoing commands route correctly for addon target ID.
9. Existing addons (Eject/Fist-IT/Streaming) still function unchanged.

---

## 10) Current codebase caveats you must remember

1. Addons registry currently hardcodes 3-slot enabled-index storage in `addonsStreaming.cpp`.
- Expanding addon count requires updating this storage.

2. There are legacy/backup helper files in folder (`addonsStreaming copy.*.txt`).
- Do not integrate against those; use active `.cpp`/`.h` files.

3. Addon communication path is intentionally ESP-NOW-first.
- Do not accidentally route addon commands into BLE OSSM path.

---

## 11) Practical example flow for a new addon

If adding addon `Pulse` with target `PULSE_ID = 4`:

1. Create `Pulse.h/.cpp` with APIs matching section 3.
2. Add `PULSE_ID` and `PULSE_*` command constants.
3. In `OnDataRecv`, route `esp_sender == PULSE_ID` to `PulseHandleIncomingEspNowFrame`.
4. In both dispatchers, route target `PULSE_ID` to `PulseSendCommand`.
5. Startup call `PulseSetAddonEnabled(true)` in setup.
6. Add `activatePulse()` and a `s_addon_defs[]` entry.
7. Increase `NUM_ADDONS` and storage capacity in addons list logic.
8. Add UI texts in `language.h`.
9. If dedicated screen state needed, add `ST_UI_PULSE` and `handleScreens` case.

That is the complete end-to-end integration path.

---

## 12) Copy-paste templates included in this repo

You can start from these template files:

1. `src/addons/templates/AddonTemplate.h.txt`
2. `src/addons/templates/AddonTemplate.cpp.txt`

How to use:

1. Copy them to `src/addons/YourAddon.h` and `src/addons/YourAddon.cpp`.
2. Replace all `YourAddon` tokens with your addon name.
3. Replace `YOUR_ADDON_ID` and example command IDs with your real constants from `EspNowComm.h`.
4. Wire routing and menu integration using Section 5 checklist.

Note:
- The templates are `.txt` on purpose so they do not compile until you explicitly copy/rename them.
