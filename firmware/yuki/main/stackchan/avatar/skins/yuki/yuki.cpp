/*
 * SPDX-FileCopyrightText: 2026 Bury Huang
 *
 * SPDX-License-Identifier: MIT
 */
#include "yuki.h"

#include <algorithm>
#include <string>

using namespace stackchan::avatar;
using namespace uitk;
using namespace uitk::lvgl_cpp;

namespace {

constexpr lv_color_t kBackground = LV_COLOR_MAKE(0, 0, 0);
constexpr lv_color_t kEye = LV_COLOR_MAKE(67, 225, 255);
constexpr lv_color_t kEyeDim = LV_COLOR_MAKE(31, 104, 118);
constexpr lv_color_t kMouth = LV_COLOR_MAKE(67, 225, 255);

int approach(int current, int target, int step)
{
    step = std::max(step, 1);
    if (current < target) {
        return std::min(current + step, target);
    }
    if (current > target) {
        return std::max(current - step, target);
    }
    return current;
}

int map_value(int value, int in_min, int in_max, int out_min, int out_max)
{
    return out_min + (value - in_min) * (out_max - out_min) / (in_max - in_min);
}

lv_obj_t* make_shape(lv_obj_t* parent, int width, int height, lv_color_t color, int radius)
{
    lv_obj_t* object = lv_obj_create(parent);
    lv_obj_remove_flag(object, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(object, width, height);
    lv_obj_set_style_bg_color(object, color, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(object, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(object, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(object, radius, LV_PART_MAIN);
    lv_obj_set_style_pad_all(object, 0, LV_PART_MAIN);
    return object;
}

void align_center(lv_obj_t* object, int x, int y)
{
    lv_obj_align(object, LV_ALIGN_CENTER, x, y);
}

ExpressionParameters sanitize_expression(ExpressionParameters parameters)
{
    parameters.eye_width = std::clamp(parameters.eye_width, 28, 128);
    parameters.eye_height = std::clamp(parameters.eye_height, 10, 76);
    parameters.eye_roundness = std::clamp(parameters.eye_roundness, 0, 38);
    parameters.eyelid_open = std::clamp(parameters.eyelid_open, 0, 100);
    parameters.left_rotation = std::clamp(parameters.left_rotation, -300, 300);
    parameters.right_rotation = std::clamp(parameters.right_rotation, -300, 300);
    parameters.asymmetry = std::clamp(parameters.asymmetry, -100, 100);
    parameters.gaze_offset_x = std::clamp(parameters.gaze_offset_x, -100, 100);
    parameters.gaze_offset_y = std::clamp(parameters.gaze_offset_y, -100, 100);
    parameters.gaze_range_x = std::clamp(parameters.gaze_range_x, 0, 36);
    parameters.gaze_range_y = std::clamp(parameters.gaze_range_y, 0, 24);
    parameters.gaze_move_speed = std::clamp(parameters.gaze_move_speed, 1, 40);
    parameters.shape_move_speed = std::clamp(parameters.shape_move_speed, 1, 24);
    parameters.micro_motion = std::clamp(parameters.micro_motion, 0, 20);
    parameters.blink_interval_ms = std::clamp(parameters.blink_interval_ms, 500, 15000);
    parameters.blink_duration_ms = std::clamp(parameters.blink_duration_ms, 60, 1000);
    parameters.neck_yaw = std::clamp(parameters.neck_yaw, -350, 350);
    parameters.neck_pitch = std::clamp(parameters.neck_pitch, 30, 220);
    parameters.neck_speed = std::clamp(parameters.neck_speed, 20, 300);
    return parameters;
}

}  // namespace

ExpressionParameters stackchan::avatar::ExpressionPresetForEmotion(const Emotion& emotion)
{
    ExpressionParameters parameters;

    switch (emotion) {
        case Emotion::Happy:
            parameters.eye_width = 94;
            parameters.eye_height = 44;
            parameters.left_rotation = -35;
            parameters.right_rotation = 35;
            parameters.gaze_move_speed = 10;
            break;
        case Emotion::Angry:
            parameters.eye_width = 94;
            parameters.eye_height = 40;
            parameters.eyelid_open = 82;
            parameters.left_rotation = 105;
            parameters.right_rotation = -105;
            parameters.gaze_move_speed = 12;
            parameters.neck_speed = 150;
            break;
        case Emotion::Sad:
            parameters.eye_width = 84;
            parameters.eye_height = 38;
            parameters.eyelid_open = 78;
            parameters.left_rotation = -70;
            parameters.right_rotation = 70;
            parameters.gaze_range_y = 7;
            parameters.gaze_move_speed = 5;
            parameters.neck_pitch = 112;
            parameters.neck_speed = 70;
            break;
        case Emotion::Doubt:
            parameters.eye_width = 88;
            parameters.eye_height = 44;
            parameters.eyelid_open = 92;
            parameters.left_rotation = 45;
            parameters.right_rotation = -20;
            parameters.asymmetry = 28;
            parameters.gaze_offset_x = 8;
            parameters.gaze_move_speed = 6;
            break;
        case Emotion::Sleepy:
            parameters.eye_width = 92;
            parameters.eye_height = 30;
            parameters.eyelid_open = 42;
            parameters.gaze_range_x = 10;
            parameters.gaze_range_y = 5;
            parameters.gaze_move_speed = 3;
            parameters.shape_move_speed = 3;
            parameters.blink_interval_ms = 1800;
            parameters.blink_duration_ms = 240;
            parameters.neck_pitch = 120;
            parameters.neck_speed = 55;
            break;
        case Emotion::Neutral:
        default:
            break;
    }

    return sanitize_expression(parameters);
}

YukiEyes::YukiEyes(lv_obj_t* parent, bool is_left_eye) : is_left_eye_(is_left_eye)
{
    container_ = make_shape(parent, 88, 48, kEye, 18);
    lv_obj_set_style_transform_pivot_x(container_, 44, LV_PART_MAIN);
    lv_obj_set_style_transform_pivot_y(container_, 24, LV_PART_MAIN);

    eyelid_ = make_shape(container_, 88, 1, kBackground, 14);

    closed_line_left_ = make_shape(parent, 37, 5, kEye, LV_RADIUS_CIRCLE);
    closed_line_right_ = make_shape(parent, 37, 5, kEye, LV_RADIUS_CIRCLE);
    lv_obj_set_style_transform_pivot_x(closed_line_left_, 37, LV_PART_MAIN);
    lv_obj_set_style_transform_pivot_x(closed_line_right_, 0, LV_PART_MAIN);
    lv_obj_set_style_transform_rotation(closed_line_left_, 3580, LV_PART_MAIN);
    lv_obj_set_style_transform_rotation(closed_line_right_, 20, LV_PART_MAIN);
    lv_obj_add_flag(closed_line_left_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(closed_line_right_, LV_OBJ_FLAG_HIDDEN);

    target_expression_ = ExpressionParameters{};
    current_expression_ = target_expression_;
    setWeight(100);
    current_position_ = _position;
    apply();
}

void YukiEyes::setPosition(const Vector2i& position)
{
    Element::setPosition(position);
}

void YukiEyes::setWeight(int weight)
{
    Feature::setWeight(weight);
}

void YukiEyes::setRotation(int rotation)
{
    if (is_left_eye_) {
        target_expression_.left_rotation = rotation;
    } else {
        target_expression_.right_rotation = rotation;
    }
    Element::setRotation(rotation);
}

void YukiEyes::setEmotion(const Emotion& emotion)
{
    if (getIgnoreEmotion()) {
        return;
    }
    setExpressionParameters(ExpressionPresetForEmotion(emotion));
}

void YukiEyes::setExpressionParameters(const ExpressionParameters& parameters)
{
    target_expression_ = sanitize_expression(parameters);
}

void YukiEyes::setVisible(bool visible)
{
    Element::setVisible(visible);
    lv_obj_t* parts[] = {container_, closed_line_left_, closed_line_right_};
    for (auto* part : parts) {
        if (visible) {
            lv_obj_remove_flag(part, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(part, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (visible) {
        apply();
    }
}

void YukiEyes::setSize(int size)
{
    Feature::setSize(size);
}

void YukiEyes::_update()
{
    current_weight_ = _weight;
    current_size_ = _size;

    const int shape_step = target_expression_.shape_move_speed;
    current_expression_.eye_width = approach(current_expression_.eye_width, target_expression_.eye_width, shape_step);
    current_expression_.eye_height = approach(current_expression_.eye_height, target_expression_.eye_height, shape_step);
    current_expression_.eye_roundness = approach(current_expression_.eye_roundness, target_expression_.eye_roundness, shape_step);
    current_expression_.eyelid_open = approach(current_expression_.eyelid_open, target_expression_.eyelid_open, shape_step);
    current_expression_.left_rotation = approach(current_expression_.left_rotation, target_expression_.left_rotation, shape_step * 8);
    current_expression_.right_rotation = approach(current_expression_.right_rotation, target_expression_.right_rotation, shape_step * 8);
    current_expression_.asymmetry = approach(current_expression_.asymmetry, target_expression_.asymmetry, shape_step);
    current_expression_.gaze_offset_x = approach(current_expression_.gaze_offset_x, target_expression_.gaze_offset_x, shape_step);
    current_expression_.gaze_offset_y = approach(current_expression_.gaze_offset_y, target_expression_.gaze_offset_y, shape_step);
    current_expression_.gaze_range_x = approach(current_expression_.gaze_range_x, target_expression_.gaze_range_x, shape_step);
    current_expression_.gaze_range_y = approach(current_expression_.gaze_range_y, target_expression_.gaze_range_y, shape_step);
    current_expression_.gaze_move_speed = target_expression_.gaze_move_speed;
    current_expression_.shape_move_speed = target_expression_.shape_move_speed;
    current_expression_.micro_motion = approach(current_expression_.micro_motion, target_expression_.micro_motion, 1);
    current_expression_.blink_interval_ms = target_expression_.blink_interval_ms;
    current_expression_.blink_duration_ms = target_expression_.blink_duration_ms;
    current_expression_.neck_yaw = target_expression_.neck_yaw;
    current_expression_.neck_pitch = target_expression_.neck_pitch;
    current_expression_.neck_speed = target_expression_.neck_speed;

    current_position_.x = approach(current_position_.x, _position.x, target_expression_.gaze_move_speed);
    current_position_.y = approach(current_position_.y, _position.y, target_expression_.gaze_move_speed);
    current_rotation_ = is_left_eye_ ? current_expression_.left_rotation : current_expression_.right_rotation;
    apply();
}

void YukiEyes::apply()
{
    const int logical_gaze_x = std::clamp(current_position_.x + current_expression_.gaze_offset_x, -100, 100);
    const int logical_gaze_y = std::clamp(current_position_.y + current_expression_.gaze_offset_y, -100, 100);
    const int gaze_x = map_value(logical_gaze_x, -100, 100, -current_expression_.gaze_range_x,
                                 current_expression_.gaze_range_x);
    const int gaze_y = map_value(logical_gaze_y, -100, 100, -current_expression_.gaze_range_y,
                                 current_expression_.gaze_range_y);
    const int base_x = is_left_eye_ ? -62 : 62;
    const int eye_y = -8;

    const int side_asymmetry = is_left_eye_ ? -current_expression_.asymmetry : current_expression_.asymmetry;
    const int eye_width = std::clamp(current_expression_.eye_width + side_asymmetry / 8, 28, 128);
    const int eye_height = std::clamp(current_expression_.eye_height + side_asymmetry / 5, 10, 76);

    lv_obj_set_size(container_, eye_width, eye_height);
    lv_obj_set_style_radius(container_, std::min(current_expression_.eye_roundness, eye_height / 2), LV_PART_MAIN);
    lv_obj_set_style_transform_pivot_x(container_, eye_width / 2, LV_PART_MAIN);
    lv_obj_set_style_transform_pivot_y(container_, eye_height / 2, LV_PART_MAIN);
    align_center(container_, base_x + gaze_x, eye_y + gaze_y);
    lv_obj_set_style_transform_rotation(container_, current_rotation_, LV_PART_MAIN);

    // BlinkModifier still owns _weight.  ExpressionParameters adds a baseline
    // eyelid openness, so sleepiness and blinking multiply rather than fight.
    const int effective_open = std::clamp(current_expression_.eyelid_open * current_weight_ / 100, 0, 100);
    const int covered_height = eye_height * (100 - effective_open) / 100;

    if (covered_height > 0) {
        lv_obj_set_size(eyelid_, eye_width, covered_height);
        lv_obj_set_style_radius(eyelid_, std::min(current_expression_.eye_roundness, eye_height / 2), LV_PART_MAIN);
        lv_obj_align(eyelid_, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_remove_flag(eyelid_, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(eyelid_, LV_OBJ_FLAG_HIDDEN);
    }

    const bool closed = effective_open < 10;
    if (closed) {
        lv_obj_add_flag(container_, LV_OBJ_FLAG_HIDDEN);
        align_center(closed_line_left_, base_x + gaze_x - 19, eye_y + gaze_y);
        align_center(closed_line_right_, base_x + gaze_x + 19, eye_y + gaze_y);
        lv_obj_remove_flag(closed_line_left_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(closed_line_right_, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_remove_flag(container_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(closed_line_left_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(closed_line_right_, LV_OBJ_FLAG_HIDDEN);
    }
}

YukiMouth::YukiMouth(lv_obj_t* parent)
{
    mouth_mask_ = make_shape(parent, 34, 16, kBackground, LV_RADIUS_CIRCLE);
    mouth_ = make_shape(parent, 20, 3, kMouth, LV_RADIUS_CIRCLE);
    tongue_ = make_shape(mouth_, 8, 2, kEyeDim, LV_RADIUS_CIRCLE);
    left_corner_ = make_shape(parent, 3, 3, kMouth, LV_RADIUS_CIRCLE);
    right_corner_ = make_shape(parent, 3, 3, kMouth, LV_RADIUS_CIRCLE);
    lv_obj_add_flag(tongue_, LV_OBJ_FLAG_HIDDEN);

    current_position_ = _position;
    apply();
}

void YukiMouth::setPosition(const Vector2i& position)
{
    Element::setPosition(position);
}

void YukiMouth::setWeight(int weight)
{
    Feature::setWeight(weight);
}

void YukiMouth::setRotation(int rotation)
{
    Element::setRotation(rotation);
}

void YukiMouth::setEmotion(const Emotion& emotion)
{
    emotion_ = emotion;
    switch (emotion) {
        case Emotion::Happy:
            setWeight(10);
            break;
        case Emotion::Angry:
        case Emotion::Sad:
        case Emotion::Doubt:
            setWeight(4);
            break;
        case Emotion::Sleepy:
        case Emotion::Neutral:
        default:
            setWeight(0);
            break;
    }
}

void YukiMouth::setVisible(bool visible)
{
    Element::setVisible(visible);
    lv_obj_t* parts[] = {mouth_mask_, mouth_, left_corner_, right_corner_};
    for (auto* part : parts) {
        if (visible) {
            lv_obj_remove_flag(part, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(part, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void YukiMouth::_update()
{
    current_weight_ = _weight;
    current_rotation_ = approach(current_rotation_, _rotation, 80);
    current_position_.x = approach(current_position_.x, _position.x, 8);
    current_position_.y = approach(current_position_.y, _position.y, 8);
    apply();
}

void YukiMouth::apply()
{
    const int offset_x = map_value(std::clamp(current_position_.x, -100, 100), -100, 100, -8, 8);
    const int offset_y = map_value(std::clamp(current_position_.y, -100, 100), -100, 100, -5, 5);
    const bool speaking = current_weight_ >= 25;
    const int width = speaking ? 18 : 16;
    const int height = speaking ? 8 : 3;
    const int base_y = 54 + offset_y;

    align_center(mouth_mask_, offset_x, base_y);
    lv_obj_set_size(mouth_, width, height);
    align_center(mouth_, offset_x, base_y);
    lv_obj_set_style_transform_rotation(mouth_, current_rotation_, LV_PART_MAIN);

    if (speaking) {
        lv_obj_remove_flag(tongue_, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(tongue_, LV_OBJ_FLAG_HIDDEN);
    }

    align_center(left_corner_, offset_x - width / 2, base_y);
    align_center(right_corner_, offset_x + width / 2, base_y);
}

YukiSpeechBubble::YukiSpeechBubble(lv_obj_t* parent, const lv_font_t* font)
{
    bubble_ = make_shape(parent, 294, 42, LV_COLOR_MAKE(12, 18, 22), 10);
    lv_obj_set_style_bg_opa(bubble_, LV_OPA_90, LV_PART_MAIN);
    lv_obj_set_style_border_width(bubble_, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(bubble_, kEyeDim, LV_PART_MAIN);
    lv_obj_align(bubble_, LV_ALIGN_BOTTOM_MID, 0, -7);

    label_ = lv_label_create(bubble_);
    lv_obj_set_width(label_, 274);
    lv_label_set_long_mode(label_, LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
    lv_obj_set_style_text_color(label_, kEye, LV_PART_MAIN);
    lv_obj_set_style_text_font(label_, font, LV_PART_MAIN);
    lv_obj_center(label_);
    lv_obj_add_flag(bubble_, LV_OBJ_FLAG_HIDDEN);
}

void YukiSpeechBubble::setSpeech(std::string_view text)
{
    std::string owned(text);
    lv_label_set_text(label_, owned.c_str());
    setVisible(!owned.empty());
}

void YukiSpeechBubble::clearSpeech()
{
    lv_label_set_text(label_, "");
    setVisible(false);
}

void YukiSpeechBubble::setVisible(bool visible)
{
    Element::setVisible(visible);
    if (visible) {
        lv_obj_remove_flag(bubble_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(bubble_);
    } else {
        lv_obj_add_flag(bubble_, LV_OBJ_FLAG_HIDDEN);
    }
}

void YukiSpeechBubble::setTextFont(void* font)
{
    if (font) {
        lv_obj_set_style_text_font(label_, static_cast<const lv_font_t*>(font), LV_PART_MAIN);
    }
}

void YukiAvatar::init(lv_obj_t* parent, const lv_font_t* font)
{
    panel_ = std::make_unique<Container>(parent);
    panel_->align(LV_ALIGN_CENTER, 0, 0);
    panel_->setSize(320, 240);
    panel_->setRadius(0);
    panel_->setBorderWidth(0);
    panel_->setBgColor(kBackground);
    panel_->removeFlag(LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* root = panel_->get();

    _key_elements.leftEye = std::make_unique<YukiEyes>(root, true);
    _key_elements.rightEye = std::make_unique<YukiEyes>(root, false);
    _key_elements.mouth = std::make_unique<YukiMouth>(root);
    _key_elements.speechBubble = std::make_unique<YukiSpeechBubble>(root, font);

    setExpressionParameters(ExpressionParameters{});
}

void YukiAvatar::setEmotion(const Emotion& emotion)
{
    // Preserve Yuki's existing emotion contract for the rest of the firmware,
    // but translate it into the new continuous parameter layer.
    Avatar::setEmotion(emotion);
    setExpressionParameters(ExpressionPresetForEmotion(emotion));
}

void YukiAvatar::setExpressionParameters(const ExpressionParameters& parameters)
{
    expression_ = sanitize_expression(parameters);
    static_cast<YukiEyes*>(_key_elements.leftEye.get())->setExpressionParameters(expression_);
    static_cast<YukiEyes*>(_key_elements.rightEye.get())->setExpressionParameters(expression_);
}

const ExpressionParameters& YukiAvatar::getExpressionParameters() const
{
    return expression_;
}

Container* YukiAvatar::getPanel() const
{
    return panel_.get();
}
