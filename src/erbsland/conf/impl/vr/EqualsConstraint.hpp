// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Constraint.hpp"
#include "ConstraintHandlerContext.hpp"
#include "ValidationContext.hpp"

#include <cmath>
#include <limits>
#include <utility>

namespace erbsland::conf::impl {

using namespace text::literals;

/// Implement common equality checking for typed validation constraints.
/// @tparam T The expected value type.
template <typename T>
class EqualsConstraint : public Constraint {
public:
    /// Creates an equality constraint from its expected value.
    /// @tparam Fwd A forwarding reference to the expected value type.
    /// @param value The expected value.
    template <typename Fwd>
        requires(std::is_same_v<std::remove_cvref_t<Fwd>, T>)
    explicit EqualsConstraint(Fwd &&value) : _value{std::forward<Fwd>(value)} {
        setType(vr::ConstraintType::Equals);
    }

protected:
    /// Compare two values using the rule's type-specific semantics.
    /// @param a The left value.
    /// @param b The right value.
    /// @param context The active validation context.
    /// @return `true` if the values are equal.
    [[nodiscard]] auto isEqual(const T &a, const T &b, const ValidationContext &context) const -> bool {
        if constexpr (std::is_same_v<T, text::String>) {
            return a.compare(b, context.rule->caseSensitivity().asciiComparisonFn()) == std::strong_ordering::equal;
        } else if constexpr (std::is_same_v<T, mem::ByteBlock>) {
            return a == b;
        } else if constexpr (std::is_floating_point_v<T>) {
            if (std::isnan(a) && std::isnan(b)) {
                return true;
            }
            if (std::isinf(a) || std::isinf(b)) {
                return a == b; // true only for the same sign infinity
            }
            return std::abs(a - b) < std::numeric_limits<T>::epsilon();
        } else {
            return a == b;
        }
    }

    /// Test whether a value violates this equality constraint.
    /// @param validatedValue The value being validated.
    /// @param context The active validation context.
    /// @return `true` if validation must fail.
    [[nodiscard]] auto isNotValid(const T &validatedValue, const ValidationContext &context) const -> bool {
        if (isNegated()) {
            return isEqual(validatedValue, _value, context);
        }
        return !isEqual(validatedValue, _value, context);
    }

    /// Get the comparison phrase for an equality or inequality constraint.
    /// @return The comparison phrase.
    [[nodiscard]] auto comparisonText() const -> const text::String & {
        static const text::String equal = "must be equal to"_el;
        static const text::String notEqual = "must not be equal to"_el;
        return isNegated() ? notEqual : equal;
    }

protected:
    T _value;
};

/// Create an equals constraint from a parsed constraint node.
/// @param context The parsed constraint and rule context.
/// @return The created equality constraint.
auto handleEqualsConstraint(const ConstraintHandlerContext &context) -> ConstraintPtr;

}
