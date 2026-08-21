
#include <Arduino.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <cstring>

#include "main.h"
#include "language.h"
#include "display/colors.h"
#include "ui/ui.h"
#include "ui/ui_helpers.h"
#include "display/styles.h"
#include "buttonhandlers/ButtonHandlers.h"
#include "screens/ScreenHandler.h"
#include "config/debug.h"

#include "ExampleAddon.h"

// Single-definition of ExampleAddon_ID (C linkage so C code can reference it)
extern "C" const int ExampleAddon_ID = 3;
// ---------------------------------------------------------------------------
// Template variables to customize for your addon
// ---------------------------------------------------------------------------
// Change only this block to rename labels and default values.
const char *Title = "Example Addon";
const char *slider1Text = "Slider 1";
const char *slider2Text = "Slider 2";
const char *slider3Text = "Slider 3";
const char *slider4Text = "Slider 4";
const char *leftButtonText = "Left";
const char *middleButtonText = "Middle";
const char *rightButtonText = "Right";

// Optional: middle button text while ON/OFF.
const char *middleButtonTextOn = middleButtonText;   // e.g. "Pause"
const char *middleButtonTextOff = middleButtonText;  // e.g. "Start"

// Live slider values (kept in sync with encoder/slider changes below).
// Example: rotating slider2 to 20 updates slider2Value to 20.
float slider1Value = 0.0f;
float slider2Value = 0.0f;
float slider3Value = 0.0f;
float slider4Value = 0.0f;

// Slider command placeholders.
// Replace these with real command IDs from your protocol before using this addon.
static constexpr int CMD_SLIDER1 = 1001;
static constexpr int CMD_SLIDER2 = 1002;
static constexpr int CMD_SLIDER3 = 1003;
static constexpr int CMD_SLIDER4 = 1004;

// Slider ranges and command mapping.
int slider1Min = 0;
int slider1Max = 100;
int slider1Command = CMD_SLIDER1;
int slider2Min = 0;
int slider2Max = 360;
int slider2Command = CMD_SLIDER2;
int slider3Min = 0;
int slider3Max = 100;
int slider3Command = CMD_SLIDER3;
int slider4Min = 0;
int slider4Max = 100;
int slider4Command = CMD_SLIDER4;

// Additional behavior toggles.
static bool s_ramp_enabled = true;  // faster change when knob is rotated fast

// UI handle for this addon screen (referenced from other modules)
lv_obj_t *ui_ExampleAddon = nullptr;

// ---------------------------------------------------------------------------
// Integration checklist (what to add in other files)
// ---------------------------------------------------------------------------
// 1) Add addon API declarations in your addon header (PrepareScreen/GetScreen/HandleScreen/...)
// 2) Register the addon in addonsStreaming.cpp:
//    - include your addon header
//    - add activate function calling PrepareScreen() + _ui_screen_change(GetScreen(), ...)
//    - add entry to s_addon_defs[]
// 3) Add a state in ScreenHandler.h and dispatch in ScreenHandler.cpp handleScreens()
//    to call YourAddonHandleScreen(events).
// 4) Route incoming ESP-NOW frames in your comm layer to YourAddonHandleIncomingEspNowFrame(...).
// 5) Expose ui_ExampleAddon in ui.h if other modules need direct screen checks.
// 6) If you use settings persistence, add NVS read/write keys for your addon.
// 7) Rename ExampleAddon symbols to your addon name (letters/numbers only, no spaces).

// Quick Start:
// 1) Edit only the variables in the customization block above (Title, slider text, button text, ranges, commands).
//    Replace CMD_SLIDER1..4 placeholders with your real command IDs.
// 2) Rename ExampleAddon symbols in this file + header to your addon name.
// 3) Follow the integration checklist and add your addon to addonsStreaming + ScreenHandler routing.





