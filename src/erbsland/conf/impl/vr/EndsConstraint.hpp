// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringPartConstraint.hpp"

#include <utility>

namespace erbsland::conf::impl {

/// Validate that strings end with one of the expected values.
class EndsConstraint final : public StringPartConstraint {
public:
    /// Create a string-suffix constraint.
    /// @param values The expected suffixes.
    explicit EndsConstraint(text::StringList values) : StringPartConstraint(std::move(values)) {
        setType(vr::ConstraintType::Ends);
    }

protected:
    [[nodiscard]] auto partText() const -> const text::String & override {
        static const text::String text = "end with"_el;
        return text;
    }
    [[nodiscard]] auto doesPartMatch(
        const text::String &expectedValue, const text::String &testedValue, const ValidationContext &context) const
        -> bool override {

        return testedValue.endsWith(expectedValue, context.rule->caseSensitivity().asciiComparisonFn());
    }
};

}
