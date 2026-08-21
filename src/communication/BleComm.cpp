#include "BleComm.h"

#include <NimBLEDevice.h>
//#include <esp_heap_caps.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <cstring>
#include <queue>

#include "CommManager.h"
#include "../config/debug.h"
#include "../main.h"
#include "../screens/ScreenHandler.h"
#include "../ui/ui.h"


namespace {

static const char* OSSM_BLE_DEVICE_NAME = "OSSM";
static const char* OSSM_BLE_SERVICE_UUID = "522b443a-4f53-534d-0001-420badbabe69";
static const char* OSSM_BLE_COMMAND_CHAR_UUID = "522b443a-4f53-534d-1000-420badbabe69";
static const char* OSSM_BLE_SPEED_KNOB_CHAR_UUID = "522b443a-4f53-534d-1010-420badbabe69";
static const char* OSSM_BLE_STATE_CHAR_UUID = "522b443a-4f53-534d-2000-420badbabe69";
static const char* OSSM_BLE_PATTERNS_CHAR_UUID = "522b443a-4f53-534d-3000-420badbabe69";
static const char* OSSM_BLE_PATTERN_DATA_CHAR_UUID = "522b443a-4f53-534d-3010-420badbabe69";
static const char* ADVANCED_STATUS_CHAR_UUID = "4F53534D-6164-7661-6E63-656473746174";
static const char* ADVANCED_CONFIG_CHAR_UUID = "4F53534D-6164-7661-6E63-6564636F6E66";
static const char* ADVANCED_CONTROL_CHAR_UUID = "4F53534D-6164-7661-6E63-6564636F6E74";
static const char* ADVANCED_PRESETS_CHAR_UUID = "4F53534D-6164-7661-6E63-656470727374";

static constexpr uint32_t BLE_STATE_POLL_MS = 20;
static constexpr uint32_t STATE_FRESH_TIMEOUT_MS = 1200;

enum class MachineMode {
  Unknown,
  Menu,
  Homing,
  StrokeEngine,
  SimplePenetration,
  Streaming,
  Other,
};

static bool g_bleInit = false;
static bool g_bleEnabled = false;
static NimBLEClient* g_client = nullptr;
static NimBLERemoteCharacteristic* g_cmd = nullptr;
static NimBLERemoteCharacteristic* g_speedKnob = nullptr;
static NimBLERemoteCharacteristic* g_state = nullptr;
static NimBLERemoteCharacteristic* g_patterns = nullptr;
static NimBLERemoteCharacteristic* g_patternData = nullptr;
static NimBLERemoteCharacteristic* g_adv_status = nullptr;
static NimBLERemoteCharacteristic* g_adv_config = nullptr;
static NimBLERemoteCharacteristic* g_adv_control = nullptr;
static NimBLERemoteCharacteristic* g_adv_presets = nullptr;
static SemaphoreHandle_t g_bleMutex = nullptr;
static TaskHandle_t g_pollTask = nullptr;
static TaskHandle_t g_txTask = nullptr;
static SemaphoreHandle_t g_txSem = nullptr;
static SemaphoreHandle_t g_connectMutex = nullptr;
static uint32_t g_lastConnectAttemptMs = 0;
static constexpr uint32_t BLE_CONNECT_COOLDOWN_MS = 1200;
struct TxQueueItem {
  String cmd;
  bool requireConfirm;
  bool isRealtime;
};
static std::queue<TxQueueItem> g_txQueue;
static float g_lastRunSpeed = 0.0f;
static MachineMode g_machineMode = MachineMode::Unknown;
static String g_machineStateName;
static uint32_t g_lastStateUpdateMs = 0;
static uint32_t g_lastPollInfoMs = 0;
static uint32_t g_lastRawStateLogMs = 0;
static uint32_t g_pollReadOkCount = 0;
static uint32_t g_pollReadFailCount = 0;
static float g_unpauseSpeed = 0.0f;
static float g_lastRequestedStroke = -1.0f;

struct ConfirmedMachineState {
  bool valid = false;
  String raw;
  String stateName;
  MachineMode mode = MachineMode::Unknown;
  float speed = 0.0f;
  float depth = 0.0f;
  float position = -1.0f;
  float stroke = 0.0f;
  float sensation = 0.0f;
  float pattern = 0.0f;
};

static ConfirmedMachineState g_confirmedState;

enum ParamIdx { P_SPEED = 0, P_DEPTH = 1, P_STROKE = 2, P_SENSATION = 3, P_PATTERN = 4, P_COUNT = 5 };

// Forward declaration (definition is below)
static bool bleReadStateOnce();
static bool ensureAdvancedCharacteristics(bool needConfig, bool needStatus, bool needControl, bool needPresets);

// -------------------------------------------------------
// Internal capability / parsing helpers
// -------------------------------------------------------
static bool canAdvancedRead(const NimBLERemoteCharacteristic* characteristic) {
  return characteristic && characteristic->canRead();
}

static bool canAdvancedWrite(const NimBLERemoteCharacteristic* characteristic) {
  return characteristic && (characteristic->canWrite() || characteristic->canWriteNoResponse());
}

static float extractJsonNumberValue(const String& text, const String& key) {
  String pattern = String("\"") + key + String("\"");
  int keyPos = text.indexOf(pattern);
  if (keyPos < 0) return -1.0f;

  int colonPos = text.indexOf(':', keyPos + pattern.length());
  if (colonPos < 0) return -1.0f;

  int pos = colonPos + 1;
  while (pos < text.length()) {
    char c = text.charAt(pos);
    if ((c >= '0' && c <= '9') || c == '-' || c == '.') break;
    ++pos;
  }
  if (pos >= text.length()) return -1.0f;

  int end = pos;
  while (end < text.length()) {
    char c = text.charAt(end);
    if (!((c >= '0' && c <= '9') || c == '.' || c == '-')) break;
    ++end;
  }
  if (end <= pos) return -1.0f;

  return text.substring(pos, end).toFloat();
}

static void pumpUiDuringModeWait() {
  // Keep status icons and screen rendering responsive while mode gating loops.
  screenForceStatusStripRefreshNow();
  lv_task_handler();
}

static int extractPatternOptionsFromJson(const String& json, String* outOptions) {
  if (!outOptions) return 0;
  outOptions->remove(0);

  int count = 0;
  int searchPos = 0;
  while (searchPos < json.length()) {
    int nameKeyPos = json.indexOf("\"name\"", searchPos);
    if (nameKeyPos < 0) break;

    int colonPos = json.indexOf(':', nameKeyPos + 6);
    if (colonPos < 0) break;

    int startQuote = json.indexOf('"', colonPos + 1);
    if (startQuote < 0) break;

    int endQuote = json.indexOf('"', startQuote + 1);
    if (endQuote < 0) break;

    String name = json.substring(startQuote + 1, endQuote);
    name.trim();
    name.replace("\r", " ");
    name.replace("\n", " ");

    if (name.length() > 0) {
      if (outOptions->length() > 0) outOptions->concat("\n");
      outOptions->concat(name);
      ++count;
    }

    searchPos = endQuote + 1;
  }

  return count;
}

// -------------------------------------------------------
// Internal BLE client/characteristic lifecycle
// -------------------------------------------------------
static void bleResetClient() {
  g_cmd = nullptr;
  g_speedKnob = nullptr;
  g_state = nullptr;
  g_patterns = nullptr;
  g_patternData = nullptr;
  g_adv_status = nullptr;
  g_adv_config = nullptr;
  g_adv_control = nullptr;
  g_adv_presets = nullptr;

  if (!g_client) return;

  if (g_client->isConnected()) {
    g_client->disconnect();
  }
  NimBLEDevice::deleteClient(g_client);
  g_client = nullptr;
}

static bool ensureAdvancedCharacteristics(bool needConfig, bool needStatus, bool needControl, bool needPresets) {
  if (!g_client || !g_client->isConnected()) return false;
  const bool alreadyReady =
      (!needConfig || canAdvancedRead(g_adv_config)) &&
      (!needStatus || canAdvancedRead(g_adv_status)) &&
      (!needControl || canAdvancedWrite(g_adv_control)) &&
      (!needPresets || (canAdvancedRead(g_adv_presets) || canAdvancedWrite(g_adv_presets)));
  if (alreadyReady) return true;

  // Runtime service refresh (getServices(true)) is unsafe while poll/read tasks are active,
  // so AP readiness is evaluated against pointers discovered during connect.
  const bool ready =
      (!needConfig || canAdvancedRead(g_adv_config)) &&
      (!needStatus || canAdvancedRead(g_adv_status)) &&
      (!needControl || canAdvancedWrite(g_adv_control)) &&
      (!needPresets || (canAdvancedRead(g_adv_presets) || canAdvancedWrite(g_adv_presets)));
  if (!ready) {
    static uint32_t s_lastAdvDiagLogMs = 0;
    const uint32_t nowMs = millis();
    if ((nowMs - s_lastAdvDiagLogMs) > 5000U) {
      LogDebugFormatted(
          "[AP] Advanced characteristics missing/unsupported cfg=%d st=%d ctl=%d pst=%d\n",
          canAdvancedRead(g_adv_config) ? 1 : 0,
          canAdvancedRead(g_adv_status) ? 1 : 0,
          canAdvancedWrite(g_adv_control) ? 1 : 0,
          (canAdvancedRead(g_adv_presets) || canAdvancedWrite(g_adv_presets)) ? 1 : 0);
      s_lastAdvDiagLogMs = nowMs;
    }
  }

  return ready;
}

static float extractValueAfterKey(const String& text, const String& key) {
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
  return text.substring(pos, end).toFloat();
}

static bool extractJsonStringValue(const String& text, const String& key, String* outValue) {
  if (!outValue) return false;
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

static int clampPercent(float value) {
  if (value < 0.0f) return 0;
  if (value > 100.0f) return 100;
  return (int)(value + 0.5f);
}

static int mapSensationToBlePercent(float value) {
  float normalized = value / 100.0f;
  if (normalized < -1.0f) normalized = -1.0f;
  if (normalized > 1.0f) normalized = 1.0f;
  float percent = (normalized + 1.0f) * 50.0f;
  return clampPercent(percent);
}

static void bleStoreUnpauseSpeed(float speedValue) {
  g_unpauseSpeed = (float)clampPercent(speedValue);
}

static float bleGetUnpauseSpeed() {
  return g_unpauseSpeed;
}

static MachineMode parseMachineMode(const String& stateName) {
  String normalized = stateName;
  normalized.toLowerCase();

  if (normalized.startsWith("menu")) return MachineMode::Menu;
  if (normalized.startsWith("homing")) return MachineMode::Homing;
  if (normalized.startsWith("strokeengine")) return MachineMode::StrokeEngine;
  if (normalized.startsWith("simplepenetration")) return MachineMode::SimplePenetration;
  if (normalized.startsWith("streaming")) return MachineMode::Streaming;

  // Rust OSSM BLE state names report run-ready as ready/playing/paused.
  if (normalized.startsWith("ready") || normalized.startsWith("playing") || normalized.startsWith("paused")) {
    return MachineMode::StrokeEngine;
  }

  if (normalized.length() == 0) return MachineMode::Unknown;
  return MachineMode::Other;
}

static void updateCachedMachineState(const String& stateRaw) {
  String parsedStateName;
  if (extractJsonStringValue(stateRaw, "state", &parsedStateName)) {
    #ifdef SHOWBLEPOLL
    if (parsedStateName != g_machineStateName) {  //if (showBlePollSerial && .....
      Serial.printf("[BLE] State changed: %s -> %s\n",
                    g_machineStateName.length() ? g_machineStateName.c_str() : "<none>",
                    parsedStateName.c_str());
    }
    #endif
    g_machineStateName = parsedStateName;
    g_machineMode = parseMachineMode(parsedStateName);
  }

  float parsedSpeed = extractJsonNumberValue(stateRaw, "speed");
  float parsedStroke = extractJsonNumberValue(stateRaw, "stroke");
  float parsedSensation = extractJsonNumberValue(stateRaw, "sensation");
  float parsedDepth = extractJsonNumberValue(stateRaw, "depth");
  float parsedPattern = extractJsonNumberValue(stateRaw, "pattern");
  float parsedPosition = extractJsonNumberValue(stateRaw, "position");

  g_confirmedState.valid = true;
  g_confirmedState.raw = stateRaw;
  g_confirmedState.stateName = parsedStateName;
  g_confirmedState.mode = g_machineMode;
  if (parsedSpeed >= 0.0f) g_confirmedState.speed = parsedSpeed;
  if (parsedDepth >= 0.0f) g_confirmedState.depth = parsedDepth;
  if (parsedPosition >= 0.0f) g_confirmedState.position = parsedPosition;
  if (parsedStroke >= 0.0f) g_confirmedState.stroke = parsedStroke;
  if (parsedSensation >= 0.0f) g_confirmedState.sensation = parsedSensation;
  if (parsedPattern >= 0.0f) g_confirmedState.pattern = parsedPattern;

  if (parsedSpeed >= 0.0f) {
    OSSM_On = parsedSpeed > 0.5f;
  }

  // OSSM BLE control values are percentages (0..100).
  maxdepthinmm = 100.0f;
  speedlimit = 100.0f;
  g_lastStateUpdateMs = millis();
}

static bool hasFreshState() {
  if (g_lastStateUpdateMs == 0) return false;
  return (millis() - g_lastStateUpdateMs) <= STATE_FRESH_TIMEOUT_MS;
}

static void stateNotify(NimBLERemoteCharacteristic*, uint8_t* data, size_t len, bool) {
  if (!data || len == 0) return;
  String s((const char*)data, len);
  updateCachedMachineState(s);
}

// -------------------------------------------------------
// Internal BLE read/write helpers
// -------------------------------------------------------
static bool bleWriteCommand(const String& cmd) {
  if (!g_cmd || !g_client || !g_client->isConnected()) return false;
  if (g_bleMutex) xSemaphoreTake(g_bleMutex, portMAX_DELAY);
  bool ok = g_cmd->writeValue((uint8_t*)cmd.c_str(), cmd.length(), false);
  if (g_bleMutex) xSemaphoreGive(g_bleMutex);
  return ok;
}

static bool bleReadRawCharacteristic(NimBLERemoteCharacteristic* characteristic, String* outValue) {
  if (!outValue || !characteristic || !g_client || !g_client->isConnected() || !characteristic->canRead()) {
    return false;
  }

  if (g_bleMutex) xSemaphoreTake(g_bleMutex, portMAX_DELAY);
  std::string raw = characteristic->readValue();
  if (g_bleMutex) xSemaphoreGive(g_bleMutex);

  *outValue = String(raw.c_str());
  return true;
}

static void logAdvancedSnapshotOnConnect() {
  String payload;

  auto logReadableChar = [&](const char* tag, NimBLERemoteCharacteristic* characteristic) {
    if (canAdvancedRead(characteristic) && bleReadRawCharacteristic(characteristic, &payload) && payload.length() > 0) {
      Serial.printf("%s %s\n", tag, payload.c_str());
    } else {
      Serial.printf("%s <unavailable>\n", tag);
    }
  };

  logReadableChar("[BLE][CHAR_STATE]", g_state);
  logReadableChar("[BLE][CHAR_CMD]", g_cmd);
  logReadableChar("[BLE][CHAR_SPEED_KNOB]", g_speedKnob);

  logReadableChar("[BLE][ADV_CONFIG]", g_adv_config);
  logReadableChar("[BLE][ADV_STATUS]", g_adv_status);
}

static bool bleWriteRawCharacteristic(NimBLERemoteCharacteristic* characteristic, const String& payload) {
  if (!characteristic || !g_client || !g_client->isConnected() || !characteristic->canWrite()) {
    if (!characteristic || !g_client || !g_client->isConnected()) return false;
    const bool supportsWrite = characteristic->canWrite();
    const bool supportsWriteNoResp = characteristic->canWriteNoResponse();
    if (!supportsWrite && !supportsWriteNoResp) return false;
  }

  if (g_bleMutex) xSemaphoreTake(g_bleMutex, portMAX_DELAY);
  bool ok = false;
  if (characteristic->canWriteNoResponse()) {
    ok = characteristic->writeValue((uint8_t*)payload.c_str(), payload.length(), false);
  }
  if (!ok && characteristic->canWrite()) {
    ok = characteristic->writeValue((uint8_t*)payload.c_str(), payload.length(), true);
  }
  if (g_bleMutex) xSemaphoreGive(g_bleMutex);
  return ok;
}

static bool bleReadCommandResponse(String* out) {
  if (out) out->remove(0);
  if (!g_cmd || !g_client || !g_client->isConnected() || !g_cmd->canRead()) return true;

  if (g_bleMutex) xSemaphoreTake(g_bleMutex, portMAX_DELAY);
  std::string raw = g_cmd->readValue();
  if (g_bleMutex) xSemaphoreGive(g_bleMutex);

  if (raw.empty()) return true;

  String response(raw.c_str());
  if (out) *out = response;

  if (response.startsWith("{")) {
    updateCachedMachineState(response);
    return true;
  }
  if (response.startsWith("fail:")) {
    LogDebugFormatted("BLE command failed: %s\n", response.c_str());
    return false;
  }
  return true;
}

static bool bleSendCommandWithResponse(const String& cmd) {
  if (!bleWriteCommand(cmd)) return false;
  String response;
  return bleReadCommandResponse(&response);
}

// -------------------------------------------------------
// Internal mode-readiness / command queueing
// -------------------------------------------------------
static bool waitForStrokeEngineOrStreamingReady(uint32_t timeoutMs) {
  uint32_t start = millis();
  while ((millis() - start) < timeoutMs) {
    bleReadStateOnce();
    pumpUiDuringModeWait();
    if (hasFreshState() && g_machineMode == MachineMode::StrokeEngine) return true;
    vTaskDelay(pdMS_TO_TICKS(80));
  }
  return false;
}

static bool ensureStrokeEngineOrStreamingReady() {
  if (!bleCommTryConnect()) return false;

  bleReadStateOnce();
  pumpUiDuringModeWait();
  if (hasFreshState() && g_machineMode == MachineMode::StrokeEngine) return true;
  //also if in streaming mode in fresh state, return ok, since we need to be able to alter max
  if (hasFreshState() && g_machineMode == MachineMode::Streaming) return true;

  // Mandatory gating: if state is stale, force a refresh loop before deciding transitions.
  if (!hasFreshState()) {
    uint32_t start = millis();
    while ((millis() - start) < 2000 && !hasFreshState()) {
      bleReadStateOnce();
      pumpUiDuringModeWait();
      vTaskDelay(pdMS_TO_TICKS(60));
    }
  }
  if (!hasFreshState()) return false;

  // Optional safety behavior from settings: force a menu transition (homing path)
  // before entering stroke engine.
  if (ble_force_homeing && g_machineMode != MachineMode::Menu && g_machineMode != MachineMode::Homing) {
    bleSendCommandWithResponse("go:menu");
    pumpUiDuringModeWait();
    uint32_t menuStart = millis();
    while ((millis() - menuStart) < 2000) {
      bleReadStateOnce();
      pumpUiDuringModeWait();
      if (hasFreshState() && (g_machineMode == MachineMode::Menu || g_machineMode == MachineMode::Homing)) break;
      vTaskDelay(pdMS_TO_TICKS(60));
    }
  }

  // If currently homing, wait a bit for completion before forcing a mode change.
  if (g_machineMode == MachineMode::Homing) {
    if (waitForStrokeEngineOrStreamingReady(6000)) return true;
  }

  // OSSM safety model: movement commands are ignored in menu state.
  if (!bleSendCommandWithResponse("go:strokeEngine")) {
    // Retry once in case we got a transient state-dependent fail.
    pumpUiDuringModeWait();
    vTaskDelay(pdMS_TO_TICKS(120));
    if (!bleSendCommandWithResponse("go:strokeEngine")) return false;
  }

  pumpUiDuringModeWait();

  return waitForStrokeEngineOrStreamingReady(10000);
}

static bool isRealtimeSetCommand(const String& cmd, String* outType = nullptr) {
  if (!cmd.startsWith("set:")) return false;
  int colon = cmd.indexOf(':', 4);
  if (colon <= 4) return false;
  String type = cmd.substring(4, colon);
  if (!(type == "speed" || type == "depth" || type == "stroke" || type == "sensation" || type == "pattern")) {
    return false;
  }
  if (outType) *outType = type;
  return true;
}

static bool queueCommand(const String& cmd, bool requireConfirm = true) {
  if (cmd.length() == 0) return false;
  if (!g_bleMutex) return false;

  auto parseValidatedSetCommand = [](const String& input, String* outType) -> bool {
    if (!outType) return false;
    *outType = "";

    String s = input;
    s.trim();
    if (!s.startsWith("set:")) return false;

    int colon1 = s.indexOf(':');
    int colon2 = s.indexOf(':', colon1 + 1);
    if (colon1 < 0 || colon2 < 0) return false;
    if (s.indexOf(':', colon2 + 1) >= 0) return false;

    String type = s.substring(colon1 + 1, colon2);
    if (!(type == "depth" || type == "sensation" || type == "pattern" ||
          type == "speed" || type == "stroke")) {
      return false;
    }

    String valuePart = s.substring(colon2 + 1);
    int valueLen = valuePart.length();
    if (valueLen < 1 || valueLen > 3) return false;

    for (int i = 0; i < valueLen; ++i) {
      char c = valuePart.charAt(i);
      if (c < '0' || c > '9') return false;
    }

    int value = valuePart.toInt();
    if (value < 0 || value > 100) return false;

    *outType = type;
    return true;
  };

  String normalized = cmd;
  normalized.trim();

  String type;
  bool dedupeType = false;
  if (normalized.startsWith("set:")) {
    if (!parseValidatedSetCommand(normalized, &type)) {
      LogDebugFormatted("Invalid set command format: %s\n", normalized.c_str());
      return false;
    }
    dedupeType = true;
  }

  xSemaphoreTake(g_bleMutex, portMAX_DELAY);
  if (dedupeType) {
    std::queue<TxQueueItem> temp;
    while (!g_txQueue.empty()) {
      TxQueueItem existing = g_txQueue.front();
      g_txQueue.pop();
      String existingType;
      if (parseValidatedSetCommand(existing.cmd, &existingType) && existingType == type) {
        continue;
      }
      temp.push(existing);
    }
    g_txQueue = temp;
  }
  TxQueueItem item;
  item.cmd = normalized;
  item.requireConfirm = requireConfirm;
  item.isRealtime = dedupeType;
  g_txQueue.push(item);
  xSemaphoreGive(g_bleMutex);

  if (g_txSem) xSemaphoreGive(g_txSem);
  return true;
}

static bool bleReadStateOnce() {
  if (!g_state || !g_client || !g_client->isConnected() || !g_state->canRead()) return false;
  if (g_bleMutex) xSemaphoreTake(g_bleMutex, portMAX_DELAY);
  std::string raw = g_state->readValue();
  if (g_bleMutex) xSemaphoreGive(g_bleMutex);
  if (raw.empty()) return false;
  updateCachedMachineState(String(raw.c_str()));
  return true;
}

static bool waitForConfirmedStateUpdate(uint32_t timeoutMs, uint32_t previousUpdateMs) {
  uint32_t startMs = millis();
  while ((millis() - startMs) < timeoutMs) {
    if (g_lastStateUpdateMs != 0 && g_lastStateUpdateMs != previousUpdateMs) return true;
    if (bleReadStateOnce() && g_lastStateUpdateMs != 0 && g_lastStateUpdateMs != previousUpdateMs) return true;
    vTaskDelay(pdMS_TO_TICKS(40));
  }
  return false;
}

// -------------------------------------------------------
// Internal background tasks
// -------------------------------------------------------
static void blePollTask(void*) {
  for (;;) {
    if (g_bleEnabled && bleCommIsConnected()) {
      if (bleReadStateOnce()) {
        ++g_pollReadOkCount;
      } else {
        ++g_pollReadFailCount;
      }
    }

    /*{
      const uint32_t now = millis();
      if ((now - g_lastRawStateLogMs) >= 1000U) {
        if (g_confirmedState.raw.length() > 0) {
          Serial.printf("[BLE][RAW] %s\n", g_confirmedState.raw.c_str());
        } else {
          Serial.println("[BLE][RAW] <empty>");
        }
        g_lastRawStateLogMs = now;
      }
    }*/

    //if (showBlePollSerial) {
    #ifdef SHOWBLEPOLL
      uint32_t now = millis();
      if ((now - g_lastPollInfoMs) >= 1000) {
        size_t qSize = 0;
        if (g_bleMutex) xSemaphoreTake(g_bleMutex, portMAX_DELAY);
        qSize = g_txQueue.size();
        if (g_bleMutex) xSemaphoreGive(g_bleMutex);

        uint32_t stateAgeMs = (g_lastStateUpdateMs == 0) ? 0 : (now - g_lastStateUpdateMs);
        const float confirmedPosMm = g_confirmedState.position;
        Serial.printf(
          "[BLE] poll mode=%d state=%s fresh=%d age=%lums connected=%d queue=%u ok=%lu fail=%lu speed=%.1f depth=%.1f stroke=%.1f sensation=%.1f posMm=%.1f pattern=%d running=%d maxDepth=%.1f maxSpeed=%.1f\n",
          (int)g_machineMode,
          g_machineStateName.length() ? g_machineStateName.c_str() : "<none>",
          hasFreshState() ? 1 : 0,
          (unsigned long)stateAgeMs,
          bleCommIsConnected() ? 1 : 0,
          (unsigned)qSize,
          (unsigned long)g_pollReadOkCount,
          (unsigned long)g_pollReadFailCount,
          speed,
          depth,
          stroke,
          sensation,
          confirmedPosMm,
          pattern,
          OSSM_On ? 1 : 0,
          maxdepthinmm,
          speedlimit);
        g_lastPollInfoMs = now;
      }
    #endif

    vTaskDelay(pdMS_TO_TICKS(BLE_STATE_POLL_MS));
  }
}

static void bleTxTask(void*) {
  uint32_t lastRealtimeTxMs = 0;
  for (;;) {
    if (!g_txSem || xSemaphoreTake(g_txSem, portMAX_DELAY) != pdTRUE) continue;

    for (;;) {
      TxQueueItem item;
      bool hasWork = false;

      if (g_bleMutex) xSemaphoreTake(g_bleMutex, portMAX_DELAY);
      if (!g_txQueue.empty()) {
        item = g_txQueue.front();
        g_txQueue.pop();
        hasWork = true;
      }
      if (g_bleMutex) xSemaphoreGive(g_bleMutex);

      if (!hasWork) break;

      if (!bleCommTryConnect()) {
        // Push command back for retry on next wake-up.
        if (g_bleMutex) xSemaphoreTake(g_bleMutex, portMAX_DELAY);
        g_txQueue.push(item);
        if (g_bleMutex) xSemaphoreGive(g_bleMutex);
        vTaskDelay(pdMS_TO_TICKS(120));
        break;
      }

      if (item.isRealtime) {
        uint32_t nowMs = millis();
        uint32_t elapsedMs = nowMs - lastRealtimeTxMs;
        if (elapsedMs < 30) {
          vTaskDelay(pdMS_TO_TICKS(30 - elapsedMs));
        }
        lastRealtimeTxMs = millis();
      }

      uint32_t previousStateUpdateMs = g_lastStateUpdateMs;
      bleWriteCommand(item.cmd);
      if (item.requireConfirm) {
        String response;
        bleReadCommandResponse(&response);
        if (response.length() == 0 || !response.startsWith("{")) {
          waitForConfirmedStateUpdate(700, previousStateUpdateMs);
        }
      }
      vTaskDelay(pdMS_TO_TICKS(8));
    }
  }
}

}  // namespace

// -------------------------------------------------------
// Public BLE bridge state
// -------------------------------------------------------
bool newPatternIsReadFromOSSM = false;
String patternString;

// -------------------------------------------------------
// Public BLE lifecycle
// -------------------------------------------------------
void bleCommInit() {
  newPatternIsReadFromOSSM = false;

  if (!g_bleInit) {
    if (!NimBLEDevice::isInitialized()) {
      NimBLEDevice::init("M5-OSSM-Remote");
    }
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);
    g_bleInit = true;
  }
  if (!g_bleMutex) {
    g_bleMutex = xSemaphoreCreateMutex();
  }
  if (!g_txSem) {
    g_txSem = xSemaphoreCreateBinary();
  }
  if (!g_connectMutex) {
    g_connectMutex = xSemaphoreCreateMutex();
  }
  if (!g_pollTask) {
#if CONFIG_FREERTOS_UNICORE
    xTaskCreatePinnedToCore(blePollTask, "blePollTask", 6144, nullptr, 2, &g_pollTask, 0);
#else
    xTaskCreatePinnedToCore(blePollTask, "blePollTask", 6144, nullptr, 2, &g_pollTask, 1);
#endif
  }
  if (!g_txTask) {
#if CONFIG_FREERTOS_UNICORE
    xTaskCreatePinnedToCore(bleTxTask, "bleTxTask", 6144, nullptr, 3, &g_txTask, 0);
#else
    xTaskCreatePinnedToCore(bleTxTask, "bleTxTask", 6144, nullptr, 3, &g_txTask, 1);
#endif
  }
}

