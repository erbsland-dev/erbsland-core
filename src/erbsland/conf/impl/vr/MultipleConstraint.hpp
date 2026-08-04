// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Constraint.hpp"
#include "ConstraintHandlerContext.hpp"

namespace erbsland::conf::impl {

using namespace text::literals;

/// Provides common state for constraints that require a value to be a multiple.
template <typename T>
class MultipleConstraint : public Constraint {
public:
    /// Creates a multiple-of constraint.
    /// @param divisor The required divisor.
    explicit MultipleConstraint(T divisor) : _divisor{divisor} { setType(vr::ConstraintType::Multiple); }

protected:
    /// Get the comparison text for the current negation state.
    [[nodiscard]] auto comparisonText() const -> const text::String & {
        static const text::String multipleOf = "must be a multiple of"_el;
        static const text::String notMultipleOf = "must not be a multiple of"_el;
        return isNegated() ? notMultipleOf : multipleOf;
    }

protected:
    T _divisor;
};

/// Create the multiple-value constraint described by a handler context.
auto handleMultipleConstraint(const ConstraintHandlerContext &context) -> ConstraintPtr;

}
