#include "icons.h"
#include "colors.h"

void icons_render_mask_canvas(lv_obj_t* canvas,
                              uint8_t* buffer,
                              bool& isReady,
                              const char* const* maskRows,
                              int width,
                              int height,
                              uint32_t bg_hex,
                              uint32_t fg_hex)
{
  if (canvas == nullptr || buffer == nullptr || maskRows == nullptr) {
    return;
  }

  lv_canvas_set_buffer(canvas, buffer, width, height, LV_COLOR_FORMAT_ARGB8888);

  static int last_scheme_rendered = -1;
  if (last_scheme_rendered != g_active_color_scheme) {
    isReady = false;
    last_scheme_rendered = g_active_color_scheme;
  }
  if (isReady) return;

  lv_color_t fg = lv_color_hex(fg_hex);
  lv_color_t bg = lv_color_hex(bg_hex);
  lv_canvas_fill_bg(canvas, bg, LV_OPA_COVER);

  for (int y = 0; y < height; ++y) {
    const char* row = maskRows[y];
    if (row == nullptr) continue;
    for (int x = 0; x < width; ++x) {
      if (row[x] == '#') {
        lv_canvas_set_px(canvas, x, y, fg, LV_OPA_COVER);
      }
    }
  }

  isReady = true;
}
