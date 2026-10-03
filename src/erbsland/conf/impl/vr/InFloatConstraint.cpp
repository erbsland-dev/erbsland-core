// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "InFloatConstraint.hpp"

#include "ValidationError.hpp"

#include "../../../text/StringFormat.hpp"

namespace erbsland::conf::impl {

using namespace text::literals;

void InFloatConstraint::validateFloat(const ValidationContext &context, const Float value) const {
    if (isNotValid(value, context)) {
        text::StringEditor expected;
        for (const auto &allowed : _values) {
            if (!expected.isEmpty()) {
                expected.append(" or "_el);
            }
            expected.append(text::StringFormat{"{:.6}"_el}.build(allowed));
        }
        throwValidationError(
            text::StringFormat{"The value {} {} (within platform tolerance)"_el}.build(comparisonText(), expected));
    }
}

}
