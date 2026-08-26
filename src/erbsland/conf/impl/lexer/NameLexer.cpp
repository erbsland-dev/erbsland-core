// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "NameLexer.hpp"

#include "Name.hpp"
#include "Number.hpp"
#include "Text.hpp"

#include "../char/NamedChars.hpp"
#include "../constants/Limits.hpp"

#include "../../../text/AsciiCategory.hpp"
#include "../../../text/StringEditor.hpp"

#include <limits>

namespace erbsland::conf::impl {

using namespace text::literals;

void NameLexer::initialize() {
    if (_decoder.buffer().length().toSizeT() > limits::maxLineLength) {
        _decoder.throwLimitExceededError("A name path must not exceed 4kb."_el);
    }
    _decoder.initialize();
}

auto NameLexer::next() -> Name {
    _decoder.advanceWhile(text::AsciiCategory::Blank);
    bool readSeparator = false;
    if (_decoder.character().isEndOfData()) {
        return {}; // Coverage: This prevents misuse and is not used for correct operation.
    }
    if (_decoder.character() == nc::namePathSeparator) {
        if (!_afterFirstElement) {
            _decoder.throwSyntaxError("The name path must not start with a separator."_el);
        }
        _decoder.next();
        _decoder.advanceWhile(text::AsciiCategory::Blank); // Ignore spacing after the separator.
        if (_decoder.character().isEndOfData()) {
            _decoder.throwUnexpectedEndOfDataError("Name path must not end with a separator."_el);
        }
        readSeparator = true;
    }
    if (_decoder.character() == CharClass::Letter || _decoder.character() == nc::at) {
        return expectRegularName();
    }
    if (_decoder.character() == nc::doubleQuote) {
        return expectTextNameOrIndex();
    }
    if (_decoder.character() == nc::openingSquareBracket) {
        if (readSeparator) {
            _decoder.throwSyntaxError("An index must not be preceded by a separator."_el);
        }
        return expectIndex();
    }
    if (_decoder.character() == CharClass::DecimalDigit) {
        _decoder.throwSyntaxError("Regular names must not start with a digit."_el);
    }
    if (_decoder.character() == nc::underscore) {
        _decoder.throwSyntaxError("Regular names must not start with an underscore."_el);
    }
    if (_decoder.character() == nc::namePathSeparator) {
        _decoder.throwSyntaxError("Name path must not contain multiple subsequent separators."_el);
    }
    _decoder.throwSyntaxError("Expected regular name, text name or index but got something else."_el);
}

void NameLexer::expectNameSeparatorOrEnd() {
    _decoder.advanceWhile(text::AsciiCategory::Blank);
    if (!(_decoder.character() == nc::namePathSeparator || _decoder.character().isEndOfData())) {
        _decoder.throwSyntaxError(
            "Unexpected character after the last element. Expected name separator or the end of the path."_el);
    }
}

void NameLexer::expectNameSeparatorIndexOrEnd() {
    _decoder.advanceWhile(text::AsciiCategory::Blank);
    if (!(_decoder.character() == nc::namePathSeparator || _decoder.character() == nc::openingSquareBracket ||
            _decoder.character().isEndOfData())) {
        _decoder.throwSyntaxError(
            "Unexpected character after the last element. Expected name separator or the end of the path."_el);
    }
}

auto NameLexer::expectGenericIndex() -> std::size_t {
    _decoder.next(); // Skip the opening bracket.
    _decoder.advanceWhile(text::AsciiCategory::Blank);
    auto result =
        lexer::parseNumber(_decoder, text::IntegerBase::Decimal, lexer::Sign::Positive, lexer::NumberSeparators::Yes);
    _decoder.advanceWhile(text::AsciiCategory::Blank);
    if (_decoder.character() != nc::closingSquareBracket) {
        _decoder.throwSyntaxError("An index must end with a closing bracket."_el);
    }
    _decoder.next();
    if (result.value() < 0) {
        _decoder.throwSyntaxError("Index values must not be negative."_el);
    }
    const auto indexValue = static_cast<std::uint64_t>(result.value());
    if (indexValue > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        _decoder.throwLimitExceededError("The index value is too large."_el);
    }
    return static_cast<std::size_t>(indexValue);
}

auto NameLexer::expectRegularName() -> Name {
    auto [isMetaName, name] = lexer::expectRegularOrMetaName(_decoder, lexer::AcceptedNameEnd::NamePath);
    expectNameSeparatorIndexOrEnd();
    _afterFirstElement = true;
    return Name{NameType::Regular, Name::Storage{std::move(name)}, Name::PrivateTag{}};
}

auto NameLexer::expectTextNameOrIndex() -> Name {
    _decoder.next(); // Skip the opening quote.
    text::StringEditor text;
    lexer::parseText(_decoder, text);
    if (text.isEmpty()) { // empty string? must be a text index.
        if (_decoder.character() != nc::openingSquareBracket) {
            _decoder.throwSyntaxError("Empty text names are not allowed, unless followed by an index."_el);
        }
        auto index = expectGenericIndex();
        expectNameSeparatorOrEnd();
        _afterFirstElement = true;
        return Name::createTextIndex(index);
    }
    expectNameSeparatorOrEnd();
    _afterFirstElement = true;
    return Name::createText(std::move(text));
}

auto NameLexer::expectIndex() -> Name {
    auto index = expectGenericIndex();
    expectNameSeparatorIndexOrEnd();
    _afterFirstElement = true;
    return Name::createIndex(index);
}

}
