#include "OssmBLE.h"
#include "main.h"

#include <NimBLEDevice.h>
#include <math.h>
#include <freertos/semphr.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

// Configuration
static const char* OSSM_BLE_DEVICE_NAME = "OSSM";
static const char* OSSM_BLE_SERVICE_UUID = "522b443a-4f53-534d-0001-420badbabe69";
static const char* OSSM_BLE_COMMAND_CHAR_UUID = "522b443a-4f53-534d-1000-420badbabe69";
static const char* OSSM_BLE_SPEED_KNOB_CHAR_UUID = "522b443a-4f53-534d-1010-420badbabe69";
static const char* OSSM_BLE_STATE_CHAR_UUID = "522b443a-4f53-534d-2000-420badbabe69";

// Debug flag is defined in config_debug.cpp
extern bool ShowBLECommandResponses;

// State
static bool ble_initialized = false;
static bool use_ble_transport = false;

static NimBLEClient* ble_client = nullptr;
static NimBLERemoteCharacteristic* ble_command_char = nullptr;
static NimBLERemoteCharacteristic* ble_speed_knob_char = nullptr;
static NimBLERemoteCharacteristic* ble_state_char = nullptr;

static uint32_t ble_last_state_poll_ms = 0;
static String ble_last_state_raw;
static OssmBleMachineState ble_last_machine_state;
static float ble_last_requested_stroke = -1.0f;
// Suppress UI pause state when we intentionally pause the device as an
// implementation detail (e.g. to move stroke to zero). Cleared when the
// machine reports stroke==0.
static bool ble_suppress_ui_on_pause = false;
static const uint32_t BLE_STATE_POLL_INTERVAL_MS = 1000;
static bool ble_paused_state = false;

// Unpause speed storage
static float ble_unpause_speed = 0.0f;

// Mutex to serialize BLE access for operations that need it
static SemaphoreHandle_t ble_send_mutex = nullptr;

// RADR-style single command queue (matching RADR behavior)
#include <queue>

static std::queue<String> ble_command_queue;
static TaskHandle_t ble_tx_task_handle = nullptr;
static SemaphoreHandle_t ble_command_sem = nullptr; // signals TX task
// reuse ble_send_mutex to protect queue operations

// Client callbacks to handle disconnects
class OssmBleClientCallbacks : public NimBLEClientCallbacks {
  void onConnect(NimBLEClient* pClient) override {}
  void onDisconnect(NimBLEClient* pClient) override {
    Serial.println("BLE client disconnected");
    if (pClient) {
      NimBLEAddress addr = pClient->getPeerAddress();
      Serial.print("BLE disconnected from: "); Serial.println(addr.toString().c_str());
    }
    // On disconnect: leave queued commands (they will retry on next TX cycle)
    ble_command_char = nullptr;
    ble_speed_knob_char = nullptr;
    ble_state_char = nullptr;
  }
};
static OssmBleClientCallbacks g_ble_client_callbacks;

// Helpers
static int clampPercent(float value)
{
  if (value < 0.0f) return 0;
  if (value > 100.0f) return 100;
  return (int)(value + 0.5f);
}

static int mapSensationToBlePercent(float value)
{
  float normalized = value / 100.0f;
  if (normalized < -1.0f) normalized = -1.0f;
  if (normalized > 1.0f) normalized = 1.0f;
  float shaped = copysignf(powf(fabsf(normalized), 1.25f), normalized);
  float percent = (shaped + 1.0f) * 50.0f;
  return clampPercent(percent);
}

static float extractValueAfterKey(const String& text, const String& key)
{
  int keyPos = text.indexOf(key);
  if (keyPos < 0) return -1.0f;
  int pos = keyPos + key.length();
  while (pos < text.length()) {
    char c = text.charAt(pos);
    if ((c >= '0' && c <= '9') || c == '-' || c == '.') break;
    ++pos;
  }
  if (pos >= text.length()) return -1.0f;
  int end = pos;
  while (end < text.length()) {
    char c = text.charAt(end);
    if (!((c >= '0' && c <= '9') || c == '.')) break;
    ++end;
  }
  String valueStr = text.substring(pos, end);
  return valueStr.toFloat();
}

