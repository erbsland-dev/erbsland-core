// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "MinMaxDateTimeConstraint.hpp"

#include "ValidationError.hpp"

#include "../../../text/StringFormat.hpp"

namespace erbsland::conf::impl {

using namespace text::literals;

void MinMaxDateTimeConstraint::validateDate(
    [[maybe_unused]] const ValidationContext &context, const time::Date &value) const {

    if (isNotValid(time::DateTime{value, time::Time{}})) {
        throwValidationError(
            text::StringFormat{"The date must be {} {}"_el}.build(comparisonText(), _value.date().toString()));
    }
}

void MinMaxDateTimeConstraint::validateDateTime(
    [[maybe_unused]] const ValidationContext &context, const time::DateTime &value) const {

    if (isNotValid(value)) {
        throwValidationError(
            text::StringFormat{"The date-time must be {} {}"_el}.build(comparisonText(), _value.toString()));
    }
}

}
