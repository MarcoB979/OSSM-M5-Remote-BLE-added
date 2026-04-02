#include "OssmBLE.h"

#include <NimBLEDevice.h>
#include <math.h>

//***********************SHOW DEBUG INFO */

bool ShowBLECommandResponses = true; // Set to true to enable serial printing of BLE command responses for debugging
bool logState = ShowBLECommandResponses;
//***********************SHOW DEBUG INFO */

static const char* OSSM_BLE_DEVICE_NAME = "OSSM";
static const char* OSSM_BLE_SERVICE_UUID = "522b443a-4f53-534d-0001-420badbabe69";
static const char* OSSM_BLE_COMMAND_CHAR_UUID = "522b443a-4f53-534d-1000-420badbabe69";
static const char* OSSM_BLE_SPEED_KNOB_CHAR_UUID = "522b443a-4f53-534d-1010-420badbabe69";
static const char* OSSM_BLE_STATE_CHAR_UUID = "522b443a-4f53-534d-2000-420badbabe69";

static bool ble_initialized = false;
static bool use_ble_transport = false;

//static bool sendBleTextInternal(const String& command, String* response, bool preferFastWrite);

static NimBLEClient* ble_client = nullptr;
static NimBLERemoteCharacteristic* ble_command_char = nullptr;
static NimBLERemoteCharacteristic* ble_speed_knob_char = nullptr;
static NimBLERemoteCharacteristic* ble_state_char = nullptr;

static uint32_t ble_last_state_poll_ms = 0;
static String ble_last_state_raw;
static OssmBleMachineState ble_last_machine_state;
static const uint32_t BLE_STATE_POLL_INTERVAL_MS = 1000;
static OssmBleReadyState ble_ready_state = OssmBleReadyState::Unknown;
static uint32_t ble_last_go_to_stroke_engine_ms = 0;
static bool ble_paused_state = false;

static constexpr int APP_CMD_SPEED = 1;
static constexpr int APP_CMD_DEPTH = 2;
static constexpr int APP_CMD_STROKE = 3;
static constexpr int APP_CMD_SENSATION = 4;
static constexpr int APP_CMD_PATTERN = 5;
static constexpr int APP_CMD_OFF = 10;
static constexpr int APP_CMD_ON = 11;
static constexpr int APP_CMD_SETUP_D_I = 12;
static constexpr int APP_CMD_SETUP_D_I_F = 13;

// Stored resume speed for BLE transport
static float ble_unpause_speed = 0.0f;

void OssmBleStoreUnpauseSpeed(float speed)
{
  ble_unpause_speed = speed;
}

float OssmBleGetUnpauseSpeed()
{
  return ble_unpause_speed;
}

bool OssmBlePause()
{
  // OSSM command model has no explicit pause flag; speed=0 is the pause behavior.
  return OssmBleSendText("set:speed:0", nullptr);
}

bool OssmBleResume()
{
  // Resume is modeled as restoring speed.
  int resumeSpeed = (int)(ble_unpause_speed + 0.5f);
  if (resumeSpeed < 0) resumeSpeed = 0;
  if (resumeSpeed > 100) resumeSpeed = 100;
  return OssmBleSendText(String("set:speed:") + String(resumeSpeed), nullptr);
}

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

static int mapSpeedToBlePercent(float value, float maxSpeedValue)
{
  if (maxSpeedValue > 0.0f) {
    return clampPercent((value * 100.0f) / maxSpeedValue);
  }
  return clampPercent(value);
}

static void resetBleSessionState()
{
  ble_last_state_poll_ms = 0;
  ble_last_state_raw.remove(0);
  ble_last_machine_state = OssmBleMachineState();
  ble_ready_state = OssmBleReadyState::Unknown;
  ble_last_go_to_stroke_engine_ms = 0;
  ble_paused_state = false;
}

