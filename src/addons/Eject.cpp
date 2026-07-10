#include "Eject.h"

#include <Arduino.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <cstring>

#include "main.h"
#include "language.h"
#include "display/colors.h"
#include "ui/ui.h"
#include "ui/ui_helpers.h"
#include "display/colors.h"
#include "display/styles.h"
#include "communication/esp_nowCommunication.h"
#include "buttonhandlers/ButtonHandlers.h"
#include "screens/ScreenHandler.h"
#include "config/debug.h"
#include "addons/addonsStreaming.h"
#include "language.h"
#include "communication/EjectComm.h"

// Single-definition of EJECT_ID (C linkage so C code can reference it)
extern "C" const int EJECT_ID = 2;

bool ejectUnload = false;

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

static lv_obj_t *e_screen = nullptr;
lv_obj_t *ui_Eject = nullptr;

static lv_obj_t *e_title = nullptr;
static lv_obj_t *e_button_left = nullptr;
static lv_obj_t *e_button_mid = nullptr;
static lv_obj_t *e_button_right = nullptr;
static lv_obj_t *e_button_left_text = nullptr;
static lv_obj_t *e_button_mid_text = nullptr;
static lv_obj_t *e_button_right_text = nullptr;

static lv_obj_t *e_speed_label = nullptr;
static lv_obj_t *e_speed_slider = nullptr;
static lv_obj_t *e_speed_value = nullptr;
static lv_obj_t *e_batt_title = nullptr;
static lv_obj_t *e_batt_value = nullptr;
static lv_obj_t *e_time_label = nullptr;
static lv_obj_t *e_time_slider = nullptr;
static lv_obj_t *e_time_value = nullptr;
static lv_obj_t *e_size_label = nullptr;
static lv_obj_t *e_size_slider = nullptr;
static lv_obj_t *e_size_value = nullptr;
static lv_obj_t *e_accel_label = nullptr;
static lv_obj_t *e_accel_slider = nullptr;
static lv_obj_t *e_accel_value = nullptr;

static float e_speed = 0.0f;
static float e_time = 0.0f;
static float e_size = 0.0f;
static float e_accel = 0.0f;

static long e_enc1 = 0;
static long e_enc2 = 0;
static long e_enc3 = 0;
static long e_enc4 = 0;

static bool e_ramp_enabled = true;
static int e_ramp_value = 1;
static int e_ramp_time_ms = 75;
static int e_ramp_max = 8;
static int e_ramp_active_encoder = 0;
static unsigned long e_ramp_ms = 0;

