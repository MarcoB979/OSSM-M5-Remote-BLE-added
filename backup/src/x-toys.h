#pragma once

#if __has_include("lvgl.h")
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

// Public API for the X-toys addon screen
void XtoysPrepareScreen();
lv_obj_t * XtoysGetScreen();
