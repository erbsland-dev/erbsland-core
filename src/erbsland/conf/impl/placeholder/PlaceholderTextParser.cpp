// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PlaceholderTextParser.hpp"

#include "../char/NamedChars.hpp"
#include "../lexer/Text.hpp"

#include "../../../text/Literals.hpp"
#include "../../Name.hpp"

namespace erbsland::conf::impl::placeholder {

using namespace text::literals;

PlaceholderTextParser::PlaceholderTextParser(TokenDecoder &decoder, text::StringEditor &target) noexcept :
    _decoder{decoder}, _target{target} {
}

void PlaceholderTextParser::parse() {
    const auto startLocation = _decoder.location();
    try {
        _decoder.next();
        if (_decoder.character() != nc::openingCurlyBracket) {
            _target.append(nc::dollar);
            return;
        }
        _decoder.next();
        auto source = parsePart();
        auto value = _decoder.resolvePlaceholder(source.name, source.parameter);
        auto filterCount = std::size_t{};
        while (_decoder.character() == nc::pipe) {
            ++filterCount;
            if (filterCount > 16U) {
                _decoder.throwLimitExceededError("A placeholder can contain at most 16 filters."_el);
            }
            _decoder.next();
            const auto filter = parsePart();
            value = _decoder.applyPlaceholderFilter(filter.name, filter.parameter, value);
        }
        _decoder.expectAndNext(nc::closingCurlyBracket, "Expected a closing brace for the placeholder."_el);
        _target.append(value);
    } catch (const ConfError &error) {
        throw error.withLocation(startLocation);
    }
}

auto PlaceholderTextParser::parsePart() -> Part {
    auto result = Part{};
    result.name = normalizeName(parseContent(true));
    if (_decoder.character() == nc::colon) {
        _decoder.next();
        result.parameter = parseContent(false);
    }
    return result;
}

auto PlaceholderTextParser::parseContent(const bool isName) -> text::String {
    auto result = text::StringEditor{};
    while (true) {
        if (_decoder.character().isEndOfData()) {
            _decoder.throwUnexpectedEndOfDataError("Unexpected end inside a placeholder."_el);
        }
        if (_decoder.character() == CharClass::LineBreak) {
            _decoder.throwSyntaxError("A placeholder cannot cross a physical line."_el);
        }
        if (_decoder.character() == nc::pipe || _decoder.character() == nc::closingCurlyBracket ||
            (isName && _decoder.character() == nc::colon)) {
            return text::String{result};
        }
        if (_decoder.character() == nc::backslash) {
            _decoder.next();
            lexer::parseTextEscapeSequence(_decoder, result);
            continue;
        }
        if (_decoder.character() == nc::dollar) {
            _decoder.next();
            if (_decoder.character() == nc::openingCurlyBracket) {
                _decoder.throwSyntaxError("Nested placeholders are not allowed."_el);
            }
            result.append(nc::dollar);
            continue;
        }
        _decoder.checkForErrorAndThrowIt();
        result.append(_decoder.character());
        _decoder.next();
    }
}

auto PlaceholderTextParser::normalizeName(const text::String &name) -> text::String {
    auto result = Name::normalize(name);
    if (result.startsWith("@"_el)) {
        throw ConfError{ConfErrorCategory::Syntax, "Placeholder names must not be meta names."_el};
    }
    return result;
}

}