static bool e_is_paired = false;
static bool e_is_on = false;
static bool e_addon_enabled = false;
static uint8_t e_eject_addr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static constexpr uint8_t BROADCAST_ADDR[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static constexpr int LEGACY_EJECT_ID = 2;
static constexpr int LEGACY_M5_ID = M5_ID;
static uint32_t e_last_pairing_heartbeat_ms = 0;
static int e_peer_id = EJECT_ID;
static int e_local_id = M5_ID;
static bool e_flush_buttons_once = false;

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

//static void screensaver_check_activity()
//{
//  // Compatibility stub for old addon code path.
//}

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
  *rowLabel = lv_label_create(e_screen);
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

//static void createScreenIfNeeded()
static void EjectUiScreenCreateInternal()
{
  if (e_screen != nullptr) {
    return;
  }

  e_screen = lv_obj_create(nullptr);
  ui_EJECTSettings = e_screen;
  ui_Eject = e_screen;
  lv_obj_clear_flag(e_screen, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(e_screen, screenmachine, LV_EVENT_SCREEN_LOADED, nullptr);

  e_title = lv_label_create(e_screen);
  lv_obj_set_align(e_title, LV_ALIGN_TOP_MID);
  lv_obj_set_y(e_title, 12);
  lv_label_set_text(e_title, T_EJECT);
  lv_obj_set_style_text_font(e_title, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_style(e_title, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);

  e_batt_title = lv_label_create(e_screen);
  lv_obj_set_width(e_batt_title, 85);
  lv_obj_set_height(e_batt_title, 30);
  lv_obj_set_x(e_batt_title, 115);
  lv_obj_set_y(e_batt_title, -103);
  lv_obj_set_align(e_batt_title, LV_ALIGN_CENTER);
  lv_label_set_text(e_batt_title, T_BATT);

  e_batt_value = lv_label_create(e_batt_title);
  lv_obj_set_width(e_batt_value, LV_SIZE_CONTENT);
  lv_obj_set_height(e_batt_value, LV_SIZE_CONTENT);
  lv_obj_set_x(e_batt_value, 0);
  lv_obj_set_y(e_batt_value, -7);
  lv_obj_set_align(e_batt_value, LV_ALIGN_RIGHT_MID);
  lv_label_set_text(e_batt_value, T_BLANK);


  createSliderRow(&e_speed_label, &e_speed_slider, &e_speed_value, T_CUM_SPEED, -60, 0, 100, 0);
  createSliderRow(&e_time_label, &e_time_slider, &e_time_value, T_CUM_TIME, -25, 0, 360, 1);
  createSliderRow(&e_size_label, &e_size_slider, &e_size_value, T_CUM_Volume, 10, 0, 100, 2);
  createSliderRow(&e_accel_label, &e_accel_slider, &e_accel_value, T_CUM_Accel, 45, 0, 100, 3);

  e_button_left = lv_btn_create(e_screen);
  lv_obj_set_width(e_button_left, 100);
  lv_obj_set_height(e_button_left, 30);
  lv_obj_set_y(e_button_left, 100);
  lv_obj_set_x(e_button_left, lv_pct(-33));
  lv_obj_set_align(e_button_left, LV_ALIGN_CENTER);
  lv_obj_add_style(e_button_left, &style_button_l, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_style(e_button_left, &style_button_l_pressed, LV_PART_MAIN | LV_STATE_PRESSED);
  lv_obj_add_style(e_button_left, &style_button_l, LV_PART_MAIN | LV_STATE_FOCUSED);
  e_button_left_text = lv_label_create(e_button_left);
  lv_obj_set_align(e_button_left_text, LV_ALIGN_CENTER);
  lv_label_set_text(e_button_left_text, T_BACK); //was T_HOME

  e_button_mid = lv_btn_create(e_screen);
  lv_obj_set_width(e_button_mid, 100);
  lv_obj_set_height(e_button_mid, 30);
  lv_obj_set_y(e_button_mid, 100);
  lv_obj_set_x(e_button_mid, lv_pct(0));
  lv_obj_set_align(e_button_mid, LV_ALIGN_CENTER);
  lv_obj_add_style(e_button_mid, &style_button_m, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_style(e_button_mid, &style_button_m_pressed, LV_PART_MAIN | LV_STATE_PRESSED);
  lv_obj_add_style(e_button_mid, &style_button_m, LV_PART_MAIN | LV_STATE_FOCUSED);
  e_button_mid_text = lv_label_create(e_button_mid);
  lv_obj_set_align(e_button_mid_text, LV_ALIGN_CENTER);
  lv_label_set_text(e_button_mid_text, T_START);

  e_button_right = lv_btn_create(e_screen);
  lv_obj_set_width(e_button_right, 100);
  lv_obj_set_height(e_button_right, 30);
  lv_obj_set_y(e_button_right, 100);
  lv_obj_set_x(e_button_right, lv_pct(33));
  lv_obj_set_align(e_button_right, LV_ALIGN_CENTER);
  lv_obj_add_style(e_button_right, &style_button_r, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_style(e_button_right, &style_button_r_pressed, LV_PART_MAIN | LV_STATE_PRESSED);
  lv_obj_add_style(e_button_right, &style_button_r, LV_PART_MAIN | LV_STATE_FOCUSED);
  e_button_right_text = lv_label_create(e_button_right);
  lv_obj_set_align(e_button_right_text, LV_ALIGN_CENTER);
  lv_label_set_text(e_button_right_text, T_CUM_LOAD);
}

static void refreshTheme()
{
  if (e_screen == nullptr) {
    return;
  }

  // Apply the shared background style first, then apply the semantic
  // `style_option_bg` (which is defined as a black option background).
  lv_obj_add_style(e_screen, &style_background, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_style(e_screen, &style_option_bg, LV_PART_MAIN | LV_STATE_DEFAULT);

  if (e_title != nullptr) {
    lv_obj_add_style(e_title, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(e_title, &style_title_bar, LV_PART_MAIN | LV_STATE_DEFAULT);
  }

  // Battery/status label in this addon screen should also follow the
  // shared primary text style so icons/text remain readable across
  // dark/light themes.
  if (e_batt_title != nullptr) {
    lv_obj_add_style(e_batt_title, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);
  }
  if (e_batt_value != nullptr) {
    lv_obj_add_style(e_batt_value, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);
  }

  lv_obj_t *valueLabels[] = {e_speed_value, e_time_value, e_size_value, e_accel_value};
  for (lv_obj_t *lbl : valueLabels) {
    if (!lbl) continue;
    lv_obj_add_style(lbl, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
  }
  styleSlider(e_speed_slider, 0);
  styleSlider(e_time_slider, 1);
  styleSlider(e_size_slider, 2);
  styleSlider(e_accel_slider, 3);
}

static void refreshValueLabels()
{
  lv_label_set_text_fmt(e_speed_value, "%d", (int)e_speed);
  lv_label_set_text_fmt(e_time_value, "%d", (int)e_time);
  lv_label_set_text_fmt(e_size_value, "%d", (int)e_size);
  lv_label_set_text_fmt(e_accel_value, "%d", (int)e_accel);
}

static int getRampedDetentDelta(int encoderId, int detents)
{
  if (detents == 0) {
    return 0;
  }

  if (!e_ramp_enabled) {
    e_ramp_value = 1;
    e_ramp_active_encoder = encoderId;
    e_ramp_ms = millis();
    return detents;
  }

  unsigned long now = millis();
  bool sameEncoder = (encoderId == e_ramp_active_encoder);
  bool withinRampWindow = ((now - e_ramp_ms) <= (unsigned long)e_ramp_time_ms);
  if (!sameEncoder || !withinRampWindow) {
    e_ramp_value = 1;
  }

  int sign = (detents > 0) ? 1 : -1;
  int steps = abs(detents);
  int delta = 0;
  for (int i = 0; i < steps; ++i) {
    delta += sign * e_ramp_value;
    if (e_ramp_value < e_ramp_max) {
      ++e_ramp_value;
    }
  }

  e_ramp_active_encoder = encoderId;
  e_ramp_ms = now;
  return delta;
}

static void sendPairingHeartbeatIfNeeded()
{
  if (!e_addon_enabled) {
    return;
  }

  if (e_is_paired) {
    return;
  }

  const uint32_t nowMs = millis();
  if ((nowMs - e_last_pairing_heartbeat_ms) < 1000UL) {
    return;
  }
  e_last_pairing_heartbeat_ms = nowMs;

  if (!ensurePeer(BROADCAST_ADDR)) {
    return;
  }

  EjectMessage msg = {};
  msg.esp_command = HEARTBEAT;
  msg.esp_heartbeat = true;
  msg.esp_target = EJECT_ID;
  msg.esp_sender = M5_ID;
  lockEspNowChannelIfConfigured("pair-heartbeat");
  Serial.printf("ESP-NOW TX: to=%02X:%02X:%02X:%02X:%02X:%02X target=%d cmd=%d sender=%d hb=%d len=%u\n",
                BROADCAST_ADDR[0], BROADCAST_ADDR[1], BROADCAST_ADDR[2], BROADCAST_ADDR[3], BROADCAST_ADDR[4], BROADCAST_ADDR[5],
                msg.esp_target, msg.esp_command, msg.esp_sender, msg.esp_heartbeat ? 1 : 0, (unsigned)sizeof(msg));
  esp_now_send(BROADCAST_ADDR, reinterpret_cast<uint8_t *>(&msg), sizeof(msg));
}

static void setPairedAddress(const uint8_t *mac)
{
  if (memcmp(e_eject_addr, mac, 6) == 0 && e_is_paired) {
    return;
  }

  if (esp_now_is_peer_exist(e_eject_addr)) {
    esp_now_del_peer(e_eject_addr);
  }

  memcpy(e_eject_addr, mac, 6);
  if (ensurePeer(e_eject_addr)) {
    e_is_paired = true;
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
    EjectSendCommand(command, value);
  }

  return changed;
}

static void toggleOnOff()
{
  if (e_is_on) {
    EjectSendCommand(OFF, 0.0f);
    e_is_on = false;
  } else {
    EjectSendCommand(ON, 0.0f);
    e_is_on = true;
  }
  refreshValueLabels();
}

}  // namespace

extern "C" void EjectUiScreenCreate(void)
{
  EjectUiScreenCreateInternal();
}

void EjectPrepareScreen()
{
  EjectUiScreenCreate();
  refreshTheme();
  refreshValueLabels();
  // The click used to enter this screen can still be latched for one loop.
  // Flush it once so left/mid/right actions start from a clean state.
  e_flush_buttons_once = true;
}

lv_obj_t *EjectGetScreen()
{
  EjectUiScreenCreate();
  return e_screen;
}

lv_obj_t *EjectGetBatteryTitleLabel()
{
  return e_batt_title;
}

lv_obj_t *EjectGetBatteryValueLabel()
{
  return e_batt_value;
}

void EjectToggle()
{
  if (e_addon_enabled && e_is_paired) {
    toggleOnOff();
  }
}

bool EjectIsPaired()
{
  return e_addon_enabled && e_is_paired;
}

const uint8_t* EjectGetTxAddress()
{
  if (!e_addon_enabled) return BROADCAST_ADDR;
  return e_is_paired ? e_eject_addr : BROADCAST_ADDR;
}

bool EjectEnsureTxPeer()
{
  return ensurePeer(EjectGetTxAddress());
}

void EjectSetAddonEnabled(bool enabled)
{
  e_addon_enabled = enabled;

  if (!enabled) {
    if (esp_now_is_peer_exist(e_eject_addr)) {
      esp_now_del_peer(e_eject_addr);
    }
    e_is_paired = false;
    e_is_on = false;
    memset(e_eject_addr, 0xFF, sizeof(e_eject_addr));
  }

  // Allow immediate heartbeat retry when enabled.
  e_last_pairing_heartbeat_ms = 0;
}

bool EjectSendCommand(int command, float value)
{
  if (!e_addon_enabled) {
    return false;
  }

  if (!e_is_paired) {
    sendPairingHeartbeatIfNeeded();
    return false;
  }

  if (!ensurePeer(e_eject_addr)) {
    return false;
  }

  if (ejectUnload && command == CUMSIZE) {
    value = value * -1.0f; // Invert size value when unload mode is active.
  }

  EjectMessage msg = {};
  msg.esp_connected = true;
  msg.esp_command = command;
  msg.esp_value = value;
  msg.esp_target = e_peer_id;
  msg.esp_sender = e_local_id;

  Serial.printf("ESP-NOW TX: to=%02X:%02X:%02X:%02X:%02X:%02X target=%d cmd=%d sender=%d hb=%d len=%u\n",
                e_eject_addr[0], e_eject_addr[1], e_eject_addr[2], e_eject_addr[3], e_eject_addr[4], e_eject_addr[5],
                msg.esp_target, msg.esp_command, msg.esp_sender, msg.esp_heartbeat ? 1 : 0, (unsigned)sizeof(msg));
  esp_err_t result = esp_now_send(e_eject_addr, reinterpret_cast<uint8_t *>(&msg), sizeof(msg));
  Serial.printf("ESP-NOW TX result=%d\n", (int)result);
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

  if (!e_addon_enabled) {
    return false;
  }

  e_peer_id = sender;

  // Only set paired address for this addon when the sender/target indicates
  // the frame is intended for Eject (avoid stealing paired address from other addons)
  if (sender == EJECT_ID || target == EJECT_ID) {
    setPairedAddress(mac);
  }

  if (command == OFF) {
    e_is_on = false;
  } else if (command == ON) {
    e_is_on = true;
  }


  return true;
}

void EjectHandleScreen(const ButtonEvents &events)
{
  EjectUiScreenCreate();
  sendPairingHeartbeatIfNeeded();

  if (e_flush_buttons_once) {
    clearButtonFlags();
    e_flush_buttons_once = false;
  }

  applySliderFromEncoder(encoder1, 1, e_enc1, e_speed, e_speed_slider, CUMSPEED);
  applySliderFromEncoder(encoder2, 2, e_enc2, e_time, e_time_slider, CUMTIME);
  applySliderFromEncoder(encoder3, 3, e_enc3, e_size, e_size_slider, CUMSIZE);
  applySliderFromEncoder(encoder4, 4, e_enc4, e_accel, e_accel_slider, CUMACCEL);
  refreshValueLabels();

  if (events.leftShort) {
    LogDebug("Eject: Left short click - returning to previous screen");
    lv_obj_t *dest = g_addon_return_screen ? g_addon_return_screen : ui_Home;
    _ui_screen_change(dest, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    g_addon_return_screen = nullptr;
    clearButtonFlags();
  } else if (events.mxShort) {
    LogDebug("Eject: Middle short click - toggling on/off");
    toggleOnOff();
    clearButtonFlags();
  } else if (events.rightShort) {
    LogDebug("Eject: Right short click - returning to Menu screen");
    resetEncoderCounts();
    if (ejectUnload) {
      ejectUnload = false;
      //lv_label_set_text(e_button_right_text, T_CUM);
      EjectSendCommand(CUMSIZE, e_size);  //send new size (which gets - in EjectSendCommand if ejectUnload = true)
      
      lv_obj_clear_state(e_button_right, LV_STATE_CHECKED);
    } else {
      ejectUnload = true;
      //lv_label_set_text(e_button_right_text, T_CUM_LOAD);
      EjectSendCommand(CUMSIZE, e_size);  //send new size (which gets - in EjectSendCommand if ejectUnload = true)
      lv_obj_add_state(e_button_right, LV_STATE_CHECKED);
    }
    //_ui_screen_change(ui_Start, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    clearButtonFlags();
  }
}

// C-callable wrapper so C code can invoke the handler
extern "C" void EjectHandleScreen(const struct ButtonEvents *events)
{
  if (events == nullptr) return;
  EjectHandleScreen(*events);
}

bool ejectCommIsFrame(const struct_message& msg)
{
  const int announcedId = (int)(msg.esp_value + (msg.esp_value >= 0 ? 0.5f : -0.5f));
  return (msg.esp_target == CUM ||
          msg.esp_target == CUM_ID ||
          msg.esp_command == CUMSPEED ||
          msg.esp_command == CUMTIME ||
          msg.esp_command == CUMSIZE ||
          msg.esp_command == CUMACCEL ||
          ((msg.esp_command == CONNECT || msg.esp_command == CONN || msg.esp_command == HEARTBEAT) &&
           announcedId == CUM_ID));
}

void ejectCommHandleFrame(const uint8_t* mac, const struct_message& msg)
{
  (void)EjectHandleIncomingEspNowFrame(mac,
                                       msg.esp_target,
                                       msg.esp_sender,
                                       msg.esp_command,
                                       msg.esp_value,
                                       msg.esp_heartbeat);
}

bool ejectCommIsConnected()
{
  return EjectIsPaired();
}

const uint8_t* ejectCommGetTxAddress()
{
  return EjectGetTxAddress();
}

bool ejectCommEnsureTxPeer()
{
  return EjectEnsureTxPeer();
}
