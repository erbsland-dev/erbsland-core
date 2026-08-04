// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "JsonParser.hpp"

#include "../../../err/Exception.hpp"
#include "../../../err/ParseError.hpp"
#include "../../../math/SignedMagnitude.hpp"
#include "../../AnyString.hpp"
#include "../../FloatParseOptions.hpp"
#include "../../IntegerParseOptions.hpp"
#include "../../Literals.hpp"
#include "../../StringEditor.hpp"

#include <cmath>

namespace erbsland::text::json::impl {

using namespace text::literals;

JsonParser::JsonParser(String text, JsonParseOptions options) :
    _text{std::move(text)}, _options{options}, _reader{_text} {
}

auto JsonParser::parse() -> JsonValue {
    if (_text.length() > _options.maximumInputLength()) {
        fail("The JSON document exceeds the configured input limit."_el);
    }
    skipWhitespace();
    const auto result = parseValue(unit::ItemCount{});
    skipWhitespace();
    if (!_reader.isAtEnd()) {
        fail("Unexpected trailing text after the JSON value."_el);
    }
    return result;
}

auto JsonParser::parseValue(const unit::ItemCount nesting) -> JsonValue {
    countValue();
    const auto next = _reader.peek();
    if (next == U'[') {
        return parseArray(nesting);
    }
    if (next == U'{') {
        return parseObject(nesting);
    }
    if (next == U'"') {
        return JsonValue{parseString()};
    }
    if (next == U't' && _reader.advanceIf("true"_el)) {
        return JsonValue{true};
    }
    if (next == U'f' && _reader.advanceIf("false"_el)) {
        return JsonValue{false};
    }
    if (next == U'n' && _reader.advanceIf("null"_el)) {
        return {};
    }
    if (next == U'-' || next.isAsciiDigit()) {
        return parseNumber();
    }
    fail("Expected a JSON value."_el);
}

auto JsonParser::parseArray(const unit::ItemCount nesting) -> JsonValue {
    const auto nextNesting = nesting + unit::ItemCount::one();
    if (nextNesting > _options.maximumNesting()) {
        fail("The JSON document exceeds the configured nesting limit."_el);
    }
    _reader.advance();
    skipWhitespace();
    auto result = JsonArray{};
    if (_reader.advanceIf(U']')) {
        return JsonValue{std::move(result)};
    }
    while (true) {
        result.append(parseValue(nextNesting));
        skipWhitespace();
        if (_reader.advanceIf(U']')) {
            break;
        }
        if (!_reader.advanceIf(U',')) {
            fail("Expected a comma or closing bracket in the JSON array."_el);
        }
        skipWhitespace();
    }
    return JsonValue{std::move(result)};
}

auto JsonParser::parseObject(const unit::ItemCount nesting) -> JsonValue {
    const auto nextNesting = nesting + unit::ItemCount::one();
    if (nextNesting > _options.maximumNesting()) {
        fail("The JSON document exceeds the configured nesting limit."_el);
    }
    _reader.advance();
    skipWhitespace();
    auto result = JsonObject{};
    if (_reader.advanceIf(U'}')) {
        return JsonValue{std::move(result)};
    }
    while (true) {
        if (_reader.peek() != U'"') {
            fail("Expected a string key in the JSON object."_el);
        }
        const auto key = parseString();
        if (result.contains(key)) {
            fail("The JSON object contains a duplicate key."_el);
        }
        skipWhitespace();
        if (!_reader.advanceIf(U':')) {
            fail("Expected a colon after the JSON object key."_el);
        }
        skipWhitespace();
        result.set(key, parseValue(nextNesting));
        skipWhitespace();
        if (_reader.advanceIf(U'}')) {
            break;
        }
        if (!_reader.advanceIf(U',')) {
            fail("Expected a comma or closing brace in the JSON object."_el);
        }
        skipWhitespace();
    }
    return JsonValue{std::move(result)};
}

auto JsonParser::parseString() -> String {
    static const auto escapeOrEnd = []() -> CharSet {
        auto result = CharSet{U'"', U'\\'};
        result.add(CharRange{0x0000U, 0x001fU}); // ASCII control characters
        return result;
    }();
    if (!_reader.advanceIf(U'"')) {
        fail("Expected a JSON string."_el);
    }
    auto remainingMaximum = _options.maximumStringLength();
    while (true) {
        if (remainingMaximum > unit::CpLength::zero()) {
            const auto result = _reader.readToBufferUntil(escapeOrEnd, remainingMaximum);
            if (result == util::LoopResult::EndOfData) {
                fail("The JSON string is not terminated."_el);
            }
            if (result == util::LoopResult::LimitReached) {
                fail("A JSON string exceeds the configured string-length limit."_el);
            }
        }
        const auto character = _reader.read();
        if (character.isAsciiControl()) {
            fail("A JSON string contains an unescaped control character."_el);
        }
        if (character == U'"') {
            break;
        }
        if (character != U'\\') {
            throw err::LogicError("Unexpected parser state.");
        }
        parseEscapeSequence();
        if (_reader.bufferCharacterLength() > _options.maximumStringLength()) {
            fail("A JSON string exceeds the configured string-length limit."_el);
        }
        remainingMaximum = _options.maximumStringLength() - _reader.bufferCharacterLength();
    }
    return _reader.takeBuffer().toString();
}

void JsonParser::parseEscapeSequence() {
    // The backslash was already consumed.
    const auto escaped = _reader.read();
    switch (escaped.toRawValue()) {
    case U'"':
    case U'\\':
    case U'/':
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
        try {
            constexpr auto hexCodeOptions = IntegerParseOptions::fixedHex(unit::CpLength{4});
            const auto first = _reader.readIntegerOrThrow<uint16_t>(hexCodeOptions);
            if (Char::isHighSurrogate(first)) {
                if (!_reader.advanceIf(U'\\') || !_reader.advanceIf(U'u')) {
                    fail("A JSON high surrogate requires a following low surrogate."_el);
                }
                const auto second = _reader.readIntegerOrThrow<uint16_t>(hexCodeOptions);
                if (!Char::isLowSurrogate(second)) {
                    fail("A JSON high surrogate is followed by an invalid low surrogate."_el);
                }
                const auto codePoint = char32_t{0x10000U} + ((static_cast<char32_t>(first) - 0xD800U) << 10U) +
                    (static_cast<char32_t>(second) - 0xDC00U);
                _reader.appendToBuffer(codePoint);
            } else if (Char::isLowSurrogate(first)) {
                fail("A JSON low surrogate has no preceding high surrogate."_el);
            } else {
                _reader.appendToBuffer(static_cast<char32_t>(first));
            }
            break;
        } catch (const err::ParseError &) {
            fail("The JSON string contains an invalid escape sequence."_el);
        }
    default:
        fail("The JSON string contains an invalid escape sequence."_el);
    }
}

auto JsonParser::parseNumber() -> JsonValue {
    static const auto asciiDigits = CharSet::from(AsciiCategory::Digit);
    static const auto exponent = CharSet{U'e', U'E'};
    static const auto sign = CharSet{U'+', U'-'};
    // Enforce a stricter JSON number format, capture the number text for later conversion.
    _reader.startCapture();
    _reader.advanceIf(U'-');
    if (_reader.advanceIf(U'0')) {
        if (_reader.peek().isAsciiDigit()) {
            fail("A JSON number must not contain leading zeroes."_el);
        }
    } else {
        if (!_reader.peek().isAsciiDigit() || _reader.peek() == U'0') {
            fail("A JSON number requires an integer part."_el);
        }
        _reader.advanceWhile(asciiDigits);
    }
    auto isInteger = true;
    if (_reader.advanceIf(U'.')) {
        isInteger = false;
        if (!_reader.peek().isAsciiDigit()) {
            fail("A JSON fraction requires at least one digit."_el);
        }
        _reader.advanceWhile(asciiDigits);
    }
    if (_reader.advanceIf(exponent)) {
        isInteger = false;
        _reader.advanceIf(sign);
        if (_reader.advanceWhile(asciiDigits) == unit::CpLength::zero()) {
            fail("A JSON exponent requires at least one digit."_el);
        }
    }
    const auto numberText = _reader.takeCapture().toString();
    if (isInteger) {
        try {
            return JsonValue{numberText.toIntegerOrThrow<int64_t>()};
        } catch (const err::Exception &) { // NOLINT(*-empty-catch)
            // Continue with the floating-point representation for large lexical integers.
        }
    }
    try {
        const auto value = numberText.toFloatOrThrow<double>();
        if (!std::isfinite(value)) {
            fail("The JSON number is outside the finite floating-point range."_el);
        }
        return JsonValue{value};
    } catch (const err::Exception &) {
        fail("The JSON number is outside the supported range."_el);
    }
}

void JsonParser::skipWhitespace() {
    const auto whitespace = CharSet{U' ', U'\t', U'\n', U'\r'};
    _reader.advanceWhile(whitespace);
}

void JsonParser::countValue() {
    ++_valueCount;
    if (_valueCount > _options.maximumValueCount()) {
        fail("The JSON document exceeds the configured value-count limit."_el);
    }
}

void JsonParser::fail(const String &reason) const {
    throw err::ParseError{reason, _reader.position()};
}
}
