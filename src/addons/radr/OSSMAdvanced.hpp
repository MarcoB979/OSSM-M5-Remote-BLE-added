#pragma once

// OSSMAdvanced.hpp — self-contained port of the RADR "Advanced Penetration"
// device logic from:
//   fray-d/radr-wireless-remote (Advanced-Penetration branch)
//   Software/src/devices/kinkymakers/ossm/advancedPenetration.hpp
//
// The original file is built on the RADR framework (Device base class,
// boost::sml state machine, TextButton/EncoderBar widgets, GFXcanvas16 + tft).
// To keep this file re-copyable from upstream, those framework bits are
// stripped: all parsing/value/curve logic is kept verbatim-ish, and rendering
// emits plain point/line data instead of drawing to a canvas. The M5 layer
// (AP-v2.cpp) feeds this class values and draws the returned data.

#include <Arduino.h>
#include <cmath>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

class OSSMAdvanced {
  public:
    struct Control {
        float value = 0.0f;
        uint8_t minValue = 0;
        uint8_t maxValue = 100;
        Control() = default;
        Control(float v, uint8_t mn, uint8_t mx) : value(v), minValue(mn), maxValue(mx) {}
    };

    uint8_t baseIndex = 0;
    uint8_t modifierIndex = 0;
    bool isPaused = false;

    std::unordered_map<std::string, Control> advancedSettings;
    std::vector<std::string> controlNames;
    std::vector<std::string> modifierNames;
    // RADR advancedColors (RGB565).
    std::vector<uint16_t> advancedColors = {0xf860, 0xfc00, 0xffe0, 0x07e0, 0x001f, 0xa87d, 0xf81f};

    // BLE transport hook — set by the M5 layer.
    std::function<bool(const std::string &)> sendControlFn = nullptr;

    // ---- Parsing (verbatim logic from RADR) ----
    void parseConfig(std::string configValues) {
        controlNames.clear();
        modifierNames.clear();
        advancedSettings.clear();
        configValues += ',';
        std::string modifierString;
        int8_t ci = (int8_t)configValues.find(',', 0);
        while (ci > 0) {
            std::string singleConfig = configValues.substr(0, ci);
            uint8_t j = (uint8_t)singleConfig.find('(');
            uint8_t k = (uint8_t)singleConfig.find('/');
            uint8_t l = (uint8_t)singleConfig.find(')');
            std::string name = singleConfig.substr(0, j);
            controlNames.emplace_back(name);
            uint8_t minValue = (uint8_t)std::stoi(singleConfig.substr(j + 1, k - j - 1));
            uint8_t maxValue = (uint8_t)std::stoi(singleConfig.substr(k + 1, l - k - 1));
            Control newControl{float(minValue), minValue, maxValue};
            advancedSettings[name] = newControl;

            modifierString.clear();
            int8_t mi = (int8_t)singleConfig.find(':');
            if (mi > 0) modifierString = singleConfig.substr(l + 2) + ':';
            mi = (int8_t)modifierString.find(':');
            std::string iterString = modifierString;
            while (mi > 0) {
                j = (uint8_t)iterString.find('(');
                k = (uint8_t)iterString.find('/');
                l = (uint8_t)iterString.find(')');
                std::string modifierName = iterString.substr(0, j);
                if (std::find(modifierNames.begin(), modifierNames.end(), modifierName) == modifierNames.end()) {
                    modifierNames.emplace_back(modifierName);
                }
                newControl.minValue = (uint8_t)std::stoi(iterString.substr(j + 1, k - j - 1));
                newControl.maxValue = (uint8_t)std::stoi(iterString.substr(k + 1, l - k - 1));
                newControl.value = newControl.minValue;
                if (modifierName == modifierNames[0]) newControl.value = newControl.maxValue;
                advancedSettings[name + modifierName] = newControl;
                iterString = iterString.substr(mi + 1);
                mi = (int8_t)iterString.find(':');
            }
            configValues = configValues.substr(ci + 1);
            ci = (int8_t)configValues.find(',', 0);
        }
    }

