// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "MinMaxConstraint.hpp"

#include <utility>

namespace erbsland::conf::impl {

/// Store the typed limit value of a minimum or maximum constraint.
/// @tested{VrMinimumTest VrMaximumTest VrBuilderApiTest}
template <typename T>
class TypedMinMaxConstraint : public MinMaxConstraint {
public:
    /// Create a typed minimum or maximum constraint.
    explicit TypedMinMaxConstraint(const MinOrMax minOrMax, T value) :
        MinMaxConstraint{minOrMax}, _value{std::move(value)} {}

    /// Access the configured limit value.
    [[nodiscard]] auto value() const -> const T & { return _value; }

protected:
    /// Test whether a value violates this constraint.
    [[nodiscard]] auto isNotValid(const T &validatedValue) const -> bool {
        if (isNegated()) {
            return !compare(validatedValue, _value);
        }
        return compare(validatedValue, _value);
    }

protected:
    T _value;
};

}
