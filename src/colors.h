#pragma once

#include <stdint.h>

// ---------------------------------------------------------------------------
// Color scheme definition
// ---------------------------------------------------------------------------
typedef struct {
    const char* name;       // Display name shown on the selector screen
    uint32_t    title_bar;        // Header / title bar color
    uint32_t    button_l;         // Left button / general button accent
    uint32_t    button_m;         // Middle button accent
    uint32_t    button_r;         // Right button accent
    uint32_t    slider1;          // Slider slot 1 color
    uint32_t    slider2;          // Slider slot 2 color
    uint32_t    slider3;          // Slider slot 3 color
    uint32_t    slider4;          // Slider slot 4 color
    uint32_t    battery_main;     // Battery main fill
    uint32_t    battery_indicator;// Battery indicator
    uint32_t    roller;           // Roller/menu highlight
    uint32_t    focussed_element; // Accent color used for focused elements (e.g., pink in Rainbow)
    uint32_t    background;       // Screen background
    uint32_t    text_primary;     // Text used on dark/accent backgrounds (usually white)
    uint32_t    text_secondary;   // Text used on light backgrounds (usually black)
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
uint32_t getActiveTextPrimaryColor(void);   // Get current scheme's primary text color
uint32_t getActiveTextSecondaryColor(void); // Get current scheme's secondary text color
uint32_t getActiveTitleBarColor(void);
uint32_t getActiveButtonLColor(void);
uint32_t getActiveButtonMColor(void);
uint32_t getActiveButtonRColor(void);
uint32_t getActiveSlider1Color(void);
uint32_t getActiveSlider2Color(void);
uint32_t getActiveSlider3Color(void);
uint32_t getActiveSlider4Color(void);
uint32_t getActiveBatteryMainColor(void);
uint32_t getActiveBatteryIndicatorColor(void);
uint32_t getActiveRollerColor(void);
uint32_t getActiveBackgroundColor(void);





#ifdef __cplusplus
}

// C++-only helpers
struct ButtonEvents;
void colors_init();             // Load saved scheme from NVS and apply at boot
void applyColorScheme(int index); // Restyle all widgets to a given scheme index
void handleColorsScreen(const ButtonEvents &events);      // Per-loop input/encoder handler for Colors screen

#endif