bool bleCommTryConnect() {
  if (bleCommIsConnected()) return true;
  bleCommInit();

  if (!g_connectMutex) return false;
  if (xSemaphoreTake(g_connectMutex, pdMS_TO_TICKS(250)) != pdTRUE) return false;

  if (bleCommIsConnected()) {
    xSemaphoreGive(g_connectMutex);
    return true;
  }

  const uint32_t nowMs = millis();
  if ((nowMs - g_lastConnectAttemptMs) < BLE_CONNECT_COOLDOWN_MS) {
    xSemaphoreGive(g_connectMutex);
    return false;
  }
  g_lastConnectAttemptMs = nowMs;

  NimBLEScan* scanner = NimBLEDevice::getScan();
  if (!scanner) {
    xSemaphoreGive(g_connectMutex);
    return false;
  }

  scanner->stop();
  scanner->clearResults();
  scanner->setActiveScan(true);
  scanner->setInterval(160);
  scanner->setWindow(160);

  NimBLEScanResults results = scanner->getResults(2500, false);
  std::string targetAddress;
  NimBLEUUID serviceUuid(OSSM_BLE_SERVICE_UUID);

  for (int i = 0; i < results.getCount(); ++i) {
    const NimBLEAdvertisedDevice* d = results.getDevice(i);
    if (!d) continue;
    bool nameMatch = d->haveName() && (d->getName() == OSSM_BLE_DEVICE_NAME || d->getName() == "OSSM-REMOTE");
    bool serviceMatch = d->haveServiceUUID() && d->isAdvertisingService(serviceUuid);
    if (nameMatch || serviceMatch) {
      targetAddress = d->getAddress().toString();
      break;
    }
  }

  scanner->clearResults();
  if (targetAddress.empty()) {
    xSemaphoreGive(g_connectMutex);
    return false;
  }

  if (!g_client) {
    g_client = NimBLEDevice::createClient();
    if (!g_client) {
      xSemaphoreGive(g_connectMutex);
      return false;
    }
    g_client->setConnectionParams(12, 12, 0, 150);
    g_client->setConnectTimeout(5000);
  }

  NimBLEAddress remoteAddress(targetAddress, 0);
  if (!g_client->connect(remoteAddress)) {
    bleResetClient();
    xSemaphoreGive(g_connectMutex);
    return false;
  }

  const std::vector<NimBLERemoteService*>& services = g_client->getServices(true);
  if (services.empty()) {
    bleResetClient();
    xSemaphoreGive(g_connectMutex);
    return false;
  }

  g_cmd = nullptr;
  g_speedKnob = nullptr;
  g_state = nullptr;
  g_adv_status = nullptr;
  g_adv_config = nullptr;
  g_adv_control = nullptr;
  g_adv_presets = nullptr;
  g_patterns = nullptr;
  g_patternData = nullptr;

  for (NimBLERemoteService* service : services) {
    if (!service) continue;
    if (!g_cmd) g_cmd = service->getCharacteristic(NimBLEUUID(OSSM_BLE_COMMAND_CHAR_UUID));
    if (!g_speedKnob) g_speedKnob = service->getCharacteristic(NimBLEUUID(OSSM_BLE_SPEED_KNOB_CHAR_UUID));
    if (!g_state) g_state = service->getCharacteristic(NimBLEUUID(OSSM_BLE_STATE_CHAR_UUID));
    if (!g_patterns) g_patterns = service->getCharacteristic(NimBLEUUID(OSSM_BLE_PATTERNS_CHAR_UUID));
    if (!g_patternData) g_patternData = service->getCharacteristic(NimBLEUUID(OSSM_BLE_PATTERN_DATA_CHAR_UUID));

    if (!g_adv_status) g_adv_status = service->getCharacteristic(NimBLEUUID(ADVANCED_STATUS_CHAR_UUID));
    if (!g_adv_config) g_adv_config = service->getCharacteristic(NimBLEUUID(ADVANCED_CONFIG_CHAR_UUID));
    if (!g_adv_control) g_adv_control = service->getCharacteristic(NimBLEUUID(ADVANCED_CONTROL_CHAR_UUID));
    if (!g_adv_presets) g_adv_presets = service->getCharacteristic(NimBLEUUID(ADVANCED_PRESETS_CHAR_UUID));
  }

  if (!g_cmd || !g_cmd->canWrite()) {
    bleResetClient();
    xSemaphoreGive(g_connectMutex);
    return false;
  }

  if (g_speedKnob && g_speedKnob->canWrite()) {
    static const char* independentMode = "false";
    g_speedKnob->writeValue((uint8_t*)independentMode, strlen(independentMode), true);
  }

  if (g_state && g_state->canNotify()) {
    g_state->subscribe(true, stateNotify);
  }
  g_lastStateUpdateMs = 0;
  const bool stateOk = bleReadStateOnce();
  logAdvancedSnapshotOnConnect();
  xSemaphoreGive(g_connectMutex);
  return stateOk || bleCommIsConnected();
}

