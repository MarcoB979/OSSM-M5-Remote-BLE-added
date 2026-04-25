#pragma GCC optimize ("Ofast")
#include <M5Unified.h>
#include "config.h"
#include <Arduino.h>
#include <lvgl.h>
#include "ui/ui.h"
#include "main.h"
#include "screen.h"
#include "colors.h"
#include "styles.h"
#include "Eject.h"
#include "Preferences.h"      //EEPROM replacement function
#include <esp_sleep.h>


// Tasks:
// Single-definition globals
int buttonDebounceMs = 40;

void setup(){
  Serial.begin(115200);
  vTaskDelay(pdMS_TO_TICKS(50));
  Serial.println("\n\nBooting...");
  esp_sleep_wakeup_cause_t wakeCause = esp_sleep_get_wakeup_cause();
  auto cfg = M5.config();
  M5.begin(cfg);
  LogDebugFormatted("MX boot raw pin%d=%d\n", Button1.pin(), digitalRead(Button1.pin()));

  Preferences m5prf;
  m5prf.begin("m5-ctnr", false); 
  // Loads these settings at boot
  LogDebug("Loading settings...");
  dark_mode = m5prf.getBool("Darkmode", true);       // ^ (basically first boot defaults, saving settings surives a re-flash!)
  vibrate_mode = m5prf.getBool("Vibrate", true);
  touch_home= m5prf.getBool("Lefty", false);       // = touchcreen. There apears to be no actual lefthanded mode anywhere
  strokeinvert_mode = m5prf.getBool("StrokeInvert");
  int brightness = m5prf.getInt("Brightness", -1);
  if (brightness < 0) {
    brightness = 180; // your default
    m5prf.putInt("Brightness", brightness); // save default to NVS
  } else if (brightness < 30) {
    brightness = 30; // enforce minimum brightness
    m5prf.putInt("Brightness", brightness);
  }

  g_brightness_value = brightness;
  m5prf.end();
  // --- Apply brightness at startup ---
  M5.Lcd.setBrightness(brightness);
  LogDebugFormatted("Brightness applied: %d", brightness);

  // ...existing code...

  
  M5.Power.setChargeCurrent(BATTERY_CHARGE_CURRENT);
  LogDebug("\n Starting");      // Start LogDebug
  LogDebug("TEST ");

  EspNowInitCommunication();
  
  encoder1.attachFullQuad(ENC_1_CLK, ENC_1_DT);
  encoder2.attachFullQuad(ENC_2_CLK, ENC_2_DT);
  encoder3.attachFullQuad(ENC_3_CLK, ENC_3_DT);
  encoder4.attachFullQuad(ENC_4_CLK, ENC_4_DT);
  Button1.attachClick(mxclick);
  Button1.attachDoubleClick(mxdouble);
  Button1.attachLongPressStart(mxlong);
  Button1.setDebounceMs(buttonDebounceMs);
  Button1.setClickMs(200);
  Button1.setLongPressIntervalMs(380);
  Button2.attachClick(clickLeft);
  Button2.attachLongPressStart(clickLeftLong);
  Button2.setDebounceMs(buttonDebounceMs);
  Button2.setClickMs(200);
  Button2.setLongPressIntervalMs(380);
  Button3.attachClick(clickRight);
  Button3.setDebounceMs(buttonDebounceMs);
  Button3.setClickMs(200);
  Button3.attachLongPressStart(clickRightLong);
  Button3.setLongPressIntervalMs(380);
  
  // Initialize `disp_buf` display buffer with the buffer(s).
  // lv_draw_buf_init(&draw_buf, LV_HOR_RES_MAX, LV_VER_RES_MAX);
  M5.Display.setEpdMode(epd_mode_t::epd_fastest); // fastest but very-low quality.
  if (M5.Display.width() < M5.Display.height())
  { /// Landscape mode.
  M5.Display.setRotation(M5.Display.getRotation() ^ 1);
  }
  
  lv_init();
  lv_tick_set_cb(my_tick_function);

  lv_display_t *display = lv_display_create(HOR_RES, VER_RES);
  lv_display_set_flush_cb(display, my_display_flush);

  // Force a strict alignment for LVGL draw buffer.
  static lv_color_t buf1[HOR_RES * 15] __attribute__((aligned(16)));
  lv_display_set_buffers(display, buf1, nullptr, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);

  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, my_touchpad_read);

  LogDebug("Works till step 1");
  // Initialize shared LVGL styles before building the UI screens so screens
  // can add these styles during creation.
  styles_init();

  ui_init();
  colors_init();   // Load saved color scheme from NVS and apply to all widgets
  markStartScreenLoaded();
  
  if (wakeCause == ESP_SLEEP_WAKEUP_EXT1 || wakeCause == ESP_SLEEP_WAKEUP_EXT0) {
    // After deep sleep wake, return to start screen so reconnect can happen cleanly.
    lv_scr_load(ui_Start);
  }
  register_event_debug_callbacks();
  refreshHomeAndStreamingStartStopUi();
LogDebug("Works till step 2");
  
  // --- Restore stroke invert state from NVS and apply to UI (after ui_init) ---
  if (strokeinvert_mode) {
    lv_obj_add_state(ui_strokeinvert, LV_STATE_CHECKED);
  } else {
    lv_obj_clear_state(ui_strokeinvert, LV_STATE_CHECKED);
  }

  // Auto-connect once ui_Start has finished loading (first rendered frame).
  lv_obj_add_event_cb(ui_Start, [](lv_event_t *) { markStartScreenLoaded(); connectbutton(nullptr); },
                      LV_EVENT_SCREEN_LOADED, nullptr);

  lv_obj_clear_state(ui_HomeButtonL, LV_STATE_DISABLED);
LogDebug("Works till step 3");
  
  // Addon pairing state is derived from persisted AddonSlot bindings.
  addonsInitFromStorage();
  // ui_ejectaddon is a legacy widget that no longer exists in the active UI screens.
  // eject_status is now managed by addonsInitFromStorage via the AddonSlot NVS keys.
  if (ui_ejectaddon != nullptr) {
    if(eject_status == true){
      lv_obj_add_state(ui_ejectaddon, LV_STATE_CHECKED);
    } else {
      lv_obj_clear_state(ui_ejectaddon, LV_STATE_CHECKED);
    }
  }
  if(dark_mode == true){
    lv_obj_add_state(ui_darkmode, LV_STATE_CHECKED);
  }
  if(vibrate_mode == true){
    lv_obj_add_state(ui_vibrate, LV_STATE_CHECKED);
  }
  if(touch_home == true){
    lv_obj_add_state(ui_lefty, LV_STATE_CHECKED);
  }
  lv_roller_set_selected(ui_PatternS,2,LV_ANIM_OFF);
  pattern = lv_roller_get_selected(ui_PatternS);
  lv_roller_get_selected_str(ui_PatternS,patternstr,0);
  lv_label_set_text(ui_HomePatternLabel,patternstr);
  last_activity_ms = millis();

  LogDebug("Setup complete");
}

void loop(){
  screen_power_tick();

  M5.update();
  lv_task_handler();
  Button1.tick();
  Button2.tick();
  Button3.tick();

  monitorOssmState(false);

  // Process any pending ESP-NOW UI updates from wifi task (thread-safe)
  EspNowProcessPendingUiUpdates();

  handleCurrentScreen();

  vTaskDelay(pdMS_TO_TICKS(5));
}
