// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpMediaType.hpp"

#include "../impl/http/HttpGrammar.hpp"

#include "../../err/ParameterError.hpp"
#include "../../err/ParseError.hpp"
#include "../../text/AnyString.hpp"
#include "../../text/AsciiCategory.hpp"
#include "../../text/Char.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringCharReader.hpp"
#include "../../text/StringEditor.hpp"

namespace erbsland::network {

using namespace text;
using namespace text::literals;

HttpMediaType::HttpMediaType(String type, String subtype) {
    initialize(std::move(type), std::move(subtype), {});
}

HttpMediaType::HttpMediaType(String type, String subtype, HttpMediaTypeParameters parameters) {
    initialize(std::move(type), std::move(subtype), std::move(parameters));
}

auto HttpMediaType::hasParameter(const String &name) const noexcept -> bool {
    return parameter(name).has_value();
}

auto HttpMediaType::parameter(const String &name) const noexcept -> std::optional<String> {
    if (!impl::http_grammar::isToken(name)) {
        return std::nullopt;
    }
    for (const auto &parameter : _parameters) {
        if (impl::http_grammar::equalTokenCI(parameter.name(), name)) {
            return parameter.value();
        }
    }
    return std::nullopt;
}

auto HttpMediaType::setParameter(HttpMediaTypeParameter parameterValue) -> HttpMediaType & {
    auto result = _parameters;
    const auto first = result.findFirstIf([&parameterValue](const HttpMediaTypeParameter &existing) -> bool {
        return impl::http_grammar::equalTokenCI(existing.name(), parameterValue.name());
    });
    if (first.isNoIndex()) {
        if (result.count() >= cMaximumParameterCount) {
            throw err::ParameterError{"The media type exceeds the parameter-count limit."_el, "parameter"_el};
        }
        result.append(std::move(parameterValue));
    } else {
        result.set(first, std::move(parameterValue));
    }
    _parameters = std::move(result);
    return *this;
}

auto HttpMediaType::setParameter(String name, String value) -> HttpMediaType & {
    return setParameter(HttpMediaTypeParameter{std::move(name), std::move(value)});
}

auto HttpMediaType::removeParameter(const String &name) -> HttpMediaType & {
    if (!impl::http_grammar::isToken(name)) {
        return *this;
    }
    _parameters.removeIf([&name](const HttpMediaTypeParameter &parameterValue) -> bool {
        return impl::http_grammar::equalTokenCI(parameterValue.name(), name);
    });
    return *this;
}

auto HttpMediaType::toString() const -> String {
    if (!isValid()) {
        return {};
    }
    auto editor = StringEditor{};
    editor.append(_type).append(U'/').append(_subtype);
    for (const auto &parameterValue : _parameters) {
        editor.append("; "_el).append(parameterValue.name()).append(U'=');
        writeParameterValue(editor, parameterValue.value());
    }
    return String{editor};
}

auto HttpMediaType::fromString(const String &text) noexcept -> HttpMediaType {
    try {
        return fromStringOrThrow(text);
    } catch (const err::ParseError &) {
        return {};
    }
}

auto HttpMediaType::fromStringOrThrow(const String &text) -> HttpMediaType {
    if (text.length() > cMaximumTextLength) {
        throw err::ParseError{"The media type exceeds the configured text limit."_el};
    }
    if (!text.isValidUtf8()) {
        throw err::ParseError{"A media type must be valid UTF-8."_el};
    }
    auto reader = StringCharReader{text};
    const auto readToken = [&reader]() -> String {
        reader.startCapture();
        reader.advanceWhile(AsciiCategory::HttpToken);
        return reader.takeCapture().toString();
    };

    reader.advanceWhile(AsciiCategory::Blank);
    auto type = readToken();
    if (type.isEmpty() || !reader.advanceIf(U'/')) {
        throw err::ParseError{"A media type requires a type and subtype separated by a slash."_el};
    }
    auto subtype = readToken();
    if (subtype.isEmpty()) {
        throw err::ParseError{"A media type requires a non-empty subtype token."_el};
    }
    auto parameters = HttpMediaTypeParameters{};
    reader.advanceWhile(AsciiCategory::Blank);
    while (!reader.isAtEnd()) {
        if (!reader.advanceIf(U';')) {
            throw err::ParseError{"Unexpected text follows the media type subtype."_el};
        }
        reader.advanceWhile(AsciiCategory::Blank);
        auto name = readToken();
        reader.advanceWhile(AsciiCategory::Blank);
        if (name.isEmpty() || !reader.advanceIf(U'=')) {
            throw err::ParseError{"A media-type parameter requires a token name and equals sign."_el};
        }
        reader.advanceWhile(AsciiCategory::Blank);
        auto value = String{};
        if (reader.advanceIf(U'\"')) {
            reader.clearBuffer();
            auto closed = false;
            while (!reader.isAtEnd()) {
                const auto character = reader.peek();
                if (character == U'\"') {
                    reader.advance();
                    closed = true;
                    break;
                }
                if (character == U'\\') {
                    reader.advance();
                    if (reader.isAtEnd()) {
                        throw err::ParseError{"A quoted media-type parameter ends after an escape."_el};
                    }
                    reader.readToBuffer();
                    continue;
                }
                if ((character.isAsciiControl() && character != U'\t') || character == U'\x7f') {
                    throw err::ParseError{"A quoted media-type parameter contains a prohibited control."_el};
                }
                reader.readToBuffer();
            }
            if (!closed) {
                throw err::ParseError{"A quoted media-type parameter is not terminated."_el};
            }
            value = reader.takeBuffer().toString();
        } else {
            value = readToken();
            if (value.isEmpty()) {
                throw err::ParseError{"A media-type parameter requires a token or quoted value."_el};
            }
        }
        try {
            auto parameterValue = HttpMediaTypeParameter{std::move(name), std::move(value)};
            if (parameters.anyOf([&parameterValue](const HttpMediaTypeParameter &existing) -> bool {
                    return impl::http_grammar::equalTokenCI(existing.name(), parameterValue.name());
                })) {
                throw err::ParseError{"A media-type parameter name is duplicated."_el};
            }
            parameters.append(std::move(parameterValue));
        } catch (const err::ParameterError &) {
            throw err::ParseError{"A media-type parameter is invalid."_el};
        }
        if (parameters.count() > cMaximumParameterCount) {
            throw err::ParseError{"The media type exceeds the parameter-count limit."_el};
        }
        reader.advanceWhile(AsciiCategory::Blank);
    }
    try {
        return HttpMediaType{std::move(type), std::move(subtype), std::move(parameters)};
    } catch (const err::ParameterError &) {
        throw err::ParseError{"The media type contains an invalid token."_el};
    }
}

void HttpMediaType::initialize(String type, String subtype, HttpMediaTypeParameters parameters) {
    if (!impl::http_grammar::isToken(type) || !impl::http_grammar::isToken(subtype)) {
        throw err::ParameterError{"A media type requires non-empty ASCII type and subtype tokens."_el, "type"_el};
    }
    if (parameters.count() > cMaximumParameterCount || !hasUniqueNames(parameters)) {
        throw err::ParameterError{
            "Media-type parameter names must be unique and within the count limit."_el, "parameters"_el};
    }
    _type = type.transformed(Char::toAsciiLowercase);
    _subtype = subtype.transformed(Char::toAsciiLowercase);
    _parameters = std::move(parameters);
}

auto HttpMediaType::hasUniqueNames(const HttpMediaTypeParameters &parameters) noexcept -> bool {
    for (auto first = unit::ItemIndex::zero(); first.isWithin(parameters.count()); ++first) {
        const auto &firstValue = parameters.getRef(first);
        for (auto second = first.advanced(unit::ItemCount::one()); second.isWithin(parameters.count()); ++second) {
            if (impl::http_grammar::equalTokenCI(firstValue.name(), parameters.getRef(second).name())) {
                return false;
            }
        }
    }
    return true;
}

void HttpMediaType::writeParameterValue(StringEditor &editor, const String &value) {
    if (impl::http_grammar::isToken(value)) {
        editor.append(value);
        return;
    }
    editor.append(U'\"');
    auto reader = StringCharReader{value};
    while (!reader.isAtEnd()) {
        const auto character = reader.read();
        if (character == U'\"' || character == U'\\') {
            editor.append(U'\\');
        }
        editor.append(character);
    }
    editor.append(U'\"');
}

}
