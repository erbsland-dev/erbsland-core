// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EqualsConstraint.hpp"

namespace erbsland::conf::impl {

/// Validate equality of value-matrix row and column counts.
class EqualsMatrixConstraint final : public EqualsConstraint<Integer> {
public:
    /// Creates a matrix-size equality constraint.
    /// @param rows The expected row count.
    /// @param columns The expected column count.
    explicit EqualsMatrixConstraint(Integer rows, Integer columns);

protected:
    /// Test whether a matrix column count violates this constraint.
    /// @param validatedValue The column count being validated.
    /// @param context The active validation context.
    /// @return `true` if validation must fail.
    [[nodiscard]] auto isNotValidColumns(const Integer &validatedValue, const ValidationContext &context) const -> bool;
    void validateValueList(const ValidationContext &context) const override;

private:
    Integer _columns;
};

}
