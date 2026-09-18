# M5 Remote — File Anatomy Convention

Every source file follows the same layout, in the same order, so any developer
can open any file and know where things are. This mirrors the discipline used
in the OSSM-Lite and OSSM-RS firmware.

## Header files (`.h`)

1. `#pragma once`
2. Guard includes of third-party headers (`lvgl.h`, etc.)
3. `extern "C"` block only where a C-language TU must link against it
4. Shared types/constants, then declarations
5. Section banners: `// ---- <Section> ----` before each group

## Source files (`.cpp`)

Fixed order, top to bottom:

1. **File header comment** — `// <File> — <one-line purpose>` plus a short
   "what owns what" note when the module interacts with others.
2. **Own header first**, then a blank line, then other includes grouped:
   system, libraries, then project headers (alphabetical within each group).
3. **Public/exported definitions** — non-static globals and `extern "C"` ids.
4. **Anonymous namespace** `namespace { ... }` wrapping all internal state.
5. Inside the namespace, fixed section order with banners:
   - `// ---- Constants ----` (static constexpr)
   - `// ---- Persistent settings ----` (Preferences load/save)
   - `// ---- Local state ----` (statics: widgets, values, encoders, BLE)
   - `// ---- Internal helpers ----`
   - `// ---- BLE connection ----`
   - `// ---- Screen construction ----`
   - `// ---- Screen lifecycle ----` (PrepareScreen / GetScreen / HandleScreen)
   - `// ---- Background service ----` (Tick / IsPaired / TryConnect / SetAddonEnabled)
6. **Public functions** (declared in the `.h`) after the namespace.

## Naming

- File-local state: `s_` prefix (`s_speed`, `s_screen`).
- File-local types: `PascalCase` (`CoyoteSettings`).
- Internal static functions: `lowerCamelCase` (`coyoteTryConnect`).
- Public functions: module prefix + `PascalCase` (`EjectPrepareScreen`).
- Constants: `UPPER_SNAKE_CASE`.

## Braces & spacing

- K&R braces (opening brace on the same line).
- 4-space indent.
- No `e_`/`f_` ad-hoc prefixes — every module uses `s_`.

## Screen/addon modules (addons)

An addon `.cpp` is self-contained: it owns its screen, its BLE device, its
state, and its background tick. The only external surface is its `.h` plus the
central `addons.cpp` service layer. Core code never `#include`s an addon.

## What is intentionally NOT part of this convention

- Mass renaming of already-public symbols (screens/widgets named by
  SquareLine, command ids, etc.) — those stay stable.
- Moving LVGL object construction out of `ui.c` (hand-maintained, not
  regenerated).
