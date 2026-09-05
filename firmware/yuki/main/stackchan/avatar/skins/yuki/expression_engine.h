/*
 * SPDX-FileCopyrightText: 2026 Phantom-Theatre88
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include "expression_presets.h"

#include <cstdint>

namespace stackchan::avatar {

// Expression Engine v1, Steps 1-4:
// - owns the stable 1..20 selection
// - applies a preset directly to the existing Yuki two-eye renderer
// - no Heart Engine / Vision / event effects / color-glow timing yet
class YukiExpressionEngine {
public:
    void Init(YukiAvatar& avatar, uint8_t initial_expression_id = 1);

    bool SetExpression(uint8_t expression_id);
    bool SetExpression(ExpressionId expression_id);

    uint8_t GetCurrentExpression() const;
    const ExpressionPreset& GetCurrentPreset() const;

    bool NextExpression();
    bool PreviousExpression();

private:
    void ApplyCurrent();

    YukiAvatar* avatar_ = nullptr;
    uint8_t current_expression_id_ = 1;
};

// Lightweight direct-call helper for places that do not need to own an
// engine instance yet. Returns false when id is outside 1..20.
bool ApplyExpressionPreset(YukiAvatar& avatar, uint8_t expression_id);

}  // namespace stackchan::avatar
