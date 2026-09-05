/*
 * SPDX-FileCopyrightText: 2026 Phantom-Theatre88
 *
 * SPDX-License-Identifier: MIT
 */
#include "expression_presets.h"

#include <array>

namespace stackchan::avatar {
namespace {

ExpressionParameters base()
{
    return ExpressionParameters{};
}

const std::array<ExpressionPreset, kExpressionPresetCount> kPresets = {
    [] { auto p = base(); return ExpressionPreset{ExpressionId::Exp01, "01", p}; }(),
    [] { auto p = base(); p.eye_width=96; p.eye_height=42; p.eye_roundness=22; p.left_rotation=-35; p.right_rotation=35; p.blink_interval_ms=5000; return ExpressionPreset{ExpressionId::Exp02, "02", p}; }(),
    [] { auto p = base(); p.eye_width=80; p.eye_height=66; p.eye_roundness=31; p.gaze_range_x=24; p.gaze_range_y=16; p.shape_move_speed=12; p.blink_interval_ms=6500; return ExpressionPreset{ExpressionId::Exp03, "03", p}; }(),
    [] { auto p = base(); p.eye_width=88; p.eye_height=46; p.left_rotation=35; p.right_rotation=-10; p.asymmetry=24; p.gaze_offset_x=14; p.gaze_offset_y=-8; p.gaze_move_speed=5; return ExpressionPreset{ExpressionId::Exp04, "04", p}; }(),
    [] { auto p = base(); p.eye_width=94; p.eye_height=30; p.eyelid_open=48; p.gaze_range_x=9; p.gaze_range_y=5; p.gaze_move_speed=3; p.shape_move_speed=3; p.blink_interval_ms=1700; p.blink_duration_ms=260; return ExpressionPreset{ExpressionId::Exp05, "05", p}; }(),
    [] { auto p = base(); p.eye_width=94; p.eye_height=44; p.left_eyelid_open=4; p.right_eyelid_open=100; p.left_rotation=-20; p.right_rotation=25; p.asymmetry=12; return ExpressionPreset{ExpressionId::Exp06, "06", p}; }(),
    [] { auto p = base(); p.eye_width=98; p.eye_height=38; p.eyelid_open=82; p.left_rotation=115; p.right_rotation=-115; p.gaze_move_speed=11; p.shape_move_speed=9; return ExpressionPreset{ExpressionId::Exp07, "07", p}; }(),
    [] { auto p = base(); p.eye_width=82; p.eye_height=58; p.eye_roundness=26; p.gaze_offset_y=8; p.asymmetry=-18; p.shape_move_speed=10; return ExpressionPreset{ExpressionId::Exp08, "08", p}; }(),
    [] { auto p = base(); p.eye_width=84; p.eye_height=38; p.eyelid_open=76; p.left_rotation=-70; p.right_rotation=70; p.gaze_offset_y=10; p.gaze_move_speed=4; return ExpressionPreset{ExpressionId::Exp09, "09", p}; }(),
    [] { auto p = base(); p.eye_width=90; p.eye_height=54; p.eye_roundness=23; p.left_rotation=18; p.right_rotation=-12; p.asymmetry=18; p.gaze_offset_x=10; p.gaze_range_x=24; p.gaze_range_y=14; return ExpressionPreset{ExpressionId::Exp10, "10", p}; }(),
    [] { auto p = base(); p.eye_width=100; p.eye_height=40; p.eye_roundness=20; p.eyelid_open=88; p.left_rotation=-18; p.right_rotation=18; p.gaze_offset_y=-4; return ExpressionPreset{ExpressionId::Exp11, "11", p}; }(),
    [] { auto p = base(); p.eye_width=88; p.eye_height=42; p.eye_roundness=24; p.eyelid_open=86; p.asymmetry=-16; p.gaze_offset_x=-8; p.gaze_offset_y=6; return ExpressionPreset{ExpressionId::Exp12, "12", p}; }(),
    [] { auto p = base(); p.eye_width=96; p.eye_height=36; p.eye_roundness=15; p.eyelid_open=84; p.left_rotation=50; p.right_rotation=-18; p.asymmetry=28; p.gaze_offset_x=18; return ExpressionPreset{ExpressionId::Exp13, "13", p}; }(),
    [] { auto p = base(); p.eye_width=100; p.eye_height=34; p.eye_roundness=14; p.eyelid_open=80; p.left_rotation=25; p.right_rotation=-25; p.gaze_offset_x=-12; p.asymmetry=12; return ExpressionPreset{ExpressionId::Exp14, "14", p}; }(),
    [] { auto p = base(); p.eye_width=98; p.eye_height=50; p.eye_roundness=25; p.left_rotation=-12; p.right_rotation=12; p.gaze_offset_y=-6; p.blink_interval_ms=5600; return ExpressionPreset{ExpressionId::Exp15, "15", p}; }(),
    [] { auto p = base(); p.eye_width=84; p.eye_height=58; p.eye_roundness=22; p.left_rotation=28; p.right_rotation=-28; p.gaze_range_x=27; p.gaze_range_y=17; p.gaze_move_speed=18; p.micro_motion=5; return ExpressionPreset{ExpressionId::Exp16, "16", p}; }(),
    [] { auto p = base(); p.eye_width=96; p.eye_height=32; p.eye_roundness=17; p.eyelid_open=64; p.gaze_offset_x=-20; p.gaze_offset_y=8; p.gaze_move_speed=3; p.micro_motion=1; return ExpressionPreset{ExpressionId::Exp17, "17", p}; }(),
    [] { auto p = base(); p.eye_width=90; p.eye_height=26; p.eyelid_open=34; p.gaze_offset_y=12; p.gaze_move_speed=2; p.shape_move_speed=2; p.blink_interval_ms=1300; p.blink_duration_ms=320; return ExpressionPreset{ExpressionId::Exp18, "18", p}; }(),
    [] { auto p = base(); p.eye_width=92; p.eye_height=34; p.eye_roundness=18; p.eyelid_open=74; p.left_rotation=-55; p.right_rotation=55; p.asymmetry=10; p.blink_interval_ms=2400; return ExpressionPreset{ExpressionId::Exp19, "19", p}; }(),
    [] { auto p = base(); p.eye_width=82; p.eye_height=40; p.eye_roundness=19; p.eyelid_open=72; p.left_rotation=-75; p.right_rotation=75; p.gaze_offset_y=14; p.asymmetry=-8; p.gaze_move_speed=4; return ExpressionPreset{ExpressionId::Exp20, "20", p}; }(),
};

}  // namespace

bool IsValidExpressionId(uint8_t id)
{
    return id >= 1 && id <= kExpressionPresetCount;
}

const ExpressionPreset& GetExpressionPreset(ExpressionId id)
{
    return GetExpressionPreset(static_cast<uint8_t>(id));
}

const ExpressionPreset& GetExpressionPreset(uint8_t id)
{
    if (!IsValidExpressionId(id)) {
        id = 1;
    }
    return kPresets[id - 1];
}

}  // namespace stackchan::avatar
