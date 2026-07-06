# M5 Remote UI Manual

## 1) Goal of this manual

This document explains the complete UI system of M5 Remote:
- how the UI is initialized,
- which screens exist,
- what controls (buttons/sliders/rollers/checkboxes) each screen has,
- exactly which function is called on touch click, long-press, physical button press, and encoder rotation,
- how touch input is enabled/disabled.

This manual is written for beginners, but it is accurate to the current code.

---

## 2) UI architecture overview

The UI is split into 3 layers:

1. Screen construction layer
- Files: `src/ui/ui.c`, `src/addons/strokeMode.cpp`, `src/addons/addonsStreaming.cpp`, `src/display/colors.cpp`, `src/addons/FistIT.cpp`, `src/addons/Eject.cpp`
- Purpose: creates LVGL objects (screens, labels, sliders, buttons).

2. Event callback layer
- File: `src/ui/ui.c` and custom files listed above
- Purpose: touch callbacks that run when LVGL objects are clicked/pressed/loaded.

3. Runtime input state machine
- File: `src/screens/ScreenHandler.cpp`
- Purpose: maps physical buttons and rotary encoders to current screen behavior every loop.

Important: many interactions are not only touch-driven. Physical input handling is centralized in `handleScreens()`.

---

## 3) Initialization sequence

From `src/main.cpp` setup:

1. `displayInit()`
- Initializes LVGL, styles, display flush callback, and touch input device.
- Registers touch read callback `my_touchpad_read`.

2. `ui_init()`
- Creates all UI screens and widgets.
- Calls screen init functions in this order:
  - `ui_Start_screen_init()`
  - `ui_Home_screen_init()`
  - `ui_Menu_screen_init()`
  - `ui_Pattern_screen_init()`
  - `ui_Torqe_screen_init()`
  - `ui_EJECTSettings_screen_init()`
  - `ui_Settings_screen_init()`
  - `ui_Stroke_screen_init()`
  - `ui_Streaming_screen_init()`
  - `ui_Addons_screen_init()`
  - `colors_ui_screen_init()`
- Loads `ui_Start` as first screen.

3. `buttonInit()`
- Configures four rotary encoders and three OneButton physical buttons.
- Hooks callbacks that set press flags.

4. `screenInit()`
- Loads user settings from Preferences.
- Applies checkboxes, brightness, initial pattern, colors, battery visuals.

Main loop:
- `lv_task_handler()` updates LVGL.
- `Button1/2/3.tick()` updates OneButton.
- `handleScreens()` runs screen state machine and executes physical input behavior.

---

## 4) Physical inputs and names

## 4.1 Physical buttons
In `ButtonHandlers.cpp`:

- `Button1` -> `mxclick_short_waspressed`, `mxclick_long_waspressed`
- `Button2` -> `click2_short_waspressed`, `click2_long_waspressed`
- `Button3` -> `click3_short_waspressed`, `click3_long_waspressed`, `click3_double_waspressed`

In UI terms these usually map to:
- Button2 = left soft-button behavior
- Button1 = middle soft-button behavior
- Button3 = right soft-button behavior

## 4.2 Rotary encoders
- `encoder1`, `encoder2`, `encoder3`, `encoder4`
- Meaning depends on screen (documented per screen below).

---

## 5) Touch handling (and touch disable)

Touch read callback is in `src/display/DisplaySetup.cpp`:
- function `my_touchpad_read(...)`
- reads M5 touch and reports LVGL pointer pressed/released.

Global guard:
- `touch_disabled` (defined in `ScreenHandler.cpp`) is checked inside touch read callback.
- If `touch_disabled == true`, callback does not feed normal touch data.

When touch gets disabled:
- On many operational screens, if settings checkbox `ui_lefty` is checked, code sets `touch_disabled = true`.
- Settings screen explicitly sets `touch_disabled = false`.
- Some modal notifications temporarily force touch disabled while modal is active.

Practical result:
- "Touch Disabled" setting is a safety mode that blocks normal touch interactions outside settings/modal contexts.

---

## 6) Screen state machine tracking

Current screen state lives in `st_screens` (enum-like macros in `ScreenHandler.h`).

When a screen loads, LVGL callback `screenmachine(e)` runs and sets `st_screens` based on `lv_scr_act()`.

`handleScreens()` uses `switch(st_screens)` to run per-screen input logic every loop.

This means the same UI can react to both:
- touch events (LVGL callbacks), and
- physical events (flags + encoder counters in `handleScreens()`).

---

## 7) Complete screen interaction map

## 7.1 Start screen (`ui_Start`, state `ST_UI_START`)

Controls:
- `ui_StartButtonL` (Connect)
- `ui_StartButtonM` (Settings)
- `ui_StartButtonR` (Demo/Home)

