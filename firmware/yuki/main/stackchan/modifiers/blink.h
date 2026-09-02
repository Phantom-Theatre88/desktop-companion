/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 * SPDX-FileCopyrightText: 2026 Phantom-Theatre88
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include "../modifiable.h"
#include "../utils/random.h"
#include "../avatar/skins/yuki/yuki.h"
#include "../vision/yuki_vision.h"
#include <hal/hal.h>
#include <algorithm>
#include <cstdint>

namespace stackchan {

class BlinkModifier : public Modifier {
public:
    BlinkModifier(uint32_t destroyAfterMs = 0, uint32_t openIntervalMs = 5200, uint32_t closeIntervalMs = 100)
        : _open_interval_ms(openIntervalMs), _close_interval_ms(closeIntervalMs)
    {
        uint32_t now = GetHAL().millis();
        if (destroyAfterMs > 0) {
            _destroy_at = now + destroyAfterMs;
            _has_lifetime = true;
        }
        _state = State::OPEN;
        _next_state_tick = now + _open_interval_ms;
    }

    void resyncEyeWeights()
    {
        _needs_resync = true;
    }

    void _update(Modifiable& stackchan) override
    {
        if (!stackchan.hasAvatar() || stackchan.avatar().isModifyLocked()) {
            return;
        }

        uint32_t now = GetHAL().millis();

        // Kim expression runtime: the active avatar is always YukiAvatar in this
        // build. Blink timing, subtle idle eye motion, and resting neck pose are
        // therefore driven by the same continuous ExpressionParameters.
        auto& yuki = static_cast<avatar::YukiAvatar&>(stackchan.avatar());
        const auto& expression = yuki.getExpressionParameters();
        _open_interval_ms = static_cast<uint32_t>(std::clamp(expression.blink_interval_ms, 500, 15000));
        _close_interval_ms = static_cast<uint32_t>(std::clamp(expression.blink_duration_ms, 60, 1000));

        // Subtle micro motion exists only while no face is actively being
        // tracked. YukiFaceTrackingModifier runs later and owns gaze when a face
        // is present, so the two systems do not fight each other.
        if (!YukiFaceSeenRecently(1200) && expression.micro_motion > 0 && now >= _next_micro_tick) {
            const int amplitude = std::clamp(expression.micro_motion, 0, 20);
            const int phase = static_cast<int>((now / 180) % 16);
            const int tri = phase < 8 ? phase : 16 - phase;
            const int phase_y = static_cast<int>(((now / 230) + 5) % 16);
            const int tri_y = phase_y < 8 ? phase_y : 16 - phase_y;
            const int dx = (tri - 4) * amplitude / 18;
            const int dy = (tri_y - 4) * amplitude / 24;
            stackchan.avatar().leftEye().setPosition({dx, dy});
            stackchan.avatar().rightEye().setPosition({dx, dy});
            _next_micro_tick = now + 120;
        }

        // Neck angle/speed are part of the Step 2 expression contract. When
        // vision is not tracking a person, expression owns the resting pose.
        if (!YukiFaceSeenRecently(1200) && now >= _next_neck_tick) {
            const int yaw = std::clamp(expression.neck_yaw, -350, 350);
            const int pitch = std::clamp(expression.neck_pitch, 30, 220);
            const int speed = std::clamp(expression.neck_speed, 20, 300);
            if (yaw != _last_neck_yaw || pitch != _last_neck_pitch || speed != _last_neck_speed) {
                stackchan.motion().moveWithSpeed(yaw, pitch, speed);
                _last_neck_yaw = yaw;
                _last_neck_pitch = pitch;
                _last_neck_speed = speed;
            }
            _next_neck_tick = now + 500;
        }

        if (_has_lifetime && now >= _destroy_at) {
            if (_state == State::CLOSED) {
                apply_eye_weights(stackchan, _left_eye_weight, _right_eye_weight);
            }
            requestDestroy();
            return;
        }

        if (_needs_resync) {
            _needs_resync = false;
            _left_eye_weight = stackchan.avatar().leftEye().getWeight();
            _right_eye_weight = stackchan.avatar().rightEye().getWeight();
        }

        if (now >= _next_state_tick) {
            if (_state == State::OPEN) {
                _state = State::CLOSED;
                _next_state_tick = now + _close_interval_ms;
                _left_eye_weight = stackchan.avatar().leftEye().getWeight();
                _right_eye_weight = stackchan.avatar().rightEye().getWeight();
                apply_eye_weights(stackchan, 25, 25);
            } else {
                _state = State::OPEN;
                uint32_t jitter = Random::getInstance().getInt(0, 500);
                _next_state_tick = now + _open_interval_ms + jitter;
                apply_eye_weights(stackchan, _left_eye_weight, _right_eye_weight);
            }
        }
    }

private:
    enum class State { OPEN, CLOSED };

    void apply_eye_weights(Modifiable& stackchan, int left, int right)
    {
        stackchan.avatar().leftEye().setWeight(left);
        stackchan.avatar().rightEye().setWeight(right);
    }

    State _state;
    uint32_t _next_state_tick = 0;
    uint32_t _open_interval_ms;
    uint32_t _close_interval_ms;
    uint32_t _next_micro_tick = 0;
    uint32_t _next_neck_tick = 0;

    uint32_t _destroy_at = 0;
    bool _has_lifetime = false;
    bool _needs_resync = false;
    int _left_eye_weight = 100;
    int _right_eye_weight = 100;
    int _last_neck_yaw = 9999;
    int _last_neck_pitch = 9999;
    int _last_neck_speed = -1;
};

}  // namespace stackchan
