// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TypedMinMaxConstraint.hpp"

namespace erbsland::conf::impl {

/// Validate date minimum or maximum limits.
/// @tested{VrMinimumTest VrMaximumTest VrBuilderApiTest}
class MinMaxDateConstraint final : public TypedMinMaxConstraint<time::Date> {
public:
    /// Create a date minimum or maximum constraint.
    explicit MinMaxDateConstraint(const MinOrMax minOrMax, time::Date date) :
        TypedMinMaxConstraint{minOrMax, std::move(date)} {}

protected:
    void validateDate(const ValidationContext &context, const time::Date &value) const override;
    void validateDateTime(const ValidationContext &context, const time::DateTime &value) const override;
};

}
