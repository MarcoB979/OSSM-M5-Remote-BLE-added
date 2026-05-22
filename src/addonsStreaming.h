#pragma once

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern lv_obj_t *ui_FistIT;

void addonsMoveSelection(int delta);
void addonsActivateSelection(void);
void addonsSyncSelectionVisual(void);

void addonsHandleEjectScreen(void);
void addonsHandleFistScreen(void);
void addonsFistToggle(void);

#ifdef __cplusplus
}
#endif
