// ColorSchemes.cpp — COLOR_SCHEMES array definition
// Kept separate from colors.cpp so the large initializer is easy to edit.

#include "colors.h"

const UiColorScheme COLOR_SCHEMES[COLOR_SCHEME_COUNT] = {
    // { name, title_bar, button_l, button_m, button_r,
    //   slider1, slider2, slider3, slider4,
    //   battery_main, battery_indicator, roller, focussed_element,
    //   background, text_primary, text_secondary }
    { "Deep Purple",
        0x83277B, // title_bar
        0x83277B, // button_l
        0x83277B, // button_m
        0x83277B, // button_r
        0xD591D5, // slider1 (Lavender)
        0xD591D5, // slider2
        0xD591D5, // slider3
        0xD591D5, // slider4
        0xD591D5, // battery_main
        0x83277B, // battery_indicator
        0xB481AC, // roller (Mauve)
        0xE91E63, // focussed_element (Pink)
        0x000000, // background (Black)
        0xFFFFFF, // text_primary (White)
        0x000000  // text_secondary (Black)
    },
    { "Midnight Navy",
        0x1E3A6E,
        0x1E3A6E, 0x1E3A6E, 0x1E3A6E,
        0x7A9FCC, 0x7A9FCC, 0x7A9FCC, 0x7A9FCC,
        0x7A9FCC, 0x1E3A6E,
        0x5A7CA5, // roller
        0xFF4081, // focussed_element (Fuchsia)
        0x000000, 0xFFFFFF, 0x000000
    },
    { "Army Green",
        0x3D5C3D,
        0x3D5C3D, 0x3D5C3D, 0x3D5C3D,
        0x8BA88B, 0x8BA88B, 0x8BA88B, 0x8BA88B,
        0x8BA88B, 0x3D5C3D,
        0x6B8E23, // roller
        0xFFC107, // focussed_element (Amber)
        0x000000, 0xFFFFFF, 0x000000
    },
    { "Steel Blue",
        0x2C5F7A,
        0x2C5F7A, 0x2C5F7A, 0x2C5F7A,
        0x6AABCC, 0x6AABCC, 0x6AABCC, 0x6AABCC,
        0x6AABCC, 0x2C5F7A,
        0x4682B4, // roller
        0x64B5F6, // focussed_element (Light Blue)
        0x000000, 0xFFFFFF, 0x000000
    },
    { "Amber Sunset",
        0xB8860B,
        0xB8860B, 0xB8860B, 0xB8860B,
        0xFFD700, 0xFFD700, 0xFFD700, 0xFFD700,
        0xFFD700, 0xB8860B,
        0xFFA500, // roller
        0xFF7043, // focussed_element (Coral)
        0x000000, 0xFFFFFF, 0x000000
    },
    { "Rainbow",
        0xE53935, // title_bar (Red)
        0x8E24AA, // button_l (Purple)
        0x43A047, // button_m (Green)
        0x1E88E5, // button_r (Blue)
        0xE53935, // slider1 (Red)
        0xFDD835, // slider2 (Yellow)
        0x43A047, // slider3 (Green)
        0x1E88E5, // slider4 (Blue)
        0x8E24AA, // battery_main (Purple)
        0xF4511E, // battery_indicator (Orange Red)
        0x00ACC1, // roller (Cyan)
        0xE91E63, // focussed_element (Pink)
        0x000000, 0xFFFFFF, 0x000000
    }
};
