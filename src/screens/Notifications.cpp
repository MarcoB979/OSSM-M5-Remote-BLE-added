// Notifications.cpp — modal notification overlay (blocking showNotification).
//
// Extracted from ScreenHandler.cpp during the Phase B split. Owns the modal
// overlay UI and its left/right button callbacks.

#include "ScreenHandler.h"
#include "ScreenHandler_internal.h"

#include <lvgl.h>
#include <M5Unified.h>
#include <Arduino.h>

#include "../ui/ui.h"
#include "../ui/ui_helpers.h"
#include "../main.h"
#include "../display/colors.h"
#include "../display/styles.h"
#include "../buttonhandlers/ButtonHandlers.h"
#include "language.h"

// Touch result set by the LVGL button callbacks below.
static volatile int g_notification_touch_result = NOTIFICATION_RESULT_NONE;

// -------------------------------------------------------
// Notification Overlay Helpers
// -------------------------------------------------------
static void notification_left_button_cb(lv_event_t *e)
{
    (void)e;
    g_notification_touch_result = NOTIFICATION_RESULT_LEFT;
}

static void notification_right_button_cb(lv_event_t *e)
{
    (void)e;
    g_notification_touch_result = NOTIFICATION_RESULT_RIGHT;
}

// ---------------------------------------------------------------------------
// showNotification() — blocking modal overlay (ported from backup firmware)
// ---------------------------------------------------------------------------
int showNotification(const char *title,
                     const char *text,
                     uint32_t duration,
                     bool showLeftButton,
                     const char *leftButtonText,
                     bool showRightButton,
                     const char *rightButtonText,
                     bool showFullScreen)
{
    const bool hasButtons = showLeftButton || showRightButton;
    const bool prevTouchDisabled = touch_disabled;
    const bool shouldBlockTouch  = !hasButtons;
    const uint32_t startMs = millis();
    int result = NOTIFICATION_RESULT_NONE;
    g_notification_touch_result = NOTIFICATION_RESULT_NONE;

    // Derive color scheme values
    uint32_t schemePrimary       = getActivePrimaryColor();
    uint32_t schemeSecondary     = getActiveSecondaryColor();
    uint32_t schemeTextPrimary   = getActiveTextPrimaryColor();
    uint32_t schemeTextSecondary = getActiveTextSecondaryColor();
    uint8_t pr = (schemePrimary >> 16) & 0xFF;
    uint8_t pg = (schemePrimary >>  8) & 0xFF;
    uint8_t pb =  schemePrimary        & 0xFF;
    uint32_t schemeDarker = (((pr >> 1) & 0xFF) << 16) |
                            (((pg >> 1) & 0xFF) <<  8) |
                             ((pb >> 1) & 0xFF);

    if (shouldBlockTouch) touch_disabled = true;

    // Drain stale button states before opening the modal.
    mxpress_waspressed       = false;
    mxclick_short_waspressed  = false;
    mxclick_long_waspressed   = false;
    click2_short_waspressed   = false;
    click2_long_waspressed    = false;
    click3_short_waspressed   = false;
    click3_long_waspressed    = false;
    click3_double_waspressed  = false;

    lv_obj_t *overlay = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(overlay);
    lv_obj_set_size(overlay, HOR_RES, VER_RES);
    lv_obj_center(overlay);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_50, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(overlay, lv_color_hex(schemeDarker), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(overlay, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *panel = lv_obj_create(overlay);
    if (showFullScreen) {
        const int topOffset     = 32;
        const int bottomPadding = 5;
        lv_obj_set_size(panel, 310, VER_RES - topOffset - bottomPadding);
        lv_obj_set_pos(panel, 5, topOffset);
    } else {
        lv_obj_set_size(panel, (HOR_RES * 90) / 100, (VER_RES * 75) / 100);
        lv_obj_align(panel, LV_ALIGN_CENTER, 0, 5);
    }
    lv_obj_set_style_radius(panel, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(panel, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(panel, lv_color_hex(schemePrimary), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(panel, lv_color_hex(schemeSecondary), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *titleBar = lv_obj_create(panel);
    lv_obj_remove_style_all(titleBar);
    lv_obj_set_size(titleBar, lv_pct(100), 32);
    lv_obj_align(titleBar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(titleBar, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(titleBar, lv_color_hex(schemeDarker), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *titleLabel = lv_label_create(titleBar);
    lv_label_set_text(titleLabel, (title != nullptr && title[0] != '\0') ? title : T_NOTIFICATION_TITLE);
    lv_obj_set_style_text_align(titleLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(titleLabel, lv_color_hex(schemeTextPrimary), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(titleLabel, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(titleLabel);

    lv_obj_t *bodyLabel = lv_label_create(panel);
    lv_obj_set_width(bodyLabel, lv_pct(90));
    lv_label_set_long_mode(bodyLabel, LV_LABEL_LONG_WRAP);
    lv_label_set_text(bodyLabel, (text != nullptr) ? text : "");
    lv_obj_set_style_text_align(bodyLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(bodyLabel, lv_color_hex(schemeTextPrimary), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(bodyLabel, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(bodyLabel, LV_ALIGN_TOP_MID,0, 38);

    if (hasButtons) {
        lv_obj_t *buttonRow = lv_obj_create(panel);
        lv_obj_remove_style_all(buttonRow);
        lv_obj_set_size(buttonRow, lv_pct(94), 44);
        lv_obj_align(buttonRow, LV_ALIGN_BOTTOM_MID, 0, -5);
        lv_obj_set_style_bg_opa(buttonRow, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(buttonRow, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(buttonRow, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        if (showLeftButton) {
            lv_obj_t *leftBtn = lv_btn_create(buttonRow);
            lv_obj_set_size(leftBtn, 120, 36);
            lv_obj_align(leftBtn, LV_ALIGN_LEFT_MID, 0, 0);
            lv_obj_add_style(leftBtn, &style_button_l, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_style(leftBtn, &style_button_l_pressed, LV_PART_MAIN | LV_STATE_PRESSED);
            lv_obj_t *leftLbl = lv_label_create(leftBtn);
            lv_label_set_text(leftLbl, (leftButtonText != nullptr && leftButtonText[0] != '\0') ? leftButtonText : T_LEFT);
            lv_obj_set_style_text_color(leftLbl, lv_color_hex(schemeTextPrimary), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_center(leftLbl);
            lv_obj_add_event_cb(leftBtn, notification_left_button_cb, LV_EVENT_SHORT_CLICKED, nullptr);
        }

        if (showRightButton) {
            lv_obj_t *rightBtn = lv_btn_create(buttonRow);
            lv_obj_set_size(rightBtn, 120, 36);
            lv_obj_align(rightBtn, LV_ALIGN_RIGHT_MID, 0, 0);
            lv_obj_add_style(rightBtn, &style_button_l, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_style(rightBtn, &style_button_l_pressed, LV_PART_MAIN | LV_STATE_PRESSED);
            lv_obj_t *rightLbl = lv_label_create(rightBtn);
            lv_label_set_text(rightLbl, (rightButtonText != nullptr && rightButtonText[0] != '\0') ? rightButtonText : T_RIGHT);
            lv_obj_set_style_text_color(rightLbl, lv_color_hex(schemeTextPrimary), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_center(rightLbl);
            lv_obj_add_event_cb(rightBtn, notification_right_button_cb, LV_EVENT_SHORT_CLICKED, nullptr);
        }
    }

    while (true) {
        M5.update();
        lv_task_handler();
        Button1.tick();
        Button2.tick();
        Button3.tick();

        if (duration > 0 && (millis() - startMs) >= duration) {
            result = NOTIFICATION_RESULT_NONE;
            break;
        }

        if (hasButtons) {
            if (g_notification_touch_result != NOTIFICATION_RESULT_NONE) {
                result = g_notification_touch_result;
                break;
            }
            if (showLeftButton && click2_short_waspressed) {
                result = NOTIFICATION_RESULT_LEFT;
                break;
            }
            if (showRightButton && click3_short_waspressed) {
                result = NOTIFICATION_RESULT_RIGHT;
                break;
            }
        }

        // Consume all button events so the current screen never sees stale flags.
        mxpress_waspressed       = false;
        mxclick_short_waspressed  = false;
        mxclick_long_waspressed   = false;
        click2_short_waspressed   = false;
        click2_long_waspressed    = false;
        click3_short_waspressed   = false;
        click3_long_waspressed    = false;
        click3_double_waspressed  = false;
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    lv_obj_del(overlay);

    // Clear flags after modal closes.
    mxpress_waspressed       = false;
    mxclick_short_waspressed  = false;
    mxclick_long_waspressed   = false;
    click2_short_waspressed   = false;
    click2_long_waspressed    = false;
    click3_short_waspressed   = false;
    click3_long_waspressed    = false;
    click3_double_waspressed  = false;

    if (shouldBlockTouch) touch_disabled = prevTouchDisabled;

    return result;
}

// -------------------------------------------------------
