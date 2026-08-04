// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EqualsBooleanConstraint.hpp"

#include "ValidationError.hpp"

#include "../../../text/StringFormat.hpp"

namespace erbsland::conf::impl {

using namespace text::literals;

EqualsBooleanConstraint::EqualsBooleanConstraint(const bool value) : EqualsConstraint(value) {
}

void EqualsBooleanConstraint::validateBoolean(const ValidationContext &context, const bool value) const {
    if (isNotValid(value, context)) {
        const auto expectedValue = isNegated() ? !_value : _value;
        throwValidationError(
            text::StringFormat{"The value must be {}"_el}.build(text::String{expectedValue ? "true"_el : "false"_el}));
    }
}

}
