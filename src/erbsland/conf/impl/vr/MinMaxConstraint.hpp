// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Constraint.hpp"
#include "ConstraintHandlerContext.hpp"

#include <cstdint>

namespace erbsland::conf::impl {

using namespace text::literals;

/// Provide the common behavior for minimum and maximum constraints.
/// @tested{VrMinimumTest VrMaximumTest VrBuilderApiTest}
class MinMaxConstraint : public Constraint {
public:
    /// Select whether a constraint is a minimum or maximum.
    enum MinOrMax : uint8_t { Min, Max };

public:
    /// Create a minimum or maximum constraint.
    explicit MinMaxConstraint(const MinOrMax minOrMax) :
        Constraint{minOrMax == Min ? vr::ConstraintType::Minimum : vr::ConstraintType::Maximum} {}

    /// Compare two values according to this constraint's direction.
    template <typename T>
    [[nodiscard]] auto compare(const T &a, const T &b) const -> bool {
        if (type() == vr::ConstraintType::Minimum) {
            return a < b;
        }
        return a > b;
    }

    /// Create a typed minimum or maximum constraint for a rule.
    /// @param minOrMax Selects the minimum or maximum constraint.
    /// @param rule The rule that owns the constraint.
    /// @param node The configuration value for the constraint.
    /// @return The created constraint.
    /// @throws ConfError If the value type does not match the rule.
    [[nodiscard]] static auto createForRule(MinOrMax minOrMax, const RulePtr &rule, const conf::ValuePtr &node)
        -> ConstraintPtr;

    /// Get the comparison text that describes this constraint.
    [[nodiscard]] auto comparisonText() const -> const text::String & {
        static const text::String lessThan = "less than"_el;
        static const text::String atLeast = "at least"_el;
        static const text::String atMost = "at most"_el;
        static const text::String greaterThan = "greater than"_el;
        if (type() == vr::ConstraintType::Minimum) {
            return isNegated() ? lessThan : atLeast;
        }
        return isNegated() ? greaterThan : atMost;
    }

private:
    /// Throw an error for a constraint value that does not match its rule type.
    [[noreturn]] static void throwValueTypeError(const RulePtr &rule, const conf::ValuePtr &node, ValueType expected);
};

/// Handle a minimum-constraint configuration entry.
/// @tested{VrMinimumTest}
auto handleMinimumConstraint(const ConstraintHandlerContext &context) -> ConstraintPtr;
/// Handle a maximum-constraint configuration entry.
/// @tested{VrMaximumTest}
auto handleMaximumConstraint(const ConstraintHandlerContext &context) -> ConstraintPtr;

}
