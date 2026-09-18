// MotionControl.cpp — home motion sync, command flush, speed ramp and
// zero-stroke depth jog.
//
// Extracted from ScreenHandler.cpp during the Phase B split.

#include "ScreenHandler.h"
#include "ScreenHandler_internal.h"

#include <lvgl.h>
#include <Arduino.h>
#include <cmath>

#include "../ui/ui.h"
#include "../ui/ui_helpers.h"
#include "../main.h"
#include "../config/debug.h"
#include "../communication/CommManager.h"
#include "../config/config_ids.h"
#include "../communication/BleComm.h"
#include "../display/styles.h"
#include "../buttonhandlers/ButtonHandlers.h"
#include "language.h"

// Zero-stroke depth jog + local motion sync holdoff. Moved from ScreenHandler.cpp.
static constexpr uint32_t LOCAL_MOTION_SYNC_HOLDOFF_MS = 250;
static constexpr bool ENABLE_ZERO_STROKE_DEPTH_JOG = false;
static constexpr float ZERO_STROKE_DEPTH_TOLERANCE = 1.0f;
static constexpr uint32_t ZERO_STROKE_DEPTH_JOG_TIMEOUT_MS = 25000;

static int rangeFromLimit(float limitValue) {
    int limit = (int)(limitValue + 0.5f);
    if (limit < 1) limit = 1;
    return limit;
}

void syncHomeSliderRangesToLimits() {
    if (!ui_homespeedslider || !ui_homedepthslider || !ui_homestrokeslider) return;

    const int speedMax = rangeFromLimit(speedlimit);
    const int depthMax = rangeFromLimit(maxdepthinmm);

    // Keep slider ranges aligned with current transport limits (BLE vs ESP-NOW).
    if (lv_slider_get_max_value(ui_homespeedslider) != speedMax) {
        lv_slider_set_range(ui_homespeedslider, 0, speedMax);
    }
    if (lv_slider_get_max_value(ui_homedepthslider) != depthMax) {
        lv_slider_set_range(ui_homedepthslider, 0, depthMax);
    }
    if (lv_slider_get_max_value(ui_homestrokeslider) != depthMax) {
        lv_slider_set_range(ui_homestrokeslider, 0, depthMax);
    }

    if (speed > speedMax) speed = (float)speedMax;
    if (depth > depthMax) depth = (float)depthMax;
    if (stroke > depthMax) stroke = (float)depthMax;
    if (stroke > depth) stroke = depth;
}

void syncHomeSensationSliderToTransport() {
    if (!ui_homesensationslider) return;

    const bool bleMode = commIsBleMode();
//    const int desiredMin = bleMode ? 0 : -100;
    const int desiredMin = -100;
    const int desiredMax = 100;
    const lv_slider_mode_t desiredMode = bleMode ? LV_SLIDER_MODE_NORMAL : LV_SLIDER_MODE_SYMMETRICAL;

    const bool rangeChanged =
        (lv_slider_get_min_value(ui_homesensationslider) != desiredMin) ||
        (lv_slider_get_max_value(ui_homesensationslider) != desiredMax);

    if (rangeChanged) {
        lv_slider_set_range(ui_homesensationslider, desiredMin, desiredMax);
    }

    if (lv_slider_get_mode(ui_homesensationslider) != desiredMode) {
        lv_slider_set_mode(ui_homesensationslider, desiredMode);
    }

    if (rangeChanged) {
        sensation = bleMode ? 50.0f : 0.0f;
    }

    if (sensation < desiredMin) sensation = (float)desiredMin;
    if (sensation > desiredMax) sensation = (float)desiredMax;
    lv_slider_set_value(ui_homesensationslider, (int)sensation, LV_ANIM_OFF);
}

