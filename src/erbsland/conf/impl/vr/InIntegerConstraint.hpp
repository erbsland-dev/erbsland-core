// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "InConstraint.hpp"

namespace erbsland::conf::impl {

/// Validate that an integer is one of the configured values.
class InIntegerConstraint final : public InConstraint<Integer> {
public:
    /// Creates an integer membership constraint.
    /// @param values The allowed integer values.
    explicit InIntegerConstraint(std::vector<Integer> values) : InConstraint(std::move(values)) {}

protected:
    void validateInteger(const ValidationContext &context, Integer value) const override;
};

}