Touch callbacks in `ui.c`:
- `ui_event_StartButtonL` on `LV_EVENT_CLICKED` -> `connectbutton(e)`
- `ui_event_StartButtonM` on `LV_EVENT_CLICKED` -> `_ui_screen_change(ui_Settings, ...)`
- `ui_event_StartButtonR` on `LV_EVENT_CLICKED` -> `_ui_screen_change(ui_Home, ...)`

Auto behavior:
- On first load, timer triggers `ui_StartButtonL` click automatically (auto-connect).

Physical behavior in `handleScreens()`:
- Button2 short -> sends CLICKED to `ui_StartButtonL`
- Button1 short -> sends CLICKED to `ui_StartButtonM`
- Button3 short -> sends CLICKED to `ui_StartButtonR`

Encoders:
- none on this screen.

---

## 7.2 Home screen (`ui_Home`, state `ST_UI_HOME`)

Main controls:
- Buttons: `ui_HomeButtonL`, `ui_HomeButtonM`, `ui_HomeButtonR`
- Sliders: `ui_homespeedslider`, `ui_homedepthslider`, `ui_homestrokeslider`, `ui_homesensationslider`

Touch callbacks in `ui.c`:
- `ui_event_HomeButtonL`
  - CLICKED -> `pullOut(e)`
  - LONG_PRESSED -> switch to `ui_ejectaddon`, call `ejectcreampie(e)`, `screenmachine(e)`
- `ui_event_HomeButtonM`
  - CLICKED -> `homebuttonmevent(e)`
  - LONG_PRESSED -> `emergencyStop(e)`
- `ui_event_HomeButtonR`
  - CLICKED -> set return screen and open `ui_Pattern`
  - LONG_PRESSED -> open `ui_Menu`

Physical behavior in `handleScreens()`:
- Encoder1 rotates Speed (with ramp) -> `SendCommand(SPEED, ...)`
- Encoder2 rotates Depth (and stroke coupling if dynamicStroke) -> `SendCommand(DEPTH/STROKE, ...)`
- Encoder3 rotates Stroke -> `SendCommand(STROKE/DEPTH, ...)`
- Encoder4 rotates Sensation -> `SendCommand(SENSATION, ...)`

Button mapping:
- Button2 short -> sends CLICKED to Home L
- Button2 long -> sends LONG_PRESSED to Home L
- Button1 short -> sends CLICKED to Home M
- Button1 long -> sends LONG_PRESSED to Home M and also forces stop/reset + menu transition
- Button3 short -> sends CLICKED to Home R
- Button3 long -> sends LONG_PRESSED to Home R
- Button3 double -> toggles `dynamicStroke`

Touch slider behavior:
- If slider is dragged, touch value is used.
- If not dragged, encoder updates slider and sends commands.

---

## 7.3 Menu screen (`ui_Menu`, state `ST_UI_MENU`)

Main controls:
- Big tiles: `ui_MenuButtonTL`, `ui_MenuButtonTR`, `ui_MenuButtonML`, `ui_MenuButtonMR`
- Bottom soft buttons: `ui_MenuButtonL`, `ui_MenuButtonM`, `ui_MenuButtonR`

Tile touch callbacks:
- TL short -> open Home
- TR short -> open Stroke
- ML short -> open Settings
- MR short -> open Addons

Bottom touch callbacks:
- L short -> open Colors
- M short -> triggers click on currently focused menu tile (`lv_group_get_focused(ui_g_menu)`)
- R short -> `menuRestartAction()` confirmation dialog

Focus behavior:
- `ui_g_menu` contains only the 4 tiles, wrap enabled.
- On screen load, focus forced to TL.

Physical behavior in `handleScreens()`:
- Encoder4 rotates tile focus next/prev.
- Button2 short -> click MenuButtonL
- Button1 short -> click currently focused tile
- Button3 short -> click MenuButtonR (restart prompt)
- Button3 long -> sends `SendCommand(REBOOT, 0, OSSM_ID)`

---

## 7.4 Stroke screen (`ui_Stroke`, state `ST_UI_STROKE`)

Defined in `src/addons/strokeMode.cpp`.

Controls:
- Buttons: left(Menu), middle(Start/Stop), right(Pattern)
- Sliders: speed, stroke, sensation

Touch callbacks:
- Screen load -> `ui_event_Stroke` calls `refreshStrokeStartStopUi()` and `screenmachine(e)`
- Left button short -> `_ui_screen_change(ui_Menu, ...)`
- Middle button short -> `homebuttonmevent`
- Right button short -> `_ui_screen_change(ui_Pattern, ...)`