// Pulls the OSSM-confirmed state (as last reported over BLE, regardless of
// which physically-connected M5 remote caused the change) into the Home
// screen's local UI state — this is what keeps multiple remotes in sync.
// Skipped per-control while that control is being actively driven locally
// (dragged or mid-encoder-turn), and briefly after local input settles, so
// the user's own edit is never stomped by stale confirmed state.
void syncHomeValuesFromOssm(bool speedDragged, bool depthDragged,
                                   bool strokeDragged, bool sensationDragged) {
    BleConfirmedValues confirmed{};
    if (!bleCommGetConfirmedValues(&confirmed)) return;

    const bool localInputActive = (millis() - s_last_local_motion_input_ms) < LOCAL_MOTION_SYNC_HOLDOFF_MS;
    if (localInputActive || speedDragged || depthDragged || strokeDragged || sensationDragged) return;

    if (!speedDragged) {
        if (confirmed.speed > 0.5f) {
            bleCommSetUnpauseSpeed(confirmed.speed);
            speed = confirmed.speed;
            s_last_motion_speed = speed;
        } else {
            // OSSM reports zero while paused, but keep the configured speed
            // visible so MX can resume the same speed without a UI jump.
            const float pausedSpeed = (float)bleCommGetUnpauseSpeed();
            speed = (pausedSpeed > 0.0f) ? pausedSpeed : confirmed.speed;
        }
        if (ui_homespeedslider) {
            lv_slider_set_value(ui_homespeedslider, (int)(speed + 0.5f), LV_ANIM_OFF);
        }
    }

    // Depth and stroke describe one rail range, so apply them together after
    // both controls are released to avoid showing a mixed intermediate range.
    if (!depthDragged && !strokeDragged) {
        depth = confirmed.depth;
        stroke = confirmed.stroke;
        minPos = confirmed.minPosition;
        maxPos = confirmed.maxPosition;
        if (ui_homedepthslider) {
            lv_slider_set_value(ui_homedepthslider, (int)(depth + 0.5f), LV_ANIM_OFF);
        }
    }

    if (!sensationDragged) {
        sensation = confirmed.sensation;
        if (ui_homesensationslider) {
            lv_slider_set_value(ui_homesensationslider, (int)sensation, LV_ANIM_OFF);
        }
    }

    if (ui_PatternS && patternString.length() > 0) {
        // Apply the OSSM-provided catalog before validating the confirmed
        // pattern. OSSM Lite can add entries beyond the standard patterns.
        if (newPatternIsReadFromOSSM) {
            lv_roller_set_options(ui_PatternS, patternString.c_str(), LV_ROLLER_MODE_NORMAL);
            newPatternIsReadFromOSSM = false;
        }

        const uint16_t optionCount = (uint16_t)lv_roller_get_option_count(ui_PatternS);
        if (confirmed.pattern >= 0 && confirmed.pattern < (int)optionCount) {
            pattern = confirmed.pattern;
            lv_roller_set_selected(ui_PatternS, pattern, LV_ANIM_OFF);
            lv_roller_get_selected_str(ui_PatternS, patternstr, sizeof(patternstr));
            if (ui_HomePatternLabel) lv_label_set_text(ui_HomePatternLabel, patternstr);
            if (ui_StrokePatternLabel) lv_label_set_text(ui_StrokePatternLabel, patternstr);
        }
    }
}

// -------------------------------------------------------
// -------------------------------------------------------
// Motion / Manual-Rail Helper Utilities
// -------------------------------------------------------
void syncMotionCommandCache(float motionSpeed, float motionDepth, float motionStroke)
{
    s_last_motion_speed = motionSpeed;
    s_last_motion_depth = motionDepth;
    s_last_motion_stroke = motionStroke;
    s_motion_command_cache_valid = true;
}

