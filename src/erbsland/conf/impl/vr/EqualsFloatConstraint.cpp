// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EqualsFloatConstraint.hpp"

#include "ValidationError.hpp"

#include "../../../text/StringFormat.hpp"

namespace erbsland::conf::impl {

using namespace text::literals;

EqualsFloatConstraint::EqualsFloatConstraint(const Float value) : EqualsConstraint(value) {
}

void EqualsFloatConstraint::validateFloat(const ValidationContext &context, const Float value) const {
    if (isNotValid(value, context)) {
        throwValidationError(
            text::StringFormat{"The value {} {:.6} (within platform tolerance)"_el}.build(comparisonText(), _value));
    }
}

}