Physical behavior in `handleScreens()`:
- Encoder1 -> Stroke speed slider + `SendCommand(SPEED, ...)`
- Encoder2 -> Stroke amount slider + derived depth + `SendCommand(STROKE/DEPTH, ...)`
- Encoder4 -> Sensation slider + `SendCommand(SENSATION, ...)`
- Button2 short -> go Menu
- Button1 short -> Start/Stop (`homebuttonmevent`) and `refreshStrokeStartStopUi()`
- Button3 short -> go Pattern (return target set to Stroke)

Touch slider behavior:
- Dragging each slider directly changes corresponding values/commands.

---

## 7.5 Colors screen (`ui_Colors`, state `ST_UI_COLORS`)

Defined in `src/display/colors.cpp`.

Controls:
- 5 visible carousel buttons (schemes)
- Back button (left)
- Select button (right)

Touch callbacks:
- Scheme button short -> set focused scheme + apply/save
- Back short -> open Menu
- Select short -> apply/save focused scheme
- Screen load -> update visible scheme list + `screenmachine(e)`

Physical behavior in `handleScreens()`:
- Encoder4 -> `colorsScrollFocus(+/-1)`
- Button2 short -> back to Menu
- Button1 short OR Button3 short -> apply selected scheme (`colorSchemeSelectIndex(colorsGetFocusIndex())`)

---

## 7.6 Pattern screen (`ui_Pattern`, state `ST_UI_PATTERN`)

Controls:
- Roller: `ui_PatternS`
- Buttons: L(Menu), M(Home), R(Save)

Touch callbacks:
- L click -> open Menu
- M click -> open Home
- R click -> `savepattern(e)` then open return screen (`g_pattern_return_screen`, else Home)

Physical behavior in `handleScreens()`:
- Encoder4 up/down -> sends `LV_EVENT_KEY` (UP/DOWN) to roller
- Button2 short -> click Pattern L
- Button1 short -> click Pattern M
- Button3 short -> click Pattern R

---

## 7.7 Torque screen (`ui_Torqe`, state `ST_UI_Torqe`)

Controls:
- Sliders: `ui_outtroqeslider`, `ui_introqeslider`
- Buttons: L(blank), M(Home), R(Menu)

Touch callbacks:
- M click -> open Home
- R click -> open Menu
- L callback exists but no action implemented

Physical behavior in `handleScreens()`:
- Encoder1 controls `ui_outtroqeslider` -> `SendCommand(TORQE_F, ...)`
- Encoder4 controls `ui_introqeslider` -> `SendCommand(TORQE_R, ...)`
- Button2 short -> click Torque L (currently does nothing)
- Button1 short -> click Torque M (Home)
- Button3 short -> click Torque R (Menu)

Touch slider behavior:
- Dragging sliders updates encoder counts and sends torque commands.

---

## 7.8 Eject screen (`ui_EJECTSettings`, state `ST_UI_EJECTSETTINGS`)

Base screen objects created in `ui.c`, addon controls managed in `src/addons/Eject.cpp`.

Touch callbacks in `ui.c`:
- Left click -> open Home
- Right click -> open Menu
- Middle touch callback in `ui.c` is empty

Runtime behavior in `ScreenHandler`:
- Delegated to `EjectHandleScreen(events)` each loop.
- Physical buttons passed as `ButtonEvents {leftShort, mxShort, rightShort}`.

Inside Eject addon handler:
- Encoders and slider drag update Eject parameters + send addon commands.
- leftShort -> Home
- mxShort -> toggle Eject on/off
- rightShort -> Menu

---

## 7.9 Fist-IT screen (`ui_FistIT`, state `ST_UI_FISTIT`)

Screen is created in `src/addons/FistIT.cpp` (not in generated `ui.c`).

Controls:
- 4 sliders (speed, rotation, pause, accel)
- 3 bottom buttons (home/toggle/menu labels)

Touch callbacks:
- No direct LVGL button callbacks are attached in current FistIT screen creation.
- Slider touch drag is still effective because handler checks slider values every loop.

Runtime behavior in `ScreenHandler`:
- Delegated to `FistITHandleScreen(events)` each loop.

Inside FistIT handler:
- Encoders map to sliders/commands.
- leftShort -> Home
- mxShort -> toggle on/off
- rightShort -> Menu

---

## 7.10 Streaming screen (`ui_Streaming`, state `ST_UI_STREAMING`)

Defined in `src/addons/addonsStreaming.cpp`.

Controls:
- Sliders: speed, depth, stroke
- Buttons: L(Back), M(Start/Stop streaming pause), R(Addons)

Touch callbacks:
- Screen loaded -> `event_streaming_screen`
  - calls `screenmachine(e)`
  - queues BLE mode transitions (`go:menu`, then delayed `go:streaming`)
- Screen unload start -> cancels timer + sends `go:menu`
- L short -> Menu
- M short -> toggles pause state and updates label
- R short -> Addons screen

