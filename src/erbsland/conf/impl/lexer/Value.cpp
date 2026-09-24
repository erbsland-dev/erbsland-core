// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Value.hpp"

#include "Core.hpp"
#include "Name.hpp"
#include "ValueBytes.hpp"
#include "ValueDateTime.hpp"
#include "ValueFloat.hpp"
#include "ValueInteger.hpp"
#include "ValueLiteral.hpp"
#include "ValueMultiLine.hpp"
#include "ValueText.hpp"

#include "../char/NamedChars.hpp"
#include "../utilities/YieldMacros.hpp"

#include "../../../text/AsciiCategory.hpp"
#include "../../../text/Literals.hpp"

namespace erbsland::conf::impl::lexer {

using namespace text::literals;

auto expectNameAndValue(TokenDecoder &decoder) -> TokenGenerator {
    decoder.clearIndentationPattern(); // Clear the indentation pattern at the start of a name/value line.
    const auto isMetaValue = decoder.character() == nc::at;
    const auto expandPlaceholders = !isMetaValue;
    if (decoder.character() == CharClass::Letter || decoder.character() == nc::at) {
        EL_YIELD(expectRegularOrMetaNameToken(decoder));
    } else {
        // assumes this must be a text name; otherwise this method was called from the wrong context.
        if (decoder.character() != nc::doubleQuote) {
            decoder.throwInternalError("Function 'expectNameAndValue' called from the wrong context."_el);
        }
        EL_YIELD(expectTextName(decoder));
    }
    EL_YIELD_OPTIONAL(scanForSpacing(decoder));
    decoder.expectAndNext(
        CharClass::NameValueSeparator, "Expected a value separator after the name, but got something else."_el);
    EL_YIELD_TOKEN(TokenType::NameValueSeparator);
    EL_YIELD_OPTIONAL(scanForSpacing(decoder));
    if (decoder.character() == nc::commentStart || decoder.character() == CharClass::LineBreak) {
        EL_YIELD_FROM(expectEndOfLine(decoder, ExpectMore::Yes)); // The Value is defined on the next line.
        decoder.expectMore("Expected a value on the next line."_el);
        EL_YIELD(expectAndCheckIndentation(decoder));
        EL_YIELD_FROM(expectValueOrValueList(decoder, NextLine::Yes, MultiLineAllowed::Yes, expandPlaceholders));
    } else if (decoder.character().isEndOfData()) {
        decoder.throwUnexpectedEndOfDataError("Expected a value after the name separator."_el);
    } else {
        EL_YIELD_FROM(expectValueOrValueList(decoder, NextLine::No, MultiLineAllowed::Yes, expandPlaceholders));
    }
    co_return;
}

auto expectMultiLineValueList(TokenDecoder &decoder, const bool expandPlaceholders) -> TokenGenerator {
    if (decoder.character() != nc::asterisk) {
        decoder.throwInternalError("Called 'expectMultiLineValueList' in the wrong state."_el);
    }
    decoder.next();
    EL_YIELD_TOKEN(TokenType::MultiLineValueListSeparator);
    EL_YIELD_OPTIONAL(scanForSpacing(decoder));
    decoder.expectMore("Unexpected end in multi-line value list. Expected a value."_el);
    EL_YIELD_FROM(expectSingleLineValueOrValueList(decoder, expandPlaceholders));
    // At this point, we are on the following line.
    if (decoder.character().isEndOfData()) {
        co_return; // This is a valid end of the document.
    }
    if (decoder.character() != CharClass::Spacing) {
        co_return; // If there is something else following the list, it is a valid end of the list.
    }
    if (!decoder.hasIndentationPattern()) {
        decoder.throwInternalError("Expected to have an indentation patten at this point."_el);
    }
    // If the next line starts with spacing, it is potentially a continuation of the value list.
    while (decoder.character() == CharClass::Spacing) {
        auto transaction = Transaction{decoder};
        decoder.advanceWhile(text::AsciiCategory::Blank);
        if (decoder.character() == CharClass::EndOfLineStart) {
            // This is a valid empty line. Therefore, also a valid end of the list.
            transaction.rollback();
            co_return;
        }
        if (transaction.capturedString() != decoder.indentationPattern()) {
            EL_YIELD_TOKEN(TokenType::Indentation); // Consume the spacing as an indentation token.
            transaction.commit();
            decoder.throwError(
                ConfErrorCategory::Indentation,
                "The indentation pattern does not match the one on the previous line."_el);
        }
        if (decoder.character() != nc::asterisk) {
            EL_YIELD_TOKEN(TokenType::Indentation); // Consume the spacing as an indentation token.
            transaction.commit();
            decoder.throwSyntaxError("Expected the asterisk for a value list continuation, but got something else."_el);
        }
        transaction.commit();
        EL_YIELD_TOKEN(TokenType::Indentation);                 // Consume the spacing.
        decoder.next();
        EL_YIELD_TOKEN(TokenType::MultiLineValueListSeparator); // Consume the asterisk.
        EL_YIELD_OPTIONAL(scanForSpacing(decoder));
        decoder.expectMore("Unexpected end in multi-line value list. Expected a value."_el);
        EL_YIELD_FROM(expectSingleLineValueOrValueList(decoder, expandPlaceholders));
    }
    co_return;
}

auto expectValueOrValueList(
    TokenDecoder &decoder,
    const NextLine nextLine,
    const MultiLineAllowed multiLineAllowed,
    const bool expandPlaceholders) -> TokenGenerator {

    if (nextLine == NextLine::Yes && decoder.character() == nc::asterisk) {
        EL_YIELD_FROM(expectMultiLineValueList(decoder, expandPlaceholders));
        co_return;
    }
    // Check for multi-line values at this point.
    if (multiLineAllowed == MultiLineAllowed::Yes && decoder.character() == CharClass::OpeningBracket) {
        if (auto multiLineOpenToken = scanMultiLineOpen(decoder)) {
            const auto tokenType = multiLineOpenToken.value().type();
            co_yield std::move(multiLineOpenToken).value();
            switch (tokenType.raw()) {
            case TokenType::MultiLineTextOpen:
                EL_YIELD_FROM(expectMultiLineText(decoder, tokenType, expandPlaceholders));
                break;
            case TokenType::MultiLineCodeOpen:
                EL_YIELD_FROM(expectMultiLineText(decoder, tokenType));
                break;
            case TokenType::MultiLineRegexOpen:
                EL_YIELD_FROM(expectMultiLineText(decoder, tokenType));
                break;
            case TokenType::MultiLineBytesOpen:
                EL_YIELD_FROM(expectMultiLineBytes(decoder));
                break;
            default:
                throw err::LogicError("Unexpected token type after opening bracket."_el);
            }
            co_return;
        }
    }
    EL_YIELD_FROM(expectSingleLineValueOrValueList(decoder, expandPlaceholders));
    co_return;
}

auto expectSingleLineValueOrValueList(TokenDecoder &decoder, const bool expandPlaceholders) -> TokenGenerator {
    EL_YIELD(expectSingleLineValue(decoder, expandPlaceholders));
    EL_YIELD_OPTIONAL(scanForSpacing(decoder));
    while (decoder.character() == nc::valueListSeparator) { // Is this a list?
        decoder.next();
        EL_YIELD_TOKEN(TokenType::ValueListSeparator);
        EL_YIELD_OPTIONAL(scanForSpacing(decoder));
        if (decoder.character() == CharClass::LineBreakOrEnd) {
            decoder.throwSyntaxOrUnexpectedEndError("Expected another value after the value list separator."_el);
        }
        EL_YIELD(expectSingleLineValue(decoder, expandPlaceholders));
        EL_YIELD_OPTIONAL(scanForSpacing(decoder));
    }
    decoder.expect(CharClass::EndOfLineStart, "Expected end of line or a value separator, but got something else."_el);
    EL_YIELD_FROM(expectEndOfLine(decoder, ExpectMore::No));
    co_return;
}

auto expectSingleLineValue(TokenDecoder &decoder, const bool expandPlaceholders) -> LexerToken {
    // The ORDER of the following scan functions is IMPORTANT!
    const auto valueScannerFunctions = {
        &scanLiteralFloat,       // test for literal floats first.
        &scanLiteral,            // test other literals. Throws error if value starts with letter and does not match.
        &scanDateOrDateTime,     // test a date `2026-01-01`, or date time `2026-01-01t12:00`
        &scanTime,               // test for a single time `10:00:32z`
        &scanFloatFractionOnly,  // test for floats, like `.1928`
        &scanFloatWithWholePart, // test for floats, like `283.1293`
        &scanIntegerOrTimeDelta, // test for `123` or `123 days`
    };
    for (const auto &scannerFunction : valueScannerFunctions) {
        if (auto optToken = (*scannerFunction)(decoder)) {
            return std::move(optToken).value();
        }
    }
    if (auto optToken = scanSingleLineText(decoder, expandPlaceholders)) {
        return std::move(optToken).value();
    }
    if (auto optToken = scanBytes(decoder)) {
        return std::move(optToken).value();
    }
    decoder.throwSyntaxOrUnexpectedEndError("Expected a value, but got something else."_el);
}

}