void syncHomeMotionUi(bool invertStroke)
{
    if (ui_homedepthslider) {
        lv_slider_set_value(ui_homedepthslider, depth, LV_ANIM_OFF);
    }
    if (ui_homedepthvalue) {
        char depth_v[7];
        dtostrf(depth, 6, 0, depth_v);
        lv_label_set_text(ui_homedepthvalue, depth_v);
    }

    if (ui_homestrokeslider) {
        if (invertStroke) {
            if (lv_bar_get_mode(ui_homestrokeslider) != LV_BAR_MODE_RANGE) {
                lv_bar_set_mode(ui_homestrokeslider, LV_BAR_MODE_RANGE);
            }
            lv_bar_set_start_value(ui_homestrokeslider, depth - stroke, LV_ANIM_OFF);
            lv_slider_set_value(ui_homestrokeslider, depth, LV_ANIM_OFF);
        } else {
            if (lv_bar_get_mode(ui_homestrokeslider) != LV_BAR_MODE_NORMAL) {
                lv_bar_set_mode(ui_homestrokeslider, LV_BAR_MODE_NORMAL);
            }
            lv_bar_set_start_value(ui_homestrokeslider, 0, LV_ANIM_OFF);
            lv_slider_set_value(ui_homestrokeslider, stroke, LV_ANIM_OFF);
        }
    }
    if (ui_homestrokevalue) {
        char stroke_v[7];
        dtostrf(stroke, 6, 0, stroke_v);
        lv_label_set_text(ui_homestrokevalue, stroke_v);
    }
}

void rampHomeStartSpeed(int targetSpeed)
{
    s_home_speed_ramp_active = true;
    s_home_speed_ramp_current = HOME_START_RAMP_THRESHOLD + 1;
    s_home_speed_ramp_target = targetSpeed;
    s_home_speed_ramp_step = 1;
    s_home_speed_ramp_interval_ms = HOME_START_RAMP_INTERVAL_MS;
    s_home_speed_ramp_next_ms = millis() + HOME_START_RAMP_INTERVAL_MS;
}

void rampHomeStopSpeed(int startSpeed)
{
    s_home_speed_ramp_active = true;
    s_home_speed_ramp_current = startSpeed - 1;
    s_home_speed_ramp_target = HOME_START_RAMP_THRESHOLD;
    s_home_speed_ramp_step = -1;
    s_home_speed_ramp_interval_ms = HOME_START_RAMP_INTERVAL_MS / 2;
    s_home_speed_ramp_next_ms = millis() + s_home_speed_ramp_interval_ms;
}

void serviceHomeSpeedRamp()
{
    if (!s_home_speed_ramp_active) return;

    const uint32_t now = millis();
    if (now < s_home_speed_ramp_next_ms) return;

    const bool reachedTarget =
        (s_home_speed_ramp_step > 0 && s_home_speed_ramp_current > s_home_speed_ramp_target) ||
        (s_home_speed_ramp_step < 0 && s_home_speed_ramp_current < s_home_speed_ramp_target);
    if (reachedTarget) {
        s_home_speed_ramp_active = false;
        return;
    }

    SendCommand(SPEED, (float)s_home_speed_ramp_current, OSSM_ID);
    s_home_speed_ramp_current += s_home_speed_ramp_step;
    s_home_speed_ramp_next_ms = now + s_home_speed_ramp_interval_ms;
}

// -------------------------------------------------------
// Zero-Stroke Depth Jog + Motion Command Flush
// -------------------------------------------------------
static void startZeroStrokeDepthJog(float previousDepth, float targetDepth)
{
    s_zero_stroke_depth_jog_active = true;
    s_zero_stroke_depth_target = targetDepth;
    s_zero_stroke_depth_jog_start_ms = millis();
    s_zero_stroke_debug_log_ms = 0;
    if (targetDepth > previousDepth) {
        s_zero_stroke_depth_direction = 1;
    } else if (targetDepth < previousDepth) {
        s_zero_stroke_depth_direction = -1;
    } else {
        s_zero_stroke_depth_direction = 0;
    }
}

void stopZeroStrokeDepthJog()
{
    s_zero_stroke_depth_jog_active = false;
    s_zero_stroke_depth_target = 0.0f;
    s_zero_stroke_depth_jog_start_ms = 0;
    s_zero_stroke_debug_log_ms = 0;
    s_zero_stroke_depth_direction = 0;
}

