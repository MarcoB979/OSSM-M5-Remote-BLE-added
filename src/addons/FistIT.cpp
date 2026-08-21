#include "FistIT.h"

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <cstring>

#include "main.h"
#include "language.h"
#include "display/colors.h"
#include "ui/ui.h"
#include "ui/ui_helpers.h"
#include "display/colors.h"
#include "display/styles.h"
#include "buttonhandlers/ButtonHandlers.h"
#include "screens/ScreenHandler.h"
#include "communication/CommManager.h"
#include "config/debug.h"
#include "config/config_ids.h"

#ifdef FIST_ID
#undef FIST_ID
#endif
// Single-definition of FIST_ID (C linkage so C code can reference it)
extern "C" const int FIST_ID = 3;

static bool handleIncomingState(int target, int sender, int command);

lv_obj_t *ui_FistIT = nullptr;

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
static bool s_ble_init = false;
static NimBLEClient* s_ble_client = nullptr;
static NimBLERemoteCharacteristic* s_ble_rx = nullptr;
static NimBLERemoteCharacteristic* s_ble_tx = nullptr;
static uint32_t s_last_connect_attempt_ms = 0;
static constexpr uint32_t FIST_CONNECT_RETRY_MS = 3000;
static const char* FIST_BLE_DEVICE_NAME = "Fist-IT";
static const char* FIST_BLE_SERVICE_UUID = "5f8bb6f0-9f17-4aa8-9c42-3d8b8b4d9001";
static const char* FIST_BLE_RX_UUID = "5f8bb6f1-9f17-4aa8-9c42-3d8b8b4d9001";
static const char* FIST_BLE_TX_UUID = "5f8bb6f2-9f17-4aa8-9c42-3d8b8b4d9001";
static int s_peer_id = FIST_ID;
static int s_local_id = M5_ID;
static bool s_flush_buttons_once = false;

static void fistBleResetClient()
{
  const bool wasPaired = s_is_paired;
  s_ble_rx = nullptr;
  s_ble_tx = nullptr;
  s_is_paired = false;
  if (wasPaired) {
    screenRequestStatusStripRefresh();
  }

  if (!s_ble_client) {
    return;
  }

  if (s_ble_client->isConnected()) {
    s_ble_client->disconnect();
  }
  NimBLEDevice::deleteClient(s_ble_client);
  s_ble_client = nullptr;
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

static void fistBleNotifyCb(NimBLERemoteCharacteristic* ch, uint8_t* data, size_t len, bool isNotify)
{
  (void)ch;
  (void)isNotify;

  if (len != sizeof(FistMessage)) {
    LogDebugFormatted("FistIT BLE RX invalid size=%d expected=%d\n", (int)len, (int)sizeof(FistMessage));
    return;
  }

  FistMessage msg = {};
  memcpy(&msg, data, sizeof(msg));
  (void)handleIncomingState(msg.esp_target, msg.esp_sender, msg.esp_command);
}

static void fistBleInitOnce()
{
  if (s_ble_init) {
    return;
  }

  if (!NimBLEDevice::isInitialized()) {
    NimBLEDevice::init("M5-FistIT-Addon");
  }
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);
  s_ble_init = true;
}

static bool fistBleTryConnect(bool force = false)
{
  if (!s_addon_enabled) {
    return false;
  }

  const uint32_t nowMs = millis();
  if (!force && s_last_connect_attempt_ms != 0 && (nowMs - s_last_connect_attempt_ms) < FIST_CONNECT_RETRY_MS) {
    return false;
  }
  s_last_connect_attempt_ms = nowMs;

  if (s_ble_client && s_ble_client->isConnected() && s_ble_rx != nullptr) {
    s_is_paired = true;
    return true;
  }

  fistBleInitOnce();

  NimBLEScan* scanner = NimBLEDevice::getScan();
  if (!scanner) {
    return false;
  }

  scanner->stop();
  scanner->clearResults();
  scanner->setActiveScan(true);
  scanner->setInterval(160);
  scanner->setWindow(160);

  NimBLEScanResults results = scanner->getResults(600, false);
  NimBLEAddress targetAddress;
  bool found = false;
  NimBLEUUID serviceUuid(FIST_BLE_SERVICE_UUID);

  for (int i = 0; i < results.getCount(); ++i) {
    const NimBLEAdvertisedDevice* device = results.getDevice(i);
    if (!device) {
      continue;
    }
    const bool nameMatch = device->haveName() && (device->getName() == FIST_BLE_DEVICE_NAME);
    const bool serviceMatch = device->haveServiceUUID() && device->isAdvertisingService(serviceUuid);
    if (nameMatch || serviceMatch) {
      targetAddress = device->getAddress();
      found = true;
      break;
    }
  }
  scanner->clearResults();

  if (!found) {
    return false;
  }

  if (!s_ble_client) {
    s_ble_client = NimBLEDevice::createClient();
    if (!s_ble_client) {
      return false;
    }
    s_ble_client->setConnectionParams(12, 12, 0, 150);
    s_ble_client->setConnectTimeout(5000);
  } else if (s_ble_client->isConnected()) {
    s_ble_client->disconnect();
  }

  if (!s_ble_client->connect(targetAddress)) {
    fistBleResetClient();
    return false;
  }

  NimBLERemoteService* service = s_ble_client->getService(FIST_BLE_SERVICE_UUID);
  if (!service) {
    fistBleResetClient();
    return false;
  }

  s_ble_rx = service->getCharacteristic(FIST_BLE_RX_UUID);
  s_ble_tx = service->getCharacteristic(FIST_BLE_TX_UUID);
  if (!s_ble_rx) {
    fistBleResetClient();
    return false;
  }

  if (s_ble_tx && s_ble_tx->canNotify()) {
    s_ble_tx->subscribe(true, fistBleNotifyCb);
  }

  s_is_paired = true;
  screenRequestStatusStripRefresh();
  return true;
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
  ui_FistIT = s_screen;
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
  lv_label_set_text(s_button_left_text, T_BACK); //was T_HOME

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
  lv_label_set_text(s_button_mid_text, T_START);

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
  if (s_button_mid_text != nullptr) {
    lv_label_set_text(s_button_mid_text, s_is_on ? T_PAUSE : T_START);
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
    if (FistITSendCommand(OFF, 0.0f)) {
      s_is_on = false;
    }
  } else {
    if (FistITSendCommand(ON, 0.0f)) {
      s_is_on = true;
    }
  }
  refreshValueLabels();
}

}  // namespace

