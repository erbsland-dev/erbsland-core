// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Http1FramingParser.hpp"

#include "../HttpGrammar.hpp"

#include "../../../../err/OverflowError.hpp"
#include "../../../../err/ParseError.hpp"
#include "../../../../text/AnyString.hpp"
#include "../../../../text/AsciiCategory.hpp"
#include "../../../../text/IntegerBase.hpp"
#include "../../../../text/IntegerParseOptions.hpp"
#include "../../../../text/Literals.hpp"
#include "../../../../text/ParseNumberError.hpp"
#include "../../../../text/StringCharReader.hpp"

namespace erbsland::network::impl {

using namespace text;
using namespace text::literals;

Http1FramingParser::Http1FramingParser(const HttpHeaders &headers) {
    const auto fields = headers.fields();
    for (const auto &field : fields) {
        if (field.name().type() == HttpFieldType::ContentLength) {
            parseContentLength(field.value());
        } else if (field.name().type() == HttpFieldType::TransferEncoding) {
            parseTransferEncoding(field.value());
        }
    }
    if (_codingCount > 1U) {
        _transferCoding = Http1TransferCoding::Unsupported;
    }
}

void Http1FramingParser::parseContentLength(const String &text) {
    static const auto cNumberOptions = []() -> IntegerParseOptions {
        auto result = IntegerParseOptions{};
        result.setFixedBase(IntegerBase::Decimal).setMinimumDigits(unit::CpLength::one());
        return result;
    }();
    auto reader = StringCharReader{text};
    if (reader.isAtEnd()) {
        throw err::ParseError{"Content-Length must not be empty."_el};
    }
    for (;;) {
        reader.advanceWhile(AsciiCategory::Blank);
        reader.startCapture();
        reader.advanceWhile(AsciiCategory::Digit);
        const auto digits = reader.takeCapture().toString();
        if (digits.isEmpty()) {
            throw err::ParseError{"Content-Length has a malformed list value."_el};
        }
        auto value = std::uint64_t{};
        try {
            value = digits.toIntegerOrThrow<std::uint64_t>(cNumberOptions);
        } catch (const ParseNumberError &) {
            throw err::ParseError{"Content-Length has a malformed numeric value."_el};
        } catch (const err::OverflowError &) {
            throw err::ParseError{"Content-Length overflows its numeric range."_el};
        }
        if (_contentLength && *_contentLength != value) {
            throw err::ParseError{"Content-Length values disagree."_el};
        }
        _contentLength = value;
        reader.advanceWhile(AsciiCategory::Blank);
        if (reader.isAtEnd()) {
            break;
        }
        if (!reader.advanceIf(U',')) {
            throw err::ParseError{"Content-Length has a malformed list delimiter."_el};
        }
        if (reader.isAtEnd()) {
            throw err::ParseError{"Content-Length has an empty list member."_el};
        }
    }
}

void Http1FramingParser::parseTransferEncoding(const String &text) {
    auto reader = StringCharReader{text};
    if (reader.isAtEnd()) {
        throw err::ParseError{"Transfer-Encoding must not be empty."_el};
    }
    for (;;) {
        reader.advanceWhile(AsciiCategory::Blank);
        reader.startCapture();
        reader.advanceWhile(AsciiCategory::HttpToken);
        const auto coding = reader.takeCapture().toString();
        if (coding.isEmpty()) {
            throw err::ParseError{"Transfer-Encoding has a malformed list value."_el};
        }
        ++_codingCount;
        _transferCoding = http_grammar::equalTokenCI(coding, "chunked"_el) ? Http1TransferCoding::Chunked
                                                                           : Http1TransferCoding::Unsupported;
        reader.advanceWhile(AsciiCategory::Blank);
        if (reader.isAtEnd()) {
            break;
        }
        if (!reader.advanceIf(U',')) {
            throw err::ParseError{"Transfer-Encoding has a malformed list delimiter."_el};
        }
        if (reader.isAtEnd()) {
            throw err::ParseError{"Transfer-Encoding has an empty list member."_el};
        }
    }
}

auto Http1FramingParser::chunkSize(const String &line) -> std::uint64_t {
    static const auto cNumberOptions = []() -> IntegerParseOptions {
        auto result = IntegerParseOptions{};
        result.setFixedBase(IntegerBase::Hexadecimal).setMinimumDigits(unit::CpLength::one());
        return result;
    }();
    auto reader = StringCharReader{line};
    reader.startCapture();
    reader.advanceWhile(AsciiCategory::HexDigit);
    const auto digits = reader.takeCapture().toString();
    if (digits.isEmpty()) {
        throw err::ParseError{"A chunk-size line requires hexadecimal digits."_el};
    }
    auto result = std::uint64_t{};
    try {
        result = digits.toIntegerOrThrow<std::uint64_t>(cNumberOptions);
    } catch (const ParseNumberError &) {
        throw err::ParseError{"The chunk size is malformed."_el};
    } catch (const err::OverflowError &) {
        throw err::ParseError{"The chunk size overflows its numeric range."_el};
    }
    while (!reader.isAtEnd()) {
        reader.advanceWhile(AsciiCategory::Blank);
        if (reader.isAtEnd() || !reader.advanceIf(U';')) {
            throw err::ParseError{"A chunk extension is malformed."_el};
        }
        reader.advanceWhile(AsciiCategory::Blank);
        reader.startCapture();
        reader.advanceWhile(AsciiCategory::HttpToken);
        if (reader.takeCapture().toString().isEmpty()) {
            throw err::ParseError{"A chunk extension name is missing."_el};
        }
        reader.advanceWhile(AsciiCategory::Blank);
        if (reader.advanceIf(U'=')) {
            reader.advanceWhile(AsciiCategory::Blank);
            if (reader.advanceIf(U'\"')) {
                auto closed = false;
                while (!reader.isAtEnd()) {
                    auto character = reader.read();
                    if (character == U'\"') {
                        closed = true;
                        break;
                    }
                    if (character == U'\\') {
                        if (reader.isAtEnd()) {
                            break;
                        }
                        character = reader.read();
                    }
                    if ((character.isAsciiControl() && character != U'\t') || character == U'\x7f') {
                        throw err::ParseError{"A chunk extension contains a prohibited control."_el};
                    }
                }
                if (!closed) {
                    throw err::ParseError{"A quoted chunk extension is not terminated."_el};
                }
            } else {
                reader.startCapture();
                reader.advanceWhile(AsciiCategory::HttpToken);
                if (reader.takeCapture().toString().isEmpty()) {
                    throw err::ParseError{"A chunk extension value is missing."_el};
                }
            }
        }
        reader.advanceWhile(AsciiCategory::Blank);
    }
    return result;
}

}