static bool extractJsonStringValue(const String& text, const String& key, String* outValue)
{
  if (outValue == nullptr) return false;
  String pattern = String("\"") + key + String("\"");
  int keyPos = text.indexOf(pattern);
  if (keyPos < 0) return false;
  int colonPos = text.indexOf(':', keyPos + pattern.length());
  if (colonPos < 0) return false;
  int startQuote = text.indexOf('"', colonPos + 1);
  if (startQuote < 0) return false;
  int endQuote = text.indexOf('"', startQuote + 1);
  if (endQuote < 0) return false;
  *outValue = text.substring(startQuote + 1, endQuote);
  return true;
}

static OssmBleMachineMode parseBleMachineMode(const String& stateName)
{
  if (stateName == "homing.forward") return OssmBleMachineMode::HomingForward;
  if (stateName == "homing.backward") return OssmBleMachineMode::HomingBackward;
  if (stateName == "strokeEngine.idle") return OssmBleMachineMode::StrokeEngineIdle;
  if (stateName.startsWith("strokeEngine.")) return OssmBleMachineMode::StrokeEngineActive;
  if (stateName.startsWith("simplePenetration")) return OssmBleMachineMode::SimplePenetration;
  if (stateName.startsWith("streaming")) return OssmBleMachineMode::Streaming;
  if (stateName.startsWith("menu")) return OssmBleMachineMode::Menu;
  if (stateName.length() == 0) return OssmBleMachineMode::Unknown;
  return OssmBleMachineMode::Other;
}

static bool parseBleMachineState(const String& rawState, OssmBleMachineState* outState)
{
  if (outState == nullptr) return false;
  String parsedStateName;
  if (!extractJsonStringValue(rawState, "state", &parsedStateName)) return false;
  float parsedSpeed = extractValueAfterKey(rawState, "speed");
  float parsedStroke = extractValueAfterKey(rawState, "stroke");
  float parsedSensation = extractValueAfterKey(rawState, "sensation");
  float parsedDepth = extractValueAfterKey(rawState, "depth");
  float parsedPattern = extractValueAfterKey(rawState, "pattern");
  if (parsedSpeed < 0.0f || parsedStroke < 0.0f || parsedSensation < 0.0f || parsedDepth < 0.0f || parsedPattern < 0.0f) return false;
  outState->valid = true;
  outState->raw = rawState;
  outState->stateName = parsedStateName;
  outState->mode = parseBleMachineMode(parsedStateName);
  outState->speed = (int)(parsedSpeed + 0.5f);
  outState->stroke = (int)(parsedStroke + 0.5f);
  outState->sensation = (int)(parsedSensation + 0.5f);
  outState->depth = (int)(parsedDepth + 0.5f);
  outState->pattern = (int)(parsedPattern + 0.5f);
  return true;
}

static void updateBleMachineStateCache(const String& rawState)
{
  OssmBleMachineState parsedState;
  if (parseBleMachineState(rawState, &parsedState)) {
    ble_last_machine_state = parsedState;
  }
}

static String toBleCommandString(OssmBleCommand command, float value, float speedValue, float maxDepthMm, float maxSpeedValue)
{
  switch (command) {
    case OssmBleCommand::SetSpeed:
      return String("set:speed:") + String(clampPercent(value));
    case OssmBleCommand::SetDepth:
      return String("set:depth:") + String(clampPercent(value));
    case OssmBleCommand::SetStroke:
      return String("set:stroke:") + String(clampPercent(value));
    case OssmBleCommand::SetSensation:
      return String("set:sensation:") + String(mapSensationToBlePercent(value));
    case OssmBleCommand::SetPattern: {
      int patternIdx = (int)(value + 0.5f);
      if (patternIdx < 0) patternIdx = 0;
      if (patternIdx > 6) patternIdx = 6;
      return String("set:pattern:") + String(patternIdx);
    }
    case OssmBleCommand::On: {
      int resumeSpeed = clampPercent(speedValue);
      if (resumeSpeed <= 0) resumeSpeed = 0;
      return String("set:speed:") + String(resumeSpeed);
    }
    case OssmBleCommand::Off:
      return String("set:speed:0");
    default:
      return String("");
  }
}

