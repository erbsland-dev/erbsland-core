// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "MultipleConstraint.hpp"

namespace erbsland::conf::impl {

/// Validate matrix dimensions against row and column divisors.
class MultipleMatrixConstraint final : public MultipleConstraint<Integer> {
public:
    /// Create a matrix-dimension multiple-of constraint.
    /// @param rowsDivisor The required row divisor.
    /// @param columnsDivisor The required column divisor.
    explicit MultipleMatrixConstraint(Integer rowsDivisor, Integer columnsDivisor);

protected:
    void validateValueList(const ValidationContext &context) const override;

private:
    /// Test if a row count violates the configured divisor.
    [[nodiscard]] auto isNotValidRows(Integer tested) const -> bool;
    /// Test if a column count violates the configured divisor.
    [[nodiscard]] auto isNotValidColumns(Integer tested) const -> bool;

private:
    Integer _columnsDivisor;
};

}
