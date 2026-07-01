#include "AP-mode.h"

#include <Arduino.h>
#include <M5Unified.h>
#include <esp_heap_caps.h>
#include <algorithm>
#include <cmath>
#include <string>
#include <unordered_map>
#include <vector>

#include "buttonhandlers/ButtonHandlers.h"
#include "communication/BleComm.h"
#include "config/debug.h"
#include "display/styles.h"
#include "language.h"
#include "screens/ScreenHandler.h"
#include "ui/ui.h"
#include "ui/ui_helpers.h"

extern "C" const int AP_ID = 4;

namespace {

struct Control {
    float value = 0.0f;
    uint8_t minValue = 0;
    uint8_t maxValue = 100;
};

static lv_obj_t *s_screen = nullptr;
static lv_obj_t *s_title_label = nullptr;
static lv_obj_t *s_state_label = nullptr;
static lv_obj_t *s_transport_label = nullptr;
static lv_obj_t *s_warning_label = nullptr;
static lv_obj_t *s_preset_label = nullptr;
static lv_obj_t *s_help_label = nullptr;
static lv_obj_t *s_tabs[6] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
static lv_obj_t *s_tab_labels[6] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
static lv_obj_t *s_graph_panel = nullptr;
static lv_obj_t *s_curve_main_pts[48] = {};
static lv_obj_t *s_curve_mod_pts[24] = {};
static constexpr int k_modifier_trace_count = 6;
static constexpr int k_modifier_trace_segments = 64;
static lv_obj_t *s_modifier_trace_lines[k_modifier_trace_count][k_modifier_trace_segments] = {};
static lv_point_precise_t s_modifier_trace_line_pts[k_modifier_trace_count][k_modifier_trace_segments][2] = {};
static int s_modifier_trace_allocated_segments[k_modifier_trace_count] = {0, 0, 0, 0, 0, 0};
static lv_obj_t *s_modifier_canvas = nullptr;
static uint8_t *s_modifier_canvas_buffer = nullptr;
static size_t s_modifier_canvas_buffer_size = 0;
static lv_obj_t *s_left_meter_bg = nullptr;
static lv_obj_t *s_left_meter_fill = nullptr;
static lv_obj_t *s_right_meter_bg = nullptr;
static lv_obj_t *s_right_meter_fill = nullptr;
static lv_obj_t *s_bottom_line = nullptr;
static lv_obj_t *s_top_line = nullptr;
static lv_obj_t *s_mid_line = nullptr;
static lv_obj_t *s_split_line_a = nullptr;
static lv_obj_t *s_split_line_b = nullptr;
static lv_obj_t *s_btn_l = nullptr;
static lv_obj_t *s_btn_m = nullptr;
static lv_obj_t *s_btn_r = nullptr;
static lv_obj_t *s_btn_l_label = nullptr;
static lv_obj_t *s_btn_m_label = nullptr;
static lv_obj_t *s_btn_r_label = nullptr;
static lv_obj_t *s_hdr_l = nullptr;
static lv_obj_t *s_hdr_r = nullptr;
static lv_obj_t *s_hdr_l_label = nullptr;
static lv_obj_t *s_hdr_r_label = nullptr;
static lv_obj_t *s_batt_label = nullptr;
static lv_obj_t *s_batt_value = nullptr;
static lv_obj_t *s_batt_bar = nullptr;
static bool s_enabled = false;
static bool s_running = false;
static bool s_needs_redraw = true;
static int s_last_nonzero_speed = 30;
static bool s_preset_mode = false;
static bool s_modifier_view = false;
static int s_preset_selection = 0;

static int s_base_index = 0;
static int s_modifier_index = 0;
static std::unordered_map<std::string, Control> s_advanced_settings;
static std::vector<std::string> s_control_names;
static std::vector<std::string> s_modifier_names;
static std::vector<std::string> s_preset_names;

// Parsed from AP status payload, but toggled locally for now.
static int s_read_count = 0;
static bool s_live_ap_available = false;
static uint32_t s_last_live_bootstrap_attempt_ms = 0;
static uint32_t s_last_live_status_poll_ms = 0;
static uint32_t s_last_live_presets_poll_ms = 0;
static uint32_t s_last_live_unavailable_log_ms = 0;
static std::string s_last_logged_status_frame;
static std::string s_last_logged_presets_frame;
static uint32_t s_last_status_log_ms = 0;
static uint32_t s_last_presets_log_ms = 0;
static uint32_t s_last_ui_input_ms = 0;

static long s_enc1 = 0;
static long s_enc2 = 0;
static long s_enc3 = 0;
static long s_enc4 = 0;
static const uint16_t s_ap_colors_565[7] = {0xf860, 0xfc00, 0xffe0, 0x07e0, 0x001f, 0xa87d, 0xf81f};

static lv_color_t colorFrom565(uint16_t c) {
    uint8_t r = (uint8_t)((((c >> 11) & 0x1F) * 255U) / 31U);
    uint8_t g = (uint8_t)((((c >> 5) & 0x3F) * 255U) / 63U);
    uint8_t b = (uint8_t)(((c & 0x1F) * 255U) / 31U);
    return lv_color_make(r, g, b);
}

static bool ensureModifierCanvasBuffer(int canvasW, int canvasH) {
    if (canvasW <= 0 || canvasH <= 0) return false;

    const size_t needed = LV_CANVAS_BUF_SIZE(canvasW, canvasH, 8, LV_DRAW_BUF_STRIDE_ALIGN);
    if (s_modifier_canvas_buffer && s_modifier_canvas_buffer_size >= needed) {
        return true;
    }

    if (s_modifier_canvas_buffer) {
        heap_caps_free(s_modifier_canvas_buffer);
        s_modifier_canvas_buffer = nullptr;
        s_modifier_canvas_buffer_size = 0;
    }

    s_modifier_canvas_buffer = static_cast<uint8_t *>(heap_caps_malloc(needed, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!s_modifier_canvas_buffer) {
        s_modifier_canvas_buffer = static_cast<uint8_t *>(heap_caps_malloc(needed, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    }
    if (!s_modifier_canvas_buffer) {
        return false;
    }

    s_modifier_canvas_buffer_size = needed;
    return true;
}

static float toFloatSafe(const std::string &s, float fallback = 0.0f) {
    try {
        return std::stof(s);
    } catch (...) {
        return fallback;
    }
}

static int toIntSafe(const std::string &s, int fallback = 0) {
    try {
        return std::stoi(s);
    } catch (...) {
        return fallback;
    }
}

static float getSettingValue(const std::string &key, float fallback = 0.0f) {
    auto it = s_advanced_settings.find(key);
    if (it == s_advanced_settings.end()) return fallback;
    return it->second.value;
}

static Control *getSetting(const std::string &key) {
    auto it = s_advanced_settings.find(key);
    if (it == s_advanced_settings.end()) return nullptr;
    return &it->second;
}

static const std::string &speedControlName() {
    static const std::string kLegacySpeed = "SP";
    if (!s_control_names.empty()) {
        return s_control_names.back();
    }
    return kLegacySpeed;
}

static int speedControlIndex() {
    if (!s_control_names.empty()) {
        return (int)s_control_names.size() - 1;
    }
    return 6;
}

static void normalizeIndexes() {
    int baseCount = (int)s_control_names.size() - 1;
    if (baseCount < 1) {
        s_base_index = 0;
    } else {
        if (s_base_index < 0) s_base_index = 0;
        if (s_base_index >= baseCount) s_base_index = baseCount - 1;
    }

    int modCount = (int)s_modifier_names.size();
    if (modCount < 1) {
        s_modifier_index = 0;
    } else {
        if (s_modifier_index < 0) s_modifier_index = 0;
        if (s_modifier_index >= modCount) s_modifier_index = modCount - 1;
    }
}

static void normalizePresetSelection() {
    if (s_preset_names.empty()) {
        s_preset_selection = 0;
        return;
    }
    if (s_preset_selection < 0) s_preset_selection = 0;
    if (s_preset_selection >= (int)s_preset_names.size()) s_preset_selection = (int)s_preset_names.size() - 1;
}

static void parsePresetsString(const std::string &presetsRaw) {
    // Port map: mirrors OSSMAdvanced::loadPresets() source list behavior.
    s_preset_names.clear();

    std::string presetList = presetsRaw;
    if (!presetList.empty() && presetList.back() != ',') {
        presetList.push_back(',');
    }

    size_t pi = presetList.find(',');
    while (pi != std::string::npos && pi > 0) {
        std::string presetName = presetList.substr(0, pi);
        if (!presetName.empty()) {
            s_preset_names.emplace_back(presetName);
        }
        presetList = presetList.substr(pi + 1);
        pi = presetList.find(',');
    }

    s_preset_names.emplace_back("Save New Preset");
    normalizePresetSelection();
}

static void ensureDefaultModel() {
    if (!s_control_names.empty() && !s_modifier_names.empty() && !s_advanced_settings.empty()) {
        return;
    }

    s_control_names = {"Top", "Bottom", "Rise", "Fall", "CurveIn", "CurveOut", "SP"};
    s_modifier_names = {"Amount", "StepA", "StepB", "StepC", "StepD", "Phase"};
    s_advanced_settings.clear();

    for (size_t i = 0; i < s_control_names.size(); ++i) {
        Control base{};
        base.minValue = 0;
        base.maxValue = 100;
        base.value = 50.0f;
        if (s_control_names[i] == "SP") base.value = 0.0f;
        s_advanced_settings[s_control_names[i]] = base;

        if (s_control_names[i] == "SP") continue;
        for (size_t m = 0; m < s_modifier_names.size(); ++m) {
            Control mod{};
            mod.minValue = 0;
            mod.maxValue = 100;
            mod.value = (m == 0) ? 100.0f : 50.0f;
            s_advanced_settings[s_control_names[i] + s_modifier_names[m]] = mod;
        }
    }

    normalizeIndexes();
}

static void parseConfigString(const std::string &configRaw) {
    // Port map: mirrors OSSMAdvanced::parseConfig() data grammar.
    std::string configValues = configRaw;
    if (configValues.empty()) return;
    configValues.push_back(',');

    std::vector<std::string> controlNames;
    std::vector<std::string> modifierNames;
    std::unordered_map<std::string, Control> settings;
    std::string modifierString;

    size_t ci = configValues.find(',');
    while (ci != std::string::npos && ci > 0) {
        std::string singleConfig = configValues.substr(0, ci);
        size_t j = singleConfig.find('(');
        size_t k = singleConfig.find('/');
        size_t l = singleConfig.find(')');
        if (j == std::string::npos || k == std::string::npos || l == std::string::npos || !(j < k && k < l)) {
            configValues = configValues.substr(ci + 1);
            ci = configValues.find(',');
            continue;
        }

        std::string name = singleConfig.substr(0, j);
        if (name.empty()) {
            configValues = configValues.substr(ci + 1);
            ci = configValues.find(',');
            continue;
        }

        controlNames.emplace_back(name);

        int minValueI = toIntSafe(singleConfig.substr(j + 1, k - (j + 1)), 0);
        int maxValueI = toIntSafe(singleConfig.substr(k + 1, l - (k + 1)), 100);
        if (minValueI < 0) minValueI = 0;
        if (maxValueI > 100) maxValueI = 100;
        if (minValueI > maxValueI) std::swap(minValueI, maxValueI);

        Control newControl{};
        newControl.minValue = (uint8_t)minValueI;
        newControl.maxValue = (uint8_t)maxValueI;
        newControl.value = (float)minValueI;
        settings[name] = newControl;

        size_t mi = singleConfig.find(':');
        if (mi != std::string::npos && (l + 2) < singleConfig.size()) {
            modifierString = singleConfig.substr(l + 2) + ':';
        }

        std::string iterString = modifierString;
        mi = iterString.find(':');
        while (mi != std::string::npos && mi > 0) {
            size_t mj = iterString.find('(');
            size_t mk = iterString.find('/');
            size_t ml = iterString.find(')');
            if (mj == std::string::npos || mk == std::string::npos || ml == std::string::npos || !(mj < mk && mk < ml)) {
                break;
            }

            std::string modifierName = iterString.substr(0, mj);
            if (!modifierName.empty() && std::find(modifierNames.begin(), modifierNames.end(), modifierName) == modifierNames.end()) {
                modifierNames.emplace_back(modifierName);
            }

            int modMinI = toIntSafe(iterString.substr(mj + 1, mk - (mj + 1)), 0);
            int modMaxI = toIntSafe(iterString.substr(mk + 1, ml - (mk + 1)), 100);
            if (modMinI < 0) modMinI = 0;
            if (modMaxI > 100) modMaxI = 100;
            if (modMinI > modMaxI) std::swap(modMinI, modMaxI);

            Control mod{};
            mod.minValue = (uint8_t)modMinI;
            mod.maxValue = (uint8_t)modMaxI;
            mod.value = (float)modMinI;
            if (!modifierNames.empty() && modifierName == modifierNames[0]) {
                mod.value = (float)mod.maxValue;
            }
            settings[name + modifierName] = mod;

            iterString = iterString.substr(mi + 1);
            mi = iterString.find(':');
        }

        configValues = configValues.substr(ci + 1);
        ci = configValues.find(',');
    }

    if (!controlNames.empty() && !modifierNames.empty() && !settings.empty()) {
        s_control_names = controlNames;
        s_modifier_names = modifierNames;
        s_advanced_settings = settings;
        normalizeIndexes();
    }
}

static void parseStatusString(const std::string &statusRaw) {
    // Port map: mirrors OSSMAdvanced::parseStatus() value decoding.
    if (s_control_names.empty()) return;

    std::string statusString = statusRaw;
    if (statusString.empty()) return;
    statusString.push_back(',');

    int controlCounter = 0;
    size_t si = statusString.find(',');
    while (si != std::string::npos && si > 0 && controlCounter < (int)s_control_names.size()) {
        std::string singleStatus = statusString.substr(0, si);
        float value = toFloatSafe(singleStatus, getSettingValue(s_control_names[controlCounter], 0.0f));
        Control *base = getSetting(s_control_names[controlCounter]);
        if (base) {
            if (value < base->minValue) value = (float)base->minValue;
            if (value > base->maxValue) value = (float)base->maxValue;
            base->value = value;
        }

        size_t mi = singleStatus.find(':');
        int modifierCounter = 0;
        if (mi == std::string::npos && !s_modifier_names.empty()) {
            Control *mod0 = getSetting(s_control_names[controlCounter] + s_modifier_names[0]);
            if (mod0) mod0->value = 100.0f;
        }
        while (mi != std::string::npos && mi > 0 && modifierCounter < (int)s_modifier_names.size()) {
            singleStatus = singleStatus.substr(mi + 1);
            value = toFloatSafe(singleStatus, getSettingValue(s_control_names[controlCounter] + s_modifier_names[modifierCounter], 0.0f));
            Control *mod = getSetting(s_control_names[controlCounter] + s_modifier_names[modifierCounter]);
            if (mod) {
                if (value < mod->minValue) value = (float)mod->minValue;
                if (value > mod->maxValue) value = (float)mod->maxValue;
                mod->value = value;
            }
            ++modifierCounter;
            mi = singleStatus.find(':');
        }

        statusString = statusString.substr(si + 1);
        si = statusString.find(',');
        ++controlCounter;
    }

    const int speedNow = (int)(getSettingValue(speedControlName(), 0.0f) + 0.5f);
    s_running = speedNow > 0;
    if (speedNow > 0) s_last_nonzero_speed = speedNow;

    s_read_count = 0;
}

static void refreshModelFromSyntheticFrames() {
    // Adapter stage: keep this payload-driven path identical to the original model parser.
    // A future AP transport bridge can replace these strings with live BLE responses.
    static bool initialized = false;
    if (initialized) return;

    const std::string configFrame =
        "Top(0/100):Amount(0/100):StepA(0/100):StepB(0/100):StepC(0/100):StepD(0/100):Phase(0/100),"
        "Bottom(0/100):Amount(0/100):StepA(0/100):StepB(0/100):StepC(0/100):StepD(0/100):Phase(0/100),"
        "Rise(0/100):Amount(0/100):StepA(0/100):StepB(0/100):StepC(0/100):StepD(0/100):Phase(0/100),"
        "Fall(0/100):Amount(0/100):StepA(0/100):StepB(0/100):StepC(0/100):StepD(0/100):Phase(0/100),"
        "CurveIn(0/100):Amount(0/100):StepA(0/100):StepB(0/100):StepC(0/100):StepD(0/100):Phase(0/100),"
        "CurveOut(0/100):Amount(0/100):StepA(0/100):StepB(0/100):StepC(0/100):StepD(0/100):Phase(0/100),"
        "SP(0/100)";

    const std::string statusFrame =
        "50:100:50:50:50:50:50,"
        "50:100:50:50:50:50:50,"
        "50:100:50:50:50:50:50,"
        "50:100:50:50:50:50:50,"
        "50:100:50:50:50:50:50,"
        "50:100:50:50:50:50:50,"
        "0";

    const std::string presetsFrame =
        "Default,Soft,Intense,Wave";

    parseConfigString(configFrame);
    parseStatusString(statusFrame);
    parsePresetsString(presetsFrame);
    initialized = true;
}

static bool shouldLogLiveFrame(const std::string &frame, std::string *lastFrame, uint32_t *lastLogMs, uint32_t heartbeatMs) {
    if (!lastFrame || !lastLogMs) return true;
    const uint32_t nowMs = millis();
    if (frame != *lastFrame || (nowMs - *lastLogMs) > heartbeatMs) {
        *lastFrame = frame;
        *lastLogMs = nowMs;
        return true;
    }
    return false;
}

static bool tryBootstrapModelFromLiveBle(bool fetchExtras = true) {
    const uint32_t nowMs = millis();
    if ((nowMs - s_last_live_bootstrap_attempt_ms) < 1500U) {
        return s_live_ap_available;
    }
    s_last_live_bootstrap_attempt_ms = nowMs;

    String configFrame;
    if (!bleCommReadAdvancedConfig(&configFrame) || configFrame.length() == 0) {
        const uint32_t now = millis();
        if ((now - s_last_live_unavailable_log_ms) > 5000U) {
            LogDebug("[AP] Live config read unavailable, using fallback when needed");
            s_last_live_unavailable_log_ms = now;
        }
        s_live_ap_available = false;
        return false;
    }

    LogDebugFormatted("[AP] Live config RX: %s\n", configFrame.c_str());

    parseConfigString(std::string(configFrame.c_str()));

    if (fetchExtras) {
        String statusFrame;
        if (bleCommReadAdvancedStatus(&statusFrame) && statusFrame.length() > 0) {
            const std::string statusStd(statusFrame.c_str());
            if (shouldLogLiveFrame(statusStd, &s_last_logged_status_frame, &s_last_status_log_ms, 5000U)) {
                LogDebugFormatted("[AP] Live status RX: %s\n", statusFrame.c_str());
            }
            parseStatusString(statusStd);
        }

        String presetsFrame;
        if (bleCommReadAdvancedPresets(&presetsFrame) && presetsFrame.length() > 0) {
            const std::string presetsStd(presetsFrame.c_str());
            if (shouldLogLiveFrame(presetsStd, &s_last_logged_presets_frame, &s_last_presets_log_ms, 5000U)) {
                LogDebugFormatted("[AP] Live presets RX: %s\n", presetsFrame.c_str());
            }
            parsePresetsString(presetsStd);
        }
    }

    const bool liveReady = !s_control_names.empty() && !s_advanced_settings.empty();
    s_live_ap_available = liveReady;
    return liveReady;
}

static void refreshFromLiveStatusIfDue() {
    if (!s_live_ap_available) return;

    const uint32_t nowMs = millis();
    const uint32_t pollIntervalMs = s_modifier_view ? 900U : 250U;
    if ((nowMs - s_last_live_status_poll_ms) < pollIntervalMs) return;
    if (s_modifier_view && (nowMs - s_last_ui_input_ms) < 600U) return;
    s_last_live_status_poll_ms = nowMs;

    String statusFrame;
    if (!bleCommReadAdvancedStatus(&statusFrame) || statusFrame.length() == 0) {
        return;
    }

    const std::string statusStd(statusFrame.c_str());
    if (shouldLogLiveFrame(statusStd, &s_last_logged_status_frame, &s_last_status_log_ms, 5000U)) {
        LogDebugFormatted("[AP] Live status RX: %s\n", statusFrame.c_str());
    }

    parseStatusString(statusStd);
    if (!s_modifier_view || (nowMs - s_last_ui_input_ms) > 200U) {
        s_needs_redraw = true;
    }
}

static void refreshFromLivePresetsIfDue(bool force = false) {
    if (!s_live_ap_available) return;

    const uint32_t nowMs = millis();
    if (!force && (nowMs - s_last_live_presets_poll_ms) < 1200U) return;
    s_last_live_presets_poll_ms = nowMs;

    String presetsFrame;
    if (!bleCommReadAdvancedPresets(&presetsFrame) || presetsFrame.length() == 0) {
        return;
    }

    const std::string presetsStd(presetsFrame.c_str());
    if (shouldLogLiveFrame(presetsStd, &s_last_logged_presets_frame, &s_last_presets_log_ms, 5000U)) {
        LogDebugFormatted("[AP] Live presets RX: %s\n", presetsFrame.c_str());
    }

    parsePresetsString(presetsStd);
    s_needs_redraw = true;
}

static void sendAdvancedControlIfLive(const std::string &payload) {
    if (!s_live_ap_available) return;
    if (payload.empty()) return;

    LogDebugFormatted("[AP] Live control TX: %s\n", payload.c_str());
    if (!bleCommWriteAdvancedControl(String(payload.c_str()))) {
        LogDebug("[AP] Live control TX failed, disabling live AP transport");
        s_live_ap_available = false;
    }
}

static void sendAdvancedPresetIfLive(const std::string &payload) {
    if (!s_live_ap_available) return;
    if (payload.empty()) return;
    LogDebugFormatted("[AP] Live presets TX: %s\n", payload.c_str());
    if (!bleCommWriteAdvancedPresets(String(payload.c_str()))) {
        LogDebug("[AP] Live presets TX failed, disabling live AP transport");
        s_live_ap_available = false;
    }
}

static int detentsFromEncoder(ESP32Encoder &enc, long *state) {
    if (!state) return 0;
    long count = enc.getCount();
    if (count >= (*state + 2)) {
        *state = count;
        return 1;
    }
    if (count <= (*state - 2)) {
        *state = count;
        return -1;
    }
    return 0;
}

static void clampValue(int *value, int minValue, int maxValue) {
    if (!value) return;
    if (*value < minValue) *value = minValue;
    if (*value > maxValue) *value = maxValue;
}

static float bezierMath(float v0, float v1, float v2, float v3, float t) {
    float output = std::pow(1.0f - t, 3.0f) * v0;
    output += 3.0f * std::pow(1.0f - t, 2.0f) * t * v1;
    output += 3.0f * (1.0f - t) * std::pow(t, 2.0f) * v2;
    output += std::pow(t, 3.0f) * v3;
    return output;
}

static void setDot(lv_obj_t *dot, int x, int y, bool visible) {
    if (!dot) return;
    if (!visible) {
        lv_obj_add_flag(dot, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    lv_obj_clear_flag(dot, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_pos(dot, x, y);
}

static void leaveApToAddons() {
    LogDebug("[AP] Leaving AP mode to Addons screen");
    if (g_addon_return_screen != nullptr) {
        _ui_screen_change(g_addon_return_screen, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    } else {
        _ui_screen_change(ui_Addons, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
    }
}

static void stepTabLeft() {
    if (s_preset_mode) {
        if (s_preset_selection > 0) {
            --s_preset_selection;
        } else {
            leaveApToAddons();
            return;
        }
    } else if (s_modifier_view) {
        int modCount = (int)s_modifier_names.size();
        if (modCount > 0) {
            s_modifier_index = (s_modifier_index + modCount - 1) % modCount;
        }
    } else {
        int baseCount = (int)s_control_names.size() - 1;
        if (baseCount > 0) {
            if (s_base_index > 0) {
                --s_base_index;
            } else {
                leaveApToAddons();
                return;
            }
            s_modifier_index = 0;
        }
    }
    s_needs_redraw = true;
}

static void stepTabRight() {
    if (s_preset_mode) {
        if (!s_preset_names.empty() && s_preset_selection < (int)s_preset_names.size() - 1) {
            ++s_preset_selection;
        }
    } else if (s_modifier_view) {
        int modCount = (int)s_modifier_names.size();
        if (modCount > 0) {
            s_modifier_index = (s_modifier_index + 1) % modCount;
        }
    } else {
        int baseCount = (int)s_control_names.size() - 1;
        if (baseCount > 0) {
            s_base_index = (s_base_index + 1) % baseCount;
            s_modifier_index = 0;
        }
    }
    s_needs_redraw = true;
}

static bool setSpeedValue(int value);

static void returnToPatternScreen() {
    s_preset_mode = false;
    s_modifier_view = false;
    s_modifier_index = 0;
    s_needs_redraw = true;
}

static void handleLeftShortAction() {
    LogDebug("[AP] Left button short press");
    s_last_ui_input_ms = millis();
    if (s_preset_mode || s_modifier_view) {
        LogDebug("[AP] Returning to pattern screen");
        returnToPatternScreen();
    } else {
        LogDebug("[AP] Entering preset mode");
        s_preset_mode = true;
        refreshFromLivePresetsIfDue(true);
        s_needs_redraw = true;
    }
}

static void handleMiddleShortAction() {
    LogDebug("[AP] Middle button short press");
    s_last_ui_input_ms = millis();
    if (s_preset_mode) {
        if (!s_preset_names.empty()) {
            const bool saveNew = (s_preset_selection == (int)s_preset_names.size() - 1);
            if (saveNew) {
                sendAdvancedPresetIfLive(">");
                refreshFromLivePresetsIfDue(true);
            } else {
                const std::string selected = s_preset_names[s_preset_selection];
                sendAdvancedPresetIfLive(std::string(":") + selected);
            }

            // After preset apply/save, fetch current state and presets quickly.
            refreshFromLiveStatusIfDue();
            refreshFromLivePresetsIfDue(true);
            s_needs_redraw = true;
        }
    } else {
        if (s_running) {
            if (setSpeedValue(0)) {
                s_running = false;
                s_needs_redraw = true;
            }
        } else {
            int resume = s_last_nonzero_speed;
            if (resume <= 0) resume = 30;
            if (setSpeedValue(resume)) {
                s_running = true;
                s_needs_redraw = true;
            }
        }
    }
}

static void handleRightShortAction() {
    LogDebug("[AP] Right button short press");
    s_last_ui_input_ms = millis();
    if (s_preset_mode) {
        s_preset_mode = false;
        s_needs_redraw = true;
        LogDebug("[AP] Exiting preset mode");
    } else if (s_modifier_view) {
        LogDebug("[AP] Exiting modifier view");
        leaveApToAddons();
        s_needs_redraw = true;
    } else {
        LogDebug("[AP] Entering modifier view");
        s_modifier_view = true;
        s_needs_redraw = true;
    }
}

static void event_hdr_l(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    stepTabLeft();
}

static void event_hdr_r(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    stepTabRight();
}

static void event_btn_l(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    handleLeftShortAction();
}

static void event_btn_m(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    handleMiddleShortAction();
}

static void event_btn_r(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    handleRightShortAction();
}

static void setHorizontalGuide(lv_obj_t *line, int x, int y, int width, uint16_t color) {
    if (!line) return;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    lv_obj_set_size(line, width, 2);
    lv_obj_set_x(line, x);
    lv_obj_set_y(line, y);
    lv_obj_set_style_bg_color(line, colorFrom565(color), LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void setVerticalGuide(lv_obj_t *line, int x, int y, int height, uint16_t color) {
    if (!line) return;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    lv_obj_set_size(line, 2, height);
    lv_obj_set_x(line, x);
    lv_obj_set_y(line, y);
    lv_obj_set_style_bg_color(line, colorFrom565(color), LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void hideCurveDots() {
    const int sampleMain = (int)(sizeof(s_curve_main_pts) / sizeof(s_curve_main_pts[0]));
    const int sampleMod = (int)(sizeof(s_curve_mod_pts) / sizeof(s_curve_mod_pts[0]));
    for (int i = 0; i < sampleMain; ++i) setDot(s_curve_main_pts[i], 0, 0, false);
    for (int i = 0; i < sampleMod; ++i) setDot(s_curve_mod_pts[i], 0, 0, false);
}

static void hideModifierTraces() {
    for (int t = 0; t < k_modifier_trace_count; ++t) {
        const int allocated = s_modifier_trace_allocated_segments[t];
        for (int i = 0; i < allocated; ++i) {
            if (s_modifier_trace_lines[t][i]) {
                lv_obj_add_flag(s_modifier_trace_lines[t][i], LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}

static lv_obj_t *ensureModifierTraceLine(int traceIndex, int segmentIndex) {
    if (!s_graph_panel) return nullptr;
    if (traceIndex < 0 || traceIndex >= k_modifier_trace_count) return nullptr;
    if (segmentIndex < 0 || segmentIndex >= k_modifier_trace_segments) return nullptr;

    lv_obj_t *line = s_modifier_trace_lines[traceIndex][segmentIndex];
    if (!line) {
        line = lv_line_create(s_graph_panel);
        s_modifier_trace_lines[traceIndex][segmentIndex] = line;
        lv_obj_set_style_line_color(line, colorFrom565(s_ap_colors_565[traceIndex]), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_line_width(line, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_line_rounded(line, false, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_flag(line, LV_OBJ_FLAG_HIDDEN);
        if ((segmentIndex + 1) > s_modifier_trace_allocated_segments[traceIndex]) {
            s_modifier_trace_allocated_segments[traceIndex] = segmentIndex + 1;
        }
    }
    return line;
}

static void setModifierLineSegment(int traceIndex, int segmentIndex, float x0, float y0, float x1, float y1,
                                   bool highlight, float xMin, float xMax, float yMin, float yMax) {
    if (traceIndex < 0 || traceIndex >= k_modifier_trace_count) return;
    if (segmentIndex < 0 || segmentIndex >= k_modifier_trace_segments) return;

    if (x0 < xMin) x0 = xMin;
    if (x0 > xMax) x0 = xMax;
    if (x1 < xMin) x1 = xMin;
    if (x1 > xMax) x1 = xMax;
    if (y0 < yMin) y0 = yMin;
    if (y0 > yMax) y0 = yMax;
    if (y1 < yMin) y1 = yMin;
    if (y1 > yMax) y1 = yMax;

    lv_obj_t *line = ensureModifierTraceLine(traceIndex, segmentIndex);
    if (!line) return;

    s_modifier_trace_line_pts[traceIndex][segmentIndex][0].x = (int32_t)x0;
    s_modifier_trace_line_pts[traceIndex][segmentIndex][0].y = (int32_t)y0;
    s_modifier_trace_line_pts[traceIndex][segmentIndex][1].x = (int32_t)x1;
    s_modifier_trace_line_pts[traceIndex][segmentIndex][1].y = (int32_t)y1;

    lv_line_set_points(line, s_modifier_trace_line_pts[traceIndex][segmentIndex], 2);
    lv_obj_set_style_line_width(line, highlight ? 3 : 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(line, LV_OBJ_FLAG_HIDDEN);
}

static void drawSingleModifierTrace(int controlIndex, int traceIndex, bool highlight, float xMin, float xSpan, float yMin,
                                    float ySpan, float maxSteps) {
    if (controlIndex < 0 || traceIndex < 0 || traceIndex >= k_modifier_trace_count) return;

    const int baseCount = std::max(0, (int)s_control_names.size() - 1);
    const int modCount = (int)s_modifier_names.size();
    if (controlIndex >= baseCount || modCount < 1 || maxSteps <= 0.0f) return;

    const std::string &controlName = s_control_names[controlIndex];
    const float baseValueRatio = 1.0f - getSettingValue(controlName, 50.0f) / 100.0f;
    const float modValueRatio = 1.0f - getSettingValue(controlName + s_modifier_names[0], 100.0f) / 100.0f;

    float strokeRatio = 1.0f - baseValueRatio;
    if (controlIndex < 2 && baseCount > 1) {
        strokeRatio = (getSettingValue(s_control_names[0], 50.0f) - getSettingValue(s_control_names[1], 50.0f)) / 100.0f;
    }

    const float baseY = yMin + ySpan * baseValueRatio;
    float modY = baseY + ySpan * strokeRatio * modValueRatio;
    if (controlIndex == 1) {
        modY = baseY - ySpan * strokeRatio * modValueRatio;
    }

    const int phaseIndex = std::min(5, modCount - 1);
    const float stepWidth = xSpan / maxSteps;
    float startX = xMin - stepWidth * getSettingValue(controlName + s_modifier_names[phaseIndex], 0.0f);

    int segmentIndex = 0;
    int segment = 0;
    int guard = 0;
    while (startX < (xMin + xSpan) && guard < 96 && segmentIndex < k_modifier_trace_segments) {
        const int stepIndex = std::min(segment + 1, modCount - 1);
        const float step = std::max(1.0f, getSettingValue(controlName + s_modifier_names[stepIndex], 1.0f)) * stepWidth;
        const float endX = startX + step;

        float y0 = baseY;
        float y1 = baseY;
        if (segment == 0) {
            y0 = baseY;
            y1 = modY;
        } else if (segment == 1) {
            y0 = modY;
            y1 = modY;
        } else if (segment == 2) {
            y0 = modY;
            y1 = baseY;
        }

        setModifierLineSegment(traceIndex, segmentIndex, startX, y0, endX, y1, highlight, xMin, xMin + xSpan, yMin,
                               yMin + ySpan);
        ++segmentIndex;
        startX = endX;
        segment = (segment + 1) % 4;
        ++guard;
    }

    while (segmentIndex < k_modifier_trace_segments) {
        lv_obj_t *line = s_modifier_trace_lines[traceIndex][segmentIndex];
        if (line) lv_obj_add_flag(line, LV_OBJ_FLAG_HIDDEN);
        ++segmentIndex;
    }
}

static void updateModifierPreview() {
    if (!s_graph_panel) return;

    const int panelW = lv_obj_get_width(s_graph_panel);
    const int panelH = lv_obj_get_height(s_graph_panel);
    if (panelW < 8 || panelH < 8) return;

    hideCurveDots();
    hideModifierTraces();

    const int baseCount = std::max(0, (int)s_control_names.size() - 1);
    const int traceCount = std::min(baseCount, k_modifier_trace_count);
    if (traceCount <= 0 || s_modifier_names.empty()) return;

    if (s_modifier_canvas) lv_obj_add_flag(s_modifier_canvas, LV_OBJ_FLAG_HIDDEN);

    float maxSteps = 4.0f;
    for (int c = 0; c < traceCount; ++c) {
        float modSteps = 0.0f;
        const std::string &controlName = s_control_names[c];
        const int maxStepIndexExclusive = (int)s_control_names.size() - 2;
        for (int m = 1; m < maxStepIndexExclusive && m < (int)s_modifier_names.size(); ++m) {
            modSteps += getSettingValue(controlName + s_modifier_names[m], 0.0f);
        }
        if (modSteps > maxSteps) maxSteps = modSteps;
    }

    for (int c = 0; c < traceCount; ++c) {
        drawSingleModifierTrace(c, c, c == s_base_index, 0, 300, 0, 150, maxSteps);
    }
    drawSingleModifierTrace(s_base_index, s_base_index, true, 0, 300, 0, 150, maxSteps);
    drawSingleModifierTrace(s_base_index, s_base_index, true, 1, 300, 0, 150, maxSteps);
    drawSingleModifierTrace(s_base_index, s_base_index, true, 1, 301, 1, 151, maxSteps);
}

static void updateCurvePreview(const std::string &baseName) {
    if (!s_graph_panel) return;

    const int panelW = lv_obj_get_width(s_graph_panel);
    const int panelH = lv_obj_get_height(s_graph_panel);
    if (panelW < 6 || panelH < 6) return;

    const float xMin = 1.0f;
    const float xSpan = (float)(panelW - 4);
    const float yMin = 1.0f;
    const float ySpan = (float)(panelH - 4);

    const int sampleMain = (int)(sizeof(s_curve_main_pts) / sizeof(s_curve_main_pts[0]));
    const int sampleMod = (int)(sizeof(s_curve_mod_pts) / sizeof(s_curve_mod_pts[0]));

    if (s_preset_mode) {
        hideModifierTraces();
    }

    if (s_modifier_view) {
        if (s_modifier_canvas) lv_obj_clear_flag(s_modifier_canvas, LV_OBJ_FLAG_HIDDEN);
        updateModifierPreview();
        return;
    }

    if (s_modifier_canvas) lv_obj_add_flag(s_modifier_canvas, LV_OBJ_FLAG_HIDDEN);

    hideModifierTraces();

    const bool liveApCurveLayout =
        s_control_names.size() >= 7 &&
        !s_modifier_names.empty() &&
        s_control_names[0] == "Max Depth" &&
        s_control_names[1] == "Min Depth";

    if (liveApCurveLayout) {
        const std::string &curveMax = s_control_names[0];
        const std::string &curveMin = s_control_names[1];
        const std::string &inSpeedKey = s_control_names[2];
        const std::string &outSpeedKey = s_control_names[3];
        const std::string &curveInKey = s_control_names[4];
        const std::string &curveOutKey = s_control_names[5];
        const std::string &mod0 = s_modifier_names[0];
        const float yMax = yMin + (1.0f - getSettingValue(curveMax, 50.0f) / 100.0f) * ySpan;
        const float yMinDepth = yMin + (1.0f - getSettingValue(curveMin, 50.0f) / 100.0f) * ySpan;
        const float yMinBound = std::min(yMax, yMinDepth);
        const float yMaxBound = std::max(yMax, yMinDepth);

        const float diff = yMinDepth - yMax;
        const float yMaxMod =
            (1.0f - getSettingValue(curveMax + mod0, 100.0f) / 100.0f) * diff + yMax;
        const float yMinMod =
            yMinDepth - (1.0f - getSettingValue(curveMin + mod0, 100.0f) / 100.0f) * diff;

        const float a = std::max(1.0f, getSettingValue(inSpeedKey, 1.0f));
        const float b = std::max(1.0f, getSettingValue(outSpeedKey, 1.0f));
        const float am = std::max(1.0f, getSettingValue(inSpeedKey + mod0, 100.0f) / 100.0f * a);
        const float bm = std::max(1.0f, getSettingValue(outSpeedKey + mod0, 100.0f) / 100.0f * b);

        float splitXf = xMin + (1.0f - (a / b) / ((a / b) + 1.0f)) * xSpan;
        if (splitXf < xMin) splitXf = xMin;
        if (splitXf > (xMin + xSpan)) splitXf = xMin + xSpan;

        float splitXm = xMin + (1.0f - (am / bm) / ((am / bm) + 1.0f)) * xSpan;
        if (splitXm < xMin) splitXm = xMin;
        if (splitXm > (xMin + xSpan)) splitXm = xMin + xSpan;

        const float c = std::max(1.0f, getSettingValue(curveInKey, 50.0f));
        const float d = std::max(1.0f, getSettingValue(curveOutKey, 50.0f));
        const float cm = std::max(1.0f, getSettingValue(curveInKey + mod0, 100.0f) / 100.0f * c);
        const float dm = std::max(1.0f, getSettingValue(curveOutKey + mod0, 100.0f) / 100.0f * d);

        const float rIn = 0.1f + 0.4f * (1.0f - c / 100.0f);
        const float rOut = 0.1f + 0.4f * (1.0f - d / 100.0f);
        const float rInMod = 0.1f + 0.4f * (1.0f - cm / 100.0f);
        const float rOutMod = 0.1f + 0.4f * (1.0f - dm / 100.0f);

        for (int i = 0; i < sampleMain; ++i) {
            float t = (float)i / (float)(sampleMain - 1);
            float x = xMin + t * xSpan;
            float y = 0.0f;
            if (x <= splitXf) {
                float tl = (splitXf <= xMin) ? 0.0f : ((x - xMin) / (splitXf - xMin));
                float cp = (splitXf - xMin) * rIn;
                y = bezierMath(yMinDepth, yMinDepth, yMax, yMax, tl);
                (void)cp;
            } else {
                float tr = (x - splitXf) / ((xMin + xSpan) - splitXf);
                float cp = ((xMin + xSpan) - splitXf) * rOut;
                y = bezierMath(yMax, yMax, yMinDepth, yMinDepth, tr);
                (void)cp;
            }
            y = std::max(yMinBound, std::min(y, yMaxBound));

            float yM = 0.0f;
            if (x <= splitXm) {
                float tl = (splitXm <= xMin) ? 0.0f : ((x - xMin) / (splitXm - xMin));
                float cp = (splitXm - xMin) * rInMod;
                yM = bezierMath(yMinMod, yMinMod, yMaxMod, yMaxMod, tl);
                (void)cp;
            } else {
                float tr = (x - splitXm) / ((xMin + xSpan) - splitXm);
                float cp = ((xMin + xSpan) - splitXm) * rOutMod;
                yM = bezierMath(yMaxMod, yMaxMod, yMinMod, yMinMod, tr);
                (void)cp;
            }
            yM = std::max(yMinBound, std::min(yM, yMaxBound));

            setDot(s_curve_main_pts[i], (int)x, (int)y, true);
            if ((i % 2) == 0) setDot(s_curve_mod_pts[i / 2], (int)x, (int)yM, true);
        }
        for (int i = sampleMain / 2; i < sampleMod; ++i) {
            if ((i * 2) >= sampleMain) setDot(s_curve_mod_pts[i], 0, 0, false);
        }
        return;
    }

    if (s_modifier_index == 0 && s_control_names.size() >= 6) {
        float yTop = yMin + (1.0f - getSettingValue("Top", 50.0f) / 100.0f) * ySpan;
        float yBottom = yMin + (1.0f - getSettingValue("Bottom", 50.0f) / 100.0f) * ySpan;

        float rise = std::max(1.0f, getSettingValue("Rise", 50.0f));
        float fall = std::max(1.0f, getSettingValue("Fall", 50.0f));
        float splitXf = xMin + (1.0f - (rise / fall) / ((rise / fall) + 1.0f)) * xSpan;
        if (splitXf < xMin) splitXf = xMin;
        if (splitXf > (xMin + xSpan)) splitXf = xMin + xSpan;

        float curveIn = getSettingValue("CurveIn", 50.0f);
        float curveOut = getSettingValue("CurveOut", 50.0f);
        float rIn = 0.1f + 0.4f * (1.0f - curveIn / 100.0f);
        float rOut = 0.1f + 0.4f * (1.0f - curveOut / 100.0f);

        const float amountTop = getSettingValue("TopAmount", 100.0f) / 100.0f;
        const float amountBottom = getSettingValue("BottomAmount", 100.0f) / 100.0f;
        float yTopMod = yTop + (yBottom - yTop) * (1.0f - amountTop);
        float yBottomMod = yBottom - (yBottom - yTop) * (1.0f - amountBottom);

        for (int i = 0; i < sampleMain; ++i) {
            float t = (float)i / (float)(sampleMain - 1);
            float x = xMin + t * xSpan;
            float y = 0.0f;
            if (x <= splitXf) {
                float tl = (splitXf <= xMin) ? 0.0f : ((x - xMin) / (splitXf - xMin));
                y = bezierMath(yBottom, yBottom, yTop, yTop, tl);
            } else {
                float tr = (x - splitXf) / ((xMin + xSpan) - splitXf);
                y = bezierMath(yTop, yTop, yBottom, yBottom, tr);
            }

            float yM = 0.0f;
            if (x <= splitXf) {
                float tl = (splitXf <= xMin) ? 0.0f : ((x - xMin) / (splitXf - xMin));
                yM = bezierMath(yBottomMod, yBottomMod, yTopMod, yTopMod, tl);
            } else {
                float tr = (x - splitXf) / ((xMin + xSpan) - splitXf);
                yM = bezierMath(yTopMod, yTopMod, yBottomMod, yBottomMod, tr);
            }

            setDot(s_curve_main_pts[i], (int)x, (int)y, true);
            if ((i % 2) == 0) setDot(s_curve_mod_pts[i / 2], (int)x, (int)yM, true);
        }
        for (int i = sampleMain / 2; i < sampleMod; ++i) {
            if (i >= (sampleMain / 2)) {
                // keep any non-used mod points hidden
                if ((i * 2) >= sampleMain) setDot(s_curve_mod_pts[i], 0, 0, false);
            }
        }
        return;
    }

    const float base = getSettingValue(baseName, 50.0f) / 100.0f;
    const std::string amountKey = baseName + "Amount";
    const std::string stepAKey = baseName + "StepA";
    const std::string stepBKey = baseName + "StepB";
    const std::string stepCKey = baseName + "StepC";
    const std::string stepDKey = baseName + "StepD";
    const std::string phaseKey = baseName + "Phase";

    float modRatio = 1.0f - (getSettingValue(amountKey, 100.0f) / 100.0f);
    float yBase = yMin + (1.0f - base) * ySpan;
    float yMod = yBase + (ySpan * 0.4f) * modRatio;

    float steps[4] = {
        std::max(1.0f, getSettingValue(stepAKey, 25.0f)),
        std::max(1.0f, getSettingValue(stepBKey, 25.0f)),
        std::max(1.0f, getSettingValue(stepCKey, 25.0f)),
        std::max(1.0f, getSettingValue(stepDKey, 25.0f)),
    };
    float sumSteps = steps[0] + steps[1] + steps[2] + steps[3];
    float phase = getSettingValue(phaseKey, 0.0f) / 100.0f;

    for (int i = 0; i < sampleMain; ++i) {
        float t = (float)i / (float)(sampleMain - 1);
        float wave = std::fmod((t + phase) * sumSteps, sumSteps);
        int segment = 0;
        while (segment < 3 && wave > steps[segment]) {
            wave -= steps[segment];
            ++segment;
        }

        float y = yBase;
        if (segment == 0) {
            y = yBase + (yMod - yBase) * (wave / steps[0]);
        } else if (segment == 1) {
            y = yMod;
        } else if (segment == 2) {
            y = yMod + (yBase - yMod) * (wave / steps[2]);
        }

        int x = (int)(xMin + t * xSpan);
        setDot(s_curve_main_pts[i], x, (int)y, true);
        if ((i % 2) == 0) setDot(s_curve_mod_pts[i / 2], x, (int)yBase, true);
    }
}

static void drawApScreen() {
    ensureDefaultModel();
    normalizeIndexes();

    const bool hasBase = ((int)s_control_names.size() - 1) > 0;
    const bool hasMod = !s_modifier_names.empty();
    const std::string baseName = hasBase ? s_control_names[s_base_index] : std::string("-");
    const std::string modName = hasMod ? s_modifier_names[s_modifier_index] : std::string("-");
    const float baseValue = hasBase ? getSettingValue(baseName, 0.0f) : 0.0f;
    const float modValue = (hasBase && hasMod) ? getSettingValue(baseName + modName, 0.0f) : 0.0f;
    const float speed = getSettingValue(speedControlName(), 0.0f);
    const bool liveActive = s_live_ap_available;
    const bool hasPresets = !s_preset_names.empty();
    const std::string selectedPreset = hasPresets ? s_preset_names[s_preset_selection] : std::string("-");

    if (!s_title_label) return;

    const int speedInt = (int)(speed + 0.5f);
    const int baseInt = (int)(baseValue + 0.5f);
    const int modInt = (int)(modValue + 0.5f);
    const int tabCount = s_modifier_view ? std::min(6, (int)s_modifier_names.size())
                                         : std::min(6, std::max(0, (int)s_control_names.size() - 1));
    const std::string focusedName = s_modifier_view ? modName : baseName;
    const int focusedValue = s_modifier_view ? modInt : baseInt;
    lv_label_set_text(s_title_label, focusedName.c_str());
    lv_label_set_text_fmt(s_state_label, "Speed %d", speedInt);
//    lv_label_set_text(s_transport_label, "Enc1 speed");

    for (int i = 0; i < 6; ++i) {
        if (!s_tabs[i] || !s_tab_labels[i]) continue;
        if (i < tabCount) {
            lv_obj_clear_flag(s_tabs[i], LV_OBJ_FLAG_HIDDEN);
            const std::string tabName = s_modifier_view ? s_modifier_names[i] : s_control_names[i];
            const int tabValue = s_modifier_view
                                     ? (int)(getSettingValue(baseName + tabName, 0.0f) + 0.5f)
                                     : (int)(getSettingValue(tabName, 0.0f) + 0.5f);
            lv_label_set_text_fmt(s_tab_labels[i], "%d", tabValue);

            const bool active = (!s_preset_mode && ((s_modifier_view && s_modifier_index == i) ||
                                                    (!s_modifier_view && s_base_index == i)));
            lv_color_t c = active ? colorFrom565(s_ap_colors_565[i]) : lv_color_hex(0x3A3A3A);
            lv_obj_set_style_bg_color(s_tabs[i], c, LV_PART_MAIN | LV_STATE_DEFAULT);
        } else {
            lv_obj_add_flag(s_tabs[i], LV_OBJ_FLAG_HIDDEN);
        }
    }

    updateCurvePreview(baseName);

    if (s_left_meter_fill && s_left_meter_bg) {
        const int meterH = lv_obj_get_height(s_left_meter_bg) - 4;
        int fill = (meterH * speedInt) / 100;
        if (fill < 2) fill = 2;
        if (fill > meterH) fill = meterH;
        lv_obj_set_height(s_left_meter_fill, fill);
        lv_obj_set_y(s_left_meter_fill, (lv_obj_get_height(s_left_meter_bg) - 2) - fill);
    }
    if (s_right_meter_fill && s_right_meter_bg) {
        const int meterH = lv_obj_get_height(s_right_meter_bg) - 4;
        int fill = (meterH * focusedValue) / 100;
        if (fill < 2) fill = 2;
        if (fill > meterH) fill = meterH;
        lv_obj_set_height(s_right_meter_fill, fill);
        lv_obj_set_y(s_right_meter_fill, (lv_obj_get_height(s_right_meter_bg) - 2) - fill);
    }

    if (!s_modifier_view && s_control_names.size() >= 2) {
        const int panelW = lv_obj_get_width(s_graph_panel);
        const int panelH = lv_obj_get_height(s_graph_panel);
        const int innerW = panelW - 2;
        const float topValue = getSettingValue(s_control_names[0], 50.0f);
        const float bottomValue = getSettingValue(s_control_names[1], 50.0f);
        const int topY = 1 + (int)((1.0f - topValue / 100.0f) * (float)(panelH - 4));
        const int bottomY = 1 + (int)((1.0f - bottomValue / 100.0f) * (float)(panelH - 4));
        setHorizontalGuide(s_top_line, 1, topY, innerW, 0xF860);
        setHorizontalGuide(s_bottom_line, 1, bottomY, innerW, 0xFC00);
        lv_obj_clear_flag(s_top_line, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(s_bottom_line, LV_OBJ_FLAG_HIDDEN);
    } else {
        if (s_top_line) lv_obj_add_flag(s_top_line, LV_OBJ_FLAG_HIDDEN);
        if (s_bottom_line) lv_obj_add_flag(s_bottom_line, LV_OBJ_FLAG_HIDDEN);
    }

    lv_label_set_text_fmt(s_preset_label, "Preset: %s", selectedPreset.c_str());

    if (s_warning_label) {
        if (!liveActive) {
            lv_label_set_text(s_warning_label, "fallback");
            lv_obj_clear_flag(s_warning_label, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(s_warning_label, LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (s_btn_l_label && s_btn_m_label && s_btn_r_label) {
        if (s_preset_mode) {
            lv_label_set_text(s_btn_l_label, "Back");
            lv_label_set_text(s_btn_m_label, "Select");
            lv_label_set_text(s_btn_r_label, "Back");
        } else if (s_modifier_view) {
            lv_label_set_text(s_btn_l_label, "Back");
            lv_label_set_text(s_btn_m_label, s_running ? "Pause" : "Run");
            lv_label_set_text(s_btn_r_label, "Return");
        } else {
            lv_label_set_text(s_btn_l_label, "Presets");
            lv_label_set_text(s_btn_m_label, s_running ? "Pause" : "Run");
            lv_label_set_text(s_btn_r_label, "Modifier");
        }
    }
/*
    if (s_help_label) {
        if (s_preset_mode) {
            lv_label_set_text(s_help_label, "Enc2 Select preset");
        } else if (s_modifier_view) {
            lv_label_set_text(s_help_label, "Enc1 Speed  Enc2 Modifier  Enc4 Value  R: Pattern");
        } else {
            lv_label_set_text(s_help_label, "Enc1 Speed  Enc2 Command  Enc3 Value");
        }
    }
*/
}

static void onScreenLoaded(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_SCREEN_LOADED) return;
    screenmachine(e);
    s_needs_redraw = true;
}

static void createScreenIfNeeded() {
    if (s_screen != nullptr) return;
    s_screen = lv_obj_create(nullptr);
    lv_obj_clear_flag(s_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(s_screen, onScreenLoaded, LV_EVENT_ALL, nullptr);

    lv_obj_set_style_bg_color(s_screen, lv_color_hex(0x0A0A12), LV_PART_MAIN | LV_STATE_DEFAULT);

/*    s_hdr_l = lv_btn_create(s_screen);
    lv_obj_set_size(s_hdr_l, 64, 22);
    lv_obj_set_align(s_hdr_l, LV_ALIGN_TOP_LEFT);
    lv_obj_set_x(s_hdr_l, 8);
    lv_obj_set_y(s_hdr_l, 4);
    lv_obj_set_style_bg_color(s_hdr_l, lv_color_hex(0xDDE6F0), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(s_hdr_l, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    s_hdr_l_label = lv_label_create(s_hdr_l);
    lv_obj_center(s_hdr_l_label);
    lv_label_set_text(s_hdr_l_label, "<<");
    lv_obj_add_event_cb(s_hdr_l, event_hdr_l, LV_EVENT_SHORT_CLICKED, nullptr);

    s_hdr_r = lv_btn_create(s_screen);
    lv_obj_set_size(s_hdr_r, 64, 22);
    lv_obj_set_align(s_hdr_r, LV_ALIGN_TOP_RIGHT);
    lv_obj_set_x(s_hdr_r, -8);
    lv_obj_set_y(s_hdr_r, 4);
    lv_obj_set_style_bg_color(s_hdr_r, lv_color_hex(0xDDE6F0), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(s_hdr_r, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    s_hdr_r_label = lv_label_create(s_hdr_r);
    lv_obj_center(s_hdr_r_label);
    lv_label_set_text(s_hdr_r_label, ">>");
    lv_obj_add_event_cb(s_hdr_r, event_hdr_r, LV_EVENT_SHORT_CLICKED, nullptr);
*/
    s_title_label = lv_label_create(s_screen);
    lv_obj_set_align(s_title_label, LV_ALIGN_TOP_LEFT);
    lv_obj_set_x(s_title_label, 92);
    lv_obj_set_y(s_title_label, 4);
    lv_obj_set_style_text_font(s_title_label, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(s_title_label, &style_title_bar, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(s_title_label, "AP");

    // Battery display (top-right), styled like other M5 remote screens.
    s_batt_label = lv_label_create(s_screen);
    lv_obj_set_width(s_batt_label, 85);
    lv_obj_set_height(s_batt_label, 30);
    lv_obj_set_x(s_batt_label, 115);
    lv_obj_set_y(s_batt_label, -103);
    lv_obj_set_align(s_batt_label, LV_ALIGN_CENTER);
    lv_label_set_text(s_batt_label, T_BATT);

    s_batt_value = lv_label_create(s_batt_label);
    lv_obj_set_width(s_batt_value, LV_SIZE_CONTENT);
    lv_obj_set_height(s_batt_value, LV_SIZE_CONTENT);
    lv_obj_set_x(s_batt_value, 0);
    lv_obj_set_y(s_batt_value, -7);
    lv_obj_set_align(s_batt_value, LV_ALIGN_RIGHT_MID);
    lv_label_set_text(s_batt_value, T_BLANK);

    s_batt_bar = lv_bar_create(s_batt_label);
    lv_bar_set_range(s_batt_bar, 0, 100);
    lv_obj_set_width(s_batt_bar, 80);
    lv_obj_set_height(s_batt_bar, 10);
    lv_obj_set_x(s_batt_bar, 0);
    lv_obj_set_y(s_batt_bar, 10);
    lv_obj_set_align(s_batt_bar, LV_ALIGN_CENTER);
    lv_obj_add_style(s_batt_bar, &style_battery_main, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(s_batt_bar, &style_battery_indicator, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    s_state_label = lv_label_create(s_screen);
    lv_obj_set_align(s_state_label, LV_ALIGN_TOP_RIGHT);
    lv_obj_set_x(s_state_label, -8);
    lv_obj_set_y(s_state_label, 54);
    lv_obj_set_style_text_font(s_state_label, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

    s_transport_label = lv_label_create(s_screen);
    lv_obj_set_align(s_transport_label, LV_ALIGN_TOP_LEFT);
    lv_obj_set_x(s_transport_label, 8);
    lv_obj_set_y(s_transport_label, 54);
    lv_obj_set_style_text_font(s_transport_label, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

    const int tabsY = 32;
    const int tabGap = 4;
    const int tabW = 46;
    for (int i = 0; i < 6; ++i) {
        s_tabs[i] = lv_btn_create(s_screen);
        lv_obj_set_size(s_tabs[i], tabW, 20);
        lv_obj_set_align(s_tabs[i], LV_ALIGN_TOP_LEFT);
        lv_obj_set_x(s_tabs[i], 10 + i * (tabW + tabGap));
        lv_obj_set_y(s_tabs[i], tabsY);
        lv_obj_set_style_radius(s_tabs[i], 4, LV_PART_MAIN | LV_STATE_DEFAULT);

        s_tab_labels[i] = lv_label_create(s_tabs[i]);
        lv_obj_center(s_tab_labels[i]);
        lv_obj_set_style_text_font(s_tab_labels[i], &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(s_tab_labels[i], "0");
    }

    s_graph_panel = lv_obj_create(s_screen);
    lv_obj_set_size(s_graph_panel, 280, 132);
    lv_obj_set_align(s_graph_panel, LV_ALIGN_TOP_LEFT);
    lv_obj_set_x(s_graph_panel, 20);
    lv_obj_set_y(s_graph_panel, 70);
    lv_obj_set_style_bg_color(s_graph_panel, lv_color_hex(0x0F172A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(s_graph_panel, lv_color_hex(0x334155), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s_graph_panel, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(s_graph_panel, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(s_graph_panel, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < (int)(sizeof(s_curve_main_pts) / sizeof(s_curve_main_pts[0])); ++i) {
        s_curve_main_pts[i] = lv_obj_create(s_graph_panel);
        lv_obj_set_size(s_curve_main_pts[i], 2, 2);
        lv_obj_set_style_bg_color(s_curve_main_pts[i], lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(s_curve_main_pts[i], 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(s_curve_main_pts[i], 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_flag(s_curve_main_pts[i], LV_OBJ_FLAG_HIDDEN);
    }
    for (int i = 0; i < (int)(sizeof(s_curve_mod_pts) / sizeof(s_curve_mod_pts[0])); ++i) {
        s_curve_mod_pts[i] = lv_obj_create(s_graph_panel);
        lv_obj_set_size(s_curve_mod_pts[i], 2, 2);
        lv_obj_set_style_bg_color(s_curve_mod_pts[i], lv_color_hex(0x94A3B8), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(s_curve_mod_pts[i], 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(s_curve_mod_pts[i], 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_flag(s_curve_mod_pts[i], LV_OBJ_FLAG_HIDDEN);
    }

    s_bottom_line = lv_obj_create(s_graph_panel);
    lv_obj_set_size(s_bottom_line, 278, 2);
    lv_obj_set_x(s_bottom_line, 1);
    lv_obj_set_y(s_bottom_line, 129);
    lv_obj_set_style_bg_color(s_bottom_line, colorFrom565(0xFC00), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s_bottom_line, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(s_bottom_line, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    s_top_line = lv_obj_create(s_graph_panel);
    lv_obj_set_size(s_top_line, 278, 2);
    lv_obj_set_x(s_top_line, 1);
    lv_obj_set_y(s_top_line, 1);
    lv_obj_set_style_bg_color(s_top_line, colorFrom565(0xF860), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s_top_line, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(s_top_line, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(s_top_line, LV_OBJ_FLAG_HIDDEN);

    s_mid_line = lv_obj_create(s_graph_panel);
    lv_obj_set_size(s_mid_line, 278, 2);
    lv_obj_set_x(s_mid_line, 1);
    lv_obj_set_y(s_mid_line, 65);
    lv_obj_set_style_bg_color(s_mid_line, lv_color_hex(0xC084FC), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s_mid_line, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(s_mid_line, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(s_mid_line, LV_OBJ_FLAG_HIDDEN);

    s_split_line_a = lv_obj_create(s_graph_panel);
    lv_obj_set_size(s_split_line_a, 2, 126);
    lv_obj_set_x(s_split_line_a, 1);
    lv_obj_set_y(s_split_line_a, 2);
    lv_obj_set_style_bg_color(s_split_line_a, colorFrom565(0xFFE0), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s_split_line_a, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(s_split_line_a, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(s_split_line_a, LV_OBJ_FLAG_HIDDEN);

    s_split_line_b = lv_obj_create(s_graph_panel);
    lv_obj_set_size(s_split_line_b, 2, 126);
    lv_obj_set_x(s_split_line_b, 1);
    lv_obj_set_y(s_split_line_b, 2);
    lv_obj_set_style_bg_color(s_split_line_b, colorFrom565(0x07E0), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s_split_line_b, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(s_split_line_b, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(s_split_line_b, LV_OBJ_FLAG_HIDDEN);

    s_left_meter_bg = lv_obj_create(s_screen);
    lv_obj_set_size(s_left_meter_bg, 8, 124);
    lv_obj_set_x(s_left_meter_bg, 6);
    lv_obj_set_y(s_left_meter_bg, 74);
    lv_obj_set_style_bg_color(s_left_meter_bg, lv_color_hex(0x0F172A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(s_left_meter_bg, lv_color_hex(0xF8FAFC), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s_left_meter_bg, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(s_left_meter_bg, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(s_left_meter_bg, LV_OBJ_FLAG_SCROLLABLE);

    s_left_meter_fill = lv_obj_create(s_left_meter_bg);
    lv_obj_set_size(s_left_meter_fill, 4, 20);
    lv_obj_set_x(s_left_meter_fill, 2);
    lv_obj_set_y(s_left_meter_fill, 100);
    lv_obj_set_style_bg_color(s_left_meter_fill, lv_color_hex(0xF8FAFC), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s_left_meter_fill, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(s_left_meter_fill, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(s_left_meter_fill, LV_OBJ_FLAG_SCROLLABLE);

    s_right_meter_bg = lv_obj_create(s_screen);
    lv_obj_set_size(s_right_meter_bg, 8, 124);
    lv_obj_set_x(s_right_meter_bg, 306);
    lv_obj_set_y(s_right_meter_bg, 74);
    lv_obj_set_style_bg_color(s_right_meter_bg, lv_color_hex(0x0F172A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(s_right_meter_bg, lv_color_hex(0xF8FAFC), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s_right_meter_bg, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(s_right_meter_bg, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(s_right_meter_bg, LV_OBJ_FLAG_SCROLLABLE);

    s_right_meter_fill = lv_obj_create(s_right_meter_bg);
    lv_obj_set_size(s_right_meter_fill, 4, 20);
    lv_obj_set_x(s_right_meter_fill, 2);
    lv_obj_set_y(s_right_meter_fill, 100);
    lv_obj_set_style_bg_color(s_right_meter_fill, lv_color_hex(0xF97316), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s_right_meter_fill, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(s_right_meter_fill, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(s_right_meter_fill, LV_OBJ_FLAG_SCROLLABLE);

    s_preset_label = lv_label_create(s_screen);
    lv_obj_set_align(s_preset_label, LV_ALIGN_BOTTOM_LEFT);
    lv_obj_set_x(s_preset_label, 10);
    lv_obj_set_y(s_preset_label, -30);
    lv_obj_set_style_text_font(s_preset_label, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(s_preset_label, "Preset: -");

    s_warning_label = lv_label_create(s_screen);
    lv_obj_set_width(s_warning_label, 100);
    lv_obj_set_align(s_warning_label, LV_ALIGN_TOP_LEFT);
    lv_obj_set_x(s_warning_label, 8);
    lv_obj_set_y(s_warning_label, 6);
    lv_obj_set_style_text_color(s_warning_label, lv_color_hex(0xF59E0B), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(s_warning_label, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_long_mode(s_warning_label, LV_LABEL_LONG_WRAP);
    lv_label_set_text(s_warning_label, "");
    lv_obj_add_flag(s_warning_label, LV_OBJ_FLAG_HIDDEN);

    s_btn_l = lv_btn_create(s_screen);
    lv_obj_set_size(s_btn_l, 100, 24);
    lv_obj_set_align(s_btn_l, LV_ALIGN_BOTTOM_LEFT);
    lv_obj_set_x(s_btn_l, 8);
    lv_obj_set_y(s_btn_l, -4);
    lv_obj_add_style(s_btn_l, &style_button_l, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(s_btn_l, &style_button_l_pressed, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_add_style(s_btn_l, &style_button_l_focused, LV_PART_MAIN | LV_STATE_FOCUSED);
    s_btn_l_label = lv_label_create(s_btn_l);
    lv_obj_center(s_btn_l_label);
    lv_label_set_text(s_btn_l_label, "Presets");
    lv_obj_add_event_cb(s_btn_l, event_btn_l, LV_EVENT_CLICKED, nullptr);

    s_btn_m = lv_btn_create(s_screen);
    lv_obj_set_size(s_btn_m, 100, 24);
    lv_obj_set_align(s_btn_m, LV_ALIGN_BOTTOM_MID);
    lv_obj_set_y(s_btn_m, -4);
    lv_obj_add_style(s_btn_m, &style_button_m, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(s_btn_m, &style_button_m_pressed, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_add_style(s_btn_m, &style_button_m_focused, LV_PART_MAIN | LV_STATE_FOCUSED);
    s_btn_m_label = lv_label_create(s_btn_m);
    lv_obj_center(s_btn_m_label);
    lv_label_set_text(s_btn_m_label, "Pause");
    lv_obj_add_event_cb(s_btn_m, event_btn_m, LV_EVENT_CLICKED, nullptr);

    s_btn_r = lv_btn_create(s_screen);
    lv_obj_set_size(s_btn_r, 100, 24);
    lv_obj_set_align(s_btn_r, LV_ALIGN_BOTTOM_RIGHT);
    lv_obj_set_x(s_btn_r, -8);
    lv_obj_set_y(s_btn_r, -4);
    lv_obj_add_style(s_btn_r, &style_button_r, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(s_btn_r, &style_button_r_pressed, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_add_style(s_btn_r, &style_button_r_focused, LV_PART_MAIN | LV_STATE_FOCUSED);
    s_btn_r_label = lv_label_create(s_btn_r);
    lv_obj_center(s_btn_r_label);
    lv_label_set_text(s_btn_r_label, "Modifier");
    lv_obj_add_event_cb(s_btn_r, event_btn_r, LV_EVENT_CLICKED, nullptr);

    s_help_label = lv_label_create(s_screen);
    lv_obj_set_width(s_help_label, 220);
    lv_obj_set_align(s_help_label, LV_ALIGN_BOTTOM_RIGHT);
    lv_obj_set_x(s_help_label, -8);
    lv_obj_set_y(s_help_label, -30);
    lv_obj_set_style_text_font(s_help_label, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(s_help_label, lv_color_hex(0x94A3B8), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_long_mode(s_help_label, LV_LABEL_LONG_WRAP);
    lv_label_set_text(s_help_label, "AP-Mode loading...");
}

static void resetLocalEncoders() {
    s_enc1 = encoder1.getCount();
    s_enc2 = encoder2.getCount();
    s_enc3 = encoder3.getCount();
    s_enc4 = encoder4.getCount();
}

static bool setSpeedValue(int value) {
    // Port map: mirrors OSSMAdvanced::setSpeed().
    Control *edit = getSetting(speedControlName());
    if (!edit) return false;
    int clamped = value;
    clampValue(&clamped, edit->minValue, edit->maxValue);
    if ((int)(edit->value + 0.5f) == clamped) return false;
    edit->value = (float)clamped;
    if (clamped > 0) s_last_nonzero_speed = clamped;
    s_read_count = -2;
    sendAdvancedControlIfLive(std::to_string(speedControlIndex()) + ":" + std::to_string(clamped) + ",");
    return true;
}

static bool setBaseValue(int value) {
    // Port map: mirrors OSSMAdvanced::setBaseValue().
    if ((int)s_control_names.size() < 2) return false;
    const std::string &baseName = s_control_names[s_base_index];
    Control *edit = getSetting(baseName);
    if (!edit) return false;

    int clamped = value;
    clampValue(&clamped, edit->minValue, edit->maxValue);
    if ((int)(edit->value + 0.5f) == clamped) return false;

    edit->value = (float)clamped;
    if (s_base_index == 0 && s_control_names.size() > 1) {
        Control *other = getSetting(s_control_names[1]);
        if (other) other->maxValue = (uint8_t)clamped;
    }
    if (s_base_index == 1 && s_control_names.size() > 1) {
        Control *other = getSetting(s_control_names[0]);
        if (other) other->minValue = (uint8_t)clamped;
    }

    s_read_count = -2;
    sendAdvancedControlIfLive(std::to_string(s_base_index) + ":" + std::to_string(clamped) + ",");
    return true;
}

static bool setModifierValue(int value) {
    // Port map: mirrors OSSMAdvanced::setModifierValue().
    if ((int)s_control_names.size() < 2 || s_modifier_names.empty()) return false;
    std::string key = s_control_names[s_base_index] + s_modifier_names[s_modifier_index];
    Control *edit = getSetting(key);
    if (!edit) return false;

    int clamped = value;
    clampValue(&clamped, edit->minValue, edit->maxValue);
    if ((int)(edit->value + 0.5f) == clamped) return false;

    edit->value = (float)clamped;
    s_read_count = -2;
    sendAdvancedControlIfLive(std::to_string(s_base_index) + ":" + std::to_string(s_modifier_index) + ":" + std::to_string(clamped) + ",");
    return true;
}

}  // namespace

void APModeSetAddonEnabled(bool enabled) { s_enabled = enabled; }

bool APModeIsAddonEnabled() { return s_enabled; }

lv_obj_t *APModeGetScreen() {
    createScreenIfNeeded();
    return s_screen;
}

bool APModeOwnsActiveScreen() {
    return (s_screen != nullptr) && (lv_scr_act() == s_screen);
}

void APModePrepareScreen() {
    createScreenIfNeeded();
    ensureDefaultModel();
    if (!tryBootstrapModelFromLiveBle(false)) {
        refreshModelFromSyntheticFrames();
    }
    refreshFromLivePresetsIfDue(true);
    normalizeIndexes();
    normalizePresetSelection();
    resetLocalEncoders();
    const int speedNow = (int)(getSettingValue(speedControlName(), 0.0f) + 0.5f);
    s_running = speedNow > 0;
    if (speedNow > 0) s_last_nonzero_speed = speedNow;
    s_last_ui_input_ms = millis();
    s_needs_redraw = true;
}

void APModeHandleScreen(const ButtonEvents &events) {
    if (!s_enabled) {
        _ui_screen_change(ui_Addons, LV_SCR_LOAD_ANIM_FADE_ON, 20, 0);
        return;
    }

    ensureDefaultModel();
    if (!s_live_ap_available && !tryBootstrapModelFromLiveBle(true)) {
        refreshModelFromSyntheticFrames();
    }
    const uint32_t nowMsPreInput = millis();

    // Handle bottom-button intents first so UI navigation and start/stop feel immediate.
    if (events.leftShort) {
        handleLeftShortAction();
    }

    if (events.mxShort) {
        handleMiddleShortAction();
    }

    if (events.rightShort) {
        handleRightShortAction();
        if (!APModeOwnsActiveScreen()) return;
    }

    refreshFromLiveStatusIfDue();
    refreshFromLivePresetsIfDue();
    normalizeIndexes();
    normalizePresetSelection();

    const int d1 = detentsFromEncoder(encoder1, &s_enc1);
    const int d2 = detentsFromEncoder(encoder2, &s_enc2);
    const int d3 = detentsFromEncoder(encoder3, &s_enc3);
    const int d4 = detentsFromEncoder(encoder4, &s_enc4);

    if (d1 != 0) {
        s_last_ui_input_ms = millis();
        Control *speed = getSetting(speedControlName());
        int current = speed ? (int)(speed->value + 0.5f) : 0;
        if (setSpeedValue(current + d1 * 2)) s_needs_redraw = true;
    }

    if (d2 != 0) {
        s_last_ui_input_ms = millis();
        if (s_preset_mode) {
            if (!s_preset_names.empty()) {
                s_preset_selection += d2;
                normalizePresetSelection();
                s_needs_redraw = true;
            }
        } else if (s_modifier_view) {
            int modCount = (int)s_modifier_names.size();
            if (modCount > 0) {
                s_modifier_index += d2;
                if (s_modifier_index < 0) s_modifier_index = modCount - 1;
                if (s_modifier_index >= modCount) s_modifier_index = 0;
                s_needs_redraw = true;
            }
        } else {
            int baseCount = (int)s_control_names.size() - 1;
            if (baseCount > 0) {
                s_base_index += d2;
                if (s_base_index < 0) s_base_index = baseCount - 1;
                if (s_base_index >= baseCount) s_base_index = 0;
                s_modifier_index = 0;
                s_needs_redraw = true;
            }
        }
    }

    if (d3 != 0) {
        s_last_ui_input_ms = millis();
        if (!s_preset_mode && (int)s_control_names.size() > 1) {
            if (s_modifier_view && !s_modifier_names.empty()) {
                std::string key = s_control_names[s_base_index] + s_modifier_names[s_modifier_index];
                Control *mod = getSetting(key);
                int current = mod ? (int)(mod->value + 0.5f) : 0;
                if (setModifierValue(current + d3)) {
                    s_needs_redraw = true;
                }
            } else if (!s_modifier_view) {
                Control *base = getSetting(s_control_names[s_base_index]);
                int current = base ? (int)(base->value + 0.5f) : 0;
                if (setBaseValue(current + d3)) s_needs_redraw = true;
            }
        }
    }

    if (d4 != 0) {
        s_last_ui_input_ms = millis();
        if (!s_preset_mode && (int)s_control_names.size() > 1 && !s_modifier_names.empty()) {
            std::string key = s_control_names[s_base_index] + s_modifier_names[s_modifier_index];
            Control *mod = getSetting(key);
            int current = mod ? (int)(mod->value + 0.5f) : 0;
            if (setModifierValue(current + d4)) {
                s_needs_redraw = true;
            }
        }
    }

    if (s_needs_redraw) {
        drawApScreen();
        s_needs_redraw = false;
    }
}

extern "C" void APModeHandleScreen(const struct ButtonEvents *events) {
    if (!events) return;
    APModeHandleScreen(*events);
}

lv_obj_t *APModeGetBatteryTitleLabel() { return s_batt_label; }
lv_obj_t *APModeGetBatteryValueLabel() { return s_batt_value; }
lv_obj_t *APModeGetBatteryBar() { return s_batt_bar; }