static bool isRealtimeAppCommand(int appCommand)
{
  return appCommand == SPEED || appCommand == DEPTH || appCommand == STROKE || appCommand == SENSATION || appCommand == PATTERN;
}

static bool isRealtimeBleCommand(OssmBleCommand command)
{
  return command == OssmBleCommand::SetSpeed || command == OssmBleCommand::SetDepth || command == OssmBleCommand::SetStroke || command == OssmBleCommand::SetSensation || command == OssmBleCommand::SetPattern;
}

// Notification handlers: forward machine-state updates to state cache
static void bleStateNotifyCallback(NimBLERemoteCharacteristic* rc, uint8_t* data, size_t length, bool isNotify)
{
  (void)isNotify;
  if (data == nullptr || length == 0) return;
  String s((const char*)data, length);
  if (ShowBLECommandResponses) {
    Serial.print("BLE notify: "); Serial.println(s);
  }
  // Treat all non-empty notifications as potential machine state updates
  updateBleMachineStateCache(s);
}

static void bleCommandNotifyCallback(NimBLERemoteCharacteristic* rc, uint8_t* data, size_t length, bool isNotify)
{
  (void)isNotify;
  if (data == nullptr || length == 0) return;
  String s((const char*)data, length);
  if (ShowBLECommandResponses) {
    Serial.print("BLE cmd notify: "); Serial.println(s);
  }
  updateBleMachineStateCache(s);
}

// TX task: drain RADR-style command queue and perform direct writes (no per-command ACK)
static void bleTxTask(void* pv)
{
  (void)pv;
  for (;;) {
    // wait for a command to be enqueued
    if (ble_command_sem == nullptr || xSemaphoreTake(ble_command_sem, portMAX_DELAY) != pdTRUE) continue;

    // drain all available commands
    for (;;) {
      String cmd;
      // pop next command under mutex
      if (ble_send_mutex) xSemaphoreTake(ble_send_mutex, portMAX_DELAY);
      bool had = false;
      if (!ble_command_queue.empty()) {
        cmd = ble_command_queue.front();
        ble_command_queue.pop();
        had = true;
      }
      if (ble_send_mutex) xSemaphoreGive(ble_send_mutex);

      if (!had) break;

      if (!OssmBleTryConnect()) {
        Serial.println("bleTxTask: OssmBleTryConnect failed, will retry");
        // push back and wait before retrying
        if (ble_send_mutex) xSemaphoreTake(ble_send_mutex, portMAX_DELAY);
        ble_command_queue.push(cmd);
        if (ble_send_mutex) xSemaphoreGive(ble_send_mutex);
        vTaskDelay(pdMS_TO_TICKS(200));
        break;
      }

      if (ble_command_char) {
        ble_command_char->writeValue((uint8_t*)cmd.c_str(), cmd.length(), false);
      }
      // small spacing between writes
      vTaskDelay(pdMS_TO_TICKS(10));
    }
  }
}

bool OssmBleConnected()
{
  return (ble_client != nullptr && ble_client->isConnected() && ble_command_char != nullptr);
}

void OssmBleInit()
{
  if (!ble_initialized) {
    NimBLEDevice::init("M5-OSSM-Remote");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);
    ble_initialized = true;
  }
  if (ble_send_mutex == nullptr) {
    ble_send_mutex = xSemaphoreCreateMutex();
  }
  if (ble_command_sem == nullptr) {
    ble_command_sem = xSemaphoreCreateBinary();
  }
  if (ble_tx_task_handle == nullptr) {
    #if CONFIG_FREERTOS_UNICORE
    xTaskCreatePinnedToCore(bleTxTask, "bleTxTask", 6144, nullptr, tskIDLE_PRIORITY+2, &ble_tx_task_handle, 0);
    #else
    xTaskCreatePinnedToCore(bleTxTask, "bleTxTask", 6144, nullptr, tskIDLE_PRIORITY+2, &ble_tx_task_handle, 1);
    #endif
  }
}

