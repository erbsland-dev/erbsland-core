// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TypedMinMaxConstraint.hpp"

namespace erbsland::conf::impl {

/// Validate date-time minimum or maximum limits.
/// @tested{VrMinimumTest VrMaximumTest VrBuilderApiTest}
class MinMaxDateTimeConstraint final : public TypedMinMaxConstraint<time::DateTime> {
public:
    /// Create a date-time minimum or maximum constraint.
    explicit MinMaxDateTimeConstraint(const MinOrMax minOrMax, time::DateTime dateTime) :
        TypedMinMaxConstraint{minOrMax, std::move(dateTime)} {}

protected:
    void validateDate(const ValidationContext &context, const time::Date &value) const override;
    void validateDateTime(const ValidationContext &context, const time::DateTime &value) const override;
};

}
