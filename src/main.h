// main.h — process-wide globals defined in main.cpp and shared with the UI
// and communication layers.
#pragma once

// OSSM machine limits, kept in sync with the connected OSSM by BleComm.
extern float maxdepthinmm;
extern float speedlimit;

// UI toggles
extern bool dark_mode;
extern bool AtStartup;   // true on first boot; drives the Start-screen auto-connect