bool bleCommIsConnected() {
  return g_client && g_client->isConnected() && g_cmd;
}

// -------------------------------------------------------
// Public state / mode helpers
// -------------------------------------------------------
void bleCommSetEnabled(bool enabled) {
  g_bleEnabled = enabled;
  if (!enabled) {
    newPatternIsReadFromOSSM = false;
  }
}

bool bleCommIsEnabled() {
  return g_bleEnabled;
}

bool bleCommHasFreshState() {
  return hasFreshState();
}

bool bleCommIsHoming() {
  return hasFreshState() && g_machineMode == MachineMode::Homing;
}

int bleCommGetHomingDirection() {
  if (!hasFreshState() || g_machineMode != MachineMode::Homing) return 0;
  String s = g_machineStateName;
  s.toLowerCase();
  if (s.indexOf("forward") >= 0 || s.indexOf("up") >= 0) return 1;
  if (s.indexOf("backward") >= 0 || s.indexOf("down") >= 0) return -1;
  return 0;
}

float bleCommGetConfirmedPosition() {
  if (!g_confirmedState.valid) return -1.0f;
  if (!hasFreshState()) return -1.0f;
  return g_confirmedState.position;
}

int bleCommSetUnpauseSpeed(float speedValue) {
  bleStoreUnpauseSpeed(speedValue);
  return (int)bleGetUnpauseSpeed();
}

