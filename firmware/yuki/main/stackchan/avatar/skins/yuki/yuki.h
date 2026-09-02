/*
 * SPDX-FileCopyrightText: 2026 Bury Huang
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include "../../avatar/avatar.h"
#include "../../avatar/elements/feature.h"
#include <lvgl.h>
#include <smooth_lvgl.hpp>
#include <memory>

namespace stackchan::avatar {

// Continuous expression state for the Kim-edition two-eye face.
// Heart Engine will eventually write these values directly. The expression
// engine must remain continuous: named states are only parameter presets.
struct ExpressionParameters {
    int eye_width = 88;          // px
    int eye_height = 48;         // px
    int eye_roundness = 18;      // LVGL radius
    int eyelid_open = 100;       // 0=closed, 100=fully open
    int left_rotation = 0;       // LVGL 0.1-degree units
    int right_rotation = 0;
    int asymmetry = 0;           // -100..100; negative favors left, positive right
    int gaze_offset_x = 0;       // -100..100 logical offset
    int gaze_offset_y = 0;
    int gaze_range_x = 18;       // max px generated from gaze input
    int gaze_range_y = 10;
    int gaze_move_speed = 8;     // logical units per update
    int shape_move_speed = 6;    // px/parameter units per update
    int micro_motion = 0;        // 0..20 subtle idle eye movement
    int blink_interval_ms = 3500;
    int blink_duration_ms = 140;
    int neck_yaw = 0;
    int neck_pitch = 90;
    int neck_speed = 120;
};

// Step 2 minimum states from the implementation canon. These are NOT fixed
// faces; each state resolves to ExpressionParameters and can be blended or
// overridden parameter-by-parameter by the future Heart Engine.
enum class ExpressionState {
    Normal,
    Comfort,
    Curiosity,
    Surprise,
    Displeasure,
    Sleepiness,
    Alertness,
};

ExpressionParameters ExpressionPresetForEmotion(const Emotion& emotion);
ExpressionParameters ExpressionPresetForState(ExpressionState state);

class YukiEyes : public Feature {
public:
    YukiEyes(lv_obj_t* parent, bool is_left_eye);

    void setPosition(const uitk::Vector2i& position) override;
    void setWeight(int weight) override;
    void setRotation(int rotation) override;
    void setEmotion(const Emotion& emotion) override;
    void setVisible(bool visible) override;
    void setSize(int size) override;
    void setExpressionParameters(const ExpressionParameters& parameters);
    void _update() override;

private:
    void apply();

    bool is_left_eye_ = false;
    int current_weight_ = 100;
    int current_size_ = 0;
    int current_rotation_ = 0;
    uitk::Vector2i current_position_;
    ExpressionParameters target_expression_;
    ExpressionParameters current_expression_;

    lv_obj_t* container_ = nullptr;
    lv_obj_t* eyelid_ = nullptr;
    lv_obj_t* closed_line_left_ = nullptr;
    lv_obj_t* closed_line_right_ = nullptr;
};

class YukiMouth : public Feature {
public:
    explicit YukiMouth(lv_obj_t* parent);

    void setPosition(const uitk::Vector2i& position) override;
    void setWeight(int weight) override;
    void setRotation(int rotation) override;
    void setEmotion(const Emotion& emotion) override;
    void setVisible(bool visible) override;
    void _update() override;

private:
    void apply();

    int current_weight_ = 0;
    int current_rotation_ = 0;
    uitk::Vector2i current_position_;
    Emotion emotion_ = Emotion::Neutral;

    lv_obj_t* mouth_ = nullptr;
    lv_obj_t* tongue_ = nullptr;
    lv_obj_t* mouth_mask_ = nullptr;
    lv_obj_t* left_corner_ = nullptr;
    lv_obj_t* right_corner_ = nullptr;
};

class YukiSpeechBubble : public SpeechBubble {
public:
    YukiSpeechBubble(lv_obj_t* parent, const lv_font_t* font);

    void setSpeech(std::string_view text) override;
    void clearSpeech() override;
    void setVisible(bool visible) override;
    void setTextFont(void* font) override;

private:
    lv_obj_t* bubble_ = nullptr;
    lv_obj_t* label_ = nullptr;
};

class YukiAvatar : public Avatar {
public:
    void init(lv_obj_t* parent, const lv_font_t* font = &lv_font_montserrat_16);
    void setEmotion(const Emotion& emotion) override;
    void setExpressionParameters(const ExpressionParameters& parameters);
    const ExpressionParameters& getExpressionParameters() const;
    uitk::lvgl_cpp::Container* getPanel() const;

private:
    std::unique_ptr<uitk::lvgl_cpp::Container> panel_;
    ExpressionParameters expression_;
};

void ApplyExpressionState(YukiAvatar& avatar, ExpressionState state);

}  // namespace stackchan::avatar
