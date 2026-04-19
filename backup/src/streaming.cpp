#include <Arduino.h>

#include "main.h"
#include "OssmBLE.h"
#include "language.h"
#include "ui/ui.h"
#include "ui/ui_helpers.h"

static constexpr int OSSM_TARGET_ID = 1;

bool g_streaming_entry_flow_pending = false;
bool g_streaming_controls_locked = false;

extern "C" void requestStreamingEntryFlow(void){
  g_streaming_entry_flow_pending = true;
}

extern "C" void streamingReturnToMenu(void){
  g_streaming_controls_locked = false;
  if (OssmBleIsMode()) {
    OssmBleGoToMenu();
  }
  _ui_screen_change(ui_Menu, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
}

static void prepareStreamingAtFullScale(){
  speed = 100;
  depth = 100;
  stroke = 100;

  lv_slider_set_value(ui_streamingspeedslider, 100, LV_ANIM_OFF);
  lv_slider_set_value(ui_streamingdepthslider, 100, LV_ANIM_OFF);
  lv_slider_set_value(ui_streamingstrokeslider, 100, LV_ANIM_OFF);
  lv_label_set_text(ui_streamingspeedvalue, "100");
  lv_label_set_text(ui_streamingdepthvalue, "100");
  lv_label_set_text(ui_streamingstrokevalue, "100");

  SendCommand(SPEED, 100, OSSM_TARGET_ID);
  SendCommand(DEPTH, 100, OSSM_TARGET_ID);
  SendCommand(STROKE, 100, OSSM_TARGET_ID);
}

void handleStreamingEntryFlow(){
  g_streaming_entry_flow_pending = false;
  g_streaming_controls_locked = true;

  showNotification(T_STREAMING_CAUTION_TITLE,
                   T_STREAMING_CAUTION_TEXT,
                   5000,
                   false,
                   nullptr,
                   false,
                   nullptr,
                   true);

  if (OssmBleIsMode()) {
    if (!OssmBleGoToStreaming()) {
      showNotification(T_STREAMING_FAIL_TITLE,
                       T_STREAMING_FAIL_TEXT,
                       2500,
                       false,
                       nullptr,
                       false,
                       nullptr,
                       true);
      streamingReturnToMenu();
      return;
    }

    vTaskDelay(pdMS_TO_TICKS(200));

    // Set buffer to 0: OSSM acts on each stream:pos:time frame immediately,
    // with no pre-buffering delay. Required for low-latency external stream sources.
    OssmBleSetBuffer(0.0f);
  }

  prepareStreamingAtFullScale();

  // Show START prompt
  while (true) {
    int result = showNotification(T_STREAMING_ACTIVE_TITLE,
                                  T_STREAMING_ACTIVE_TEXT,
                                  0,
                                  true,
                                  T_STOP_STREAMING,
                                  true,
                                  T_START,
                                  true);

    if (result == NOTIFICATION_RESULT_LEFT) {
      streamingReturnToMenu();
      return;
    }

    if (result == NOTIFICATION_RESULT_RIGHT) {
      streamingStartOnlyAction();
      break;  // Exit to show active streaming notification
    }
  }

  // Show ACTIVE streaming notification with only Stop button
  while (true) {
    int result = showNotification(T_STREAMING_RUNNING_TITLE,
                                  T_STREAMING_RUNNING_TEXT,
                                  0,
                                  true,
                                  T_STOP_STREAMING,
                                  false,
                                  nullptr,
                                  false);

    if (result == NOTIFICATION_RESULT_LEFT) {
      streamingReturnToMenu();
      return;
    }
  }
}
