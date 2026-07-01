# AP-Mode Addon User Manual

This guide explains how to use the AP-Mode addon on the M5 Remote.

## What AP-Mode Is

AP-Mode is an Advanced Penetration control addon.
It provides:

- live parameter control from the M5 Remote
- preset selection and save actions
- fallback operation when advanced BLE characteristics are not available

## Before You Start

1. Flash firmware that includes AP-Mode.
2. Power on your M5 Remote.
3. Ensure your target OSSM/AP device is powered and advertising over BLE.
4. Connect from the Start screen as usual.

## Open AP-Mode

1. Go to the Addons screen.
2. Select AP-Mode.
3. Press the right button to open AP Preset Mode when needed.

## Screen Indicators

AP-Mode shows:

- State: RUNNING or PAUSED
- Speed (SP)
- Current Base parameter and value
- Current Modifier parameter and value
- Transport status:
  - BLE live = advanced AP characteristics found and active
  - fallback = AP characteristics not available

If fallback is active, a warning line is shown:

- Warning: AP BLE chars not detected

## Controls (Normal Mode)

In normal mode (Preset mode OFF):

- Left button: back to previous screen
- Middle button: start/stop (resume from last non-zero speed)
- Right button: toggle Preset mode ON

Encoders in normal mode:

- Encoder 1: speed (SP)
- Encoder 2: select base parameter
- Encoder 3: adjust selected base value
- Encoder 4: adjust selected modifier value

## Controls (Preset Mode)

In preset mode (Preset mode ON):

- Left button: back to previous screen
- Middle button:
  - apply selected preset
  - or save a new preset when "Save New Preset" is selected
- Right button: toggle Preset mode OFF

Encoders in preset mode:

- Encoder 2: select preset entry

Preset behavior:

- Normal preset entry sends apply command
- "Save New Preset" sends save command

## Live vs Fallback Behavior

### BLE live

When AP characteristics are detected:

- config/status/presets are read from the target device
- control updates are transmitted live to target
- preset apply/save commands are transmitted live

### fallback

When AP characteristics are not detected:

- AP-Mode runs with internal default model
- UI remains usable for test/navigation
- live control and preset commands are not sent

## Troubleshooting

1. Transport stays on fallback:
- verify target firmware exposes AP characteristic UUIDs
- reconnect BLE from start screen
- power-cycle both devices

2. Presets list does not update:
- ensure BLE live is active
- enter preset mode and wait a moment for refresh

3. Start/stop does not affect device:
- check BLE live transport status
- verify AP target accepts advanced control characteristic writes

## Safety Notes

- Start with low speed values when testing a new setup.
- Verify physical setup and limits before applying saved presets.
- Stop immediately if motion does not match expected behavior.

## Notes for Maintainers

- This addon is implemented in AP wrapper files and BLE bridge.
- The original advanced source header is intentionally left unchanged.
