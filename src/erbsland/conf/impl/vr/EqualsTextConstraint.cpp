// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EqualsTextConstraint.hpp"

#include "ValidationError.hpp"

#include "../../../text/StringFormat.hpp"

namespace erbsland::conf::impl {

using namespace text::literals;

void EqualsTextConstraint::validateText(const ValidationContext &context, const text::String &value) const {
    if (isNotValid(value, context)) {
        throwValidationError(
            text::StringFormat{"The text {} \"{:/display}\" ({})"_el}.build(
                comparisonText(), _value, context.rule->caseSensitivity().toString()));
    }
}

}