static void requestBleStrokeEngineIfNeeded()
{
  uint32_t now = millis();
  if ((now - ble_last_go_to_stroke_engine_ms) >= 2000UL) {
    OssmBleGoToStrokeEngine();
    ble_last_go_to_stroke_engine_ms = now;
  }
}

static bool mapAppCommandToBleCommand(int appCommand, OssmBleCommand* outCommand)
{
  if (outCommand == nullptr) {
    return false;
  }

  switch (appCommand) {
    case APP_CMD_SPEED:
      *outCommand = OssmBleCommand::SetSpeed;
      return true;
    case APP_CMD_DEPTH:
      *outCommand = OssmBleCommand::SetDepth;
      return true;
    case APP_CMD_STROKE:
      *outCommand = OssmBleCommand::SetStroke;
      return true;
    case APP_CMD_SENSATION:
      *outCommand = OssmBleCommand::SetSensation;
      return true;
    case APP_CMD_PATTERN:
      *outCommand = OssmBleCommand::SetPattern;
      return true;
    case APP_CMD_OFF:
      *outCommand = OssmBleCommand::Off;
      return true;
    case APP_CMD_ON:
      *outCommand = OssmBleCommand::On;
      return true;
    default:
      return false;
  }
}

static bool isRealtimeAppCommand(int appCommand)
{
  return appCommand == APP_CMD_SPEED ||
         appCommand == APP_CMD_DEPTH ||
         appCommand == APP_CMD_STROKE ||
         appCommand == APP_CMD_SENSATION ||
         appCommand == APP_CMD_PATTERN;
}

static bool isRealtimeBleCommand(OssmBleCommand command)
{
  return command == OssmBleCommand::SetSpeed ||
         command == OssmBleCommand::SetDepth ||
         command == OssmBleCommand::SetStroke ||
         command == OssmBleCommand::SetSensation ||
         command == OssmBleCommand::SetPattern;
}

static int mapDepthToBlePercent(float value, float maxDepthMm)
{
  if (maxDepthMm > 0.0f) {
    return clampPercent((value * 100.0f) / maxDepthMm);
  }
  return clampPercent(value);
}

static float extractValueAfterKey(const String& text, const String& key)
{
  int keyPos = text.indexOf(key);
  if (keyPos < 0) {
    return -1.0f;
  }

  int pos = keyPos + key.length();
  while (pos < text.length()) {
    char c = text.charAt(pos);
    if ((c >= '0' && c <= '9') || c == '-' || c == '.') {
      break;
    }
    ++pos;
  }

  if (pos >= text.length()) {
    return -1.0f;
  }

  int end = pos;
  while (end < text.length()) {
    char c = text.charAt(end);
    if (!((c >= '0' && c <= '9') || c == '.')) {
      break;
    }
    ++end;
  }

  String valueStr = text.substring(pos, end);
  return valueStr.toFloat();
}

static bool extractJsonStringValue(const String& text, const String& key, String* outValue)
{
  if (outValue == nullptr) {
    return false;
  }

  String pattern = String("\"") + key + String("\"");
  int keyPos = text.indexOf(pattern);
  if (keyPos < 0) {
    return false;
  }

  int colonPos = text.indexOf(':', keyPos + pattern.length());
  if (colonPos < 0) {
    return false;
  }

  int startQuote = text.indexOf('"', colonPos + 1);
  if (startQuote < 0) {
    return false;
  }

  int endQuote = text.indexOf('"', startQuote + 1);
  if (endQuote < 0) {
    return false;
  }

  *outValue = text.substring(startQuote + 1, endQuote);
  return true;
}

