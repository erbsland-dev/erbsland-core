// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpFieldName.hpp"

#include "../impl/http/HttpGrammar.hpp"

#include "../../err/ParameterFailure.hpp"
#include "../../err/ParseError.hpp"
#include "../../text/Char.hpp"
#include "../../text/Literals.hpp"

namespace erbsland::network {

using namespace text;
using namespace text::literals;

HttpFieldName::HttpFieldName(const HttpFieldType type) noexcept : _text{type.toString()}, _type{type} {
}

HttpFieldName::HttpFieldName(String text) {
    if (!impl::http_grammar::isToken(text)) {
        throw err::ParameterFailure{"An HTTP field name must be a non-empty ASCII token."_el, "text"_el};
    }
    _type = HttpFieldType::fromString(text);
    _text = std::move(text);
}

auto HttpFieldName::operator<=>(const HttpFieldName &other) const noexcept -> std::strong_ordering {
    return _text.compare(other._text, Char::compareAsciiFolded);
}

auto HttpFieldName::operator==(const HttpFieldName &other) const noexcept -> bool {
    return operator<=>(other) == std::strong_ordering::equal;
}

auto HttpFieldName::fromString(const String &text) noexcept -> HttpFieldName {
    try {
        return fromStringOrThrow(text);
    } catch (const err::ParseError &) {
        return {};
    }
}

auto HttpFieldName::fromStringOrThrow(const String &text) -> HttpFieldName {
    if (!impl::http_grammar::isToken(text)) {
        throw err::ParseError{"An HTTP field name must be a non-empty ASCII token."_el};
    }
    return HttpFieldName{text, HttpFieldType::fromString(text)};
}

}
