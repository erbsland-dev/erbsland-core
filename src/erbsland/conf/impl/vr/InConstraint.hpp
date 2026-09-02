// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Constraint.hpp"
#include "ConstraintHandlerContext.hpp"
#include "ValidationContext.hpp"

#include "../../../text/CaseSensitivity.hpp"
#include "../../../text/StringList.hpp"
#include "../../ConfError.hpp"

#include <cmath>
#include <iterator>
#include <limits>
#include <utility>
#include <vector>

namespace erbsland::conf::impl {

using namespace text::literals;

/// Implement membership checking for typed validation constraints.
/// @tparam T The constrained value type.
template <typename T>
class InConstraint : public Constraint {
public:
    using Values = std::conditional_t<std::is_same_v<T, text::String>, text::StringList, std::vector<T>>;

    /// Creates a membership constraint from its allowed values.
    /// @tparam Fwd A forwarding reference to the allowed-values collection.
    /// @param values The allowed values.
    template <typename Fwd>
        requires(std::is_same_v<std::remove_cvref_t<Fwd>, Values>)
    explicit InConstraint(Fwd &&values) : Constraint{vr::ConstraintType::In}, _values(std::forward<Fwd>(values)) {}

public:
    /// Test whether a collection contains equivalent duplicate values.
    /// @param values The values to inspect.
    /// @param cs The text comparison case sensitivity.
    /// @return `true` if two values compare equal.
    [[nodiscard]] static auto hasDuplicate(const Values &values, const text::CaseSensitivity cs) -> bool {
        for (auto first = values.begin(); first != values.end(); ++first) {
            for (auto other = std::next(first); other != values.end(); ++other) {
                if (areEqual(*first, *other, cs)) {
                    return true;
                }
            }
        }
        return false;
    }

protected:
    /// Compare two values using the active rule's case-sensitivity setting.
    /// @param a The left value.
    /// @param b The right value.
    /// @param context The active validation context.
    /// @return `true` if the values compare equal.
    [[nodiscard]] auto isEqual(const T &a, const T &b, const ValidationContext &context) const -> bool {
        return areEqual(a, b, context.rule->caseSensitivity());
    }

    /// Compare two values using explicitly supplied case sensitivity.
    /// @param a The left value.
    /// @param b The right value.
    /// @param cs The text comparison case sensitivity.
    /// @return `true` if the values compare equal.
    [[nodiscard]] static auto areEqual(const T &a, const T &b, const text::CaseSensitivity cs) -> bool {
        if constexpr (std::is_same_v<T, text::String>) {
            return a.compare(b, cs.asciiComparisonFn()) == std::strong_ordering::equal;
        } else if constexpr (std::is_same_v<T, mem::ByteBlock>) {
            return a == b;
        } else if constexpr (std::is_floating_point_v<T>) {
            return std::abs(a - b) < std::numeric_limits<T>::epsilon();
        } else {
            return a == b;
        }
    }

    /// Test whether the allowed values contain a value.
    /// @param value The value to find.
    /// @param context The active validation context.
    /// @return `true` if the value is allowed.
    [[nodiscard]] auto contains(const T &value, const ValidationContext &context) const -> bool {
        for (const auto &v : _values) {
            if (isEqual(v, value, context)) {
                return true;
            }
        }
        return false;
    }

    /// Test whether a value violates this membership constraint.
    /// @param validatedValue The value being validated.
    /// @param context The active validation context.
    /// @return `true` if validation must fail.
    [[nodiscard]] auto isNotValid(const T &validatedValue, const ValidationContext &context) const -> bool {
        if (isNegated()) {
            // invalid if it is in the list when negated
            return contains(validatedValue, context);
        }
        // invalid if it is not in the list when not negated
        return !contains(validatedValue, context);
    }

    /// Get the comparison phrase for a membership or non-membership constraint.
    /// @return The comparison phrase.
    [[nodiscard]] auto comparisonText() const -> const text::String & {
        static const text::String inText = "must be one of"_el;
        static const text::String notInText = "must not be one of"_el;
        return isNegated() ? notInText : inText;
    }

protected:
    Values _values;
};

/// Create a membership constraint from a parsed constraint node.
/// @param context The parsed constraint and rule context.
/// @return The created membership constraint.
auto handleInConstraint(const ConstraintHandlerContext &context) -> ConstraintPtr;

}