static OssmBleMachineMode parseBleMachineMode(const String& stateName)
{
  if (stateName == "homing.forward") {
    return OssmBleMachineMode::HomingForward;
  }
  if (stateName == "homing.backward") {
    return OssmBleMachineMode::HomingBackward;
  }
  if (stateName == "strokeEngine.idle") {
    return OssmBleMachineMode::StrokeEngineIdle;
  }
  if (stateName.startsWith("strokeEngine.")) {
    return OssmBleMachineMode::StrokeEngineActive;
  }
  if (stateName.startsWith("simplePenetration")) {
    return OssmBleMachineMode::SimplePenetration;
  }
  if (stateName.startsWith("streaming")) {
    return OssmBleMachineMode::Streaming;
  }
  if (stateName.startsWith("menu")) {
    return OssmBleMachineMode::Menu;
  }
  if (stateName.length() == 0) {
    return OssmBleMachineMode::Unknown;
  }
  return OssmBleMachineMode::Other;
}

static bool parseBleMachineState(const String& rawState, OssmBleMachineState* outState)
{
  if (outState == nullptr) {
    return false;
  }

  String parsedStateName;
  if (!extractJsonStringValue(rawState, "state", &parsedStateName)) {
    return false;
  }

  float parsedSpeed = extractValueAfterKey(rawState, "speed");
  float parsedStroke = extractValueAfterKey(rawState, "stroke");
  float parsedSensation = extractValueAfterKey(rawState, "sensation");
  float parsedDepth = extractValueAfterKey(rawState, "depth");
  float parsedPattern = extractValueAfterKey(rawState, "pattern");

  if (parsedSpeed < 0.0f || parsedStroke < 0.0f || parsedSensation < 0.0f || parsedDepth < 0.0f || parsedPattern < 0.0f) {
    return false;
  }

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

const char* OssmBleMachineModeName(OssmBleMachineMode mode)
{
  switch (mode) {
    case OssmBleMachineMode::Unknown:
      return "unknown";
    case OssmBleMachineMode::HomingForward:
      return "homing.forward";
    case OssmBleMachineMode::HomingBackward:
      return "homing.backward";
    case OssmBleMachineMode::StrokeEngineIdle:
      return "strokeEngine.idle";
    case OssmBleMachineMode::StrokeEngineActive:
      return "strokeEngine.active";
    case OssmBleMachineMode::SimplePenetration:
      return "simplePenetration";
    case OssmBleMachineMode::Streaming:
      return "streaming";
    case OssmBleMachineMode::Menu:
      return "menu";
    case OssmBleMachineMode::Other:
      return "other";
    default:
      return "unknown";
  }
}

bool OssmBleIsHoming(const OssmBleMachineState& state)
{
  return state.mode == OssmBleMachineMode::HomingForward ||
         state.mode == OssmBleMachineMode::HomingBackward;
}

bool OssmBleIsReadyForStrokeEngine(const OssmBleMachineState& state)
{
  return state.mode == OssmBleMachineMode::StrokeEngineIdle ||
         state.mode == OssmBleMachineMode::StrokeEngineActive ||
         state.mode == OssmBleMachineMode::Streaming;
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
  switch (command)
  {
    case OssmBleCommand::SetSpeed:
    {
      return String("set:speed:") + String(clampPercent(value));
    }
    case OssmBleCommand::SetDepth:
    {
      return String("set:depth:") + String(clampPercent(value));
    }
    case OssmBleCommand::SetStroke:
    {
      return String("set:stroke:") + String(clampPercent(value));
    }
    case OssmBleCommand::SetSensation:
    {
      return String("set:sensation:") + String(mapSensationToBlePercent(value));
    }
    case OssmBleCommand::SetPattern:
    {
      int patternIdx = (int)(value + 0.5f);
      if (patternIdx < 0) patternIdx = 0;
      if (patternIdx > 6) patternIdx = 6;
      return String("set:pattern:") + String(patternIdx);
    }
    case OssmBleCommand::On:
    {
      int resumeSpeed = clampPercent(speedValue);
      if (resumeSpeed <= 0) {
        resumeSpeed = 0;
      }
      return String("set:speed:") + String(resumeSpeed);
    }
    case OssmBleCommand::Off:
      return String("set:speed:0");
    default:
      return String("");
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
}

bool OssmBleTryConnect(void (*postInitCallback)())
{
  if (OssmBleConnected()) {
    return true;
  }

  bool wasUninitialized = !ble_initialized;
  OssmBleInit(); // no-op if already initialised
  // If we just ran NimBLEDevice::init() it disrupts the SPI DMA, so let the
  // caller re-flush the display now that the BLE stack is stable.
  if (wasUninitialized && postInitCallback) {
    postInitCallback();
  }

  NimBLEScan* scanner = NimBLEDevice::getScan();
  if (scanner == nullptr) {
    return false;
  }

  scanner->setActiveScan(true);
  NimBLEScanResults results = scanner->start(3, false);

  std::string targetAddress;
  NimBLEUUID serviceUuid(OSSM_BLE_SERVICE_UUID);
  for (int i = 0; i < results.getCount(); ++i) {
    NimBLEAdvertisedDevice device = results.getDevice(i);

    bool nameMatch = device.haveName() && device.getName() == OSSM_BLE_DEVICE_NAME;
    bool serviceMatch = device.haveServiceUUID() && device.isAdvertisingService(serviceUuid);
    if (nameMatch || serviceMatch) {
      targetAddress = device.getAddress().toString();
      break;
    }
  }

  scanner->clearResults();

  if (targetAddress.empty()) {
    Serial.println("OssmBleTryConnect: no target address found");
    return false;
  }

  if (ble_client == nullptr) {
    ble_client = NimBLEDevice::createClient();
    if (ble_client == nullptr) {
      return false;
    }
  }

  NimBLEAddress remoteAddress(targetAddress);
  bool connected = ble_client->connect(remoteAddress);

  if (!connected) {
    Serial.println("OssmBleTryConnect: failed to connect to remote");
    ble_command_char = nullptr;
    ble_speed_knob_char = nullptr;
    ble_state_char = nullptr;
    return false;
  }

  Serial.print("OssmBleTryConnect: connected to ");
  Serial.println(targetAddress.c_str());

  NimBLERemoteService* service = ble_client->getService(NimBLEUUID(OSSM_BLE_SERVICE_UUID));
  if (service == nullptr) {
    Serial.println("OssmBleTryConnect: service not found");
    ble_client->disconnect();
    ble_command_char = nullptr;
    return false;
  }

  ble_command_char = service->getCharacteristic(NimBLEUUID(OSSM_BLE_COMMAND_CHAR_UUID));
  if (ble_command_char == nullptr || !ble_command_char->canWrite()) {
    Serial.println("OssmBleTryConnect: command characteristic missing or not writable");
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

  if (ble_state_char != nullptr) {
    Serial.println("OssmBleTryConnect: state characteristic found");
  } else {
    Serial.println("OssmBleTryConnect: state characteristic NOT found");
  }

  return true;
}

void OssmBleSetMode(bool enabled)
{
  use_ble_transport = enabled;
  if (!enabled) {
    resetBleSessionState();
  }
}

bool OssmBleIsMode()
{
  return use_ble_transport;
}

void OssmBleBeginConnectFlow()
{
  ble_ready_state = OssmBleReadyState::WaitingForStrokeEngine;
  ble_last_go_to_stroke_engine_ms = 0;
}

OssmBleReadyState OssmBleUpdateReadyState(bool forceRefresh)
{
  if (!OssmBleIsMode()) {
    return OssmBleReadyState::Unknown;
  }

  OssmBleMachineState bleState;
  if (!OssmBleGetCurrentState(&bleState, forceRefresh)) {
    if (ble_ready_state != OssmBleReadyState::Ready) {
      ble_ready_state = OssmBleReadyState::WaitingForStrokeEngine;
      requestBleStrokeEngineIfNeeded();
    }
    return ble_ready_state;
  }

  if (OssmBleIsReadyForStrokeEngine(bleState)) {
    ble_ready_state = OssmBleReadyState::Ready;
    return ble_ready_state;
  }

  ble_ready_state = OssmBleReadyState::WaitingForStrokeEngine;
  if (!OssmBleIsHoming(bleState)) {
    requestBleStrokeEngineIfNeeded();
  }
  return ble_ready_state;
}

bool OssmBleIsWaitingForReady()
{
  return ble_ready_state == OssmBleReadyState::WaitingForStrokeEngine;
}

bool OssmBleCanSendControlCommands()
{
  return OssmBleIsMode() && ble_ready_state == OssmBleReadyState::Ready;
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

  if (OssmBleUpdateReadyState(true) != OssmBleReadyState::Ready) {
    return OssmBleHomeToggleResult::BlockedNotReady;
  }

  if (OssmBleResume()) {
    ble_paused_state = false;
    return OssmBleHomeToggleResult::Started;
  }

  return OssmBleHomeToggleResult::Failed;
}

bool OssmBleSendText(const String& command, String* response)
{
  return sendBleTextInternal(command, response, false);
}

static bool sendBleTextInternal(const String& command, String* response, bool preferFastWrite)
{
  if (command.length() == 0) {
    return false;
  }

  if (!OssmBleTryConnect()) {
    return false;
  }

  bool useWriteResponse = true;
  if (preferFastWrite && ble_command_char->canWriteNoResponse()) {
    useWriteResponse = false;
  }

  bool writeOk = ble_command_char->writeValue((uint8_t*)command.c_str(), command.length(), useWriteResponse);
  if (!writeOk) {
    return false;
  }

  if (!useWriteResponse) {
    if (response != nullptr) {
      response->remove(0);
    }
    return true;
  }

  if (ble_command_char->canRead()) {
    std::string raw = ble_command_char->readValue();
    if (ShowBLECommandResponses) {
      Serial.print("BLE rx (command response): ");
      Serial.println(raw.c_str());
    }
    if (response != nullptr) {
      *response = String(raw.c_str());
    }

    if (raw.rfind("ok:", 0) == 0) {
      return true;
    }
    if (raw.rfind("fail:", 0) == 0) {
      return false;
    }
  }

  return true;
}

bool OssmBleSendCommand(OssmBleCommand command, float value, float speedValue, float maxDepthMm, float maxSpeedValue, String* response)
{
  String bleCommand = toBleCommandString(command, value, speedValue, maxDepthMm, maxSpeedValue);
  if (bleCommand.length() == 0) {
    return false;
  }

  Serial.print("BLE tx: ");
  Serial.println(bleCommand);

  bool preferFastWrite = (response == nullptr) && isRealtimeBleCommand(command);
  return sendBleTextInternal(bleCommand, response, preferFastWrite);
}

bool OssmBleSendAppCommand(int appCommand, float value, float currentSpeed, float currentDepth, float currentStroke, bool isRunning, float maxDepthMm, float maxSpeedValue, String* response)
{
  if (appCommand == APP_CMD_SPEED && !isRunning) {
    OssmBleStoreUnpauseSpeed(value);
    OssmBleSendPausedSpeed(value);
    return true;
  }

  float speedParam = currentSpeed;
  if (appCommand == APP_CMD_ON) {
    speedParam = (value > 0.001f) ? value : OssmBleGetUnpauseSpeed();
  }

  bool clearDepthAndStrokeFirst =
    appCommand == APP_CMD_SPEED && (currentDepth <= 0.5f || currentStroke <= 0.5f);

  Serial.printf(
    "BLE send params - Command:%d Value:%f speedParam:%f unpause:%f\n",
    appCommand,
    value,
    speedParam,
    OssmBleGetUnpauseSpeed());

  String* effectiveResponse = isRealtimeAppCommand(appCommand) ? nullptr : response;

  return OssmBleSendMappedAppCommand(
    appCommand,
    value,
    speedParam,
    clearDepthAndStrokeFirst,
    maxDepthMm,
    maxSpeedValue,
    effectiveResponse);
}

bool OssmBleSendMappedAppCommand(int appCommand, float value, float speedValue, bool clearDepthAndStrokeFirst, float maxDepthMm, float maxSpeedValue, String* response)
{
  if (!OssmBleCanSendControlCommands()) {
    Serial.printf("BLE command blocked while waiting for readiness: command=%d value=%f\n", appCommand, value);
    return false;
  }

  if (appCommand == APP_CMD_SETUP_D_I || appCommand == APP_CMD_SETUP_D_I_F) {
    return OssmBleGoToSimplePenetration();
  }

  if (appCommand == APP_CMD_SPEED && clearDepthAndStrokeFirst) {
    OssmBleSendCommand(OssmBleCommand::SetDepth, 0, speedValue, maxDepthMm, maxSpeedValue, nullptr);
    OssmBleSendCommand(OssmBleCommand::SetStroke, 0, speedValue, maxDepthMm, maxSpeedValue, nullptr);
  }

  OssmBleCommand bleCommand;
  if (!mapAppCommandToBleCommand(appCommand, &bleCommand)) {
    Serial.printf("BLE command mapping missing for app command %d\n", appCommand);
    return false;
  }

  return OssmBleSendCommand(bleCommand, value, speedValue, maxDepthMm, maxSpeedValue, response);
}

static bool waitForBleDepthAndStrokeZero(uint32_t timeoutMs)
{
  uint32_t startMs = millis();

  while ((millis() - startMs) < timeoutMs) {
    OssmBleMachineState state;
    if (OssmBleGetCurrentState(&state, true)) {
      if (state.depth == 0 && state.stroke == 0) {
        return true;
      }
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }

  return false;
}

bool OssmBleExecutePulloutStop(float currentSpeed, float maxDepthMm, float maxSpeedValue)
{
  if (!OssmBleCanSendControlCommands()) {
    Serial.println("BLE pullout stop blocked while waiting for readiness");
    return false;
  }

  bool ok = true;
  OssmBleStoreUnpauseSpeed(currentSpeed);
  ok = OssmBleGoToSimplePenetration() && ok;
  ok = OssmBleSendCommand(OssmBleCommand::SetDepth, 0.0f, 0.0f, maxDepthMm, maxSpeedValue, nullptr) && ok;
  ok = OssmBleSendCommand(OssmBleCommand::SetStroke, 0.0f, 0.0f, maxDepthMm, maxSpeedValue, nullptr) && ok;

  if (!waitForBleDepthAndStrokeZero(3000UL)) {
    Serial.println("BLE pullout stop timed out waiting for depth/stroke to reach zero");
    return false;
  }

  ok = OssmBleSendCommand(OssmBleCommand::SetSpeed, 0.0f, 0.0f, maxDepthMm, maxSpeedValue, nullptr) && ok;
  return ok;
}

bool OssmBleGoToStrokeEngine()
{
  return OssmBleSendText("go:strokeEngine", nullptr);
}

bool OssmBleGoToSimplePenetration()
{
  return OssmBleSendText("go:simplePenetration", nullptr);
}

bool OssmBleGoToStreaming()
{
  return OssmBleSendText("go:streaming", nullptr);
}

bool OssmBleGoToMenu()
{
  return OssmBleSendText("go:menu", nullptr);
}

bool OssmBleSetBuffer(float value)
{
  return OssmBleSendText(String("set:buffer:") + String(clampPercent(value)), nullptr);
}

bool OssmBleSetWifi(const String& ssid, const String& password)
{
  if (ssid.length() == 0) {
    return false;
  }

  String cmd = String("set:wifi:") + ssid + String("|") + password;
  return OssmBleSendText(cmd, nullptr);
}

bool OssmBleStreamPosition(float position, int timeMs)
{
  int pos = clampPercent(position);
  int t = timeMs;
  if (t < 0) {
    t = 0;
  }
  String cmd = String("stream:") + String(pos) + String(":") + String(t);
  return OssmBleSendText(cmd, nullptr);
}

// Compatibility shim: OSSM does not support set:ispaused.
bool OssmBleSendIsPaused(int paused)
{
  if (paused != 0 && paused != 1) return false;

  if (paused == 1) {
    return OssmBlePause();
  }

  // For paused=0 we do nothing here; caller should send target speed explicitly.
  return true;
}

// Compatibility shim: OSSM does not support set:pausedSpeed.
bool OssmBleSendPausedSpeed(float speed)
{
  OssmBleStoreUnpauseSpeed((float)clampPercent(speed));
  return true;
}

bool OssmBleReadState(String* stateText, bool logState)
{
  if (stateText == nullptr) {
    return false;
  }

  stateText->remove(0);

  if (!OssmBleTryConnect()) {
    return false;
  }

  if (ble_state_char == nullptr || !ble_state_char->canRead()) {
    return false;
  }

  std::string raw = ble_state_char->readValue();
  if (raw.empty()) {
    return false;
  }

  *stateText = String(raw.c_str());
//  Serial.print("[BLE] Raw state: ");
//  Serial.println(stateText->c_str());
  updateBleMachineStateCache(*stateText);
  return true; //comment if you want to log state on every read, but it can be chatty and impact performance

  if (logState) {
    Serial.print("BLE rx (state): ");
    Serial.println(stateText->c_str());
  }
  return true;
}

bool OssmBleGetCurrentState(OssmBleMachineState* outState, bool forceRefresh)
{
  if (outState == nullptr) {
    return false;
  }

  if (forceRefresh) {
    String ignored;
    OssmBleReadState(&ignored);
  }

  if (!ble_last_machine_state.valid) {
    return false;
  }

  *outState = ble_last_machine_state;
  return true;
}

bool OssmBlePollLimits(float* outMaxDepthMm, float* outMaxSpeedValue)
{
  if (outMaxDepthMm == nullptr || outMaxSpeedValue == nullptr) {
    return false;
  }

  if (!OssmBleIsMode()) {
    return false;
  }

  uint32_t nowMs = millis();
  if ((nowMs - ble_last_state_poll_ms) < BLE_STATE_POLL_INTERVAL_MS) {
    return false;
  }
  ble_last_state_poll_ms = nowMs;

  String state;
  if (!OssmBleReadState(&state) || state.length() == 0 || state == ble_last_state_raw) {
    return false;
  }
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
  if (parsedMaxDepth > 0.0f) {
    *outMaxDepthMm = parsedMaxDepth;
    updated = true;
  }
  if (parsedMaxSpeed > 0.0f) {
    *outMaxSpeedValue = parsedMaxSpeed;
    updated = true;
  }

  return updated;
}

// Helper to compare two OssmBleMachineState objects (mode, speed, depth, stroke, sensation)
static bool OssmBleStatesAreDifferent(const OssmBleMachineState& a, const OssmBleMachineState& b) {
    return a.mode != b.mode ||
           a.speed != b.speed ||
           a.depth != b.depth ||
           a.stroke != b.stroke ||
           a.sensation != b.sensation ||
           a.pattern != b.pattern;
}

// Polls the OSSM BLE device for the latest machine state every 100ms
// Updates ble_last_machine_state and returns true if successful
bool OssmBlePollState() {
    if (!OssmBleConnected()) {
        ble_last_machine_state.valid = false;
        return false;
    }
    OssmBleMachineState newState;

    if (OssmBleGetCurrentState(&newState, true)) {
        bool changed = OssmBleStatesAreDifferent(newState, ble_last_machine_state);
        ble_last_machine_state = newState;
        ble_last_machine_state.valid = true;
        ble_last_state_poll_ms = millis();
        if (changed) {
            Serial.printf("[BLE] State changed: mode=%d speed=%d depth=%d stroke=%d sensation=%d pattern=%d\n",
                newState.mode, newState.speed, newState.depth, newState.stroke, newState.sensation, newState.pattern);
        }
        return true;
    } else {
        ble_last_machine_state.valid = false;
        return false;
    }
}
