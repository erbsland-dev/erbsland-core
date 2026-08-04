// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "InIntegerConstraint.hpp"

#include "ValidationError.hpp"

#include "../../../text/StringFormat.hpp"

namespace erbsland::conf::impl {

using namespace text::literals;

void InIntegerConstraint::validateInteger(const ValidationContext &context, const Integer value) const {
    if (isNotValid(value, context)) {
        text::StringEditor expected;
        for (std::size_t i = 0; i < _values.size(); ++i) {
            if (i != 0) {
                expected.append(" or "_el);
            }
            expected.append(text::StringFormat{"{}"_el}.build(_values[i]));
        }
        throwValidationError(text::StringFormat{"The value {} {}"_el}.build(comparisonText(), expected));
    }
}

}