int bleCommGetUnpauseSpeed() {
  return (int)bleGetUnpauseSpeed();
}

// -------------------------------------------------------
// Public command bridge
// -------------------------------------------------------
bool bleCommSendAppCommand(int appCommand, float value, float currentSpeed,
                           float currentDepth, float currentStroke,
                           float maxDepthMm, float maxSpeedValue) {
  (void)currentDepth;
  (void)currentStroke;
  (void)maxDepthMm;
  (void)maxSpeedValue;

  if (!bleCommTryConnect()) return false;
  bool SafeStartStop   = true; //default to true, but will be set to false if the checkbox is unchecked in the UI

  const bool isMotionControl =
      (appCommand == SPEED || appCommand == DEPTH || appCommand == STROKE ||
       appCommand == SENSATION || appCommand == PATTERN || appCommand == ON ||
       appCommand == OFF);

  if (isMotionControl) {
    // Keep fast path cheap: only run full readiness check when mode/state are not already known-good.
    if (!(hasFreshState() && (g_machineMode == MachineMode::StrokeEngine || g_machineMode == MachineMode::Streaming))) {
      if (!ensureStrokeEngineOrStreamingReady()) return false;
    }
  }

  if (appCommand == SETUP_D_I) {
    return queueCommand("go:simplePenetration", true);
  }
  if (appCommand == SETUP_D_I_F) {
    return queueCommand("go:strokeEngine", true);
  }
  if (appCommand == CONN) {
    return bleCommTryConnect();
  }

  if (appCommand == SPEED && !OSSM_On) {
    bleStoreUnpauseSpeed(value);
    return true;
  }

  float speedParam = currentSpeed;
  if (appCommand == ON) {
    speedParam = (value > 0.001f) ? value : bleGetUnpauseSpeed();
  }

  if (appCommand == STROKE) {
    const float requestedStroke = value;
    float prevStroke = 0.0f;
    if (g_lastRequestedStroke >= 0.0f) {
      prevStroke = g_lastRequestedStroke;
    } else {
      prevStroke = currentStroke;
    }

    if (requestedStroke <= 0.001f && prevStroke > 0.001f) {
      if (currentSpeed > 0.001f) {
        bleStoreUnpauseSpeed(currentSpeed);
      }
      queueCommand("set:speed:0", true);
    }
/*  CHECK MARCO
//    if (requestedStroke > 0.001f && prevStroke <= 0.001f && OSSM_On) {
    if (requestedStroke > 0.001f && prevStroke <= 0.001f) {
      SafeStartStop = (lv_obj_has_state(ui_safeStartStop, LV_STATE_CHECKED) == 1);
      float resume = bleGetUnpauseSpeed();
      if (resume > 0.001f) {
        bool ok1 = queueCommand(String("set:stroke:") + String(clampPercent(requestedStroke)), true);
        if(!SafeStartStop) {
          g_lastRequestedStroke = requestedStroke;
          LogDebugFormatted("BLE: Resume speed %.1f after stroke %.1f\n", resume, requestedStroke); 
          for (int currentSpeed = HOME_START_RAMP_THRESHOLD + 1; currentSpeed <= resume; ++currentSpeed) {
            delay(HOME_START_RAMP_INTERVAL_MS);
            bool ok2 = queueCommand(String("set:speed:") + String(clampPercent(currentSpeed)), true);
            return ok1 && ok2;
          }

        }
        else {
          g_lastRequestedStroke = requestedStroke;
          LogDebugFormatted("BLE: Resume speed %.1f after stroke %.1f\n", resume, requestedStroke); // TEST AFTER EAU COMMENTS
          bool ok2 = queueCommand(String("set:speed:") + String(clampPercent(resume)), true);
          return ok1 && ok2;
        }


      }
    }


*/
  g_lastRequestedStroke = requestedStroke;
  }

  String cmd;
  switch (appCommand) {
    case SPEED:
      cmd = String("set:speed:") + String(clampPercent(value)) + "\n";
      LogDebugFormatted("BLE: Set speed command queued: %s\n", cmd.c_str());// TEST AFTER EAU COMMENTS

      if (value > 0.5f) g_lastRunSpeed = value;
      break;
    case DEPTH:
      cmd = String("set:depth:") + String(clampPercent(value)) + "\n";
      break;
    case STROKE:
      cmd = String("set:stroke:") + String(clampPercent(value)) + "\n";
      break;
    case SENSATION:
      cmd = String("set:sensation:") + String(mapSensationToBlePercent(value)) + "\n";
      break;
    case PATTERN: {
      int idx = (int)(value + 0.5f);
      if (idx < 0) idx = 0;
      cmd = String("set:pattern:") + String(idx) + "\n";
      break;
    }
    case OFF:
    LogDebugFormatted("BLE: OFF command queued, storing unpause speed %.1f\n", currentSpeed); // TEST AFTER EAU COMMENTS
    bleStoreUnpauseSpeed(currentSpeed);
      cmd = "set:speed:0\n";
      break;
    case ON: {
      
      int resume = clampPercent(speedParam > 0.001f ? speedParam : currentSpeed);
      cmd = String("set:speed:") + String(resume) + "\n";
      break;
    }
    default:
      return false;
  }

  const bool isRealtime = (appCommand == SPEED || appCommand == DEPTH || appCommand == STROKE ||
                           appCommand == SENSATION || appCommand == PATTERN);
  return queueCommand(cmd, !isRealtime);
}

