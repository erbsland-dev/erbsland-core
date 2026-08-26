// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ValueBytes.hpp"

#include "Core.hpp"
#include "ValueMultiLine.hpp"

#include "../char/NamedChars.hpp"
#include "../utilities/YieldMacros.hpp"

#include "../../../mem/ByteBlockEditor.hpp"
#include "../../../text/AsciiCategory.hpp"

namespace erbsland::conf::impl::lexer {

using namespace text::literals;

auto scanSingleLineFormatIdentifier(TokenDecoder &decoder) -> text::String {
    auto prefixTransaction = Transaction{decoder};
    auto formatIdentifier = scanFormatOrLanguageIdentifier(decoder, false);
    if (!formatIdentifier.isEmpty() && decoder.character() == nc::colon) {
        decoder.next(); // after the colon, the actual bytes start.
        prefixTransaction.commit();
        return formatIdentifier;
    }
    // Without colon at the end, this must be hex bytes.
    prefixTransaction.rollback();
    return "hex"_el;
}

auto scanBytes(TokenDecoder &decoder) -> std::optional<LexerToken> {
    if (decoder.character() != nc::lessThan) {
        return std::nullopt;
    }
    decoder.next();
    decoder.expectMoreInLine("Unexpected end in bytes value."_el);
    // Check for a format identifier after the opening angle bracket.
    auto formatIdentifier = scanSingleLineFormatIdentifier(decoder);
    if (formatIdentifier != "hex"_el) {
        decoder.throwError(ConfErrorCategory::Unsupported, "Unknown bytes-data format."_el);
    }
    decoder.expectMoreInLine("Unexpected end in bytes value."_el);
    auto bytes = mem::ByteBlockEditor{};
    while (decoder.character() != nc::greaterThan) {
        decoder.expectMoreInLine("Unexpected end in bytes value."_el);
        decoder.advanceWhile(text::AsciiCategory::Blank);
        decoder.checkForErrorAndThrowIt();
        if (decoder.character() == nc::greaterThan) {
            break; // Valid end of bytes.
        }
        decoder.expectMoreInLine("Unexpected end in bytes value."_el);
        if (decoder.character() != CharClass::HexDigit) {
            decoder.throwSyntaxError("Expected first hex digit of a byte, got something else."_el);
        }
        auto value =
            static_cast<uint8_t>(decoder.character().digitValue(text::IntegerBase::Hexadecimal).value_or(0U) << 4U);
        decoder.next();
        decoder.expectMoreInLine("Unexpected end in bytes value."_el);
        if (decoder.character() != CharClass::HexDigit) {
            decoder.throwSyntaxError("Expected second hex digit of a byte, got something else."_el);
        }
        value |= static_cast<uint8_t>(decoder.character().digitValue(text::IntegerBase::Hexadecimal).value_or(0U));
        decoder.next();
        bytes.append(mem::Byte{value});
    }
    decoder.next();
    return decoder.createToken(TokenType::Bytes, mem::ByteBlock{bytes});
}

auto parseMultiLineBytesHexLine(TokenDecoder &decoder) -> TokenGenerator {
    // Initial check so we avoid creating a mem::ByteBlock object.
    if (!isAtMultiLineEnd(decoder, TokenType::MultiLineBytes)) {
        mem::ByteBlockEditor decodedBytes;
        // Carefully consume the text block by block, so we can skip trailing spacing.
        while (!isAtMultiLineEnd(decoder, TokenType::MultiLineBytes)) {
            decoder.advanceWhile(text::AsciiCategory::Blank);
            decoder.checkForErrorAndThrowIt();
            if (isAtMultiLineEnd(decoder, TokenType::MultiLineBytes)) {
                break;
            }
            if (decoder.character() != CharClass::HexDigit) {
                decoder.throwSyntaxError("Expected first hex digit of a byte, got something else."_el);
            }
            auto value =
                static_cast<uint8_t>(decoder.character().digitValue(text::IntegerBase::Hexadecimal).value_or(0U) << 4U);
            decoder.next();
            if (isAtMultiLineEnd(decoder, TokenType::MultiLineBytes)) {
                decoder.throwSyntaxError("Expected second hex digit of a byte, not the end of the line."_el);
            }
            if (decoder.character() != CharClass::HexDigit) {
                decoder.throwSyntaxError("Expected second hex digit of a byte, got something else."_el);
            }
            value |= static_cast<uint8_t>(decoder.character().digitValue(text::IntegerBase::Hexadecimal).value_or(0U));
            decoder.next();
            decodedBytes.append(mem::Byte{value});
        }
        EL_YIELD_TOKEN(TokenType::MultiLineBytes, mem::ByteBlock{decodedBytes});
    }
    // Read the end-of-line tokens (may include a comment if at #).
    EL_YIELD_FROM(expectEndOfLine(decoder, ExpectMore::No));
    // Do the check for more data after creating all tokens for the line.
    decoder.expectMore("Unexpected end in a multi-line bytes-data."_el);
    co_return;
}

auto expectMultiLineBytes(TokenDecoder &decoder) -> TokenGenerator {
    // expect to be at the character just after the opening angle bracket.
    decoder.expectMore("Unexpected end in bytes value."_el);
    if (auto formatIdentifier = scanFormatOrLanguageIdentifier(decoder, true); !formatIdentifier.isEmpty()) {
        if (decoder.character() != CharClass::EndOfLineStart) {
            decoder.throwSyntaxError("Unexpected characters in bytes-data format identifier."_el);
        }
        if (formatIdentifier != "hex"_el) {
            decoder.throwError(ConfErrorCategory::Unsupported, "Unknown bytes-data format."_el);
        }
        EL_YIELD_TOKEN(TokenType::MultiLineBytesFormat, std::move(formatIdentifier));
    }
    if (decoder.character() != CharClass::EndOfLineStart) {
        decoder.throwSyntaxError("Unexpected characters in bytes-data format identifier"_el);
    }
    // Process any text following the opening bracket sequence.
    EL_YIELD_FROM(expectMultiLineAfterOpen(decoder));
    // Next, process the bytes data, line by line.
    // At the start of this while loop, the decoder should be at the indented continued line.
    while (!decoder.character().isEndOfData()) {
        // Test if we get the closing bracket sequence.
        if (auto closeToken = scanMultiLineClose(decoder, TokenType::MultiLineBytesOpen); closeToken.has_value()) {
            co_yield std::move(closeToken).value();
            co_return;
        }
        EL_YIELD_FROM(parseMultiLineBytesHexLine(decoder));
        // if the following line starts with spacing, expect the correct indentation pattern.
        if (decoder.character() == CharClass::Spacing) {
            EL_YIELD(expectAndCheckIndentation(decoder));
            decoder.expectMore("Unexpected end in multi-line byte-data."_el);
        } else if (decoder.character() != CharClass::LineBreak) {
            decoder.throwSyntaxError("Missing indentation in multi-line byte-data."_el);
        }
    }
    // Unexpected, if the data ends, just after the opening sequence.
    decoder.throwUnexpectedEndOfDataError("Unexpected end in multi-line byte-data."_el);
}

}
