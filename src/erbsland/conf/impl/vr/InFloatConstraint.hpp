// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "InConstraint.hpp"

namespace erbsland::conf::impl {

/// Validate that a floating-point value is one of the configured values.
class InFloatConstraint final : public InConstraint<Float> {
public:
    /// Creates a floating-point membership constraint.
    /// @param values The allowed floating-point values.
    explicit InFloatConstraint(std::vector<Float> values) : InConstraint(std::move(values)) {}

protected:
    void validateFloat(const ValidationContext &context, Float value) const override;
};

}
