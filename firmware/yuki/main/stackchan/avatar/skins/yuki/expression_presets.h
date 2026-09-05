/*
 * SPDX-FileCopyrightText: 2026 Phantom-Theatre88
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include "yuki.h"

#include <cstdint>

namespace stackchan::avatar {

// Stable numeric IDs intentionally match the LOCKED 20-expression sheet.
// Keep the IDs stable even if individual parameter values are tuned later.
enum class ExpressionId : uint8_t {
    Exp01 = 1,
    Exp02,
    Exp03,
    Exp04,
    Exp05,
    Exp06,
    Exp07,
    Exp08,
    Exp09,
    Exp10,
    Exp11,
    Exp12,
    Exp13,
    Exp14,
    Exp15,
    Exp16,
    Exp17,
    Exp18,
    Exp19,
    Exp20,
};

struct ExpressionPreset {
    ExpressionId id;
    const char* debug_label;
    ExpressionParameters parameters;
};

constexpr uint8_t kExpressionPresetCount = 20;

const ExpressionPreset& GetExpressionPreset(ExpressionId id);
const ExpressionPreset& GetExpressionPreset(uint8_t id);
bool IsValidExpressionId(uint8_t id);

}  // namespace stackchan::avatar
