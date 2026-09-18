# M5 Remote — Code Review & Restructure Plan

Status: audit + first cleanup pass done; deeper refactors staged in phases below.

## 1. Current architecture (as-built)

| Path | Role | Health |
|---|---|---|
| `src/main.cpp` / `main.h` | Entry point, shared globals, loop | Small; leaked addon calls (now routed through `addonsInit/addonsTick`) |
| `src/screens/ScreenHandler.cpp/.h` | Core UI: screen state machine, Home/Pattern/Settings/Torqe screens, battery, screensaver, notifications, motion commands, addon reconnect scheduler | **God-file (~3200 lines, 100+ functions)** |
| `src/ui/` | LVGL screen objects (`ui.c`/`ui.h`), helpers (`ui_helpers.c`), logo (`image50x50.c`) | Originally SquareLine-generated; now hand-maintained. `ui_events.c` was dead (moved). |
| `src/communication/` | `BleComm` (NimBLE client, telemetry), `CommManager` (transport + command dispatch) | Reasonable; BLE read path has two parallel writers (see §3 F7) |
| `src/addons/` | Eject, FistIT, Coyote, AP-mode, strokeMode, addonsStreaming | Good per-addon isolation, but a few leaks (see §3 F2) |
| `src/buttonhandlers/` | Physical buttons + haptics | Clean |
| `src/display/`, `src/config/`, `src/platform/`, `src/screens/icons.*` | Display setup, colors, config, platform shims, icons | Mostly fine |
| `src/Manuals-templates/` | Docs/templates (excluded from build) | Fine |

## 2. What the code *should* look like (target)

- **Core** (`main`, `communication`, `screens`) never `#include` an addon and never reference addon symbols.
- **Each addon** is fully self-contained in its `.cpp`/`.h`; the only shared surface is a small common interface + the central `addons` service layer.
- **One module = one responsibility**: screen routing in a dedicated module, battery/power in another, motion command assembly in another — not all in `ScreenHandler.cpp`.
- **Hand-maintained UI** (no SquareLine Studio dependency), header guards and module names consistent with the project (not `OSSM-White`).

## 3. Findings (prioritised)

**F1 — `ScreenHandler.cpp` is a god-file.** It mixes 6 concerns:
screen routing/state machine, battery estimation + charging, screensaver/deep-sleep,
modal notifications, settings carousel + NVS, and Home/Pattern/Torqe screen logic
plus the addon background-connect scheduler. This is the #1 readability/safety risk.

**F2 — Addon code leaks outside addon files.**
- `main.cpp` called `CoyoteBackgroundTick()` and `*SetAddonEnabled()` directly → **fixed** (now `addonsInit()`/`addonsTick()`).
- `ScreenHandler.cpp` contains `serviceAddonBackgroundConnect()` (addon reconnect scheduler) and references `FistITGetScreen()` / addon status icons → **to move** into the addon service layer.

**F3 — `struct ButtonEvents` is duplicated** in Eject.h, FistIT.h, Coyote.h, AP-mode.h via an `ADDON_BUTTON_EVENTS_DEFINED` include-guard hack. → move to one shared header.

**F4 — Global mutable state is scattered** (`speed/depth/stroke/sensation/...` in ScreenHandler, `maxdepthinmm/speedlimit` in main, transport state in CommManager, confirmed state in BleComm). → group into explicit "app state" structs/modules.

**F5 — SquareLine remnants.** `ui_events.c` was an empty generated stub (dead); header guards branded `OSSM_White`; `SquareLine_Studio/` project folder present. → cleaned (see §4).

**F6 — Header bugs (fixed).** `ScreenHandler.h` had a duplicate `extern float minPos, maxPos;` and `static int HOME_START_RAMP_THRESHOLD` / `static uint32_t HOME_START_RAMP_INTERVAL_MS` *defined in the header* (each including TU got its own copy). Moved to `ScreenHandler.cpp` as `static constexpr`.

**F7 — BLE state has two writers.** `updateCachedMachineState()` (JSON) and `numericNotify()`/`stateNameNotify()` (individual UUIDs) both write the same cache; side effects (`OSSM_On`, unpause speed) live only in the JSON path. → consolidate behind one mutex-guarded apply function (small, safety-relevant).

**F8 — Dead/backup files.** `Coyote.cpp.old.txt`, `CommManager.cpp.txt`, `Backup testing/` → moved to `src/Files-to-delete/` (excluded from build).

**F9 — `#pragma GCC optimize ("Ofast")` at the top of `main.cpp`.** A global, translation-unit-wide optimization pragma is unusual and can change floating-point behaviour. Recommend removing it and relying on normal build flags.

**F10 — Mixed C/C++ boilerplate.** Addon headers wrap everything in `extern "C"` even though they're C++-only; inconsistent guard styles. Cosmetic, but worth normalising.

