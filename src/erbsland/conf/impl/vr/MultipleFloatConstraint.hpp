// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "MultipleConstraint.hpp"

namespace erbsland::conf::impl {

/// Validate that floating-point values are multiples of a divisor.
class MultipleFloatConstraint final : public MultipleConstraint<Float> {
public:
    /// Create a floating-point multiple-of constraint.
    /// @param divisor The required divisor.
    explicit MultipleFloatConstraint(Float divisor);

protected:
    void validateFloat([[maybe_unused]] const ValidationContext &context, Float value) const override;

private:
    /// Test if a floating-point value violates the configured multiple constraint.
    [[nodiscard]] auto isNotValid(Float tested) const -> bool;
};

}
