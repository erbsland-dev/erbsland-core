// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CommonHostTests.hpp"

#include "../../err/ParseError.hpp"
#include "../../text/String.hpp"
#include "../../text/StringFormat.hpp"

namespace erbsland::network::impl {

using namespace erbsland::unit;
using namespace erbsland::text;
using namespace erbsland::text::literals;

void testCommonHostText(const String &text, const StringLiteral &parsedObject) {
    if (text.isEmpty()) {
        throw err::ParseError{StringFormat{"The {} must not be empty."_el}.build(parsedObject)};
    }
    if (text.length() > ByteLength{1024U}) {
        throw err::ParseError{StringFormat{"The {} exceeds the maximum length of 1024 bytes."_el}.build(parsedObject)};
    }
}

}
