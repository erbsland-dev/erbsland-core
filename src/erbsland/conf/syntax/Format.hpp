// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "FormatRule.hpp"

namespace erbsland::conf::syntax {

/// A base style and ordered rules, resolved independently for each property.
/// @tested{SyntaxFormatTest SyntaxFormatterTest}
class Format final {
public:
    /// Create repository defaults, including relative-section and meta-value rules.
    Format();

public: // configuration
    /// Get the base style.
    auto baseStyle() const noexcept -> const FormatStyle & { return _base; }
    /// Replace the base style.
    auto setBaseStyle(FormatStyle style) -> Format &;
    /// Add a rule; later rules win equal-specificity ties.
    auto addRule(FormatRule rule) -> Format &;
    /// Remove all rules, including default rules.
    auto clearRules() noexcept -> Format &;

public: // cascade
    /// Resolve a complete style for one statement.
    /// @param statement Original source statement.
    /// @return Effective formatting properties.
    [[nodiscard]] auto resolve(const Statement &statement) const -> FormatStyle;

private:
    FormatStyle _base;              ///< Base defaults.
    std::vector<FormatRule> _rules; ///< Rules in insertion order.
};

}
