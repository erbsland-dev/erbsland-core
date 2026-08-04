// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Rule_fwd.hpp"

#include "../value/Value_fwd.hpp"

#include <cstddef>
#include <utility>

namespace erbsland::conf::impl {

/// Stores a value and rule while traversing the second validation pass.
/// @notest{Used only by DocumentValidator.}
struct Pass2Frame final {
    conf::ValuePtr value;
    RulePtr rule;
    std::size_t addedIndexes{0}; ///< Number of indexes added on entering this frame.
    bool isExit{false};          ///< If this frame performs the matching exit operation.

    /// Create an enter frame.
    [[nodiscard]] static auto createEnter(conf::ValuePtr valueNode, RulePtr ruleNode) noexcept -> Pass2Frame {
        return {.value = std::move(valueNode), .rule = std::move(ruleNode)};
    }
    /// Create the matching exit frame.
    [[nodiscard]] auto createExit() const noexcept -> Pass2Frame {
        return {.value = value, .rule = rule, .addedIndexes = addedIndexes, .isExit = true};
    }
};

}
