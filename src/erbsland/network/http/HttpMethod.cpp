// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpMethod.hpp"

#include "../impl/http/HttpGrammar.hpp"

#include "../../err/ParseError.hpp"
#include "../../text/Literals.hpp"

namespace erbsland::network {

using namespace text::literals;

HttpMethod::HttpMethod(const HttpMethodType type) noexcept : _type{type} {
    switch (type) {
    case HttpMethodType::Connect:
        _text = "CONNECT"_el;
        break;
    case HttpMethodType::Delete:
        _text = "DELETE"_el;
        break;
    case HttpMethodType::Get:
        _text = "GET"_el;
        break;
    case HttpMethodType::Head:
        _text = "HEAD"_el;
        break;
    case HttpMethodType::Options:
        _text = "OPTIONS"_el;
        break;
    case HttpMethodType::Patch:
        _text = "PATCH"_el;
        break;
    case HttpMethodType::Post:
        _text = "POST"_el;
        break;
    case HttpMethodType::Put:
        _text = "PUT"_el;
        break;
    case HttpMethodType::Trace:
        _text = "TRACE"_el;
        break;
    case HttpMethodType::None:
    case HttpMethodType::All:
        _type = HttpMethodType::None;
        break;
    }
}

auto HttpMethod::standardType() const noexcept -> std::optional<HttpMethodType> {
    return isStandard() ? std::optional{_type} : std::nullopt;
}

auto HttpMethod::fromString(const text::String &text) noexcept -> HttpMethod {
    try {
        return fromStringOrThrow(text);
    } catch (const err::ParseError &) {
        return {};
    }
}

auto HttpMethod::fromStringOrThrow(const text::String &text) -> HttpMethod {
    if (!impl::http_grammar::isToken(text)) {
        throw err::ParseError{"An HTTP method must be a non-empty ASCII token."_el};
    }
    return HttpMethod{text, classify(text)};
}

auto HttpMethod::classify(const text::String &text) noexcept -> HttpMethodType {
    if (text == "CONNECT"_el)
        return HttpMethodType::Connect;
    if (text == "DELETE"_el)
        return HttpMethodType::Delete;
    if (text == "GET"_el)
        return HttpMethodType::Get;
    if (text == "HEAD"_el)
        return HttpMethodType::Head;
    if (text == "OPTIONS"_el)
        return HttpMethodType::Options;
    if (text == "PATCH"_el)
        return HttpMethodType::Patch;
    if (text == "POST"_el)
        return HttpMethodType::Post;
    if (text == "PUT"_el)
        return HttpMethodType::Put;
    if (text == "TRACE"_el)
        return HttpMethodType::Trace;
    return HttpMethodType::None;
}

}
