// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EqualsBytesConstraint.hpp"

#include "ValidationError.hpp"

#include "../../../text/StringFormat.hpp"

namespace erbsland::conf::impl {

using namespace text::literals;

void EqualsBytesConstraint::validateBytes(const ValidationContext &context, const mem::ByteBlock &value) const {
    if (isNotValid(value, context)) {
        throwValidationError(
            text::StringFormat{"The byte sequence {} \"{:bytes:maximum=16,truncate=middle}\""_el}.build(
                comparisonText(), _value));
    }
}

}
