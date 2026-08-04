// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "MinMaxFloatConstraint.hpp"

#include "ValidationError.hpp"

#include "../../../text/StringFormat.hpp"

#include <cmath>

namespace erbsland::conf::impl {

using namespace text::literals;

void MinMaxFloatConstraint::validateFloat([[maybe_unused]] const ValidationContext &context, const Float value) const {

    if (std::isnan(value) || isNotValid(value)) {
        throwValidationError(text::StringFormat{"The value must be {} {}"_el}.build(comparisonText(), _value));
    }
}

}