bool bleCommIsMenu() {
  return hasFreshState() && g_machineMode == MachineMode::Menu;
}

bool bleCommGoToMenu() {
  if (!bleCommTryConnect()) return false;
  if (bleCommIsMenu()) return true;
  return queueCommand("go:menu", true);
}

bool bleCommGoToStreaming() {
  if (!bleCommTryConnect()) return false;
  return queueCommand("go:streaming", true);
}

bool bleCommGoToStrokeEngine() {
  return bleCommEnsureStrokeEngineOrStreamingReady();
}

bool bleCommEnsureStrokeEngineOrStreamingReady() {
  return ensureStrokeEngineOrStreamingReady();
}

String bleCommGetMachineStateName(bool lowerCase) {
  String s = g_machineStateName;
  if (lowerCase) s.toLowerCase();
  return s;
}

bool bleCommSendStreamCommand(int position, int durationMs) {
  if (!bleCommTryConnect()) return false;
  if (durationMs < 0) durationMs = 0;
  if (position < 0) position = 0;
  if (position > 100) position = 100;
  String cmd = String("stream:") + String(position) + String(":") + String(durationMs) + "\n";
  return queueCommand(cmd, true);
}

bool bleCommReadAdvancedConfig(String* outConfig) {
  if (!outConfig) return false;
  outConfig->remove(0);
  if (!bleCommTryConnect()) return false;
  if (!ensureAdvancedCharacteristics(true, false, false, false)) return false;
  return bleReadRawCharacteristic(g_adv_config, outConfig);
}

