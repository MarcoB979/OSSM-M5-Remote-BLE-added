// Eject.cpp — Eject addon: BLE link to the Eject device, its settings screen,
// and the command bridge used by the Home "creampie" flow. Fully self-contained
// (screen, BLE client, state); the core only talks to it via Eject.h.

#include "Eject.h"

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <cstring>

#include "main.h"
#include "language.h"
#include "display/colors.h"
#include "display/styles.h"
#include "ui/ui.h"
#include "ui/ui_helpers.h"
#include "buttonhandlers/ButtonHandlers.h"
#include "communication/CommManager.h"
#include "communication/BleComm.h"
#include "communication/BleBackground.h"
#include "config/config_ids.h"
#include "config/debug.h"
#include "screens/ScreenHandler.h"

// Single-definition of EJECT_ID (C linkage so C code can reference it)
// ---- Public id ----
extern "C" const int EJECT_ID = 2;

static bool handleIncomingState(int target, int sender, int command);

bool ejectUnload = false;

namespace {

// ---- Local state ----
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

static lv_obj_t *s_screen = nullptr;
lv_obj_t *ui_Eject = nullptr;

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

static bool s_is_paired = false;
static bool s_is_on = false;
static bool s_addon_enabled = false;
static bool s_ble_init = false;
static NimBLEClient* s_ble_client = nullptr;
static NimBLERemoteCharacteristic* s_ble_rx = nullptr;
static NimBLERemoteCharacteristic* s_ble_tx = nullptr;
static uint32_t s_last_connect_attempt_ms = 0;
static constexpr uint32_t EJECT_CONNECT_RETRY_MS = 3000;
static constexpr uint32_t EJECT_BG_SCAN_MS = 120;
static constexpr uint32_t EJECT_FG_SCAN_MS = 600;
static constexpr uint32_t EJECT_BG_CONNECT_TIMEOUT_MS = 1200;
static constexpr uint32_t EJECT_FG_CONNECT_TIMEOUT_MS = 5000;
static const char* EJECT_BLE_DEVICE_NAME = "Eject";
static const char* EJECT_BLE_SERVICE_UUID = "5f8bb7f0-9f17-4aa8-9c42-3d8b8b4d9001";
static const char* EJECT_BLE_RX_UUID = "5f8bb7f1-9f17-4aa8-9c42-3d8b8b4d9001";
static const char* EJECT_BLE_TX_UUID = "5f8bb7f2-9f17-4aa8-9c42-3d8b8b4d9001";
static int s_peer_id = EJECT_ID;
static int s_local_id = M5_ID;
static bool s_flush_buttons_once = false;

// ---- BLE connection ----
static void ejectBleResetClient()
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

static void ejectBleNotifyCb(NimBLERemoteCharacteristic* ch, uint8_t* data, size_t len, bool isNotify)
{
  (void)ch;
  (void)isNotify;

  if (len != sizeof(EjectMessage)) {
    LogDebugFormatted("Eject BLE RX invalid size=%d expected=%d\n", (int)len, (int)sizeof(EjectMessage));
    return;
  }

  EjectMessage msg = {};
  memcpy(&msg, data, sizeof(msg));
  (void)handleIncomingState(msg.esp_target, msg.esp_sender, msg.esp_command);
}

static void ejectBleInitOnce()
{
  if (s_ble_init) {
    return;
  }
  if (bleRadioIsSuspended()) {
    return;  // WiFi portal owns the radio/RAM; do not re-init NimBLE
  }

  if (!NimBLEDevice::isInitialized()) {
    NimBLEDevice::init("M5-Eject-Addon");
  }
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);
  s_ble_init = true;
}

// ---- Internal helpers ----
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
// ---- Screen construction ----
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

