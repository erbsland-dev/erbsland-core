// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringPartConstraint.hpp"

#include <utility>

namespace erbsland::conf::impl {

/// Validate that strings contain one of the expected values.
class ContainsConstraint final : public StringPartConstraint {
public:
    /// Create a string-containment constraint.
    /// @param values The expected parts.
    explicit ContainsConstraint(text::StringList values) :
        StringPartConstraint{vr::ConstraintType::Contains, std::move(values)} {}

protected:
    [[nodiscard]] auto partText() const -> const text::String & override {
        static const text::String text = "contain"_el;
        return text;
    }
    [[nodiscard]] auto doesPartMatch(
        const text::String &expectedValue, const text::String &testedValue, const ValidationContext &context) const
        -> bool override {

        return testedValue.contains(expectedValue, context.rule->caseSensitivity().asciiComparisonFn());
    }
};

}