void serviceZeroStrokeDepthJog()
{
    if (!ENABLE_ZERO_STROKE_DEPTH_JOG) {
        if (s_zero_stroke_depth_jog_active) {
            stopZeroStrokeDepthJog();
        }
        return;
    }

    if (!s_zero_stroke_depth_jog_active) return;

    if (s_manual_rail_length_mm <= 0.0f) {
        LogDebugFormatted("Error depth jogging - no rail length set\n");
        SendCommand(STROKE, 0.0f, OSSM_ID);
        SendCommand(SPEED, 0.0f, OSSM_ID);
        stopZeroStrokeDepthJog();
        return;
    }
    bleCommGetConfirmedPosition(); // refresh BLE state
    const bool stillEligible = commIsBleMode() && speed > 0.5f && stroke <= 0.001f && depth > 0.0f && s_manual_rail_length_mm > 0.0f;
    if (!stillEligible) {
        SendCommand(STROKE, 0.0f, OSSM_ID);
        stopZeroStrokeDepthJog();
        return;
    }

    const bool timedOut = s_zero_stroke_depth_jog_start_ms != 0 &&
                          (millis() - s_zero_stroke_depth_jog_start_ms) > ZERO_STROKE_DEPTH_JOG_TIMEOUT_MS;
    const float currentPositionMm = bleCommGetConfirmedPosition();
    float targetMm = (s_zero_stroke_depth_target * s_manual_rail_length_mm) / 100.0f;
    bool reachedDepth = false;
    if (bleCommHasFreshState() && currentPositionMm >= 0.0f) {
        if (s_zero_stroke_depth_direction > 0) {
//            reachedDepth = currentPositionMm >= targetMm;
            float targetMm = ((s_zero_stroke_depth_target-1) * s_manual_rail_length_mm) / 100.0f;
            reachedDepth = currentPositionMm >= targetMm;
        } else if (s_zero_stroke_depth_direction < 0) {
//            reachedDepth = currentPositionMm <= targetMm;
            float targetMm = ((s_zero_stroke_depth_target+1) * s_manual_rail_length_mm) / 100.0f;
            reachedDepth = currentPositionMm <= targetMm;
        } else {
            reachedDepth = fabsf(currentPositionMm - targetMm) <= ZERO_STROKE_DEPTH_TOLERANCE;
        }
    }

    const uint32_t nowMs = millis();
    if ((nowMs - s_zero_stroke_debug_log_ms) >= 1000U) {
        const char* dir = (s_zero_stroke_depth_direction > 0) ? "out" :
                          (s_zero_stroke_depth_direction < 0) ? "in" : "none";
        LogDebugFormatted(
            "BLE: zero-stroke jog dir=%s targetDepth=%.1f targetMm=%.1f currentMm=%.1f reached=%d timeout=%d\n",
            dir,
            s_zero_stroke_depth_target,
            targetMm,
            currentPositionMm,
            reachedDepth ? 1 : 0,
            timedOut ? 1 : 0);
        s_zero_stroke_debug_log_ms = nowMs;
    }

    if (reachedDepth || timedOut) {
        const float targetDepth = s_zero_stroke_depth_target;
        SendCommand(STROKE, 0.0f, OSSM_ID);
        stopZeroStrokeDepthJog();
        if (timedOut) {
            LogDebugFormatted("BLE: zero-stroke depth jog timeout at target %.1f\n", targetDepth);
        }
    }
}