bool OssmBleTryConnect(void (*postInitCallback)())
{
  if (OssmBleConnected()) return true;
  bool wasUninitialized = !ble_initialized;
  OssmBleInit();
  if (wasUninitialized && postInitCallback) postInitCallback();

  NimBLEScan* scanner = NimBLEDevice::getScan();
  if (scanner == nullptr) return false;
  Serial.println("OssmBleTryConnect: starting scan");
  scanner->setActiveScan(true);
  NimBLEScanResults results = scanner->start(3, false);
  Serial.printf("OssmBleTryConnect: scan returned %d results\n", results.getCount());

  std::string targetAddress;
  NimBLEUUID serviceUuid(OSSM_BLE_SERVICE_UUID);
  for (int i = 0; i < results.getCount(); ++i) {
    NimBLEAdvertisedDevice device = results.getDevice(i);
    bool nameMatch = device.haveName() && device.getName() == OSSM_BLE_DEVICE_NAME;
    bool serviceMatch = device.haveServiceUUID() && device.isAdvertisingService(serviceUuid);
    if (nameMatch || serviceMatch) {
      targetAddress = device.getAddress().toString();
      Serial.print("OssmBleTryConnect: matched device "); Serial.println(targetAddress.c_str());
      break;
    }
  }
  scanner->clearResults();
  if (targetAddress.empty()) return false;

  if (ble_client == nullptr) {
    ble_client = NimBLEDevice::createClient();
    if (ble_client == nullptr) return false;
    // attach client callbacks to handle disconnects
    ble_client->setClientCallbacks(&g_ble_client_callbacks, false);
    Serial.println("OssmBleTryConnect: created BLE client");
  }

  NimBLEAddress remoteAddress(targetAddress);
  Serial.print("OssmBleTryConnect: connecting to "); Serial.println(targetAddress.c_str());
  bool connected = ble_client->connect(remoteAddress);
  Serial.printf("OssmBleTryConnect: connect returned %d\n", connected ? 1 : 0);
  if (!connected) {
    Serial.println("OssmBleTryConnect: connect failed");
    ble_command_char = nullptr;
    ble_speed_knob_char = nullptr;
    ble_state_char = nullptr;
    return false;
  }

  NimBLERemoteService* service = ble_client->getService(NimBLEUUID(OSSM_BLE_SERVICE_UUID));
  if (service == nullptr) {
    ble_client->disconnect();
    ble_command_char = nullptr;
    return false;
  }

  ble_command_char = service->getCharacteristic(NimBLEUUID(OSSM_BLE_COMMAND_CHAR_UUID));
  if (ble_command_char == nullptr || !ble_command_char->canWrite()) {
    ble_client->disconnect();
    ble_command_char = nullptr;
    ble_speed_knob_char = nullptr;
    ble_state_char = nullptr;
    return false;
  }

  ble_speed_knob_char = service->getCharacteristic(NimBLEUUID(OSSM_BLE_SPEED_KNOB_CHAR_UUID));
  if (ble_speed_knob_char != nullptr && ble_speed_knob_char->canWrite()) {
    static const char* independentMode = "false";
    ble_speed_knob_char->writeValue((uint8_t*)independentMode, strlen(independentMode), true);
  }

  ble_state_char = service->getCharacteristic(NimBLEUUID(OSSM_BLE_STATE_CHAR_UUID));
  if (ble_state_char != nullptr && ble_state_char->canNotify()) {
    ble_state_char->subscribe(true, bleStateNotifyCallback);
    // prime machine state cache on connect by reading once if possible
    if (ble_state_char->canRead()) {
      std::string raw = ble_state_char->readValue();
      if (!raw.empty()) updateBleMachineStateCache(String(raw.c_str()));
    }
  }
  Serial.println("OssmBleTryConnect: subscribe/read state char done");
  // subscribe to command notifications if available
  if (ble_command_char != nullptr && ble_command_char->canNotify()) {
    ble_command_char->subscribe(true, bleCommandNotifyCallback);
    Serial.println("OssmBleTryConnect: subscribed to command char");
  }
  return true;
}

