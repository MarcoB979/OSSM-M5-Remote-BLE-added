# AP-Mode Update Workflow (Delta Port Guide)

Use this when a new version of advancedPenetration.hpp arrives.

## Goal

Port only what changed in advancedPenetration.hpp into AP-mode with minimal risk and no unnecessary rewrites.

## Scope

- Source of truth for Advanced mode behavior: advancedPenetration.hpp
- Runtime addon implementation in this repo: AP-mode.cpp + BLE bridge in BleComm.cpp

## Quick Rule

- UI-only changes in advancedPenetration.hpp do not auto-apply to AP-mode.
- Protocol or parser changes must be ported to AP-mode/BLE bridge.

## 5-Step Delta Port Process

1. Replace advancedPenetration.hpp with the new upstream version.
2. Run the delta check script:
   - powershell -ExecutionPolicy Bypass -File scripts/ap_mode_delta_check.ps1
3. Compare these mapped areas first:
   - parseConfig -> AP parseConfigString
   - parseStatus -> AP parseStatusString
   - setSpeed -> AP setSpeedValue
   - setBaseValue -> AP setBaseValue
   - setModifierValue -> AP setModifierValue
   - loadPresets -> AP parsePresetsString / preset flow
4. Apply only required changes in AP-mode.cpp and BleComm.cpp.
5. Build both targets and test on hardware.

## Change Matrix (What to Update)

### A) Visual-only changes in upstream

Examples:
- color palette changes
- text labels
- layout tweaks

Update:
- AP-mode.cpp drawApScreen only

### B) Config grammar changes

Examples:
- new token format in config characteristic
- changed min/max encoding

Update:
- AP-mode.cpp parseConfigString

### C) Status grammar changes

Examples:
- changed separators
- added/removed modifier status values

Update:
- AP-mode.cpp parseStatusString

### D) Control command changes

Examples:
- changed control payload format
- changed meaning of indexes

Update:
- AP-mode.cpp setSpeedValue/setBaseValue/setModifierValue
- possibly BleComm.cpp advanced control write logic

### E) Preset flow changes

Examples:
- new save command token
- different preset select command format

Update:
- AP-mode.cpp preset mode handlers
- BleComm.cpp advanced presets write logic if characteristic behavior changes

## Hardware Test Checklist

1. Enter AP-mode from Addons.
2. Confirm transport shows BLE live (or expected fallback).
3. Turn Encoder 1 and confirm speed updates correctly.
4. Turn Encoder 2/3/4 and verify base/modifier behavior.
5. Toggle start/stop with middle button.
6. Enter preset mode and apply a preset.
7. Trigger Save New Preset.
8. Exit AP-mode and re-enter to confirm state consistency.

## Files Usually Involved

- src/addons/advancedPenetration.hpp
- src/addons/AP-mode.cpp
- src/addons/AP-mode.h
- src/communication/BleComm.cpp
- src/communication/BleComm.h

## Notes

- Keep advancedPenetration.hpp unmodified unless explicitly required.
- AP-mode.cpp contains port-map comments near key mirrored functions.
- If upstream adds major features, do incremental ports with build/test after each chunk.
