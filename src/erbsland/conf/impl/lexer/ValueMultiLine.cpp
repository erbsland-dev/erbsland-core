// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ValueMultiLine.hpp"

#include "Core.hpp"
#include "Recovery.hpp"
#include "Text.hpp"
#include "ValueBytes.hpp"

#include "../char/NamedChars.hpp"
#include "../utilities/YieldMacros.hpp"

#include "../../../text/Literals.hpp"

namespace erbsland::conf::impl::lexer {

using namespace text::literals;

auto scanRepeatingCharacters(TokenDecoder &decoder, const text::Char expectedChar, const bool isOpen)
    -> std::optional<LexerToken> {
    // Avoid a transaction if there is no need.
    if (decoder.character() != expectedChar) {
        return std::nullopt;
    }
    // Count until we get three characters.
    int expectedCharCount = 0;
    auto transaction = Transaction{decoder};
    while (decoder.character() == expectedChar) {
        decoder.next();
        ++expectedCharCount;
        if (expectedCharCount == 3) {
            transaction.commit();
            if (isOpen) {
                return decoder.createToken(TokenType::fromMultiLineOpen(expectedChar));
            }
            return decoder.createToken(TokenType::fromMultiLineClose(expectedChar));
        }
    }
    return std::nullopt; // Too few.
}

auto scanMultiLineOpen(TokenDecoder &decoder) -> std::optional<LexerToken> {
    return scanRepeatingCharacters(decoder, decoder.character(), true);
}

auto scanMultiLineClose(TokenDecoder &decoder, TokenType openTokenType) -> std::optional<LexerToken> {
    text::Char expectedChar;
    switch (openTokenType.raw()) {
    case TokenType::MultiLineTextOpen:
        expectedChar = nc::doubleQuote;
        break;
    case TokenType::MultiLineCodeOpen:
        expectedChar = nc::backtick;
        break;
    case TokenType::MultiLineRegexOpen:
        expectedChar = nc::slash;
        break;
    case TokenType::MultiLineBytesOpen:
        expectedChar = nc::greaterThan;
        break;
    default:
        throw err::LogicFailure("Unexpected open token type."_el);
    }
    return scanRepeatingCharacters(decoder, expectedChar, false);
}

auto expectMultiLineHeader(TokenDecoder &decoder, const TokenType openTokenType) -> TokenGenerator {
    if (openTokenType == TokenType::MultiLineCodeOpen) {
        if (auto language = scanFormatOrLanguageIdentifier(decoder, true); !language.isEmpty()) {
            EL_YIELD_TOKEN(TokenType::MultiLineCodeLanguage, std::move(language));
            decoder.expectMore("Unexpected end in multi-line code block."_el);
        }
    } else if (openTokenType == TokenType::MultiLineBytesOpen) {
        decoder.expectMore("Unexpected end in bytes value."_el);
        if (auto format = scanFormatOrLanguageIdentifier(decoder, true); !format.isEmpty()) {
            decoder.expect(CharClass::EndOfLineStart, "Unexpected characters in bytes-data format identifier."_el);
            if (format != "hex"_el) {
                decoder.throwError(ConfErrorCategory::Unsupported, "Unknown bytes-data format."_el);
            }
            EL_YIELD_TOKEN(TokenType::MultiLineBytesFormat, std::move(format));
        }
        decoder.expect(CharClass::EndOfLineStart, "Unexpected characters in bytes-data format identifier"_el);
    }
    EL_YIELD_FROM(expectEndOfLine(decoder, ExpectMore::Yes));
    decoder.expectMore("Unexpected end in multi-line expression."_el);
}

auto expectMultiLineBodyLine(
    TokenDecoder &decoder, const TokenType openTokenType, const bool expandPlaceholders, bool &closed)
    -> TokenGenerator {
    if (decoder.character() == CharClass::Spacing) {
        EL_YIELD(expectAndCheckIndentation(decoder));
        decoder.expectMore("Unexpected end in multi-line text, code-block or regular expression."_el);
    } else if (decoder.character() != CharClass::LineBreak) {
        decoder.throwSyntaxError("Missing indentation in multi-line value."_el);
    }
    if (auto close = scanMultiLineClose(decoder, openTokenType)) {
        closed = true;
        co_yield std::move(*close);
        EL_YIELD_FROM(expectEndOfLine(decoder, ExpectMore::No));
        co_return;
    }
    switch (openTokenType.raw()) {
    case TokenType::MultiLineTextOpen:
        EL_YIELD_FROM(parseMultiLineString(
            decoder, nc::backslash, parseTextEscapeSequence, TokenType::MultiLineText, expandPlaceholders));
        break;
    case TokenType::MultiLineCodeOpen:
        EL_YIELD_FROM(parseMultiLineString(decoder, {}, {}, TokenType::MultiLineCode));
        break;
    case TokenType::MultiLineRegexOpen:
        EL_YIELD_FROM(parseMultiLineString(
            decoder, nc::backslash, parseRegularExpressionEscapeSequence, TokenType::MultiLineRegex));
        break;
    case TokenType::MultiLineBytesOpen:
        EL_YIELD_FROM(parseMultiLineBytesHexLine(decoder));
        break;
    default:
        decoder.throwInternalError("Unexpected multiline opening token."_el);
    }
}

auto expectMultiLineValue(TokenDecoder &decoder, const TokenType openTokenType, const bool expandPlaceholders)
    -> TokenGenerator {
    auto header = Recovery{decoder};
    EL_YIELD_FROM(header.tokens(expectMultiLineHeader(decoder, openTokenType)));
    if (header.failed() && (decoder.character().isEndOfData() || decoder.isAtStatementStart())) {
        co_return;
    }
    while (!decoder.character().isEndOfData()) {
        // A new root statement belongs to the outer production, even when this block has no closing delimiter.
        if (decoder.isAtStatementStart()) {
            decoder.throwSyntaxError("Missing indentation in multi-line value."_el);
        }
        auto closed = false;
        auto line = Recovery{decoder};
        EL_YIELD_FROM(line.tokens(expectMultiLineBodyLine(decoder, openTokenType, expandPlaceholders, closed)));
        if (closed || (line.failed() && (decoder.character().isEndOfData() || decoder.isAtStatementStart()))) {
            co_return;
        }
    }
    decoder.throwUnexpectedEndOfDataError();
}

auto isAtMultiLineEnd(const TokenDecoder &decoder, const TokenType tokenType) -> bool {
    const auto areCommentsAllowed = (tokenType == TokenType::MultiLineRegex || tokenType == TokenType::MultiLineBytes);
    return decoder.character() == CharClass::LineBreakOrEnd ||
        (areCommentsAllowed && decoder.character() == nc::commentStart);
}

}
