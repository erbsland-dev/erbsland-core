// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EqualsConstraint.hpp"

namespace erbsland::conf::impl {

/// Validate equality of Boolean values.
class EqualsBooleanConstraint final : public EqualsConstraint<bool> {
public:
    /// Creates a Boolean equality constraint.
    /// @param value The expected Boolean value.
    explicit EqualsBooleanConstraint(bool value);

protected:
    void validateBoolean(const ValidationContext &context, bool value) const override;
};

}