void OssmBleSetMode(bool enabled)
{
  use_ble_transport = enabled;
  if (!enabled) {
    ble_last_state_raw.remove(0);
    ble_last_machine_state = OssmBleMachineState();
    ble_paused_state = false;
  }
}

bool OssmBleIsMode()
{
  return use_ble_transport;
}

bool OssmBleCanSendControlCommands()
{
  return OssmBleIsMode();
}

OssmBleHomeToggleResult OssmBleHandleHomeToggle(bool isRunning, float currentSpeed)
{
  if (ble_paused_state) {
    if (OssmBleResume()) {
      ble_paused_state = false;
      return OssmBleHomeToggleResult::Resumed;
    }
    return OssmBleHomeToggleResult::Failed;
  }
  if (isRunning) {
    OssmBleSendPausedSpeed(currentSpeed);
    if (OssmBlePause()) {
      ble_paused_state = true;
      return OssmBleHomeToggleResult::Paused;
    }
    return OssmBleHomeToggleResult::Failed;
  }
  if (!OssmBleGoToStrokeEngine()) return OssmBleHomeToggleResult::Failed;
  if (currentSpeed > 0.0f) OssmBleStoreUnpauseSpeed(currentSpeed);
  if (OssmBleResume()) {
    ble_paused_state = false;
    return OssmBleHomeToggleResult::Started;
  }
  return OssmBleHomeToggleResult::Failed;
}

OssmBleHomeToggleResult OssmBleHandleStreamingToggle(bool isRunning, float currentSpeed)
{
  return OssmBleHandleHomeToggle(isRunning, currentSpeed);
}

// Core queued send implementation (RADR-style): dedupe set: commands and enqueue
static bool enqueueBleCmdString(const String& command)
{
  if (command.length() == 0) return false;
  // protect queue operations
  if (ble_send_mutex) xSemaphoreTake(ble_send_mutex, portMAX_DELAY);

  // If this is a set:<type>:<value> command, remove any previous of same type from queue
  String cmdType;
  if (command.startsWith("set:")) {
    int colon = command.indexOf(':', 4); // find colon after "set:"
    if (colon > 4) {
      cmdType = command.substring(4, colon);
      // basic validation of type
      if (!(cmdType == "depth" || cmdType == "sensation" || cmdType == "pattern" || cmdType == "speed" || cmdType == "stroke")) {
        cmdType.remove(0);
      }
    }
  }

  if (cmdType.length() > 0) {
    std::queue<String> tempQueue;
    while (!ble_command_queue.empty()) {
      String queued = ble_command_queue.front();
      ble_command_queue.pop();
      if (queued.startsWith("set:")) {
        int qcolon = queued.indexOf(':', 4);
        if (qcolon > 4) {
          String qtype = queued.substring(4, qcolon);
          if (qtype == cmdType) {
            // drop older same-type command
            continue;
          }
        }
      }
      tempQueue.push(queued);
    }
    ble_command_queue = tempQueue;
  }

  // enqueue the new command
  ble_command_queue.push(command);

  if (ble_send_mutex) xSemaphoreGive(ble_send_mutex);

  // signal the TX task
  if (ble_command_sem) xSemaphoreGive(ble_command_sem);
  return true;
}

static bool isRealtimeCommandString(const String& cmd)
{
  if (cmd.startsWith("set:speed:")) return true;
  if (cmd.startsWith("set:depth:")) return true;
  if (cmd.startsWith("set:stroke:")) return true;
  if (cmd.startsWith("set:sensation:")) return true;
  if (cmd.startsWith("set:pattern:")) return true;
  if (cmd.startsWith("stream:")) return true;
  return false;
}

bool OssmBleSendText(const String& command, String* response)
{
  if (command.length() == 0) return false;
  OssmBleInit();

  // RADR behaviour: enqueue, dedupe same-type set: commands, no per-command ACK
  return enqueueBleCmdString(command);
}

