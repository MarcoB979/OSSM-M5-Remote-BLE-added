#include "FistIT.h"

#include <Arduino.h>
#include <esp_now.h>
#include <cstring>

#include "main.h"
#include "language.h"
#include "colors.h"
#include "ui/ui.h"
#include "ui/ui_helpers.h"
#include "colors.h"
#include "styles.h"
#include "esp_nowCommunication.h"
// Single-definition of FIST_ID (C linkage so C code can reference it)
extern "C" const int FIST_ID = 3;

namespace {

struct FistMessage {
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

static lv_obj_t *s_speed_label = nullptr;
static lv_obj_t *s_speed_slider = nullptr;
static lv_obj_t *s_speed_value = nullptr;
static lv_obj_t *s_batt_title = nullptr;
static lv_obj_t *s_batt_value = nullptr;
static lv_obj_t *s_rotation_label = nullptr;
static lv_obj_t *s_rotation_slider = nullptr;
static lv_obj_t *s_rotation_value = nullptr;
static lv_obj_t *s_pause_label = nullptr;
static lv_obj_t *s_pause_slider = nullptr;
static lv_obj_t *s_pause_value = nullptr;
static lv_obj_t *s_accel_label = nullptr;
static lv_obj_t *s_accel_slider = nullptr;
static lv_obj_t *s_accel_value = nullptr;

static float s_speed = 0.0f;
static float s_rotation = 0.0f;
static float s_pause = 0.0f;
static float s_accel = 0.0f;

static long s_enc1 = 0;
static long s_enc2 = 0;
static long s_enc3 = 0;
static long s_enc4 = 0;

static bool s_ramp_enabled = true;
static int s_ramp_value = 1;
static int s_ramp_time_ms = 75;
static int s_ramp_max = 8;
static int s_ramp_active_encoder = 0;
static unsigned long s_ramp_ms = 0;

static bool s_is_paired = false;
static bool s_is_on = false;
static bool s_addon_enabled = false;
static uint8_t s_fist_addr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static constexpr uint8_t BROADCAST_ADDR[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static constexpr int LEGACY_FIST_ID = 3;
static constexpr int LEGACY_M5_ID = M5_ID;
static uint32_t s_last_pairing_heartbeat_ms = 0;
static int s_peer_id = FIST_ID;
static int s_local_id = M5_ID;

static bool ensurePeer(const uint8_t *addr)
{
  // If this is the broadcast address, don't add it as a peer — broadcasts
  // do not require a peer entry and adding FF:FF:FF:FF:FF:FF can destabilize
  // esp_now peer management on some SDK versions.
  const bool isBroadcast = (addr[0] == 0xFF && addr[1] == 0xFF && addr[2] == 0xFF && addr[3] == 0xFF && addr[4] == 0xFF && addr[5] == 0xFF);
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
  lv_obj_clear_flag(s_screen, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(s_screen, screenmachine, LV_EVENT_SCREEN_LOADED, nullptr);

  s_title = lv_label_create(s_screen);
  lv_obj_set_align(s_title, LV_ALIGN_TOP_MID);
  lv_obj_set_y(s_title, 12);
  lv_label_set_text(s_title, "Fist-IT");
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

  createSliderRow(&s_speed_label, &s_speed_slider, &s_speed_value, T_SPEED, -60, 0, 100, 0);
  createSliderRow(&s_rotation_label, &s_rotation_slider, &s_rotation_value, T_ROTATION, -25, 0, 360, 1);
  createSliderRow(&s_pause_label, &s_pause_slider, &s_pause_value, T_PAUSE, 10, 0, 100, 2);
  createSliderRow(&s_accel_label, &s_accel_slider, &s_accel_value, T_ACCEL, 45, 0, 100, 3);

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
  lv_label_set_text(s_button_left_text, T_START);

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
  lv_label_set_text(s_button_mid_text, T_HOME);

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
  lv_label_set_text(s_button_right_text, T_MENU);
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

  lv_obj_t *valueLabels[] = {s_speed_value, s_rotation_value, s_pause_value, s_accel_value};
  for (lv_obj_t *lbl : valueLabels) {
    if (!lbl) continue;
    lv_obj_add_style(lbl, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
  }
  styleSlider(s_speed_slider, 0);
  styleSlider(s_rotation_slider, 1);
  styleSlider(s_pause_slider, 2);
  styleSlider(s_accel_slider, 3);
}

static void refreshValueLabels()
{
  if (s_speed_value != nullptr) {
    lv_label_set_text_fmt(s_speed_value, "%d", (int)s_speed);
  }
  if (s_rotation_value != nullptr) {
    lv_label_set_text_fmt(s_rotation_value, "%d", (int)s_rotation);
  }
  if (s_pause_value != nullptr) {
    lv_label_set_text_fmt(s_pause_value, "%d", (int)s_pause);
  }
  if (s_accel_value != nullptr) {
    lv_label_set_text_fmt(s_accel_value, "%d", (int)s_accel);
  }
  if (s_button_left_text != nullptr) {
    lv_label_set_text(s_button_left_text, s_is_on ? T_PAUSE : T_START);
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

  FistMessage msg = {};
  msg.esp_command = HEARTBEAT;
  msg.esp_heartbeat = true;
  msg.esp_target = FIST_ID;
  msg.esp_sender = M5_ID;
  Serial.printf("ESP-NOW TX: to=%02X:%02X:%02X:%02X:%02X:%02X target=%d cmd=%d sender=%d hb=%d len=%u\n",
                BROADCAST_ADDR[0], BROADCAST_ADDR[1], BROADCAST_ADDR[2], BROADCAST_ADDR[3], BROADCAST_ADDR[4], BROADCAST_ADDR[5],
                msg.esp_target, msg.esp_command, msg.esp_sender, msg.esp_heartbeat ? 1 : 0, (unsigned)sizeof(msg));
  esp_now_send(BROADCAST_ADDR, reinterpret_cast<uint8_t *>(&msg), sizeof(msg));
}

static void setPairedAddress(const uint8_t *mac)
{
  if (memcmp(s_fist_addr, mac, 6) == 0 && s_is_paired) {
    return;
  }

  if (esp_now_is_peer_exist(s_fist_addr)) {
    esp_now_del_peer(s_fist_addr);
  }

  memcpy(s_fist_addr, mac, 6);
  if (ensurePeer(s_fist_addr)) {
    s_is_paired = true;
    LogDebugFormatted("Fist-IT paired MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                      s_fist_addr[0], s_fist_addr[1], s_fist_addr[2],
                      s_fist_addr[3], s_fist_addr[4], s_fist_addr[5]);
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
    FistITSendCommand(command, value);
  }

  return changed;
}

static void toggleOnOff()
{
  if (s_is_on) {
    FistITSendCommand(OFF, 0.0f);
    s_is_on = false;
  } else {
    FistITSendCommand(ON, 0.0f);
    s_is_on = true;
  }
  refreshValueLabels();
}

}  // namespace

void FistITPrepareScreen()
{
  createScreenIfNeeded();
  refreshTheme();
  refreshValueLabels();
}

lv_obj_t *FistITGetScreen()
{
  createScreenIfNeeded();
  return s_screen;
}

lv_obj_t *FistITGetBatteryTitleLabel()
{
  return s_batt_title;
}

lv_obj_t *FistITGetBatteryValueLabel()
{
  return s_batt_value;
}

bool FistITIsPaired()
{
  return s_addon_enabled && s_is_paired;
}

void FistITSetAddonEnabled(bool enabled)
{
  s_addon_enabled = enabled;

  if (!enabled) {
    if (esp_now_is_peer_exist(s_fist_addr)) {
      esp_now_del_peer(s_fist_addr);
    }
    s_is_paired = false;
    s_is_on = false;
    memset(s_fist_addr, 0xFF, sizeof(s_fist_addr));
  }

  // Allow immediate heartbeat retry when enabled.
  s_last_pairing_heartbeat_ms = 0;
}

bool FistITSendCommand(int command, float value)
{
  if (!s_addon_enabled) {
    return false;
  }

  if (!s_is_paired) {
    LogDebugFormatted("TX FIST blocked: not paired cmd=%d val=%.2f\n", command, value);
    sendPairingHeartbeatIfNeeded();
    return false;
  }

  if (!ensurePeer(s_fist_addr)) {
    LogDebug("TX FIST blocked: ensurePeer failed");
    return false;
  }

  FistMessage msg = {};
  msg.esp_connected = true;
  msg.esp_command = command;
  msg.esp_value = value;
  msg.esp_target = s_peer_id;
  msg.esp_sender = s_local_id;

  LogDebugFormatted("TX FIST send cmd=%d val=%.2f target=%d sender=%d to=%02X:%02X:%02X:%02X:%02X:%02X\n",
                    command, value, msg.esp_target, msg.esp_sender,
                    s_fist_addr[0], s_fist_addr[1], s_fist_addr[2], s_fist_addr[3], s_fist_addr[4], s_fist_addr[5]);
  Serial.printf("ESP-NOW TX: to=%02X:%02X:%02X:%02X:%02X:%02X target=%d cmd=%d sender=%d hb=%d len=%u\n",
                s_fist_addr[0], s_fist_addr[1], s_fist_addr[2], s_fist_addr[3], s_fist_addr[4], s_fist_addr[5],
                msg.esp_target, msg.esp_command, msg.esp_sender, msg.esp_heartbeat ? 1 : 0, (unsigned)sizeof(msg));
  esp_err_t result = esp_now_send(s_fist_addr, reinterpret_cast<uint8_t *>(&msg), sizeof(msg));
  Serial.printf("ESP-NOW TX result=%d\n", (int)result);
  LogDebugFormatted("TX FIST result=%s err=%d\n", (result == ESP_OK) ? "OK" : "FAIL", (int)result);
  return (result == ESP_OK);
}

bool FistITHandleIncomingEspNowFrame(const uint8_t *mac,
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
  // Do NOT update s_local_id: M5 always identifies as M5_ID regardless of
  // which legacy target ID the peer used to address us.

  // Only set paired address for this addon when the sender/target indicates
  // the frame is intended for Fist-IT (avoid stealing paired address from other addons)
  if (sender == FIST_ID || target == FIST_ID) {
    setPairedAddress(mac);
  }

  if (command == OFF) {
    s_is_on = false;
  } else if (command == ON) {
    s_is_on = true;
  }

  // Do not touch LVGL here: this function is called from ESP-NOW recv path
  // (wifi task). UI is refreshed in FistITHandleScreen() on the main loop.
  return true;
}

void FistITHandleScreen(const ButtonEvents &events)
{
  createScreenIfNeeded();
  refreshTheme();
  sendPairingHeartbeatIfNeeded();

  applySliderFromEncoder(encoder1, 1, s_enc1, s_speed, s_speed_slider, FIST_SPEED);
  applySliderFromEncoder(encoder2, 2, s_enc2, s_rotation, s_rotation_slider, FIST_ROTATION);
  applySliderFromEncoder(encoder3, 3, s_enc3, s_pause, s_pause_slider, FIST_PAUSE);
  applySliderFromEncoder(encoder4, 4, s_enc4, s_accel, s_accel_slider, FIST_ACCEL);
  refreshValueLabels();

  if (events.leftShort) {
    toggleOnOff();
    clearButtonFlags();
  } else if (events.mxShort) {
    _ui_screen_change(ui_Home, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    clearButtonFlags();
  } else if (events.rightShort) {
    _ui_screen_change(ui_Menu, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    clearButtonFlags();
  }
}

// C-callable wrapper so C code can invoke the handler
extern "C" void FistITHandleScreen(const struct ButtonEvents *events)
{
  if (events == nullptr) return;
  FistITHandleScreen(*events);
}