void FistITPrepareScreen()
{
  createScreenIfNeeded();
  refreshTheme();
  refreshValueLabels();
  // The click used to enter this screen can still be latched for one loop.
  // Flush it once so left/mid/right actions start from a clean state.
  s_flush_buttons_once = true;
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

void FistITToggle()
{
  if (s_addon_enabled && s_is_paired) {
    toggleOnOff();
  }
}

bool FistITIsPaired()
{
  return s_addon_enabled && s_is_paired && s_ble_client && s_ble_client->isConnected() && s_ble_rx != nullptr;
}

const uint8_t* FistITGetTxAddress()
{
  return nullptr;
}

bool FistITEnsureTxPeer()
{
  return s_ble_client && s_ble_client->isConnected() && s_ble_rx != nullptr;
}

bool FistITTryConnectNow()
{
  return fistBleTryConnect(true);
}

void FistITSetAddonEnabled(bool enabled)
{
  s_addon_enabled = enabled;

  if (!enabled) {
    fistBleResetClient();
    s_is_on = false;
  }
}

bool FistITSendCommand(int command, float value)
{
  if (!s_addon_enabled) {
    return false;
  }

  if (!s_is_paired || !s_ble_client || !s_ble_client->isConnected() || s_ble_rx == nullptr) {
    (void)fistBleTryConnect(false);
  }

  if (!s_ble_client || !s_ble_client->isConnected() || s_ble_rx == nullptr) {
    return false;
  }

  FistMessage msg = {};
  msg.esp_connected = true;
  msg.esp_command = command;
  msg.esp_value = value;
  msg.esp_target = s_peer_id;
  msg.esp_sender = s_local_id;

  bool writeOk = false;
  if (s_ble_rx->canWrite()) {
    writeOk = s_ble_rx->writeValue(reinterpret_cast<uint8_t *>(&msg), sizeof(msg), true);
  } else if (s_ble_rx->canWriteNoResponse()) {
    writeOk = s_ble_rx->writeValue(reinterpret_cast<uint8_t *>(&msg), sizeof(msg), false);
  }

  if (!writeOk) {
    const bool wasPaired = s_is_paired;
    s_is_paired = false;
    if (wasPaired) {
      screenRequestStatusStripRefresh();
    }
  }
  return writeOk;
}

static bool handleIncomingState(int target, int sender, int command)
{
  if (!s_addon_enabled) {
    return false;
  }

  s_peer_id = sender;
  if (sender == FIST_ID || target == FIST_ID) {
    if (!s_is_paired) {
      s_is_paired = true;
      screenRequestStatusStripRefresh();
    }
  }

  if (command == OFF) {
    s_is_on = false;
  } else if (command == ON) {
    s_is_on = true;
  }


  return true;
}

void FistITHandleScreen(const ButtonEvents &events)
{
  createScreenIfNeeded();

  if (!FistITIsPaired()) {
    (void)fistBleTryConnect(false);
  }

  if (s_flush_buttons_once) {
    clearButtonFlags();
    s_flush_buttons_once = false;
  }

  applySliderFromEncoder(encoder1, 1, s_enc1, s_speed, s_speed_slider, FIST_SPEED);
  applySliderFromEncoder(encoder2, 2, s_enc2, s_rotation, s_rotation_slider, FIST_ROTATION);
  applySliderFromEncoder(encoder3, 3, s_enc3, s_pause, s_pause_slider, FIST_PAUSE);
  applySliderFromEncoder(encoder4, 4, s_enc4, s_accel, s_accel_slider, FIST_ACCEL);
  refreshValueLabels();

  if (events.leftShort) {
    //LogDebug("FistIT: Left short click - returning to previous screen");
    lv_obj_t *dest = g_addon_return_screen ? g_addon_return_screen : ui_Home;
    _ui_screen_change(dest, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    g_addon_return_screen = nullptr;
    clearButtonFlags();
  } else if (events.mxShort) {
    //LogDebug("FistIT: Middle short click - toggling on/off");
    toggleOnOff();
    clearButtonFlags();
  } else if (events.rightShort) {
    //LogDebug("FistIT: Right short click - returning to Menu screen");
    resetEncoderCounts();
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


