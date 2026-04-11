#include "Eject.h"

#include <Arduino.h>
#include <esp_now.h>
#include <cstring>

#include "main.h"
#include "language.h"
#include "colors.h"
#include "ui/ui.h"
#include "ui/ui_helpers.h"

namespace {

struct EjectMessage {
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

static bool s_ui_initialized = false;
static bool s_is_paired = false;
static bool s_is_on = false;
static bool s_addon_enabled = false;

static lv_obj_t *s_speed_label = nullptr;
static lv_obj_t *s_speed_slider = nullptr;
static lv_obj_t *s_speed_value = nullptr;
static lv_obj_t *s_time_label = nullptr;
static lv_obj_t *s_time_slider = nullptr;
static lv_obj_t *s_time_value = nullptr;
static lv_obj_t *s_size_label = nullptr;
static lv_obj_t *s_size_slider = nullptr;
static lv_obj_t *s_size_value = nullptr;
static lv_obj_t *s_accel_label = nullptr;
static lv_obj_t *s_accel_slider = nullptr;
static lv_obj_t *s_accel_value = nullptr;

static float s_speed = 0.0f;
static float s_time = 0.0f;
static float s_size = 0.0f;
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

static uint8_t s_eject_addr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static constexpr uint8_t BROADCAST_ADDR[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static constexpr int LEGACY_EJECT_ID = 2;
static constexpr int LEGACY_M5_ID = 4;
static uint32_t s_last_pairing_heartbeat_ms = 0;
static int s_peer_id = EJECT_ID;
static int s_local_id = M5_ID;

static bool ensurePeer(const uint8_t *addr)
{
  if (esp_now_is_peer_exist(addr)) {
    return true;
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, addr, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  return (esp_now_add_peer(&peerInfo) == ESP_OK);
}

static void styleSliderRow(lv_obj_t *slider)
{
  if (slider == nullptr) {
    return;
  }

  lv_obj_set_style_bg_color(slider, lv_color_hex(getActiveSecondaryColor()), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(slider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

  lv_obj_set_style_bg_color(slider, lv_color_hex(getActivePrimaryColor()), LV_PART_INDICATOR | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(slider, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_grad_color(slider, lv_color_hex(getActivePrimaryColor()), LV_PART_INDICATOR | LV_STATE_DEFAULT);

  lv_obj_set_style_bg_color(slider, lv_color_hex(getActivePrimaryColor()), LV_PART_KNOB | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(slider, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
}

static void createSliderRow(lv_obj_t *parent,
                            lv_obj_t **rowLabel,
                            lv_obj_t **rowSlider,
                            lv_obj_t **rowValue,
                            const char *labelText,
                            int y,
                            int minValue,
                            int maxValue)
{
  *rowLabel = lv_label_create(parent);
  lv_obj_set_width(*rowLabel, lv_pct(95));
  lv_obj_set_height(*rowLabel, LV_SIZE_CONTENT);
  lv_obj_set_x(*rowLabel, 0);
  lv_obj_set_y(*rowLabel, y);
  lv_obj_set_align(*rowLabel, LV_ALIGN_CENTER);
  lv_label_set_text(*rowLabel, labelText);
  lv_obj_set_style_text_font(*rowLabel, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

  *rowSlider = lv_slider_create(*rowLabel);
  lv_slider_set_range(*rowSlider, minValue, maxValue);
  lv_slider_set_value(*rowSlider, minValue, LV_ANIM_OFF);
  lv_obj_set_width(*rowSlider, 130);
  lv_obj_set_height(*rowSlider, 10);
  lv_obj_set_x(*rowSlider, -15);
  lv_obj_set_y(*rowSlider, 0);
  lv_obj_set_align(*rowSlider, LV_ALIGN_RIGHT_MID);
  styleSliderRow(*rowSlider);

  *rowValue = lv_label_create(*rowLabel);
  lv_obj_set_width(*rowValue, LV_SIZE_CONTENT);
  lv_obj_set_height(*rowValue, LV_SIZE_CONTENT);
  lv_obj_set_x(*rowValue, 80);
  lv_obj_set_y(*rowValue, 0);
  lv_obj_set_align(*rowValue, LV_ALIGN_LEFT_MID);
  lv_label_set_text(*rowValue, "0");
}

static void refreshValueLabels()
{
  lv_label_set_text_fmt(s_speed_value, "%d", (int)s_speed);
  lv_label_set_text_fmt(s_time_value, "%d", (int)s_time);
  lv_label_set_text_fmt(s_size_value, "%d", (int)s_size);
  lv_label_set_text_fmt(s_accel_value, "%d", (int)s_accel);
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

  EjectMessage msg = {};
  msg.esp_command = HEARTBEAT;
  msg.esp_heartbeat = true;
  msg.esp_target = EJECT_ID;
  msg.esp_sender = M5_ID;
  esp_now_send(BROADCAST_ADDR, reinterpret_cast<uint8_t *>(&msg), sizeof(msg));
}

static void setPairedAddress(const uint8_t *mac)
{
  if (memcmp(s_eject_addr, mac, 6) == 0 && s_is_paired) {
    return;
  }

  if (esp_now_is_peer_exist(s_eject_addr)) {
    esp_now_del_peer(s_eject_addr);
  }

  memcpy(s_eject_addr, mac, 6);
  if (ensurePeer(s_eject_addr)) {
    s_is_paired = true;
    LogDebugFormatted("EJECT paired MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                      s_eject_addr[0], s_eject_addr[1], s_eject_addr[2],
                      s_eject_addr[3], s_eject_addr[4], s_eject_addr[5]);
  }
}

static void ejectToggleAction()
{
  if (s_is_on) {
    EjectSendCommand(OFF, 0.0f);
    s_is_on = false;
  } else {
    EjectSendCommand(ON, 0.0f);
    s_is_on = true;
  }
}

static void onEjectLeftButtonClicked(lv_event_t *e)
{
  if (lv_event_get_code(e) != LV_EVENT_SHORT_CLICKED) {
    return;
  }
  ejectToggleAction();
}

static void ensureUiInitialized()
{
  if (s_ui_initialized || ui_EJECTSettings == nullptr) {
    return;
  }

  if (ui_EJECTButtonLText != nullptr) {
    lv_label_set_text(ui_EJECTButtonLText, T_CUM);
  }

  createSliderRow(ui_EJECTSettings, &s_speed_label, &s_speed_slider, &s_speed_value, T_CUM_SPEED, -60, 0, 100);
  createSliderRow(ui_EJECTSettings, &s_time_label, &s_time_slider, &s_time_value, T_CUM_TIME, -25, 0, 61);
  createSliderRow(ui_EJECTSettings, &s_size_label, &s_size_slider, &s_size_value, T_CUM_Volume, 10, 0, 100);
  createSliderRow(ui_EJECTSettings, &s_accel_label, &s_accel_slider, &s_accel_value, T_CUM_Accel, 45, 0, 100);

  if (ui_EJECTButtonL != nullptr) {
    lv_obj_add_event_cb(ui_EJECTButtonL, onEjectLeftButtonClicked, LV_EVENT_SHORT_CLICKED, nullptr);
  }

  s_ui_initialized = true;
  refreshValueLabels();
}

static bool applySliderFromEncoder(ESP32Encoder &encoder,
                                   int encoderId,
                                   long &encoderState,
                                   float &value,
                                   lv_obj_t *slider,
                                   int command)
{
  bool changed = false;

  if (lv_slider_is_dragged(slider) == false) {
    lv_slider_set_value(slider, (int)value, LV_ANIM_OFF);
    long count = encoder.getCount();
    int detents = (int)(count / 2);

    if (detents != 0) {
      value += (float)getRampedDetentDelta(encoderId, detents);
      encoder.setCount(count % 2);
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
    float txValue = value;
    if (command == CUMSPEED || command == CUMACCEL) {
      txValue = value / 10.0f;
    }
    EjectSendCommand(command, txValue);
  }

  return changed;
}

} // namespace

bool EjectIsPaired()
{
  return s_addon_enabled && s_is_paired;
}

void EjectSetAddonEnabled(bool enabled)
{
  s_addon_enabled = enabled;
  eject_status = enabled;
  if (!enabled) {
    s_is_on = false;
    s_is_paired = false;
    if (esp_now_is_peer_exist(s_eject_addr)) {
      esp_now_del_peer(s_eject_addr);
    }
    memset(s_eject_addr, 0xFF, sizeof(s_eject_addr));
  }

  // Allow immediate heartbeat retry when enabled.
  s_last_pairing_heartbeat_ms = 0;

  if (ui_EJECTSettingButton != nullptr) {
    if (enabled) {
      lv_obj_clear_state(ui_EJECTSettingButton, LV_STATE_DISABLED);
    } else {
      lv_obj_add_state(ui_EJECTSettingButton, LV_STATE_DISABLED);
    }
  }
}

bool EjectSendCommand(int command, float value)
{
  if (!s_addon_enabled) {
    return false;
  }

  if (!s_is_paired) {
    LogDebugFormatted("TX EJECT blocked: not paired cmd=%d val=%.2f\n", command, value);
    sendPairingHeartbeatIfNeeded();
    return false;
  }

  if (!ensurePeer(s_eject_addr)) {
    LogDebug("TX EJECT blocked: ensurePeer failed");
    return false;
  }

  EjectMessage msg = {};
  msg.esp_connected = true;
  msg.esp_command = command;
  msg.esp_value = value;
  msg.esp_target = s_peer_id;
  msg.esp_sender = s_local_id;

  LogDebugFormatted("TX EJECT send cmd=%d val=%.2f target=%d sender=%d to=%02X:%02X:%02X:%02X:%02X:%02X\n",
                    command, value, msg.esp_target, msg.esp_sender,
                    s_eject_addr[0], s_eject_addr[1], s_eject_addr[2], s_eject_addr[3], s_eject_addr[4], s_eject_addr[5]);
  esp_err_t result = esp_now_send(s_eject_addr, reinterpret_cast<uint8_t *>(&msg), sizeof(msg));
  LogDebugFormatted("TX EJECT result=%s err=%d\n", (result == ESP_OK) ? "OK" : "FAIL", (int)result);
  return (result == ESP_OK);
}

bool EjectHandleIncomingEspNowFrame(const uint8_t *mac,
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

  if (sender != EJECT_ID && sender != LEGACY_EJECT_ID) {
    return false;
  }

  // Accept both current and legacy M5 target IDs for backward compatibility.
  if (target != M5_ID && target != LEGACY_M5_ID) {
    return true;
  }

  s_peer_id = sender;
  // Do NOT update s_local_id: M5 always identifies as M5_ID regardless of
  // which legacy target ID the peer used to address us.

  setPairedAddress(mac);

  if (command == OFF) {
    s_is_on = false;
  } else if (command == ON) {
    s_is_on = true;
  }

  return true;
}

void EjectHandleScreen(const ButtonEvents &events)
{
  if (lv_obj_has_state(ui_lefty, LV_STATE_CHECKED) == 1) {
    touch_disabled = true;
  }

  ensureUiInitialized();
  sendPairingHeartbeatIfNeeded();

  if (s_ui_initialized) {
    applySliderFromEncoder(encoder1, 1, s_enc1, s_speed, s_speed_slider, CUMSPEED);
    applySliderFromEncoder(encoder2, 2, s_enc2, s_time, s_time_slider, CUMTIME);
    applySliderFromEncoder(encoder3, 3, s_enc3, s_size, s_size_slider, CUMSIZE);
    applySliderFromEncoder(encoder4, 4, s_enc4, s_accel, s_accel_slider, CUMACCEL);
    refreshValueLabels();
  }

  if (events.leftShort) {
    LogDebug("EJECT UI: leftShort detected -> toggle action");
    ejectToggleAction();
    clearButtonFlags();
  } else if (events.mxShort) {
    _ui_screen_change(ui_Home, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    clearButtonFlags();
  } else if (events.rightShort) {
    _ui_screen_change(ui_Menu, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    clearButtonFlags();
  }
}

