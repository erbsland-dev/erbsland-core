// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringPartConstraint.hpp"

#include <utility>

namespace erbsland::conf::impl {

/// Validate that strings start with one of the expected values.
class StartsConstraint final : public StringPartConstraint {
public:
    /// Create a string-prefix constraint.
    /// @param values The expected prefixes.
    explicit StartsConstraint(text::StringList values) :
        StringPartConstraint{vr::ConstraintType::Starts, std::move(values)} {}

protected:
    [[nodiscard]] auto partText() const -> const text::String & override {
        static const text::String text = "start with"_el;
        return text;
    }
    [[nodiscard]] auto doesPartMatch(
        const text::String &expectedValue, const text::String &testedValue, const ValidationContext &context) const
        -> bool override {

        return testedValue.startsWith(expectedValue, context.rule->caseSensitivity().asciiComparisonFn());
    }
};

}
