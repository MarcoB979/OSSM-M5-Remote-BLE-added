#pragma once

// ---------------------------------------------------------------------------
// config_power.h
// Screensaver and deep sleep defaults — safe to include from any translation unit.
// ---------------------------------------------------------------------------

#define SCREENSAVER_TIMEOUT_MS_DEFAULT     (2 * 60 * 1000)
#define SCREENSAVER_DIM_BRIGHTNESS_DEFAULT  15
#define DEEP_SLEEP_TIMEOUT_MS_DEFAULT      (15UL * 60UL * 1000UL)
// 0 = never auto power-off. Deep sleep turns WiFi off entirely, which makes the
// web server (and /remote) unreachable until the device is woken by a button
// press — that is why the pages only loaded "by accident". The screensaver
// (screen dim) still runs to save power; manual Sleep from the menu still works.
#define AUTO_IDLE_DEEP_SLEEP_ENABLED        0
