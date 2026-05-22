#include "BleComm.h"

#include <NimBLEDevice.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <cstring>
#include <queue>

#include "EspNowComm.h"
#include "../debug.h"
#include "../main.h"
#include "../screens/ScreenHandler.h"

bool showBlePollSerial = true;

namespace {

static const char* OSSM_BLE_DEVICE_NAME = "OSSM";
static const char* OSSM_BLE_SERVICE_UUID = "522b443a-4f53-534d-0001-420badbabe69";
static const char* OSSM_BLE_COMMAND_CHAR_UUID = "522b443a-4f53-534d-1000-420badbabe69";
static const char* OSSM_BLE_SPEED_KNOB_CHAR_UUID = "522b443a-4f53-534d-1010-420badbabe69";
static const char* OSSM_BLE_STATE_CHAR_UUID = "522b443a-4f53-534d-2000-420badbabe69";

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
static SemaphoreHandle_t g_bleMutex = nullptr;
static TaskHandle_t g_pollTask = nullptr;
static TaskHandle_t g_txTask = nullptr;
static SemaphoreHandle_t g_txSem = nullptr;
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
  float stroke = 0.0f;
  float sensation = 0.0f;
  float pattern = 0.0f;
};

static ConfirmedMachineState g_confirmedState;

enum ParamIdx { P_SPEED = 0, P_DEPTH = 1, P_STROKE = 2, P_SENSATION = 3, P_PATTERN = 4, P_COUNT = 5 };

// Forward declaration (definition is below)
static bool bleReadStateOnce();

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
  if (stateName.startsWith("menu")) return MachineMode::Menu;
  if (stateName.startsWith("homing")) return MachineMode::Homing;
  if (stateName.startsWith("strokeEngine")) return MachineMode::StrokeEngine;
  if (stateName.startsWith("simplePenetration")) return MachineMode::SimplePenetration;
  if (stateName.startsWith("streaming")) return MachineMode::Streaming;
  if (stateName.length() == 0) return MachineMode::Unknown;
  return MachineMode::Other;
}