## 4. Changes already made (this pass)

1. `platformio.ini`: added `-<Files-to-delete/>` to `build_src_filter`.
2. Moved dead/backup files to `src/Files-to-delete/`: `Coyote.cpp.old.txt`, `CommManager.cpp.txt`, `ui_events.c`, `Backup testing/`, `SquareLine_Studio/`.
3. `ui.h` / `ui_helpers.h`: removed SquareLine banner; renamed guards `_OSSM_WHITE_UI_H` → `M5_REMOTE_UI_H`, `_OSSM_WHITE_UI_HELPERS_H` → `M5_REMOTE_UI_HELPERS_H`.
4. `ScreenHandler.h`: removed duplicate `extern float minPos, maxPos;` and the two `static` header variables.
5. `ScreenHandler.cpp`: added `HOME_START_RAMP_THRESHOLD` / `HOME_START_RAMP_INTERVAL_MS` as `static constexpr` locals.
6. New `src/addons/addons.h/.cpp`: `addonsInit()` + `addonsTick()` central service layer.
7. `main.cpp`: now includes only `addons/addons.h`; setup uses `addonsInit()`, loop uses `addonsTick()`.
8. New shared `src/addons/ButtonEvents.h` — removed the duplicated `struct ButtonEvents` (and `ADDON_BUTTON_EVENTS_DEFINED` guard) from Eject.h / FistIT.h / Coyote.h / AP-mode.h.
9. Standard file-header comments + deduplicated/alphabetized includes on all six addon `.cpp` files; canonical `// ---- Section ----` banners applied to Eject/FistIT/AP-mode/addonsStreaming/Coyote/strokeMode.
10. **Split `ScreenHandler.cpp`** into five modules sharing `src/screens/ScreenHandler_internal.h`:
    - `ScreenHandler.cpp` — routing: globals, status strip, `screenInit`, `screenmachine`, home-button handling, `handleScreens`
    - `Notifications.cpp` — `showNotification` + callbacks
    - `PowerManagement.cpp` — screensaver, deep sleep, battery UI/sampling, power tick, menu sleep/restart
    - `SettingsScreen.cpp` — encoder ramp, settings carousel, visual-speed behaviour, rail calibration
    - `MotionControl.cpp` — home sync, motion-command flush, speed ramp, zero-stroke jog
    Shared file-local state is now explicit `extern` declarations in `ScreenHandler_internal.h`; `SpeedBehavior`/`EncRampProfile` enums moved there too.
11. **BLE state funnel (F7 fixed):** `updateCachedMachineState`, `numericNotify` and `stateNameNotify` now all serialize on `g_bleMutex` (single mutex-guarded apply point); the readers `bleCommIsHoming`/`bleCommGetHomingDirection`/`bleCommGetConfirmedPosition` also take `g_bleMutex`. Side effects (`OSSM_On`, unpause speed) intentionally stay JSON-path-only, since the legacy JSON state characteristic is present on all three OSSM firmware variants.
12. **Anatomy pass (Phase E):** standard file-header comments added to `main.h`, `config/config.h`, `config/config_ids.h`, `config/debug.h`, `display/DisplaySetup.h`, `display/styles.h`, `display/colors.h`, `platform/PlatformCompat.h`, plus the `.cpp` files `display/styles.cpp`, `display/DisplaySetup.cpp`, `platform/PlatformCompat.cpp` (`colors.cpp`, `ColorSchemes.cpp`, `ButtonHandlers.cpp` already had headers). `config_pins.h` / `config_power.h` / `buttonhandlers/ButtonHandlers.h` already carried header comments.
13. **`main.h`:** added `#pragma once` and organised the global-state declarations (`maxdepthinmm`, `speedlimit`, `dark_mode`, `AtStartup`) into a documented block.

## 5. Remaining work (intentionally deferred — high risk / low value)

- **F4 — group the UI globals into an app-state struct.** `speed/depth/stroke/sensation/minPos/maxPos/torqe_f/torqe_r/pattern/...` are `extern`-declared in `ScreenHandler.h` and referenced from ~15 translation units. Moving them into a struct would touch hundreds of call sites for a cosmetic gain; the state is already grouped and documented in `ScreenHandler.cpp` under `// ---- Screen and control state ----`. Deferred rather than risked on a device that drives a moving machine.
- **F10 — remove `extern "C"` wrappers.** The wrappers in the addon/UI headers are *functional*, not cosmetic: LVGL registers C-linkage function pointers (`lv_event_cb_t`), and a C++-linkage function cannot initialise a C function pointer. Removing the wrappers would break the LVGL callbacks, so they are kept. Only cosmetic guard/comment normalisation remains possible.

Each completed phase is behaviour-preserving and verified by compiling both `m5stack-core2` and `m5stack-cores3` (no flashing).