void flushMotionCommands(float motionSpeed,
                                float motionDepth,
                                float motionStroke,
                                bool  motionValueChanged,
                                bool  allowSend)
{
    if (!motionValueChanged) return;

    if (allowSend && motionValueChanged) {
        const float commandedSpeed = resolveVisualCompensatedSpeed(motionSpeed, motionStroke);
        const bool speedChanged = !s_motion_command_cache_valid || motionSpeed != s_last_motion_speed;
        const bool depthChanged = !s_motion_command_cache_valid || motionDepth != s_last_motion_depth;
        const bool strokeChanged = !s_motion_command_cache_valid || motionStroke != s_last_motion_stroke;
        bool sendSpeedForVsl = !speedChanged &&
                               s_visual_speed_lock && s_visual_speed_ratio_valid &&
                               strokeChanged && motionStroke > 0.001f &&
                               OSSM_On && !s_home_speed_ramp_active;
        const bool wantsZeroStrokeJog = ENABLE_ZERO_STROKE_DEPTH_JOG && commIsBleMode() && motionSpeed > 0.5f && motionStroke <= 0.001f && motionDepth > 0.0f && depthChanged;

        if (wantsZeroStrokeJog && s_manual_rail_length_mm <= 0.0f) {
            LogDebugFormatted("Error depth jogging - no rail length set\n");
            SendCommand(STROKE, 0.0f, OSSM_ID);
            SendCommand(SPEED, 0.0f, OSSM_ID);
            stopZeroStrokeDepthJog();
            syncMotionCommandCache(0.0f, motionDepth, 0.0f);
            return;
        }

        const bool zeroStrokeDepthJog = ENABLE_ZERO_STROKE_DEPTH_JOG && commIsBleMode() && motionSpeed > 0.5f && motionStroke <= 0.001f &&
                        motionDepth > 0.0f && s_manual_rail_length_mm > 0.0f && depthChanged;
        const bool forceRunForZeroStrokeJog = zeroStrokeDepthJog && !OSSM_On;
        const float previousDepth = s_motion_command_cache_valid ? s_last_motion_depth : 0.0f;
        float speedToSend = commandedSpeed;

        if (sendSpeedForVsl && s_visual_speed_last_commanded >= 0.0f) {
            const float delta = speedToSend - s_visual_speed_last_commanded;
            if (fabsf(delta) < 0.1f) {
                sendSpeedForVsl = false;
            } else if (delta > VIS_SPEED_CURVE_MAX_STEP) {
                speedToSend = s_visual_speed_last_commanded + VIS_SPEED_CURVE_MAX_STEP;
            } else if (delta < -VIS_SPEED_CURVE_MAX_STEP) {
                speedToSend = s_visual_speed_last_commanded - VIS_SPEED_CURVE_MAX_STEP;
            }
        }

        if (forceRunForZeroStrokeJog) {
            SendCommand(ON, commandedSpeed, OSSM_ID);
            LogDebugFormatted("BLE: zero-stroke depth jog forcing ON at speed %.1f\n", commandedSpeed);
        } else if ((speedChanged || sendSpeedForVsl) && !s_home_speed_ramp_active) {
            SendCommand(SPEED, speedToSend, OSSM_ID);
            s_visual_speed_last_commanded = speedToSend;
        }
        if (depthChanged) { SendCommand(DEPTH, motionDepth, OSSM_ID); }
        if (zeroStrokeDepthJog) {
            SendCommand(STROKE, 1.0f, OSSM_ID);
            startZeroStrokeDepthJog(previousDepth, motionDepth);
        } else if (strokeChanged) {
            SendCommand(STROKE, motionStroke, OSSM_ID);
            stopZeroStrokeDepthJog();
        }
        if(speedChanged ) {
            LogDebugFormatted("BLE: flushMotionCommands speed %.1f depth %.1f stroke %.1f\n Previous speed: %.1f. Speed changed: %s", motionSpeed, motionDepth, motionStroke, s_last_motion_speed, speedChanged ? "true" : "false");
            if (s_visual_speed_lock && s_visual_speed_ratio_valid && motionStroke > 0.001f) {
                const uint32_t nowMs = millis();
                if ((nowMs - s_visual_speed_log_ms) >= 250U) {
                    LogDebugFormatted("VSL: uiSpeed=%.2f stroke=%.2f cmdSpeed=%.2f\n",
                                      motionSpeed, motionStroke, commandedSpeed);
                    s_visual_speed_log_ms = nowMs;
                }
            }
            if (s_last_motion_speed == 0.0f && commandedSpeed > 0.0f) {
                LogDebugFormatted("BLE: Unpause speed %.1f\n", bleCommGetUnpauseSpeed());
                requestHomeButtonToggleOnce(); // simulate a press of the HomeButtonM to resume motion
//            SendCommand(ON, bleCommGetUnpauseSpeed(), OSSM_ID);
            }
        }

        syncMotionCommandCache(motionSpeed, motionDepth, motionStroke);

    }
}

// -------------------------------------------------------
