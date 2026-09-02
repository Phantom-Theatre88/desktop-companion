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

}  // namespace

YukiEyes::YukiEyes(lv_obj_t* parent, bool is_left_eye) : is_left_eye_(is_left_eye)
{
    // The eye itself is now the character.  No portrait image sits underneath it.
    container_ = make_shape(parent, 88, 48, kEye, 18);
    lv_obj_set_style_transform_pivot_x(container_, 44, LV_PART_MAIN);
    lv_obj_set_style_transform_pivot_y(container_, 24, LV_PART_MAIN);

    // Black eyelid sweeps down over the cyan eye during blink/sleep.
    eyelid_ = make_shape(container_, 88, 1, kBackground, 14);

    // Closed-eye line is kept as two segments so emotion can retain a little shape.
    closed_line_left_ = make_shape(parent, 37, 5, kEye, LV_RADIUS_CIRCLE);
    closed_line_right_ = make_shape(parent, 37, 5, kEye, LV_RADIUS_CIRCLE);
    lv_obj_set_style_transform_pivot_x(closed_line_left_, 37, LV_PART_MAIN);
    lv_obj_set_style_transform_pivot_x(closed_line_right_, 0, LV_PART_MAIN);
    lv_obj_set_style_transform_rotation(closed_line_left_, 3580, LV_PART_MAIN);
    lv_obj_set_style_transform_rotation(closed_line_right_, 20, LV_PART_MAIN);
    lv_obj_add_flag(closed_line_left_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(closed_line_right_, LV_OBJ_FLAG_HIDDEN);

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
    Element::setRotation(rotation);
}

void YukiEyes::setEmotion(const Emotion& emotion)
{
    if (getIgnoreEmotion()) {
        return;
    }

    switch (emotion) {
        case Emotion::Happy:
            setWeight(100);
            setRotation(is_left_eye_ ? -35 : 35);
            break;
        case Emotion::Angry:
            setWeight(100);
            setRotation(is_left_eye_ ? 105 : -105);
            break;
        case Emotion::Sad:
            setWeight(78);
            setRotation(is_left_eye_ ? -70 : 70);
            break;
        case Emotion::Doubt:
            setWeight(is_left_eye_ ? 72 : 100);
            setRotation(is_left_eye_ ? 45 : -20);
            break;
        case Emotion::Sleepy:
            setWeight(28);
            setRotation(0);
            break;
        case Emotion::Neutral:
        default:
            setWeight(100);
            setRotation(0);
            break;
    }
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
    current_rotation_ = approach(current_rotation_, _rotation, 45);
    current_position_.x = approach(current_position_.x, _position.x, 8);
    current_position_.y = approach(current_position_.y, _position.y, 8);
    apply();
}

void YukiEyes::apply()
{
    const int gaze_x = map_value(std::clamp(current_position_.x, -100, 100), -100, 100, -18, 18);
    const int gaze_y = map_value(std::clamp(current_position_.y, -100, 100), -100, 100, -10, 10);
    const int base_x = is_left_eye_ ? -62 : 62;
    const int eye_y = -8;

    align_center(container_, base_x + gaze_x, eye_y + gaze_y);
    lv_obj_set_style_transform_rotation(container_, current_rotation_, LV_PART_MAIN);

    // Weight is already used by Yuki's BlinkModifier, so keep that contract and
    // translate it into an eyelid amount instead of replacing the blink system.
    const int covered_height = current_weight_ >= 80 ? 0 :
                               (current_weight_ >= 45 ? 14 :
                                (current_weight_ >= 18 ? 30 : 48));

    if (covered_height > 0) {
        lv_obj_set_size(eyelid_, 88, covered_height);
        lv_obj_align(eyelid_, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_remove_flag(eyelid_, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(eyelid_, LV_OBJ_FLAG_HIDDEN);
    }

    const bool closed = current_weight_ < 18;
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
    // Mouth is deliberately subordinate to the eyes: a tiny cyan line that only
    // opens while speech weight rises.
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
    // Keep neutral almost mouthless. Speech animation can still raise weight.
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

    // Kim edition: no anime portrait.  The black panel and procedural eyes are
    // the face, while Yuki's existing blink/gaze/emotion hooks remain intact.
    _key_elements.leftEye = std::make_unique<YukiEyes>(root, true);
    _key_elements.rightEye = std::make_unique<YukiEyes>(root, false);
    _key_elements.mouth = std::make_unique<YukiMouth>(root);
    _key_elements.speechBubble = std::make_unique<YukiSpeechBubble>(root, font);
}

void YukiAvatar::setEmotion(const Emotion& emotion)
{
    Avatar::setEmotion(emotion);
}

Container* YukiAvatar::getPanel() const
{
    return panel_.get();
}