Physical behavior in `handleScreens()`:
- Encoder1 -> speed
- Encoder2 -> depth
- Encoder4 -> stroke
- Button2 short -> click Streaming L
- Button1 short -> click Streaming M
- Button3 short -> click Streaming R

Extra behavior:
- On first entry, sliders reset to 100/100/100.
- After homing completes, sends initial defaults.
- Pause transition sends `SPEED=0`; resume re-sends speed/depth/stroke.

---

## 7.11 Addons screen (`ui_Addons`, state `ST_UI_ADDONS`)

Defined in `src/addons/addonsStreaming.cpp`.

Controls:
- Addon list rows: `ui_AddonsItem0/1/2`
- Buttons:
  - L = Menu back
  - M = Select/Open (or toggle in manage mode)
  - R = Enable/Disable mode toggle (label changes)

Touch callbacks:
- Screen load -> load addon prefs, build enabled list, set mode/labels, `screenmachine(e)`
- Item row short -> select row and activate/toggle
- L short -> Menu
- M short -> activate/toggle selected
- R short -> enter/exit manage mode

Physical behavior in `handleScreens()`:
- Encoder4 -> `addonsMoveSelection(+/-1)`
- Button2 short -> click Addons L
- Button1 short -> click Addons M
- Button3 short -> click Addons R

Modes:
- Normal mode: shows enabled addons, M opens selected addon.
- Manage mode: shows all with `(show/hide)`, M toggles enable state and saves to NVS.

---

## 7.12 Settings screen (`ui_Settings`, state `ST_UI_SETTINGS`)

Controls:
- Buttons: Save, Home, Menu
- Checkboxes: vibrate, touch disabled (`ui_lefty`), stroke invert, force-home
- Brightness slider

Touch callbacks:
- Save click -> `savesettings(e)`
- Home click -> open Home
- Menu click -> open Menu
- Brightness slider value changed -> `brightness_slider_event_cb(e)`

Physical behavior in `handleScreens()`:
- `touch_disabled = false` enforced here.
- Encoder3 adjusts brightness slider in +/-5 steps.
- Encoder4 moves focus within `ui_g_settings`.
- Button2 short -> click Settings Save
- Button1 short -> click Settings Home
- Button3 short -> toggles focused checkbox (if checkbox focused), otherwise short-clicks focused object

---

## 8) Press type summary

Press types used in code:

- LVGL touch events:
  - `LV_EVENT_CLICKED`
  - `LV_EVENT_SHORT_CLICKED`
  - `LV_EVENT_LONG_PRESSED`
  - `LV_EVENT_SCREEN_LOADED`
  - `LV_EVENT_SCREEN_UNLOAD_START`
  - `LV_EVENT_VALUE_CHANGED`

- OneButton physical events:
  - short click
  - long press start
  - double click (Button3 only)

Not every screen uses every event type.

---

## 9) How slider rotation and touch coexist

Pattern used on most runtime screens:

1. If slider is currently dragged by touch (`lv_slider_is_dragged == true`):
- slider touch value drives the command.

2. If slider is not dragged:
- encoder deltas drive slider and command updates.

This avoids fighting between touch and encoder at the same time.

---

## 10) Safety and UX guards in UI layer

1. Button flags are cleared every loop after `handleScreens()` so presses are edge-triggered.
2. Notification modal consumes button flags while active to prevent stale actions after closing.
3. Many transitions call `screenmachine(e)` on screen load so state machine is always synchronized.
4. Status strip is updated globally and shows BLE/ESP-NOW/homing state and addon pairing icons.
5. Menu tile focus wraps to prevent "no highlighted tile" states.

---

## 11) Quick developer checklist for adding a new UI screen

1. Create screen + widgets in a screen init function.
2. Register `LV_EVENT_SCREEN_LOADED` callback and call `screenmachine(e)` inside it.
3. Add screen init call in `ui_init()`.
4. Add state constant in `ScreenHandler.h` and state handling in `screenmachine`.
5. Add `handleScreens()` case for physical button and encoder behavior.
6. If touch should be disabled in safety mode, apply `ui_lefty` logic consistently.
7. If screen has battery icon/status strip, ensure it is included in update arrays.

---

## 12) Source index for this manual

Primary files:
- `src/main.cpp`
- `src/display/DisplaySetup.cpp`
- `src/buttonhandlers/ButtonHandlers.cpp`
- `src/ui/ui.h`
- `src/ui/ui.c`
- `src/screens/ScreenHandler.h`
- `src/screens/ScreenHandler.cpp`
- `src/addons/strokeMode.cpp`
- `src/display/colors.cpp`
- `src/addons/addonsStreaming.cpp`
- `src/addons/Eject.cpp`
- `src/addons/FistIT.cpp`