bool bleCommReadAdvancedStatus(String* outStatus) {
  if (!outStatus) return false;
  outStatus->remove(0);
  if (!bleCommTryConnect()) return false;
  if (!ensureAdvancedCharacteristics(false, true, false, false)) return false;
  return bleReadRawCharacteristic(g_adv_status, outStatus);
}

bool bleCommReadAdvancedPresets(String* outPresets) {
  if (!outPresets) return false;
  outPresets->remove(0);
  if (!bleCommTryConnect()) return false;
  if (!ensureAdvancedCharacteristics(false, false, false, true)) return false;
  return bleReadRawCharacteristic(g_adv_presets, outPresets);
}

bool bleCommWriteAdvancedControl(const String& payload) {
  if (payload.length() == 0) return false;
  if (!bleCommTryConnect()) return false;
  if (!ensureAdvancedCharacteristics(false, false, true, false)) return false;
  return bleWriteRawCharacteristic(g_adv_control, payload);
}

bool bleCommWriteAdvancedPresets(const String& payload) {
  if (payload.length() == 0) return false;
  if (!bleCommTryConnect()) return false;
  if (!ensureAdvancedCharacteristics(false, false, false, true)) return false;
  return bleWriteRawCharacteristic(g_adv_presets, payload);
}

// -------------------------------------------------------
// Public pattern catalog bridge
// -------------------------------------------------------
void bleCommResetPatternReadState() {
  newPatternIsReadFromOSSM = false;
}

bool readPatternsFromOSSM() {
  if (!bleCommTryConnect()) {
    newPatternIsReadFromOSSM = false;
    return false;
  }

  if (!g_patterns || !g_patterns->canRead()) {
    newPatternIsReadFromOSSM = false;
    return false;
  }

  if (g_bleMutex) xSemaphoreTake(g_bleMutex, portMAX_DELAY);
  std::string raw = g_patterns->readValue();
  if (g_bleMutex) xSemaphoreGive(g_bleMutex);

  if (raw.empty()) {
    newPatternIsReadFromOSSM = false;
    return false;
  }

  String rawJson(raw.c_str());
  String options;
  const int parsedCount = extractPatternOptionsFromJson(rawJson, &options);
  if (parsedCount <= 0 || options.length() == 0) {
    newPatternIsReadFromOSSM = false;
    return false;
  }

  patternString = options;
  newPatternIsReadFromOSSM = true;
  return true;
}