bool OssmBleSendCommand(OssmBleCommand command, float value, float speedValue, float maxDepthMm, float maxSpeedValue, String* response)
{
  String bleCommand = toBleCommandString(command, value, speedValue, maxDepthMm, maxSpeedValue);
  if (bleCommand.length() == 0) return false;
  OssmBleInit();
  return enqueueBleCmdString(bleCommand);
}

bool OssmBleSendAppCommand(int appCommand, float value, float currentSpeed, float currentDepth, float currentStroke, bool isRunning, float maxDepthMm, float maxSpeedValue, String* response)
{
  if (appCommand == SETUP_D_I || appCommand == SETUP_D_I_F) {
    return OssmBleGoToSimplePenetration();
  }

  if (appCommand == CONN) {
    return OssmBleTryConnect();
  }

  if (appCommand == SPEED && !isRunning) {
    OssmBleStoreUnpauseSpeed(value);
    OssmBleSendPausedSpeed(value);
    return true;
  }

  float speedParam = currentSpeed;
  if (appCommand == ON) {
    speedParam = (value > 0.001f) ? value : OssmBleGetUnpauseSpeed();
  }

  bool clearDepthAndStrokeFirst = (appCommand == SPEED && (currentDepth <= 0.5f || currentStroke <= 0.5f));

  String* effectiveResponse = nullptr;
  if (!(appCommand == SPEED || appCommand == DEPTH || appCommand == STROKE || appCommand == SENSATION || appCommand == PATTERN)) {
    effectiveResponse = response;
  }

  if (clearDepthAndStrokeFirst) {
    OssmBleSendCommand(OssmBleCommand::SetDepth, 0, speedParam, maxDepthMm, maxSpeedValue, nullptr);
    OssmBleSendCommand(OssmBleCommand::SetStroke, 0, speedParam, maxDepthMm, maxSpeedValue, nullptr);
  }

  // Guard: when app requests STROKE -> 0, pause speed on the device and
  // remember the current speed so we can restore it when stroke becomes >0.
  // Use `ble_last_requested_stroke` to detect transitions because callers
  // update the global `stroke` before calling SendCommand, so the
  // `currentStroke` parameter may already equal the requested value.
  if (appCommand == STROKE) {
    const float requestedStroke = value;
    float prevStroke = 0.0f;
    if (ble_last_requested_stroke >= 0.0f) {
      prevStroke = ble_last_requested_stroke;
    } else if (ble_last_machine_state.valid) {
      prevStroke = (float)ble_last_machine_state.stroke;
    } else {
      prevStroke = currentStroke; // best-effort fallback
    }

    // Going to zero: store current speed and pause the device before sending stroke
    if (requestedStroke <= 0.001f && prevStroke > 0.001f) {
      if (currentSpeed > 0.001f) {
        OssmBleStoreUnpauseSpeed(currentSpeed);
      }
      // Mark that the pause is intentional so the UI can ignore the transient
      // paused state until the stroke has actually reached zero.
      ble_suppress_ui_on_pause = true;
      OssmBleSendCommand(OssmBleCommand::SetSpeed, 0.0f, speedParam, maxDepthMm, maxSpeedValue, nullptr);
      // allow the stroke send below to proceed
    }

    // Coming out of zero: send stroke first then restore saved speed (if any)
    if (requestedStroke > 0.001f && prevStroke <= 0.001f) {
      float resume = OssmBleGetUnpauseSpeed();
      if (resume > 0.001f) {
        bool ok1 = OssmBleSendCommand(OssmBleCommand::SetStroke, requestedStroke, speedParam, maxDepthMm, maxSpeedValue, nullptr);
        bool ok2 = OssmBleSendCommand(OssmBleCommand::SetSpeed, resume, speedParam, maxDepthMm, maxSpeedValue, nullptr);
        // update last-requested stroke and return
        ble_last_requested_stroke = requestedStroke;
        return ok1 && ok2;
      }
    }

    // update last-requested stroke so next call can detect transitions
    ble_last_requested_stroke = requestedStroke;
  }

  OssmBleCommand mapped;
  switch (appCommand) {
    case SPEED: mapped = OssmBleCommand::SetSpeed; break;
    case DEPTH: mapped = OssmBleCommand::SetDepth; break;
    case STROKE: mapped = OssmBleCommand::SetStroke; break;
    case SENSATION: mapped = OssmBleCommand::SetSensation; break;
    case PATTERN: mapped = OssmBleCommand::SetPattern; break;
    case OFF: mapped = OssmBleCommand::Off; break;
    case ON: mapped = OssmBleCommand::On; break;
    default:
      Serial.printf("BLE command mapping missing for app command %d\n", appCommand);
      return false;
  }

  return OssmBleSendCommand(mapped, value, speedParam, maxDepthMm, maxSpeedValue, effectiveResponse);
}

