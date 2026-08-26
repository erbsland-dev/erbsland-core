// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Tokenizer.hpp"

#include "../RenderError.hpp"

#include "../../../text/AnyString.hpp"
#include "../../../text/IntegerBase.hpp"
#include "../../../text/Literals.hpp"
#include "../../../text/StringSide.hpp"
#include "../../../text/u8/U8StringConstIterator.hpp"
#include "../../../unit/ColumnIndex.hpp"
#include "../../../unit/CpIndex.hpp"
#include "../../../unit/LineIndex.hpp"

namespace erbsland::text::render::impl {

using namespace text::literals;

auto Tokenizer::readNumber(const unit::CodeLocation location) -> Token {
    _reader.startCapture();
    const auto hasIntegerDigits = !readWhile(AsciiCategory::Digit).isZero();
    auto isFloat = false;
    if (!isAtClosingDelimiter() && _reader.peek() == U'.') {
        isFloat = true;
        read();
        const auto hasFractionDigits = !readWhile(AsciiCategory::Digit).isZero();
        if (!hasIntegerDigits && !hasFractionDigits) {
            throwSyntax("Invalid layout expression"_el, "A floating-point literal requires digits."_el, location);
        }
    }
    if (!isAtClosingDelimiter() && (_reader.peek() == U'e' || _reader.peek() == U'E')) {
        isFloat = true;
        read();
        if (!isAtClosingDelimiter() && (_reader.peek() == U'+' || _reader.peek() == U'-')) {
            read();
        }
        if (readWhile(AsciiCategory::Digit).isZero()) {
            throwSyntax("Invalid layout expression"_el, "A floating-point exponent requires digits."_el, location);
        }
    }
    if (!hasIntegerDigits && !isFloat) {
        throwSyntax("Invalid layout expression"_el, "A numeric literal requires digits."_el, location);
    }
    return Token{isFloat ? TokenKind::Float : TokenKind::Integer, _reader.takeCapture().toString(), location};
}

auto Tokenizer::readString(const Char quote, const unit::CodeLocation location) -> Token {
    _reader.clearBuffer();
    while (!_reader.isAtEnd()) {
        const auto characterLocation = _location;
        const auto character = read();
        if (character == quote) {
            return Token{TokenKind::String, _reader.takeBuffer().toString(), location};
        }
        if (character.isAsciiControl()) {
            throwSyntax(
                "Invalid layout expression"_el,
                "A string literal contains an unescaped control character."_el,
                characterLocation);
        }
        if (character != U'\\') {
            _reader.appendToBuffer(character);
            continue;
        }
        if (_reader.isAtEnd()) {
            throwSyntax(
                "Invalid layout expression"_el,
                "A string literal ends in an incomplete escape sequence."_el,
                characterLocation);
        }
        const auto escapedLocation = _location;
        const auto escaped = read();
        switch (escaped.toRawValue()) {
        case U'\\':
        case U'\'':
        case U'"':
            _reader.appendToBuffer(escaped);
            break;
        case U'b':
            _reader.appendToBuffer(U'\b');
            break;
        case U'f':
            _reader.appendToBuffer(U'\f');
            break;
        case U'n':
            _reader.appendToBuffer(U'\n');
            break;
        case U'r':
            _reader.appendToBuffer(U'\r');
            break;
        case U't':
            _reader.appendToBuffer(U'\t');
            break;
        case U'u':
            _reader.appendToBuffer(readUnicodeEscape(escapedLocation));
            break;
        default:
            throwSyntax(
                "Invalid layout expression"_el,
                "A string literal contains an invalid escape sequence."_el,
                escapedLocation);
        }
    }
    throwSyntax("Invalid layout expression"_el, "A string literal is not terminated."_el, location);
}

auto Tokenizer::readUnicodeEscape(const unit::CodeLocation location) -> Char {
    const auto first = readHexCodeUnit(location);
    if (Char::isHighSurrogate(first)) {
        if (read() != U'\\' || read() != U'u') {
            throwSyntax(
                "Invalid layout expression"_el,
                "A high-surrogate escape requires a following low-surrogate escape."_el,
                location);
        }
        const auto second = readHexCodeUnit(location);
        if (!Char::isLowSurrogate(second)) {
            throwSyntax(
                "Invalid layout expression"_el,
                "A high-surrogate escape is followed by an invalid low surrogate."_el,
                location);
        }
        const auto codePoint = char32_t{0x10000U} + ((static_cast<char32_t>(first) - 0xD800U) << 10U) +
            (static_cast<char32_t>(second) - 0xDC00U);
        return Char{codePoint};
    }
    if (Char::isLowSurrogate(first)) {
        throwSyntax(
            "Invalid layout expression"_el, "A low-surrogate escape has no preceding high surrogate."_el, location);
    }
    const auto result = Char{static_cast<char32_t>(first)};
    if (!result.isValidUnicode()) {
        throwSyntax(
            "Invalid layout expression"_el, "A Unicode escape does not represent a valid text character."_el, location);
    }
    return result;
}

auto Tokenizer::readHexCodeUnit(const unit::CodeLocation location) -> uint16_t {
    auto result = uint16_t{0U};
    for (auto index = 0U; index < 4U; ++index) {
        const auto value = _reader.peek().digitValue(IntegerBase::Hexadecimal);
        if (!value.has_value()) {
            throwSyntax(
                "Invalid layout expression"_el, "A Unicode escape requires four hexadecimal digits."_el, location);
        }
        result = static_cast<uint16_t>((result * 16U) + value.value());
        read();
    }
    return result;
}

}
