// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpVersion.hpp"

#include "../../err/ParseError.hpp"
#include "../../text/Literals.hpp"

namespace erbsland::network {

using namespace text::literals;

auto HttpVersion::toString() const noexcept -> text::String {
    switch (_value) {
    case Http10:
        return "HTTP/1.0"_el;
    case Http11:
        return "HTTP/1.1"_el;
    case Invalid:
        return {};
    }
    return {};
}

auto HttpVersion::fromString(const text::String &text) noexcept -> HttpVersion {
    try {
        return fromStringOrThrow(text);
    } catch (const err::ParseError &) {
        return {};
    }
}

auto HttpVersion::fromStringOrThrow(const text::String &text) -> HttpVersion {
    if (text == "HTTP/1.0"_el)
        return Http10;
    if (text == "HTTP/1.1"_el)
        return Http11;
    throw err::ParseError{"The HTTP version is malformed or unsupported."_el};
}

}
