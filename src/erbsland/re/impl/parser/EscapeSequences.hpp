// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ParserState.hpp"

#include "../text/CharClass.hpp"

#include "../../../text/Literals.hpp"
#include "../../../text/StringFormat.hpp"

#include <thread>
#include <vector>

namespace erbsland::re::impl::parser {

using namespace text::literals;

/// Read a fixed or dynamic character number.
/// @param state The state.
/// @param base The number base.
/// @param fixedDigits The number of digits to read. If 0, read until a '}' ends of the number.
/// @return The character.
inline auto readCharNumber(ParserState &state, const text::IntegerBase base, const std::size_t fixedDigits = 0)
    -> text::Char {
    if (!state.currentChar().isDigitValue(base)) {
        state.throwParsingError(text::StringFormat{"Expected a {} digit after '{{'"}.build(base.toString()));
    }
    uint32_t result = 0;
    std::size_t digitCount = 0;
    while (state.currentChar().isDigitValue(base)) {
        if (fixedDigits == 0) {
            if (digitCount > (base == text::IntegerBase::Hexadecimal ? 8U : 11U)) {
                state.throwParsingError(text::StringFormat{"Too many {} digits"}.build(base.toString()));
            }
        } else if (digitCount > fixedDigits) {
            state.throwParsingError(
                text::StringFormat{"Too many {} digits. Expected {} digits"}.build(base.toString(), fixedDigits));
        }
        result <<= (base == text::IntegerBase::Hexadecimal ? 4U : 3U);
        result |= state.currentChar().digitValue().value();
        digitCount += 1;
        state.readNext();
    }
    if (fixedDigits == 0) {
        if (state.currentChar() != U'}') {
            state.throwParsingError("Expected '}' after character number"_el);
        }
        state.readNext();
    } else if (digitCount < fixedDigits) {
        state.throwParsingError(
            text::StringFormat{"Expected {} {} digits, just got {}"}.build(fixedDigits, base.toString(), digitCount));
    }
    // Validate Unicode code point range for all numeric escapes
    const auto ch = text::Char{static_cast<char32_t>(result)};
    if (!ch.isValidUnicode()) {
        state.throwParsingError("Character code is out of the valid Unicode range"_el);
    }
    return ch;
}

/// Read octal character
inline auto readOctalChar(ParserState &state) -> text::Char {
    if (!state.hasFeature(Feature::EscapeOctal)) {
        state.throwParsingError("This legacy escape sequence is not supported"_el);
    }
    state.readNext();
    if (state.currentChar() != U'{') {
        state.throwParsingError("Expected '{' after 'o'"_el);
    }
    state.readNext();
    return readCharNumber(state, text::IntegerBase::Octal);
}

/// Read hexadecimal character
inline auto readHexadecimalChar(ParserState &state) -> text::Char {
    if (!state.hasFeature(Feature::EscapeHex)) {
        state.throwParsingError("This legacy escape sequence is not supported"_el);
    }
    state.readNext();
    if (state.currentChar() == U'{') {
        // \x{...}
        state.readNext();
        return readCharNumber(state, text::IntegerBase::Hexadecimal);
    }
    // \xHH
    return readCharNumber(state, text::IntegerBase::Hexadecimal, 2U);
}

/// Handle unicode character.
inline auto readUnicodeChar(ParserState &state) -> text::Char {
    // consume the 'u' or 'U'
    const auto isUpper = (state.readNextAndExchange() == U'U');
    if (state.currentChar() == U'{') {
        // \u{...} or \U{...}
        state.readNext();
        return readCharNumber(state, text::IntegerBase::Hexadecimal);
    }
    // \uHHHH or \UHHHHHHHH
    if (isUpper && !state.hasFeature(Feature::EscapeLongUnicode)) {
        state.throwParsingError("This escape sequence is not supported"_el);
    }
    return readCharNumber(state, text::IntegerBase::Hexadecimal, isUpper ? 8U : 4U);
}

/// Read a Unicode property syntax
inline auto readUnicodeProperty(ParserState &state) -> std::tuple<Category, bool> {
    const auto initialChar = state.readNextAndExchange();
    const auto isNegated = initialChar == U'P';
    if (state.currentChar() != U'{') {
        state.throwParsingError(
            text::StringFormat{"Expected '{{' after '\\{}'"}.build(text::String::fromCharacter(initialChar)));
    }
    state.readNext();
    text::String propertyString;
    propertyString.reserve(unit::ByteLength{Category::maximumNameLength});
    std::size_t totalReadCount = 0;
    std::size_t propertyStringLength = 0;
    while (state.currentChar() != U'}') {
        totalReadCount += 1; // read safety.
        if (propertyStringLength > Category::maximumNameLength || totalReadCount > (Category::maximumNameLength + 10)) {
            state.throwParsingError("Unicode property name is too long"_el);
        }
        if (state.isAtEnd()) {
            state.throwParsingError("Unexpected end in property name"_el);
        }
        if (!state.currentChar().isAsciiWord()) {
            state.throwParsingError("Invalid character in Unicode property name"_el);
        }
        if (state.currentChar() == U'_') {
            state.readNext();
            continue; // Ignore underscore characters.
        }
        propertyString.append(state.currentChar().toAsciiLowercase());
        state.readNext();
        propertyStringLength += 1;
    }
    auto characterClass = Category::fromString(propertyString);
    if (characterClass == Category::None) {
        state.throwParsingError("Unknown property name"_el);
    }
    // consume the closing '}'
    state.readNext();
    return {characterClass, isNegated};
}

inline auto readPcreControlCharacter(ParserState &state) -> text::Char {
    if (!state.hasFeature(Feature::EscapeControl)) {
        state.throwParsingError("This legacy escape sequence is not supported"_el);
    }
    state.readNext(); // consume the 'c'
    if (state.isAtEnd()) {
        state.throwParsingError("Unexpected end of the pattern after '\\c'"_el);
    }
    const auto nextChar = state.readNextAndExchange();
    if (!nextChar.isAsciiLetter()) {
        state.throwParsingError("Expected an ASCII letter after '\\c'"_el);
    }
    return text::Char{static_cast<char32_t>(nextChar.toRawValue() & 0x001FU)};
}

}
