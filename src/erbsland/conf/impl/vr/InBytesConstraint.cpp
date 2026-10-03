// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "InBytesConstraint.hpp"

#include "ValidationError.hpp"

#include "../../../text/StringFormat.hpp"

namespace erbsland::conf::impl {

using namespace text::literals;

void InBytesConstraint::validateBytes(const ValidationContext &context, const mem::ByteBlock &value) const {
    if (isNotValid(value, context)) {
        text::StringEditor expected;
        for (const auto &allowed : _values) {
            if (!expected.isEmpty()) {
                expected.append(" or "_el);
            }
            expected.append(text::StringFormat{"\"{:bytes:maximum=16,truncate=middle}\""_el}.build(allowed));
        }
        throwValidationError(text::StringFormat{"The byte sequence {} {}"_el}.build(comparisonText(), expected));
    }
}

}