static void updateCachedMachineState(const String& stateRaw) {
  String parsedStateName;
  if (extractJsonStringValue(stateRaw, "state", &parsedStateName)) {
    if (showBlePollSerial && parsedStateName != g_machineStateName) {
      Serial.printf("[BLE] State changed: %s -> %s\n",
                    g_machineStateName.length() ? g_machineStateName.c_str() : "<none>",
                    parsedStateName.c_str());
    }
    g_machineStateName = parsedStateName;
    g_machineMode = parseMachineMode(parsedStateName);
  }

  float parsedSpeed = extractValueAfterKey(stateRaw, "speed");
  float parsedStroke = extractValueAfterKey(stateRaw, "stroke");
  float parsedSensation = extractValueAfterKey(stateRaw, "sensation");
  float parsedDepth = extractValueAfterKey(stateRaw, "depth");
  float parsedPattern = extractValueAfterKey(stateRaw, "pattern");

  g_confirmedState.valid = true;
  g_confirmedState.raw = stateRaw;
  g_confirmedState.stateName = parsedStateName;
  g_confirmedState.mode = g_machineMode;
  if (parsedSpeed >= 0.0f) g_confirmedState.speed = parsedSpeed;
  if (parsedDepth >= 0.0f) g_confirmedState.depth = parsedDepth;
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

static bool bleWriteCommand(const String& cmd) {
  if (!g_cmd || !g_client || !g_client->isConnected()) return false;
  if (g_bleMutex) xSemaphoreTake(g_bleMutex, portMAX_DELAY);
  bool ok = g_cmd->writeValue((uint8_t*)cmd.c_str(), cmd.length(), false);
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

static bool waitForStrokeEngineReady(uint32_t timeoutMs) {
  uint32_t start = millis();
  while ((millis() - start) < timeoutMs) {
    bleReadStateOnce();
    if (hasFreshState() && g_machineMode == MachineMode::StrokeEngine) return true;
    vTaskDelay(pdMS_TO_TICKS(80));
  }
  return false;
}

static bool ensureStrokeEngineReady() {
  if (!bleCommTryConnect()) return false;

  bleReadStateOnce();
  if (hasFreshState() && g_machineMode == MachineMode::StrokeEngine) return true;

  // Mandatory gating: if state is stale, force a refresh loop before deciding transitions.
  if (!hasFreshState()) {
    uint32_t start = millis();
    while ((millis() - start) < 2000 && !hasFreshState()) {
      bleReadStateOnce();
      vTaskDelay(pdMS_TO_TICKS(60));
    }
  }
  if (!hasFreshState()) return false;

  // Optional safety behavior from settings: force a menu transition (homing path)
  // before entering stroke engine.
  if (ble_force_homeing && g_machineMode != MachineMode::Menu && g_machineMode != MachineMode::Homing) {
    bleSendCommandWithResponse("go:menu");
    uint32_t menuStart = millis();
    while ((millis() - menuStart) < 2000) {
      bleReadStateOnce();
      if (hasFreshState() && (g_machineMode == MachineMode::Menu || g_machineMode == MachineMode::Homing)) break;
      vTaskDelay(pdMS_TO_TICKS(60));
    }
  }

  // If currently homing, wait a bit for completion before forcing a mode change.
  if (g_machineMode == MachineMode::Homing) {
    if (waitForStrokeEngineReady(6000)) return true;
  }

  // OSSM safety model: movement commands are ignored in menu state.
  if (!bleSendCommandWithResponse("go:strokeEngine")) {
    // Retry once in case we got a transient state-dependent fail.
    vTaskDelay(pdMS_TO_TICKS(120));
    if (!bleSendCommandWithResponse("go:strokeEngine")) return false;
  }

  return waitForStrokeEngineReady(10000);
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

  String type;
  bool dedupeType = isRealtimeSetCommand(cmd, &type);

  xSemaphoreTake(g_bleMutex, portMAX_DELAY);
  if (dedupeType) {
    std::queue<TxQueueItem> temp;
    while (!g_txQueue.empty()) {
      TxQueueItem existing = g_txQueue.front();
      g_txQueue.pop();
      String existingType;
      if (isRealtimeSetCommand(existing.cmd, &existingType) && existingType == type) {
        continue;
      }
      temp.push(existing);
    }
    g_txQueue = temp;
  }
  TxQueueItem item;
  item.cmd = cmd;
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

static void blePollTask(void*) {
  for (;;) {
    if (g_bleEnabled && bleCommIsConnected()) {
      if (bleReadStateOnce()) {
        ++g_pollReadOkCount;
      } else {
        ++g_pollReadFailCount;
      }
    }

    if (showBlePollSerial) {
      uint32_t now = millis();
      if ((now - g_lastPollInfoMs) >= 1000) {
        size_t qSize = 0;
        if (g_bleMutex) xSemaphoreTake(g_bleMutex, portMAX_DELAY);
        qSize = g_txQueue.size();
        if (g_bleMutex) xSemaphoreGive(g_bleMutex);

        uint32_t stateAgeMs = (g_lastStateUpdateMs == 0) ? 0 : (now - g_lastStateUpdateMs);
        Serial.printf(
          "[BLE] poll mode=%d state=%s fresh=%d age=%lums connected=%d queue=%u ok=%lu fail=%lu speed=%.1f depth=%.1f stroke=%.1f sensation=%.1f pattern=%d running=%d maxDepth=%.1f maxSpeed=%.1f\n",
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
          pattern,
          OSSM_On ? 1 : 0,
          maxdepthinmm,
          speedlimit);
        g_lastPollInfoMs = now;
      }
    }

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

void bleCommInit() {
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

  NimBLEScan* scanner = NimBLEDevice::getScan();
  if (!scanner) return false;

  scanner->setActiveScan(true);
  scanner->setInterval(160);
  scanner->setWindow(160);

  NimBLEScanResults results = scanner->getResults(5000, false);
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
  if (targetAddress.empty()) return false;

  if (!g_client) {
    g_client = NimBLEDevice::createClient();
    if (!g_client) return false;
    g_client->setConnectionParams(12, 12, 0, 150);
    g_client->setConnectTimeout(5000);
  }

  NimBLEAddress remoteAddress(targetAddress, 0);
  if (!g_client->connect(remoteAddress)) {
    g_cmd = nullptr;
    g_speedKnob = nullptr;
    g_state = nullptr;
    return false;
  }

  NimBLERemoteService* service = g_client->getService(NimBLEUUID(OSSM_BLE_SERVICE_UUID));
  if (!service) {
    g_client->disconnect();
    return false;
  }

  g_cmd = service->getCharacteristic(NimBLEUUID(OSSM_BLE_COMMAND_CHAR_UUID));
  g_speedKnob = service->getCharacteristic(NimBLEUUID(OSSM_BLE_SPEED_KNOB_CHAR_UUID));
  g_state = service->getCharacteristic(NimBLEUUID(OSSM_BLE_STATE_CHAR_UUID));

  if (!g_cmd || !g_cmd->canWrite()) {
    g_client->disconnect();
    g_cmd = nullptr;
    g_speedKnob = nullptr;
    g_state = nullptr;
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
  bleReadStateOnce();
  return true;
}

bool bleCommIsConnected() {
  return g_client && g_client->isConnected() && g_cmd;
}

void bleCommSetEnabled(bool enabled) {
  g_bleEnabled = enabled;
}

bool bleCommIsEnabled() {
  return g_bleEnabled;
}

bool bleCommSendAppCommand(int appCommand, float value, float currentSpeed,
                           float currentDepth, float currentStroke,
                           float maxDepthMm, float maxSpeedValue) {
  (void)currentDepth;
  (void)currentStroke;
  (void)maxDepthMm;
  (void)maxSpeedValue;

  if (!bleCommTryConnect()) return false;

  const bool isMotionControl =
      (appCommand == SPEED || appCommand == DEPTH || appCommand == STROKE ||
       appCommand == SENSATION || appCommand == PATTERN || appCommand == ON ||
       appCommand == OFF);

  if (isMotionControl) {
    // Keep fast path cheap: only run full readiness check when mode/state are not already known-good.
    if (!(hasFreshState() && g_machineMode == MachineMode::StrokeEngine)) {
      if (!ensureStrokeEngineReady()) return false;
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

    if (requestedStroke > 0.001f && prevStroke <= 0.001f) {
      float resume = bleGetUnpauseSpeed();
      if (resume > 0.001f) {
        bool ok1 = queueCommand(String("set:stroke:") + String(clampPercent(requestedStroke)), true);
        bool ok2 = queueCommand(String("set:speed:") + String(clampPercent(resume)), true);
        g_lastRequestedStroke = requestedStroke;
        return ok1 && ok2;
      }
    }

    g_lastRequestedStroke = requestedStroke;
  }

  String cmd;
  switch (appCommand) {
    case SPEED:
      cmd = String("set:speed:") + String(clampPercent(value));
      if (value > 0.5f) g_lastRunSpeed = value;
      break;
    case DEPTH:
      cmd = String("set:depth:") + String(clampPercent(value));
      break;
    case STROKE:
      cmd = String("set:stroke:") + String(clampPercent(value));
      break;
    case SENSATION:
      cmd = String("set:sensation:") + String(mapSensationToBlePercent(value));
      break;
    case PATTERN: {
      int idx = (int)(value + 0.5f);
      if (idx < 0) idx = 0;
      if (idx > 6) idx = 6;
      cmd = String("set:pattern:") + String(idx);
      break;
    }
    case OFF:
      cmd = "set:speed:0";
      break;
    case ON: {
      int resume = clampPercent(speedParam > 0.001f ? speedParam : currentSpeed);
      cmd = String("set:speed:") + String(resume);
      break;
    }
    default:
      return false;
  }

  const bool isRealtime = (appCommand == SPEED || appCommand == DEPTH || appCommand == STROKE ||
                           appCommand == SENSATION || appCommand == PATTERN);
  return queueCommand(cmd, !isRealtime);
}