namespace {

struct ExampleAddon_Message {
  float esp_speed;
  float esp_depth;
  float esp_stroke;
  float esp_sensation;
  float esp_pattern;
  bool esp_rstate;
  bool esp_connected;
  bool esp_heartbeat;
  int esp_command;
  float esp_value;
  int esp_target;
  int esp_sender;
};

static lv_obj_t *s_screen = nullptr;

static lv_obj_t *s_title = nullptr;
static lv_obj_t *s_button_left = nullptr;
static lv_obj_t *s_button_mid = nullptr;
static lv_obj_t *s_button_right = nullptr;
static lv_obj_t *s_button_left_text = nullptr;
static lv_obj_t *s_button_mid_text = nullptr;
static lv_obj_t *s_button_right_text = nullptr;

static lv_obj_t *s_slider1_label = nullptr;
static lv_obj_t *s_slider1_slider = nullptr;
static lv_obj_t *s_slider1_value = nullptr;
static lv_obj_t *s_batt_title = nullptr;
static lv_obj_t *s_batt_value = nullptr;
static lv_obj_t *s_slider2_label = nullptr;
static lv_obj_t *s_slider2_slider = nullptr;
static lv_obj_t *s_slider2_value = nullptr;
static lv_obj_t *s_slider3_label = nullptr;
static lv_obj_t *s_slider3_slider = nullptr;
static lv_obj_t *s_slider3_value = nullptr;
static lv_obj_t *s_slider4_label = nullptr;
static lv_obj_t *s_slider4_slider = nullptr;
static lv_obj_t *s_slider4_value = nullptr;

static long s_enc1 = 0;
static long s_enc2 = 0;
static long s_enc3 = 0;
static long s_enc4 = 0;

static int s_ramp_value = 1;
static int s_ramp_time_ms = 75;
static int s_ramp_max = 8;
static int s_ramp_active_encoder = 0;
static unsigned long s_ramp_ms = 0;

static bool s_is_paired = false;
static bool s_is_on = false;
static bool s_addon_enabled = false;
static uint8_t s_example_addr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static constexpr uint8_t BROADCAST_ADDR[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static constexpr int LEGACY_ExampleAddon_ID = 3;
static constexpr int LEGACY_M5_ID = M5_ID;
static uint32_t s_last_pairing_heartbeat_ms = 0;
static int s_peer_id = ExampleAddon_ID;
static int s_local_id = M5_ID;
static bool s_flush_buttons_once = false;

static void lockEspNowChannelIfConfigured(const char *reason)
{
  if (ESP_NOW_CHANNEL <= 0) {
    return;
  }

  uint8_t current = 0;
  wifi_second_chan_t second = WIFI_SECOND_CHAN_NONE;
  if (esp_wifi_get_channel(&current, &second) != ESP_OK) {
    return;
  }
  if ((int)current == ESP_NOW_CHANNEL) {
    return;
  }

  esp_wifi_set_promiscuous(true);
  esp_err_t setResult = esp_wifi_set_channel(ESP_NOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
  //LogDebugFormatted("[Addon][CHAN] %s current=%d target=%d result=%d\n",
  //                  reason ? reason : "set",
  //                  (int)current,
  //                  ESP_NOW_CHANNEL,
  //                  (int)setResult);
}

static bool ensurePeer(const uint8_t *addr)
{
  const bool isBroadcast = (addr[0] == 0xFF && addr[1] == 0xFF && addr[2] == 0xFF &&
                            addr[3] == 0xFF && addr[4] == 0xFF && addr[5] == 0xFF);
  if (isBroadcast) {
    return true;
  }

  if (esp_now_is_peer_exist(addr)) {
    return true;
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, addr, 6);
  peerInfo.channel = ESP_NOW_CHANNEL;
  peerInfo.encrypt = false;
  return (esp_now_add_peer(&peerInfo) == ESP_OK);
}

static void clearButtonFlags()
{
  click2_short_waspressed = false;
  click2_long_waspressed = false;
  click2_double_waspressed = false;
  mxclick_short_waspressed = false;
  mxclick_long_waspressed = false;
  click3_short_waspressed = false;
  click3_long_waspressed = false;
  click3_double_waspressed = false;
  resetEncoderCounts();
}

static void screensaver_check_activity()
{
  // Compatibility stub for old addon code path.
}

// Apply shared styles to a slider using the provided slot index (0..3)
static void styleSlider(lv_obj_t *slider, int slot)
{
  if (slider == nullptr) return;
  if (slot < 0) slot = 0;
  if (slot > 3) slot = 3;
  lv_obj_add_style(slider, &style_slider_track[slot], LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_style(slider, &style_slider_indicator[slot], LV_PART_INDICATOR | LV_STATE_DEFAULT);
  lv_obj_add_style(slider, &style_slider_indicator[slot], LV_PART_KNOB | LV_STATE_DEFAULT);
}

static void createSliderRow(lv_obj_t **rowLabel,
                            lv_obj_t **rowSlider,
                            lv_obj_t **rowValue,
                            const char *labelText,
                            int y,
                            int minValue,
                            int maxValue,
                            int slot)
{
  *rowLabel = lv_label_create(s_screen);
  lv_obj_set_width(*rowLabel, lv_pct(95));
  lv_obj_set_height(*rowLabel, LV_SIZE_CONTENT);
  lv_obj_set_x(*rowLabel, 0);
  lv_obj_set_y(*rowLabel, y);
  lv_obj_set_align(*rowLabel, LV_ALIGN_CENTER);
  lv_label_set_text(*rowLabel, labelText);
  lv_obj_set_style_text_font(*rowLabel, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_style(*rowLabel, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

  *rowSlider = lv_slider_create(*rowLabel);
  lv_slider_set_range(*rowSlider, minValue, maxValue);
  lv_slider_set_value(*rowSlider, minValue, LV_ANIM_OFF);
  lv_obj_set_width(*rowSlider, 130);
  lv_obj_set_height(*rowSlider, 10);
  lv_obj_set_x(*rowSlider, -15);
  lv_obj_set_y(*rowSlider, 0);
  lv_obj_set_align(*rowSlider, LV_ALIGN_RIGHT_MID);
  styleSlider(*rowSlider, slot);

  *rowValue = lv_label_create(*rowLabel);
  lv_obj_set_width(*rowValue, LV_SIZE_CONTENT);
  lv_obj_set_height(*rowValue, LV_SIZE_CONTENT);
  lv_obj_set_x(*rowValue, 100);
  lv_obj_set_y(*rowValue, 0);
  lv_obj_set_align(*rowValue, LV_ALIGN_LEFT_MID);
  lv_label_set_text(*rowValue, "0");
  lv_obj_add_style(*rowValue, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(*rowValue, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void createScreenIfNeeded()
{
  if (s_screen != nullptr) {
    return;
  }

  s_screen = lv_obj_create(nullptr);
  ui_ExampleAddon = s_screen;
  lv_obj_clear_flag(s_screen, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(s_screen, screenmachine, LV_EVENT_SCREEN_LOADED, nullptr);

  s_title = lv_label_create(s_screen);
  lv_obj_set_align(s_title, LV_ALIGN_TOP_MID);
  lv_obj_set_y(s_title, 12);
  lv_label_set_text(s_title, Title);
  lv_obj_set_style_text_font(s_title, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_style(s_title, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

  s_batt_title = lv_label_create(s_screen);
  lv_obj_set_width(s_batt_title, 85);
  lv_obj_set_height(s_batt_title, 30);
  lv_obj_set_x(s_batt_title, 115);
  lv_obj_set_y(s_batt_title, -103);
  lv_obj_set_align(s_batt_title, LV_ALIGN_CENTER);
  lv_label_set_text(s_batt_title, T_BATT);

  s_batt_value = lv_label_create(s_batt_title);
  lv_obj_set_width(s_batt_value, LV_SIZE_CONTENT);
  lv_obj_set_height(s_batt_value, LV_SIZE_CONTENT);
  lv_obj_set_x(s_batt_value, 0);
  lv_obj_set_y(s_batt_value, -7);
  lv_obj_set_align(s_batt_value, LV_ALIGN_RIGHT_MID);
  lv_label_set_text(s_batt_value, T_BLANK);

  createSliderRow(&s_slider1_label, &s_slider1_slider, &s_slider1_value, slider1Text, -60, slider1Min, slider1Max, 0);
  createSliderRow(&s_slider2_label, &s_slider2_slider, &s_slider2_value, slider2Text, -25, slider2Min, slider2Max, 1);
  createSliderRow(&s_slider3_label, &s_slider3_slider, &s_slider3_value, slider3Text, 10, slider3Min, slider3Max, 2);
  createSliderRow(&s_slider4_label, &s_slider4_slider, &s_slider4_value, slider4Text, 45, slider4Min, slider4Max, 3);

  s_button_left = lv_btn_create(s_screen);
  lv_obj_set_width(s_button_left, 100);
  lv_obj_set_height(s_button_left, 30);
  lv_obj_set_y(s_button_left, 100);
  lv_obj_set_x(s_button_left, lv_pct(-33));
  lv_obj_set_align(s_button_left, LV_ALIGN_CENTER);
  lv_obj_add_style(s_button_left, &style_button_l, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_style(s_button_left, &style_button_l_pressed, LV_PART_MAIN | LV_STATE_PRESSED);
  lv_obj_add_style(s_button_left, &style_button_l, LV_PART_MAIN | LV_STATE_FOCUSED);
  s_button_left_text = lv_label_create(s_button_left);
  lv_obj_set_align(s_button_left_text, LV_ALIGN_CENTER);
  lv_label_set_text(s_button_left_text, leftButtonText);

  s_button_mid = lv_btn_create(s_screen);
  lv_obj_set_width(s_button_mid, 100);
  lv_obj_set_height(s_button_mid, 30);
  lv_obj_set_y(s_button_mid, 100);
  lv_obj_set_x(s_button_mid, lv_pct(0));
  lv_obj_set_align(s_button_mid, LV_ALIGN_CENTER);
  lv_obj_add_style(s_button_mid, &style_button_m, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_style(s_button_mid, &style_button_m_pressed, LV_PART_MAIN | LV_STATE_PRESSED);
  lv_obj_add_style(s_button_mid, &style_button_m, LV_PART_MAIN | LV_STATE_FOCUSED);
  s_button_mid_text = lv_label_create(s_button_mid);
  lv_obj_set_align(s_button_mid_text, LV_ALIGN_CENTER);
  lv_label_set_text(s_button_mid_text, middleButtonText);

  s_button_right = lv_btn_create(s_screen);
  lv_obj_set_width(s_button_right, 100);
  lv_obj_set_height(s_button_right, 30);
  lv_obj_set_y(s_button_right, 100);
  lv_obj_set_x(s_button_right, lv_pct(33));
  lv_obj_set_align(s_button_right, LV_ALIGN_CENTER);
  lv_obj_add_style(s_button_right, &style_button_r, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_style(s_button_right, &style_button_r_pressed, LV_PART_MAIN | LV_STATE_PRESSED);
  lv_obj_add_style(s_button_right, &style_button_r, LV_PART_MAIN | LV_STATE_FOCUSED);
  s_button_right_text = lv_label_create(s_button_right);
  lv_obj_set_align(s_button_right_text, LV_ALIGN_CENTER);
  lv_label_set_text(s_button_right_text, rightButtonText);
}

static void refreshTheme()
{
  if (s_screen == nullptr) {
    return;
  }

  // Apply the shared background style first, then apply the semantic
  // `style_option_bg` (which is defined as a black option background).
  lv_obj_add_style(s_screen, &style_background, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_style(s_screen, &style_option_bg, LV_PART_MAIN | LV_STATE_DEFAULT);

  if (s_title != nullptr) {
    lv_obj_add_style(s_title, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(s_title, &style_title_bar, LV_PART_MAIN | LV_STATE_DEFAULT);
  }

  // Battery/status label in this addon screen should also follow the
  // shared primary text style so icons/text remain readable across
  // dark/light themes.
  if (s_batt_title != nullptr) {
    lv_obj_add_style(s_batt_title, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);
  }
  if (s_batt_value != nullptr) {
    lv_obj_add_style(s_batt_value, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);
  }

  lv_obj_t *valueLabels[] = {s_slider1_value, s_slider2_value, s_slider3_value, s_slider4_value};
  for (lv_obj_t *lbl : valueLabels) {
    if (!lbl) continue;
    lv_obj_add_style(lbl, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
  }
  styleSlider(s_slider1_slider, 0);
  styleSlider(s_slider2_slider, 1);
  styleSlider(s_slider3_slider, 2);
  styleSlider(s_slider4_slider, 3);
}

static void refreshValueLabels()
{
  if (s_slider1_value != nullptr) {
    lv_label_set_text_fmt(s_slider1_value, "%d", (int)slider1Value);
  }
  if (s_slider2_value != nullptr) {
    lv_label_set_text_fmt(s_slider2_value, "%d", (int)slider2Value);
  }
  if (s_slider3_value != nullptr) {
    lv_label_set_text_fmt(s_slider3_value, "%d", (int)slider3Value);
  }
  if (s_slider4_value != nullptr) {
    lv_label_set_text_fmt(s_slider4_value, "%d", (int)slider4Value);
  }
  if (s_button_mid_text != nullptr) {
    lv_label_set_text(s_button_mid_text, s_is_on ? middleButtonTextOn : middleButtonTextOff);
  }
}

static int getRampedDetentDelta(int encoderId, int detents)
{
  if (detents == 0) {
    return 0;
  }

  if (!s_ramp_enabled) {
    s_ramp_value = 1;
    s_ramp_active_encoder = encoderId;
    s_ramp_ms = millis();
    return detents;
  }

  unsigned long now = millis();
  bool sameEncoder = (encoderId == s_ramp_active_encoder);
  bool withinRampWindow = ((now - s_ramp_ms) <= (unsigned long)s_ramp_time_ms);
  if (!sameEncoder || !withinRampWindow) {
    s_ramp_value = 1;
  }

  int sign = (detents > 0) ? 1 : -1;
  int steps = abs(detents);
  int delta = 0;
  for (int i = 0; i < steps; ++i) {
    delta += sign * s_ramp_value;
    if (s_ramp_value < s_ramp_max) {
      ++s_ramp_value;
    }
  }

  s_ramp_active_encoder = encoderId;
  s_ramp_ms = now;
  return delta;
}

static void sendPairingHeartbeatIfNeeded()
{
  if (!s_addon_enabled) {
    return;
  }

  if (s_is_paired) {
    return;
  }

  const uint32_t nowMs = millis();
  if ((nowMs - s_last_pairing_heartbeat_ms) < 1000UL) {
    return;
  }
  s_last_pairing_heartbeat_ms = nowMs;

  if (!ensurePeer(BROADCAST_ADDR)) {
    return;
  }

  ExampleAddon_Message msg = {};
  msg.esp_command = HEARTBEAT;
  msg.esp_heartbeat = true;
  msg.esp_target = ExampleAddon_ID;
  msg.esp_sender = M5_ID;
  lockEspNowChannelIfConfigured("pair-heartbeat");
  Serial.printf("ESP-NOW TX: to=%02X:%02X:%02X:%02X:%02X:%02X target=%d cmd=%d sender=%d hb=%d len=%u\n",
                BROADCAST_ADDR[0], BROADCAST_ADDR[1], BROADCAST_ADDR[2], BROADCAST_ADDR[3], BROADCAST_ADDR[4], BROADCAST_ADDR[5],
                msg.esp_target, msg.esp_command, msg.esp_sender, msg.esp_heartbeat ? 1 : 0, (unsigned)sizeof(msg));
  esp_now_send(BROADCAST_ADDR, reinterpret_cast<uint8_t *>(&msg), sizeof(msg));
}

static void setPairedAddress(const uint8_t *mac)
{
  if (memcmp(s_example_addr, mac, 6) == 0 && s_is_paired) {
    return;
  }

  if (esp_now_is_peer_exist(s_example_addr)) {
    esp_now_del_peer(s_example_addr);
  }

  memcpy(s_example_addr, mac, 6);
  if (ensurePeer(s_example_addr)) {
    s_is_paired = true;
    //LogDebug("ExampleAddon.cpp - addon paired.");

    //LogDebugFormatted("Example addon paired MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
    //                  s_example_addr[0], s_example_addr[1], s_example_addr[2],
    //                  s_example_addr[3], s_example_addr[4], s_example_addr[5]);
  }
}

static bool applySliderFromEncoder(ESP32Encoder &encoder,
                                   int encoderId,
                                   long &encoderState,
                                   float &value,
                                   lv_obj_t *slider,
                                   int command)
{
  bool changed = false;

  if (slider == nullptr) {
    return false;
  }

  if (lv_slider_is_dragged(slider) == false) {
    lv_slider_set_value(slider, (int)value, LV_ANIM_OFF);
    long count = encoder.getCount();
    int detents = (int)(count / 4);
    int rem = (int)(count - detents * 4);
    if (rem < 0) { rem += 4; detents -= 1; }

    if (detents != 0) {
      value += (float)getRampedDetentDelta(encoderId, detents);
      encoder.setCount(rem);
      changed = true;
      screensaver_check_activity();
    }

    int minV = lv_slider_get_min_value(slider);
    int maxV = lv_slider_get_max_value(slider);
    if (value < (float)minV) {
      value = (float)minV;
      changed = true;
    }
    if (value > (float)maxV) {
      value = (float)maxV;
      changed = true;
    }
  } else {
    int sliderValue = lv_slider_get_value(slider);
    if ((int)value != sliderValue) {
      value = (float)sliderValue;
      changed = true;
    }
  }

  if (changed) {
    encoderState = encoder.getCount();
    ExampleAddonSendCommand(command, value);
  }

  return changed;
}

static void toggleOnOff()
{
  if (s_is_on) {
    ExampleAddonSendCommand(OFF, 0.0f);
    s_is_on = false;
  } else {
    ExampleAddonSendCommand(ON, 0.0f);
    s_is_on = true;
  }
  refreshValueLabels();
}

}  // namespace

void ExampleAddonPrepareScreen()
{
  createScreenIfNeeded();
  refreshTheme();
  refreshValueLabels();
  // The click used to enter this screen can still be latched for one loop.
  // Flush it once so left/mid/right actions start from a clean state.
  s_flush_buttons_once = true;
}

lv_obj_t *ExampleAddonGetScreen()
{
  createScreenIfNeeded();
  return s_screen;
}

lv_obj_t *ExampleAddonGetBatteryTitleLabel()
{
  return s_batt_title;
}

lv_obj_t *ExampleAddonGetBatteryValueLabel()
{
  return s_batt_value;
}

void ExampleAddonToggle()
{
  if (s_addon_enabled && s_is_paired) {
    toggleOnOff();
  }
}

bool ExampleAddonIsPaired()
{
  return s_addon_enabled && s_is_paired;
}

void ExampleAddonSetAddonEnabled(bool enabled)
{
  s_addon_enabled = enabled;

  if (!enabled) {
    if (esp_now_is_peer_exist(s_example_addr)) {
      esp_now_del_peer(s_example_addr);
    }
    s_is_paired = false;
    s_is_on = false;
    memset(s_example_addr, 0xFF, sizeof(s_example_addr));
  }

  // Allow immediate heartbeat retry when enabled.
  s_last_pairing_heartbeat_ms = 0;
}

bool ExampleAddonSendCommand(int command, float value)
{
  if (!s_addon_enabled) {
    return false;
  }

  if (!s_is_paired) {
    //LogDebugFormatted("TX ExampleAddon blocked: not paired cmd=%d val=%.2f\n", command, value);
    sendPairingHeartbeatIfNeeded();
    return false;
  }

  if (!ensurePeer(s_example_addr)) {
    //LogDebug("TX ExampleAddon blocked: ensurePeer failed");
    return false;
  }

  ExampleAddon_Message msg = {};
  msg.esp_connected = true;
  msg.esp_command = command;
  msg.esp_value = value;
  msg.esp_target = s_peer_id;
  msg.esp_sender = s_local_id;

  //LogDebugFormatted("TX ExampleAddon send cmd=%d val=%.2f target=%d sender=%d to=%02X:%02X:%02X:%02X:%02X:%02X\n",
  //                  command, value, msg.esp_target, msg.esp_sender,
  //                  s_example_addr[0], s_example_addr[1], s_example_addr[2], s_example_addr[3], s_example_addr[4], s_example_addr[5]);
  Serial.printf("ESP-NOW TX: to=%02X:%02X:%02X:%02X:%02X:%02X target=%d cmd=%d sender=%d hb=%d len=%u\n",
                s_example_addr[0], s_example_addr[1], s_example_addr[2], s_example_addr[3], s_example_addr[4], s_example_addr[5],
                msg.esp_target, msg.esp_command, msg.esp_sender, msg.esp_heartbeat ? 1 : 0, (unsigned)sizeof(msg));
  esp_err_t result = esp_now_send(s_example_addr, reinterpret_cast<uint8_t *>(&msg), sizeof(msg));
  Serial.printf("ESP-NOW TX result=%d\n", (int)result);
  //LogDebugFormatted("TX ExampleAddon result=%s err=%d\n", (result == ESP_OK) ? "OK" : "FAIL", (int)result);
  return (result == ESP_OK);
}

bool ExampleAddonHandleIncomingEspNowFrame(const uint8_t *mac,
                                     int target,
                                     int sender,
                                     int command,
                                     float value,
                                     bool heartbeat)
{
  (void)value;
  (void)heartbeat;

  if (!s_addon_enabled) {
    return false;
  }

  s_peer_id = sender;

  // Only set paired address for this addon when the sender/target indicates
  // the frame is intended for ExampleAddon (avoid stealing paired address from other addons)
  if (sender == ExampleAddon_ID || target == ExampleAddon_ID) {
    setPairedAddress(mac);
  }

  if (command == OFF) {
    s_is_on = false;
  } else if (command == ON) {
    s_is_on = true;
  }


  return true;
}

void ExampleAddonHandleScreen(const ButtonEvents &events)
{
  createScreenIfNeeded();
  sendPairingHeartbeatIfNeeded();

  if (s_flush_buttons_once) {
    clearButtonFlags();
    s_flush_buttons_once = false;
  }

  applySliderFromEncoder(encoder1, 1, s_enc1, slider1Value, s_slider1_slider, slider1Command);
  applySliderFromEncoder(encoder2, 2, s_enc2, slider2Value, s_slider2_slider, slider2Command);
  applySliderFromEncoder(encoder3, 3, s_enc3, slider3Value, s_slider3_slider, slider3Command);
  applySliderFromEncoder(encoder4, 4, s_enc4, slider4Value, s_slider4_slider, slider4Command);
  refreshValueLabels();

  // USER ACTION HOOK: edit the three blocks below to customize what each button does.
  if (events.leftShort) {
    // Left button action: replace with your own logic if needed.
    lv_obj_t *dest = g_addon_return_screen ? g_addon_return_screen : ui_Home;
    _ui_screen_change(dest, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    g_addon_return_screen = nullptr;
    clearButtonFlags();  //ALWAYS USE THIS AFTER A BUTTON PRESS, OTHERWISE THE BUTTON PRESS WILL BE REPEATED ON THE NEXT LOOP
  } else if (events.mxShort) {
    // Middle button action: replace with your own logic if needed.
    toggleOnOff();
    clearButtonFlags();  //ALWAYS USE THIS AFTER A BUTTON PRESS, OTHERWISE THE BUTTON PRESS WILL BE REPEATED ON THE NEXT LOOP
  } else if (events.rightShort) {
    // Right button action: replace with your own logic if needed.
    resetEncoderCounts();
    _ui_screen_change(ui_Menu, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    clearButtonFlags();  //ALWAYS USE THIS AFTER A BUTTON PRESS, OTHERWISE THE BUTTON PRESS WILL BE REPEATED ON THE NEXT LOOP
  }
}

// C-callable wrapper so C code can invoke the handler
extern "C" void ExampleAddonHandleScreen(const struct ButtonEvents *events)
{
  if (events == nullptr) return;
  ExampleAddonHandleScreen(*events);
}
