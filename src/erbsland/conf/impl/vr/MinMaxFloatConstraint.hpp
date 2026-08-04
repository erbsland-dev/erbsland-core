// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TypedMinMaxConstraint.hpp"

namespace erbsland::conf::impl {

/// Validate floating-point minimum or maximum limits.
/// @tested{VrMinimumTest VrMaximumTest VrBuilderApiTest}
class MinMaxFloatConstraint final : public TypedMinMaxConstraint<Float> {
public:
    /// Create a floating-point minimum or maximum constraint.
    explicit MinMaxFloatConstraint(const MinOrMax minOrMax, const Float value) :
        TypedMinMaxConstraint{minOrMax, value} {}

protected:
    void validateFloat(const ValidationContext &context, Float value) const override;
};

}