    void parseStatus(std::string statusString) {
        int8_t controlCounter = 0;
        statusString += ',';
        int8_t si = (int8_t)statusString.find(',', 0);
        while (si > 0) {
            std::string singleStatus = statusString.substr(0, si);
            float value = std::stof(singleStatus.substr(0, singleStatus.find(':')));
            advancedSettings[controlNames[controlCounter]].value = value;

            int8_t mi = (int8_t)singleStatus.find(':');
            int8_t modifierCounter = 0;
            if (mi == -1) {
                advancedSettings[controlNames[controlCounter] + modifierNames[0]].value = 100;
            }
            while (mi > 0) {
                singleStatus = singleStatus.substr(mi + 1);
                value = std::stof(singleStatus.substr(0, singleStatus.find(':')));
                advancedSettings[controlNames[controlCounter] + modifierNames[modifierCounter]].value = value;
                modifierCounter++;
                mi = (int8_t)singleStatus.find(':');
            }
            statusString = statusString.substr(si + 1);
            si = (int8_t)statusString.find(',', 0);
            controlCounter++;
        }
    }

    // ---- Setters ----
    bool setSpeed(uint8_t speed) {
        Control *edit = &advancedSettings["Speed"];
        speed = constrain(speed, edit->minValue, edit->maxValue);
        if (speed == (uint8_t)edit->value) return true;
        edit->value = speed;
        if (sendControlFn) sendControlFn(std::string("6:") + std::to_string(speed) + ",");
        return true;
    }

    bool setBaseValue(uint8_t value) {
        Control *edit = &advancedSettings[controlNames[baseIndex]];
        value = constrain(value, edit->minValue, edit->maxValue);
        if (value == (uint8_t)edit->value) return true;
        edit->value = value;
        if (baseIndex == 0) advancedSettings[controlNames[1]].maxValue = value;
        if (baseIndex == 1) advancedSettings[controlNames[0]].minValue = value;
        if (sendControlFn) sendControlFn(std::to_string(baseIndex) + ":" + std::to_string(value) + ",");
        return true;
    }

    bool setModifierValue(uint8_t value) {
        std::string c = controlNames[baseIndex] + modifierNames[modifierIndex];
        Control *edit = &advancedSettings[c];
        value = constrain(value, edit->minValue, edit->maxValue);
        if (value == (uint8_t)edit->value) return true;
        edit->value = value;
        if (sendControlFn) sendControlFn(std::to_string(baseIndex) + ":" + std::to_string(modifierIndex) + ":" +
                                         std::to_string(value) + ",");
        return true;
    }

    // ---- Navigation (RADR shoulder buttons) ----
    void onLeftBumperClick() {
        if (modifierIndex == 0xFF) return;  // base view marker (see isModifierView)
        // handled by the layer via mode; kept for interface parity
    }
    void onRightBumperClick() {}

    // ---- Tab / value accessors for the layer ----
    int baseTabCount() const { return (int)controlNames.size() - 1; }
    int modifierTabCount() const { return (int)modifierNames.size(); }

    float speedValue() const {
        auto it = advancedSettings.find("Speed");
        return it == advancedSettings.end() ? 0.0f : it->second.value;
    }

    float baseValue(uint8_t i) const {
        return i < controlNames.size() ? advancedSettings.at(controlNames[i]).value : 0.0f;
    }
    float modifierValue(uint8_t i) const {
        return advancedSettings.at(controlNames[baseIndex] + modifierNames[i]).value;
    }
    float selectedValue() const {
        return advancedSettings.at(controlNames[baseIndex] + modifierNames[modifierIndex]).value;
    }
    float selectedBaseValue() const { return baseValue(baseIndex); }

    const char *selectedName() const {
        return controlNames[baseIndex].c_str();
    }
    uint16_t selectedColor() const {
        return advancedColors[baseIndex % advancedColors.size()];
    }

    // ---- Curve math (verbatim from RADR) ----
    float bezierMath(float v0, float v1, float v2, float v3, float t) {
        float output = std::pow(1.0f - t, 3.0f) * v0;
        output += 3.0f * std::pow(1.0f - t, 2.0f) * t * v1;
        output += 3.0f * (1.0f - t) * std::pow(t, 2.0f) * v2;
        output += std::pow(t, 3.0f) * v3;
        return output;
    }

    // Rendering output (canvas coords 300x152; the layer offsets by (10,60)).
    struct Pt { int16_t x; int16_t y; };
    struct Polyline {
        std::vector<Pt> pts;
        uint16_t color565 = 0xFFFF;  // RGB565
        bool dashed = false;
        bool segments = false;  // true = draw as independent 2-point segments (traces)
    };
    struct Render {
        std::vector<Polyline> lines;
        // Guide line (the selected control's position marker).
        bool hasGuide = false;
        int16_t guideX = 0, guideY = 0, guideW = 0, guideH = 0;
        uint16_t guideColor = 0xFFFF;
    };

