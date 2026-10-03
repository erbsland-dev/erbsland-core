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

using namespace mem;
using namespace text;
using namespace text::literals;

auto scanSingleLineFormatIdentifier(TokenDecoder &decoder) -> String {
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
    auto bytes = ByteBlockEditor{};
    while (decoder.character() != nc::greaterThan) {
        decoder.expectMoreInLine("Unexpected end in bytes value."_el);
        decoder.advanceWhile(AsciiCategory::Blank);
        decoder.checkForErrorAndThrowIt();
        if (decoder.character() == nc::greaterThan) {
            break; // Valid end of bytes.
        }
        decoder.expectMoreInLine("Unexpected end in bytes value."_el);
        if (decoder.character() != CharClass::HexDigit) {
            decoder.throwSyntaxError("Expected first hex digit of a byte, got something else."_el);
        }
        auto value = static_cast<uint8_t>(decoder.character().digitValue(IntegerBase::Hexadecimal).value_or(0U) << 4U);
        decoder.next();
        decoder.expectMoreInLine("Unexpected end in bytes value."_el);
        if (decoder.character() != CharClass::HexDigit) {
            decoder.throwSyntaxError("Expected second hex digit of a byte, got something else."_el);
        }
        value |= static_cast<uint8_t>(decoder.character().digitValue(IntegerBase::Hexadecimal).value_or(0U));
        decoder.next();
        bytes.append(Byte{value});
    }
    decoder.next();
    return decoder.createToken(TokenType::Bytes, ByteBlock{bytes});
}

auto parseMultiLineBytesHexLine(TokenDecoder &decoder) -> TokenGenerator {
    // Initial check so we avoid creating a mem::ByteBlock object.
    if (!isAtMultiLineEnd(decoder, TokenType::MultiLineBytes)) {
        ByteBlockEditor decodedBytes;
        // Carefully consume the text block by block, so we can skip trailing spacing.
        while (!isAtMultiLineEnd(decoder, TokenType::MultiLineBytes)) {
            decoder.advanceWhile(AsciiCategory::Blank);
            decoder.checkForErrorAndThrowIt();
            if (isAtMultiLineEnd(decoder, TokenType::MultiLineBytes)) {
                break;
            }
            if (decoder.character() != CharClass::HexDigit) {
                decoder.throwSyntaxError("Expected first hex digit of a byte, got something else."_el);
            }
            auto value =
                static_cast<uint8_t>(decoder.character().digitValue(IntegerBase::Hexadecimal).value_or(0U) << 4U);
            decoder.next();
            if (isAtMultiLineEnd(decoder, TokenType::MultiLineBytes)) {
                decoder.throwSyntaxError("Expected second hex digit of a byte, not the end of the line."_el);
            }
            if (decoder.character() != CharClass::HexDigit) {
                decoder.throwSyntaxError("Expected second hex digit of a byte, got something else."_el);
            }
            value |= static_cast<uint8_t>(decoder.character().digitValue(IntegerBase::Hexadecimal).value_or(0U));
            decoder.next();
            decodedBytes.append(Byte{value});
        }
        EL_YIELD_TOKEN(TokenType::MultiLineBytes, ByteBlock{decodedBytes});
    }
    // Read the end-of-line tokens (may include a comment if at #).
    EL_YIELD_FROM(expectEndOfLine(decoder, ExpectMore::No));
    // Do the check for more data after creating all tokens for the line.
    decoder.expectMore("Unexpected end in a multi-line bytes-data."_el);
    co_return;
}

}
