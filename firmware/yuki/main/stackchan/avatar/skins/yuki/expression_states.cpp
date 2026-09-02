/*
 * SPDX-FileCopyrightText: 2026 Phantom-Theatre88
 *
 * SPDX-License-Identifier: MIT
 */
#include "yuki.h"

namespace stackchan::avatar {

ExpressionParameters ExpressionPresetForState(ExpressionState state)
{
    ExpressionParameters p;

    switch (state) {
        case ExpressionState::Comfort:
            p.eye_width = 96;
            p.eye_height = 42;
            p.eye_roundness = 20;
            p.eyelid_open = 94;
            p.left_rotation = -20;
            p.right_rotation = 20;
            p.gaze_range_x = 15;
            p.gaze_range_y = 8;
            p.gaze_move_speed = 6;
            p.shape_move_speed = 5;
            p.micro_motion = 2;
            p.blink_interval_ms = 5200;
            p.blink_duration_ms = 160;
            p.neck_pitch = 95;
            p.neck_speed = 70;
            break;

        case ExpressionState::Curiosity:
            p.eye_width = 90;
            p.eye_height = 52;
            p.eye_roundness = 22;
            p.eyelid_open = 100;
            p.left_rotation = 20;
            p.right_rotation = -10;
            p.asymmetry = 20;
            p.gaze_offset_x = 8;
            p.gaze_range_x = 22;
            p.gaze_range_y = 13;
            p.gaze_move_speed = 11;
            p.shape_move_speed = 7;
            p.micro_motion = 4;
            p.blink_interval_ms = 3200;
            p.blink_duration_ms = 110;
            p.neck_yaw = 35;
            p.neck_pitch = 78;
            p.neck_speed = 115;
            break;

        case ExpressionState::Surprise:
            p.eye_width = 78;
            p.eye_height = 64;
            p.eye_roundness = 30;
            p.eyelid_open = 100;
            p.gaze_range_x = 25;
            p.gaze_range_y = 17;
            p.gaze_move_speed = 17;
            p.shape_move_speed = 12;
            p.micro_motion = 6;
            p.blink_interval_ms = 6500;
            p.blink_duration_ms = 80;
            p.neck_pitch = 62;
            p.neck_speed = 185;
            break;

        case ExpressionState::Displeasure:
            p.eye_width = 98;
            p.eye_height = 36;
            p.eye_roundness = 14;
            p.eyelid_open = 78;
            p.left_rotation = 110;
            p.right_rotation = -110;
            p.gaze_range_x = 17;
            p.gaze_range_y = 7;
            p.gaze_move_speed = 9;
            p.shape_move_speed = 8;
            p.micro_motion = 2;
            p.blink_interval_ms = 2800;
            p.blink_duration_ms = 90;
            p.neck_pitch = 86;
            p.neck_speed = 120;
            break;

        case ExpressionState::Sleepiness:
            p.eye_width = 94;
            p.eye_height = 28;
            p.eye_roundness = 18;
            p.eyelid_open = 40;
            p.gaze_range_x = 9;
            p.gaze_range_y = 5;
            p.gaze_move_speed = 3;
            p.shape_move_speed = 3;
            p.micro_motion = 1;
            p.blink_interval_ms = 1500;
            p.blink_duration_ms = 280;
            p.neck_pitch = 125;
            p.neck_speed = 45;
            break;

        case ExpressionState::Alertness:
            p.eye_width = 84;
            p.eye_height = 56;
            p.eye_roundness = 20;
            p.eyelid_open = 100;
            p.left_rotation = 30;
            p.right_rotation = -30;
            p.gaze_range_x = 27;
            p.gaze_range_y = 17;
            p.gaze_move_speed = 19;
            p.shape_move_speed = 9;
            p.micro_motion = 6;
            p.blink_interval_ms = 6000;
            p.blink_duration_ms = 85;
            p.neck_pitch = 70;
            p.neck_speed = 175;
            break;

        case ExpressionState::Normal:
        default:
            p.eye_width = 88;
            p.eye_height = 48;
            p.eye_roundness = 18;
            p.eyelid_open = 100;
            p.gaze_range_x = 18;
            p.gaze_range_y = 10;
            p.gaze_move_speed = 8;
            p.shape_move_speed = 6;
            p.micro_motion = 2;
            p.blink_interval_ms = 4200;
            p.blink_duration_ms = 120;
            p.neck_pitch = 90;
            p.neck_speed = 100;
            break;
    }

    return p;
}

void ApplyExpressionState(YukiAvatar& avatar, ExpressionState state)
{
    avatar.setExpressionParameters(ExpressionPresetForState(state));
}

}  // namespace stackchan::avatar
