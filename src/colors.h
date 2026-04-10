#pragma once

#include <stdint.h>

// ---------------------------------------------------------------------------
// Color scheme definition
// ---------------------------------------------------------------------------
typedef struct {
    const char* name;       // Display name shown on the selector screen
    uint32_t    primary;    // Main accent: buttons, slider knob/indicator, header outline
    uint32_t    secondary;  // Light accent: slider track bg, focused-state bg
} UiColorScheme;

#define COLOR_SCHEME_COUNT 6

extern const UiColorScheme COLOR_SCHEMES[COLOR_SCHEME_COUNT];
extern int g_active_color_scheme;

// ---------------------------------------------------------------------------
// Public API (C-linkage where called from ui.c / C compilation units)
// ---------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif

void colors_ui_screen_init(void);       // Build the LVGL Colors selector screen
void colorSchemeScreenLoaded(void);     // Refresh selection indicators on screen load
void colorSchemeSelectIndex(int index); // User tapped scheme N — apply + persist
uint32_t getActivePrimaryColor(void);   // Get current scheme's primary color
uint32_t getActiveSecondaryColor(void); // Get current scheme's secondary color

#ifdef __cplusplus
}

// C++-only helpers
void colors_init();             // Load saved scheme from NVS and apply at boot
void applyColorScheme(int index); // Restyle all widgets to a given scheme index
void handleColorsScreen();      // Per-loop input/encoder handler for Colors screen

#endif
