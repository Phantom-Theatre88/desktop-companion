/*
 * SPDX-FileCopyrightText: 2026 Phantom-Theatre88
 *
 * SPDX-License-Identifier: MIT
 */
#include "expression_engine.h"

namespace stackchan::avatar {

void YukiExpressionEngine::Init(YukiAvatar& avatar, uint8_t initial_expression_id)
{
    avatar_ = &avatar;
    current_expression_id_ = IsValidExpressionId(initial_expression_id) ? initial_expression_id : 1;
    ApplyCurrent();
}

bool YukiExpressionEngine::SetExpression(uint8_t expression_id)
{
    if (!avatar_ || !IsValidExpressionId(expression_id)) {
        return false;
    }

    current_expression_id_ = expression_id;
    ApplyCurrent();
    return true;
}

bool YukiExpressionEngine::SetExpression(ExpressionId expression_id)
{
    return SetExpression(static_cast<uint8_t>(expression_id));
}

uint8_t YukiExpressionEngine::GetCurrentExpression() const
{
    return current_expression_id_;
}

const ExpressionPreset& YukiExpressionEngine::GetCurrentPreset() const
{
    return GetExpressionPreset(current_expression_id_);
}

bool YukiExpressionEngine::NextExpression()
{
    if (!avatar_) {
        return false;
    }

    uint8_t next = current_expression_id_ + 1;
    if (next > kExpressionPresetCount) {
        next = 1;
    }
    return SetExpression(next);
}

bool YukiExpressionEngine::PreviousExpression()
{
    if (!avatar_) {
        return false;
    }

    uint8_t previous = current_expression_id_ <= 1 ? kExpressionPresetCount : current_expression_id_ - 1;
    return SetExpression(previous);
}

void YukiExpressionEngine::ApplyCurrent()
{
    if (!avatar_) {
        return;
    }
    avatar_->setExpressionParameters(GetExpressionPreset(current_expression_id_).parameters);
}

bool ApplyExpressionPreset(YukiAvatar& avatar, uint8_t expression_id)
{
    if (!IsValidExpressionId(expression_id)) {
        return false;
    }
    avatar.setExpressionParameters(GetExpressionPreset(expression_id).parameters);
    return true;
}

}  // namespace stackchan::avatar