bool OssmBleSendMappedAppCommand(int appCommand, float value, float speedValue, bool clearDepthAndStrokeFirst, float maxDepthMm, float maxSpeedValue, String* response)
{
  (void)clearDepthAndStrokeFirst;
  return OssmBleSendAppCommand(appCommand, value, speedValue, 0, 0, false, maxDepthMm, maxSpeedValue, response);
}

static bool waitForBleDepthAndStrokeZero(uint32_t timeoutMs)
{
  uint32_t startMs = millis();
  while ((millis() - startMs) < timeoutMs) {
    OssmBleMachineState state;
    if (OssmBleGetCurrentState(&state, true)) {
      if (state.depth == 0 && state.stroke == 0) return true;
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  return false;
}

bool OssmBleExecutePulloutStop(float currentSpeed, float maxDepthMm, float maxSpeedValue)
{
  if (!OssmBleCanSendControlCommands()) return false;
  bool ok = true;
  OssmBleStoreUnpauseSpeed(currentSpeed);
  ok = OssmBleSendCommand(OssmBleCommand::SetDepth, 0.0f, 0.0f, maxDepthMm, maxSpeedValue, nullptr) && ok;
  ok = OssmBleSendCommand(OssmBleCommand::SetStroke, 0.0f, 0.0f, maxDepthMm, maxSpeedValue, nullptr) && ok;
  if (!waitForBleDepthAndStrokeZero(3000UL)) return false;
  ok = OssmBleSendCommand(OssmBleCommand::SetSpeed, 0.0f, 0.0f, maxDepthMm, maxSpeedValue, nullptr) && ok;
  return ok;
}

bool OssmBleGoToStrokeEngine()
{
  // If we already have a recent machine-state showing the stroke engine,
  // don't re-issue a go:strokeEngine command which can cause duplicate
  // homing sequences. This mirrors RADR behaviour of avoiding redundant
  // mode transitions when already in the target mode.
  if (ble_last_machine_state.valid) {
    if (ble_last_machine_state.mode == OssmBleMachineMode::StrokeEngineIdle ||
        ble_last_machine_state.mode == OssmBleMachineMode::StrokeEngineActive) {
      return true; // already in stroke engine — nothing to do
    }
  }
  return OssmBleSendText("go:strokeEngine", nullptr);
}
bool OssmBleGoToSimplePenetration() { return OssmBleSendText("go:simplePenetration", nullptr); }
bool OssmBleGoToStreaming() { return OssmBleSendText("go:streaming", nullptr); }
bool OssmBleGoToMenu() { return OssmBleSendText("go:menu", nullptr); }

bool OssmBleSetBuffer(float value)
{
  return OssmBleSendText(String("set:buffer:") + String(clampPercent(value)), nullptr);
}

bool OssmBleSetWifi(const String& ssid, const String& password)
{
  if (ssid.length() == 0) return false;
  String cmd = String("set:wifi:") + ssid + String("|") + password;
  return OssmBleSendText(cmd, nullptr);
}

bool OssmBleStreamPosition(float position, int timeMs)
{
  int pos = clampPercent(position);
  int t = timeMs < 0 ? 0 : timeMs;
  String cmd = String("stream:") + String(pos) + String(":") + String(t);
  return OssmBleSendText(cmd, nullptr);
}

bool OssmBleSendIsPaused(int paused)
{
  if (paused != 0 && paused != 1) return false;
  if (paused == 1) return OssmBlePause();
  return true;
}

bool OssmBleSendPausedSpeed(float speed)
{
  OssmBleStoreUnpauseSpeed((float)clampPercent(speed));
  return true;
}

bool OssmBleReadState(String* stateText, bool logState)
{
  if (stateText == nullptr) return false;
  stateText->remove(0);
  if (!OssmBleTryConnect()) return false;
  if (ble_state_char == nullptr || !ble_state_char->canRead()) return false;
  std::string raw = ble_state_char->readValue();
  if (raw.empty()) return false;
  *stateText = String(raw.c_str());
  bool looksLikeCommandAck = stateText->startsWith("ok:") || stateText->startsWith("fail:");
  if (looksLikeCommandAck) return false;
  updateBleMachineStateCache(*stateText);
  return true;
}

bool OssmBleGetCurrentState(OssmBleMachineState* outState, bool forceRefresh)
{
  if (outState == nullptr) return false;
  if (forceRefresh) {
    String ignored;
    OssmBleReadState(&ignored);
  }
  if (!ble_last_machine_state.valid) return false;
  *outState = ble_last_machine_state;
  return true;
}

bool OssmBlePollLimits(float* outMaxDepthMm, float* outMaxSpeedValue)
{
  if (outMaxDepthMm == nullptr || outMaxSpeedValue == nullptr) return false;
  if (!OssmBleIsMode()) return false;
  uint32_t nowMs = millis();
  if ((nowMs - ble_last_state_poll_ms) < BLE_STATE_POLL_INTERVAL_MS) return false;
  ble_last_state_poll_ms = nowMs;
  String state;
  if (!OssmBleReadState(&state) || state.length() == 0 || state == ble_last_state_raw) return false;
  ble_last_state_raw = state;
  String lower = state;
  lower.toLowerCase();
  float parsedMaxDepth = extractValueAfterKey(lower, "maxdepth");
  if (parsedMaxDepth <= 0.0f) parsedMaxDepth = extractValueAfterKey(lower, "depthmax");
  if (parsedMaxDepth <= 0.0f) parsedMaxDepth = extractValueAfterKey(lower, "max_depth");
  float parsedMaxSpeed = extractValueAfterKey(lower, "maxspeed");
  if (parsedMaxSpeed <= 0.0f) parsedMaxSpeed = extractValueAfterKey(lower, "speedmax");
  if (parsedMaxSpeed <= 0.0f) parsedMaxSpeed = extractValueAfterKey(lower, "max_speed");
  bool updated = false;
  if (parsedMaxDepth > 0.0f) { *outMaxDepthMm = parsedMaxDepth; updated = true; }
  if (parsedMaxSpeed > 0.0f) { *outMaxSpeedValue = parsedMaxSpeed; updated = true; }
  return updated;
}

void OssmBleStoreUnpauseSpeed(float speed) { ble_unpause_speed = speed; }
float OssmBleGetUnpauseSpeed() { return ble_unpause_speed; }

bool OssmBlePause() { return OssmBleSendText("set:speed:0", nullptr); }
bool OssmBleResume()
{
  int resumeSpeed = (int)(ble_unpause_speed + 0.5f);
  if (resumeSpeed < 0) resumeSpeed = 0;
  if (resumeSpeed > 100) resumeSpeed = 100;
  return OssmBleSendText(String("set:speed:") + String(resumeSpeed), nullptr);
}

bool OssmBleShouldSuppressUiPause(int observedStroke)
{
  if (!ble_suppress_ui_on_pause) return false;
  // If the machine reports stroke==0 then the workaround is complete;
  // clear suppression and allow UI to reflect the paused state if appropriate.
  if (observedStroke == 0) {
    ble_suppress_ui_on_pause = false;
    return false;
  }
  return true;
}

#ifdef __cplusplus
extern "C" {
#endif
void OssmBleSetMode_c(int enabled) { OssmBleSetMode(enabled != 0); }
#ifdef __cplusplus
}
#endif
