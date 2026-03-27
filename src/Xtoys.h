#pragma once

#include <Arduino.h>

/**
 * X-Toys Integration - Silent Watcher Mode
 * 
 * This module allows M5 remote to release control to X-Toys app
 * while staying connected to the OSSM to prevent auto-return-to-menu.
 * 
 * Workflow:
 * 1. User in BLE mode (Home/Menu)
 * 2. User presses Right button
 * 3. M5 enters StrokeEngine mode
 * 4. M5 pauses motor (speed=0)
 * 5. M5 displays "Waiting for X-Toys..."
 * 6. M5 stays BLE-connected (prevents auto-menu)
 * 7. X-Toys app connects and streams commands
 * 8. Motor responds to X-Toys, not M5
 * 9. User can:
 *    a) Wait for X-Toys to finish and disconnect
 *    b) Press Right button again to resume M5 control
 */

// Initialize X-Toys mode (call once at startup)
void XToysInit();

// Activate X-Toys mode (called by right button handler)
void XToysActivate();

// Check if X-Toys mode is currently active
bool XToysIsActive();

// Deactivate and resume normal M5 control (called by right button or when X-Toys disconnects)
void XToysDeactivate();

// Update X-Toys status (call periodically to check connection state)
void XToysUpdate();

// Display X-Toys status on UI
void XToysDisplayStatus();
