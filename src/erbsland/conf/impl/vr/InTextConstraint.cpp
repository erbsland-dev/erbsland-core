// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "InTextConstraint.hpp"

#include "ValidationError.hpp"

#include "../../../text/StringFormat.hpp"

namespace erbsland::conf::impl {

using namespace text::literals;

void InTextConstraint::validateText(const ValidationContext &context, const text::String &value) const {
    if (isNotValid(value, context)) {
        text::StringEditor expected;
        auto isFirst = true;
        for (const auto &expectedValue : _values) {
            if (!isFirst) {
                expected.append(" or "_el);
            }
            isFirst = false;
            expected.append(text::StringFormat{"\"{:/display}\""_el}.build(expectedValue));
        }
        throwValidationError(
            text::StringFormat{"The text {} {} ({})"_el}.build(
                comparisonText(), expected, context.rule->caseSensitivity().toString()));
    }
}

}
