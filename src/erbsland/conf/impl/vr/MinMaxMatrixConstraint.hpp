// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TypedMinMaxConstraint.hpp"

namespace erbsland::conf::impl {

/// Validate matrix-dimension minimum or maximum limits.
/// @tested{VrMinimumTest VrMaximumTest VrBuilderApiTest}
class MinMaxMatrixConstraint final : public TypedMinMaxConstraint<Integer> {
public:
    /// Create a matrix minimum or maximum constraint.
    explicit MinMaxMatrixConstraint(const MinOrMax minOrMax, const Integer first, const Integer second) :
        TypedMinMaxConstraint{minOrMax, first}, _second{second} {}

    /// Access the configured second dimension limit.
    [[nodiscard]] auto secondValue() const -> Integer { return _second; }

protected:
    /// Test whether a second dimension violates this constraint.
    [[nodiscard]] auto isSecondNotValid(Integer validatedValue) const -> bool;
    void validateValueList(const ValidationContext &context) const override;

private:
    Integer _second;
};

}