    float getMaxSteps() {
        uint8_t maxSteps = 4;
        for (const std::string &controlName : controlNames) {
            uint8_t modSteps = 0;
            for (uint8_t m = 1; m + 2 < controlNames.size(); m++) {
                modSteps += (uint8_t)advancedSettings[controlName + modifierNames[m]].value;
            }
            if (modSteps > maxSteps) maxSteps = modSteps;
        }
        return (float)maxSteps;
    }

    // Port of drawSingleModifier() — emits one colored trace.
    Polyline buildSingleModifier(uint8_t c, int16_t width = 300, int16_t height = 150) {
        Polyline line;
        line.color565 = advancedColors[c % advancedColors.size()];
        line.segments = true;

        const float stepWidth = width / getMaxSteps();
        const Control control = advancedSettings[controlNames[c]];
        const float baseValueRatio = 1.0f - control.value / 100.0f;
        const float modValueRatio = 1.0f - advancedSettings[controlNames[c] + modifierNames[0]].value / 100.0f;
        float strokeRatio = 1.0f - baseValueRatio;
        if (c < 2) {
            strokeRatio = (advancedSettings[controlNames[0]].value - advancedSettings[controlNames[1]].value) / 100.0f;
        }
        const uint16_t baseY = (uint16_t)(height * baseValueRatio);
        uint16_t modY = (uint16_t)(baseY + height * strokeRatio * modValueRatio);
        if (c == 1) modY = (uint16_t)(baseY - height * strokeRatio * modValueRatio);

        int startX = (int)(-(int16_t)(stepWidth * advancedSettings[controlNames[c] + modifierNames[5]].value));
        int m = 0;
        while (startX < width) {
            const uint16_t step = (uint16_t)(advancedSettings[controlNames[c] + modifierNames[m + 1]].value * stepWidth);
            switch (m) {
                case 0: addSegment(line, startX, baseY, startX + step, modY); break;
                case 1: addSegment(line, startX, modY, startX + step, modY); break;
                case 2: addSegment(line, startX, modY, startX + step, baseY); break;
                case 3: addSegment(line, startX, baseY, startX + step, baseY); break;
            }
            startX += step;
            m = (m + 1) % 4;
        }
        return line;
    }

    // Port of drawModifierDisplay().
    Render buildModifierDisplay() {
        Render r;
        const int count = (int)controlNames.size() - 1;
        for (int c = 0; c < count; c++) {
            if (c == baseIndex) continue;
            r.lines.push_back(buildSingleModifier(c));
        }
        r.lines.push_back(buildSingleModifier(baseIndex));  // selected trace on top
        return r;
    }

