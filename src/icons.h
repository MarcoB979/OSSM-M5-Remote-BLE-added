#pragma once

#include "main.h"
#include <stdint.h>

// Render a mask (array of rows with '#' for foreground pixels) into the
// provided canvas buffer. `isReady` is a per-canvas cache flag that will be
// reset when the active color scheme changes.
void icons_render_mask_canvas(lv_obj_t* canvas,
                              uint8_t* buffer,
                              bool& isReady,
                              const char* const* maskRows,
                              int width,
                              int height,
                              uint32_t bg_hex,
                              uint32_t fg_hex);
