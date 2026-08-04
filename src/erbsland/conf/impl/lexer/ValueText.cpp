// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ValueText.hpp"

#include "Core.hpp"
#include "Text.hpp"
#include "ValueMultiLine.hpp"

#include "../char/NamedChars.hpp"
#include "../utilities/YieldMacros.hpp"

#include "../../../text/Literals.hpp"

#include <utility>

namespace erbsland::conf::impl::lexer {

using namespace text::literals;

auto scanSingleLineText(TokenDecoder &decoder) -> std::optional<LexerToken> {
    if (decoder.character() != nc::doubleQuote && decoder.character() != nc::backtick &&
        decoder.character() != nc::slash) {
        return {};
    }
    auto terminatingCharacter = decoder.character();
    decoder.next();
    text::StringEditor text;
    if (terminatingCharacter == nc::doubleQuote) {
        parseText(decoder, text);
        return decoder.createToken(TokenType::Text, std::move(text));
    }
    if (terminatingCharacter == nc::slash) {
        parseRegularExpression(decoder, text);
        return decoder.createToken(TokenType::RegEx, std::move(text));
    }
    parseCode(decoder, text);
    return decoder.createToken(TokenType::Code, std::move(text));
}

auto expectMultiLineText(TokenDecoder &decoder, const TokenType openTokenType) -> TokenGenerator {
    assert(
        openTokenType == TokenType::MultiLineTextOpen || openTokenType == TokenType::MultiLineCodeOpen ||
        openTokenType == TokenType::MultiLineRegexOpen);
    // In the case of code, accept a language identifier, just after the opening sequence.
    if (openTokenType == TokenType::MultiLineCodeOpen) {
        if (auto languageIdentifier = scanFormatOrLanguageIdentifier(decoder, true); !languageIdentifier.isEmpty()) {
            EL_YIELD_TOKEN(TokenType::MultiLineCodeLanguage, std::move(languageIdentifier));
            decoder.expectMore("Unexpected end in multi-line code block."_el);
        }
    }
    // Process any text following the opening bracket sequence.
    EL_YIELD_FROM(expectMultiLineAfterOpen(decoder));
    // Next, process the text, line by line.
    // At the start of this while loop, the decoder should be at the indented continued line.
    while (!decoder.character().isEndOfData()) {
        // Test if we get the closing bracket sequence.
        if (auto closeToken = scanMultiLineClose(decoder, openTokenType); closeToken.has_value()) {
            co_yield std::move(closeToken).value();
            co_return;
        }
        // Capture text, trailing spacing (+comment) and line-break.
        switch (openTokenType.raw()) {
        case TokenType::MultiLineTextOpen:
            EL_YIELD_FROM(
                parseMultiLineString(decoder, nc::backslash, parseTextEscapeSequence, TokenType::MultiLineText));
            break;
        case TokenType::MultiLineCodeOpen:
            EL_YIELD_FROM(parseMultiLineString(decoder, {}, {}, TokenType::MultiLineCode));
            break;
        case TokenType::MultiLineRegexOpen:
            EL_YIELD_FROM(parseMultiLineString(
                decoder, nc::backslash, parseRegularExpressionEscapeSequence, TokenType::MultiLineRegex));
            break;
        default:
            throw err::LogicError("Unexpected open token type."_el);
        }
        // if the following line starts with spacing, expect the correct indentation pattern.
        if (decoder.character() == CharClass::Spacing) {
            EL_YIELD(expectAndCheckIndentation(decoder));
            decoder.expectMore("Unexpected end in multi-line text, code-block or regular expression."_el);
        } else if (decoder.character() != CharClass::LineBreak) {
            decoder.throwSyntaxError("Missing indentation in multi-line text."_el);
        }
    }
    // Unexpected, if the data ends, just after the opening sequence.
    decoder.throwUnexpectedEndOfDataError();
}

}