//static void createScreenIfNeeded()
static void EjectUiScreenCreateInternal()
{
  if (s_screen != nullptr) {
    return;
  }

  s_screen = lv_obj_create(nullptr);
  ui_EJECTSettings = s_screen;
  ui_Eject = s_screen;
  lv_obj_clear_flag(s_screen, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(s_screen, screenmachine, LV_EVENT_SCREEN_LOADED, nullptr);

  s_title = lv_label_create(s_screen);
  lv_obj_set_align(s_title, LV_ALIGN_TOP_MID);
  lv_obj_set_y(s_title, 12);
  lv_label_set_text(s_title, T_EJECT);
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


  createSliderRow(&s_speed_label, &s_speed_slider, &s_speed_value, T_CUM_SPEED, -60, 0, 100, 0);
  createSliderRow(&s_time_label, &s_time_slider, &s_time_value, T_CUM_TIME, -25, 0, 360, 1);
  createSliderRow(&s_size_label, &s_size_slider, &s_size_value, T_CUM_Volume, 10, 0, 100, 2);
  createSliderRow(&s_accel_label, &s_accel_slider, &s_accel_value, T_CUM_Accel, 45, 0, 100, 3);

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
  lv_label_set_text(s_button_right_text, T_CUM_LOAD);
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

  lv_obj_t *valueLabels[] = {s_speed_value, s_time_value, s_size_value, s_accel_value};
  for (lv_obj_t *lbl : valueLabels) {
    if (!lbl) continue;
    lv_obj_add_style(lbl, &style_text_primary, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
  }
  styleSlider(s_speed_slider, 0);
  styleSlider(s_time_slider, 1);
  styleSlider(s_size_slider, 2);
  styleSlider(s_accel_slider, 3);
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

static bool ejectBleTryConnect(bool force = false)
{
  if (!s_addon_enabled) {
    return false;
  }

  const uint32_t nowMs = millis();
  if (!force && s_last_connect_attempt_ms != 0 && (nowMs - s_last_connect_attempt_ms) < EJECT_CONNECT_RETRY_MS) {
    return false;
  }
  s_last_connect_attempt_ms = nowMs;

  if (s_ble_client && s_ble_client->isConnected() && s_ble_rx != nullptr) {
    s_is_paired = true;
    return true;
  }

  ejectBleInitOnce();

  NimBLEScan* scanner = NimBLEDevice::getScan();
  if (!scanner) {
    return false;
  }

  scanner->stop();
  scanner->clearResults();
  scanner->setActiveScan(true);
  // Interval > window (duty cycle < 100%) leaves radio time free for the
  // active OSSM GATT connection; interval == window (continuous scan)
  // starves that connection's scheduled events and causes multi-second
  // stalls on every screen while this background probe runs.
  scanner->setInterval(96);
  scanner->setWindow(32);

  const uint32_t scanMs = force ? EJECT_FG_SCAN_MS : EJECT_BG_SCAN_MS;
  NimBLEScanResults results = scanner->getResults(scanMs, false);
  NimBLEAddress targetAddress;
  bool found = false;
  NimBLEUUID serviceUuid(EJECT_BLE_SERVICE_UUID);

  for (int i = 0; i < results.getCount(); ++i) {
    const NimBLEAdvertisedDevice* device = results.getDevice(i);
    if (!device) {
      continue;
    }
    const bool nameMatch = device->haveName() && (device->getName() == EJECT_BLE_DEVICE_NAME);
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
  } else if (s_ble_client->isConnected()) {
    s_ble_client->disconnect();
  }

  s_ble_client->setConnectTimeout(force ? EJECT_FG_CONNECT_TIMEOUT_MS : EJECT_BG_CONNECT_TIMEOUT_MS);

  if (!s_ble_client->connect(targetAddress)) {
    ejectBleResetClient();
    return false;
  }

  NimBLERemoteService* service = s_ble_client->getService(EJECT_BLE_SERVICE_UUID);
  if (!service) {
    ejectBleResetClient();
    return false;
  }

  s_ble_rx = service->getCharacteristic(EJECT_BLE_RX_UUID);
  s_ble_tx = service->getCharacteristic(EJECT_BLE_TX_UUID);
  if (!s_ble_rx) {
    ejectBleResetClient();
    return false;
  }

  if (s_ble_tx && s_ble_tx->canNotify()) {
    s_ble_tx->subscribe(true, ejectBleNotifyCb);
  }

  s_is_paired = true;
  screenRequestStatusStripRefresh();
  return true;
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
  if (s_is_on) {
    if (EjectSendCommand(OFF, 0.0f)) {
      s_is_on = false;
    }
  } else {
    if (EjectSendCommand(ON, 0.0f)) {
      s_is_on = true;
    }
  }
  refreshValueLabels();
}

}  // namespace

// ---- Public API ----
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
  s_flush_buttons_once = true;
}

lv_obj_t *EjectGetScreen()
{
  EjectUiScreenCreate();
  return s_screen;
}

lv_obj_t *EjectGetBatteryTitleLabel()
{
  return s_batt_title;
}

lv_obj_t *EjectGetBatteryValueLabel()
{
  return s_batt_value;
}

void EjectToggle()
{
  if (s_addon_enabled && s_is_paired) {
    toggleOnOff();
  }
}

bool EjectIsPaired()
{
  return s_addon_enabled && s_is_paired && s_ble_client && s_ble_client->isConnected() && s_ble_rx != nullptr;
}

const uint8_t* EjectGetTxAddress()
{
  return nullptr;
}

bool EjectEnsureTxPeer()
{
  return s_ble_client && s_ble_client->isConnected() && s_ble_rx != nullptr;
}

bool EjectTryConnectNow()
{
  return ejectBleTryConnect(true);
}

bool EjectTryConnectBackground()
{
  return ejectBleTryConnect(false);
}

void EjectSetAddonEnabled(bool enabled)
{
  s_addon_enabled = enabled;

  if (!enabled) {
    ejectBleResetClient();
    s_is_on = false;
  }
}

bool EjectSendCommand(int command, float value)
{
  if (!s_addon_enabled) {
    return false;
  }

  if (!s_ble_client || !s_ble_client->isConnected() || s_ble_rx == nullptr) {
    // Never block the UI loop on a scan/connect here — hand reconnection to the
    // background task and report not-ready for now.
    bleBackgroundRequest(BleBgJob::EjectProbe);
    return false;
  }

  if (ejectUnload && command == CUMSIZE) {
    value = value * -1.0f; // Invert size value when unload mode is active.
  }

  EjectMessage msg = {};
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
  if (sender == EJECT_ID || target == EJECT_ID) {
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

void EjectHandleScreen(const ButtonEvents &events)
{
  EjectUiScreenCreate();

  if (!EjectIsPaired()) {
    bleBackgroundRequest(BleBgJob::EjectProbe);  // reconnect in the background
  }

  if (s_flush_buttons_once) {
    clearButtonFlags();
    s_flush_buttons_once = false;
  }

  applySliderFromEncoder(encoder1, 1, s_enc1, s_speed, s_speed_slider, CUMSPEED);
  applySliderFromEncoder(encoder2, 2, s_enc2, s_time, s_time_slider, CUMTIME);
  applySliderFromEncoder(encoder3, 3, s_enc3, s_size, s_size_slider, CUMSIZE);
  applySliderFromEncoder(encoder4, 4, s_enc4, s_accel, s_accel_slider, CUMACCEL);
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
      //lv_label_set_text(s_button_right_text, T_CUM);
      EjectSendCommand(CUMSIZE, s_size);  //send new size (which gets - in EjectSendCommand if ejectUnload = true)
      
      lv_obj_clear_state(s_button_right, LV_STATE_CHECKED);
    } else {
      ejectUnload = true;
      //lv_label_set_text(s_button_right_text, T_CUM_LOAD);
      EjectSendCommand(CUMSIZE, s_size);  //send new size (which gets - in EjectSendCommand if ejectUnload = true)
      lv_obj_add_state(s_button_right, LV_STATE_CHECKED);
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


