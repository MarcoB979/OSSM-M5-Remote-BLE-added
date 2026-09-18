#pragma once

// Central addon service layer.
//
// The core firmware (main.cpp) should only talk to addons through these two
// functions — never include or call an individual addon directly. Each addon
// keeps its own state and throttling; this layer just fans the loop/setup
// hooks out to them.

// Enable/disable each addon according to the user's saved settings. Call once
// from setup(), after the BLE main task has been registered.
void addonsInit();

// Drive each addon's background work (telemetry mapping, reconnects, etc.).
// Call once per main-loop iteration. Individual addons throttle themselves.
void addonsTick();

// Staggered background reconnection for addons. Called from the screen state
// machine once per loop; probes at most one addon per tick and only after
// recent user input has settled.
void addonsBackgroundConnect();
