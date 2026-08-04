// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EqualsConstraint.hpp"

namespace erbsland::conf::impl {

/// Validate equality of floating-point values within platform tolerance.
class EqualsFloatConstraint final : public EqualsConstraint<Float> {
public:
    /// Creates a floating-point equality constraint.
    /// @param value The expected floating-point value.
    explicit EqualsFloatConstraint(Float value);

protected:
    void validateFloat(const ValidationContext &context, Float value) const override;
};

}