    // Port of drawCurveDisplay().
    Render buildCurveDisplay() {
        Render r;
        const float y1 = 150.0f - advancedSettings[controlNames[0]].value * 1.5f;
        const float y0 = 150.0f - advancedSettings[controlNames[1]].value * 1.5f;
        const float diff = y0 - y1;
        const float y1m = (1.0f - advancedSettings[controlNames[0] + modifierNames[0]].value / 100.0f) * diff + y1;
        const float y0m = y0 - (1.0f - advancedSettings[controlNames[1] + modifierNames[0]].value / 100.0f) * diff;

        const float a = advancedSettings[controlNames[2]].value;
        const float am = advancedSettings[controlNames[2] + modifierNames[0]].value / 100.0f * a;
        const float b = advancedSettings[controlNames[3]].value;
        const float bm = advancedSettings[controlNames[3] + modifierNames[0]].value / 100.0f * b;
        const uint16_t x = (uint16_t)((1.0f - (a / b) / (a / b + 1.0f)) * 300.0f);
        const uint16_t xm = (uint16_t)((1.0f - (am / bm) / (am / bm + 1.0f)) * 300.0f);

        const float c = advancedSettings[controlNames[4]].value;
        const float cm = advancedSettings[controlNames[4] + modifierNames[0]].value / 100.0f * c;
        const float d = advancedSettings[controlNames[5]].value;
        const float dm = advancedSettings[controlNames[5] + modifierNames[0]].value / 100.0f * d;

        uint16_t inColor = 0xFFFF;
        uint16_t outColor = 0xFFFF;

        // Guide line for the selected control.
        switch (baseIndex) {
            case 0:
                r.guideX = 0; r.guideY = (int16_t)y1; r.guideW = 300; r.guideH = 1;
                r.guideColor = advancedColors[baseIndex];
                if (y1 != y1m) { Polyline g; g.color565 = advancedColors[baseIndex]; g.dashed = true;
                    for (uint16_t p = 0; p <= 300; p += 5) g.pts.push_back({(int16_t)p, (int16_t)y1m});
                    r.lines.push_back(g); }
                break;
            case 1:
                r.guideX = 0; r.guideY = (int16_t)y0; r.guideW = 300; r.guideH = 1;
                r.guideColor = advancedColors[baseIndex];
                if (y0 != y0m) { Polyline g; g.color565 = advancedColors[baseIndex]; g.dashed = true;
                    for (uint16_t p = 0; p <= 300; p += 5) g.pts.push_back({(int16_t)p, (int16_t)y0m});
                    r.lines.push_back(g); }
                break;
            case 2:
            case 3:
                r.guideX = (int16_t)x; r.guideY = 0; r.guideW = 1; r.guideH = 150;
                r.guideColor = advancedColors[baseIndex];
                if (x != xm) { Polyline g; g.color565 = advancedColors[baseIndex]; g.dashed = true;
                    for (uint16_t p = 0; p <= 150; p += 5) g.pts.push_back({(int16_t)xm, (int16_t)p});
                    r.lines.push_back(g); }
                break;
            case 4: inColor = advancedColors[baseIndex]; break;
            case 5: outColor = advancedColors[baseIndex]; break;
        }
        r.hasGuide = true;

        // Main curve: rise + fall.
        Polyline mainLine;
        mainLine.color565 = 0xFFFF;
        for (uint16_t p = 0; p <= x; p++) {
            float t = x > 0 ? p / (float)x : 0.0f;
            int16_t px = (int16_t)std::floor(bezierMath(0, 0 + (x - 0) * (0.1f + 0.4f * (1.0f - c / 100.0f)), x - (x - 0) * (0.1f + 0.4f * (1.0f - c / 100.0f)), x, t));
            int16_t py = (int16_t)std::floor(bezierMath(y0, y0, y1, y1, t));
            mainLine.pts.push_back({px, py});
        }
        for (uint16_t p = 0; p <= (300 - x); p++) {
            float t = (300 - x) > 0 ? p / (float)(300 - x) : 0.0f;
            int16_t px = (int16_t)std::floor(bezierMath(300, 300 - (300 - x) * (0.1f + 0.4f * (1.0f - d / 100.0f)), x + (300 - x) * (0.1f + 0.4f * (1.0f - d / 100.0f)), x, t));
            int16_t py = (int16_t)std::floor(bezierMath(y0, y0, y1, y1, t));
            mainLine.pts.push_back({px, py});
        }
        r.lines.push_back(mainLine);

        // Modifier curve (dashed) when it differs.
        if (y0 != y0m || y1 != y1m) {
            Polyline modLine;
            modLine.color565 = 0xFFFF;
            modLine.dashed = true;
            for (uint16_t p = 0; p <= xm; p += 5) {
                float t = xm > 0 ? p / (float)xm : 0.0f;
                int16_t px = (int16_t)std::floor(bezierMath(0, 0 + xm * (0.1f + 0.4f * (1.0f - cm / 100.0f)), xm - xm * (0.1f + 0.4f * (1.0f - cm / 100.0f)), xm, t));
                int16_t py = (int16_t)std::floor(bezierMath(y0m, y0m, y1m, y1m, t));
                modLine.pts.push_back({px, py});
            }
            for (uint16_t p = 0; p <= (300 - xm); p += 5) {
                float t = (300 - xm) > 0 ? p / (float)(300 - xm) : 0.0f;
                int16_t px = (int16_t)std::floor(bezierMath(300, 300 - (300 - xm) * (0.1f + 0.4f * (1.0f - dm / 100.0f)), xm + (300 - xm) * (0.1f + 0.4f * (1.0f - dm / 100.0f)), xm, t));
                int16_t py = (int16_t)std::floor(bezierMath(y0m, y0m, y1m, y1m, t));
                modLine.pts.push_back({px, py});
            }
            r.lines.push_back(modLine);
        }
        return r;
    }

  private:
    static void addSegment(Polyline &line, int x0, int y0, int x1, int y1) {
        line.pts.push_back({(int16_t)x0, (int16_t)y0});
        line.pts.push_back({(int16_t)x1, (int16_t)y1});
    }
};
