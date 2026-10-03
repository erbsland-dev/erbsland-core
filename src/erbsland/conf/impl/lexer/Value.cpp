// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Value.hpp"

#include "Core.hpp"
#include "Name.hpp"
#include "Recovery.hpp"
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

/// Read the bullet and values of one continued list row.
/// @param decoder The decoder at the bullet.
/// @param expandPlaceholders Whether ordinary text allows placeholder expansion.
/// @return The row's tokens.
/// @throws ConfError For invalid row input.
/// @tested{LexerStandardValueListTest SyntaxParserTest LexerRecoveryTest}
auto expectListRow(TokenDecoder &decoder, const bool expandPlaceholders) -> TokenGenerator {
    decoder.expectAndNext(
        nc::asterisk, "Expected the asterisk for a value list continuation, but got something else."_el);
    EL_YIELD_TOKEN(TokenType::MultiLineValueListSeparator);
    EL_YIELD_OPTIONAL(scanForSpacing(decoder));
    decoder.expectMore("Unexpected end in multi-line value list. Expected a value."_el);
    EL_YIELD_FROM(expectSingleLineValueOrValueList(decoder, expandPlaceholders));
}

/// Read a continued list row or leave an indented blank/comment line unconsumed.
/// @param decoder The decoder at the indentation.
/// @param expandPlaceholders Whether ordinary text allows placeholder expansion.
/// @param finished Set when the list ends before a blank or comment line.
/// @return The continued row's tokens.
/// @throws ConfError For invalid indentation or row input.
/// @tested{LexerStandardValueListTest SyntaxParserTest LexerRecoveryTest}
auto expectListContinuation(TokenDecoder &decoder, const bool expandPlaceholders, bool &finished) -> TokenGenerator {
    auto transaction = Transaction{decoder};
    decoder.advanceWhile(text::AsciiCategory::Blank);
    if (decoder.character() == CharClass::EndOfLineStart) {
        transaction.rollback();
        finished = true;
        co_return;
    }
    const auto indentation = transaction.capturedString();
    transaction.commit();
    EL_YIELD_TOKEN(TokenType::Indentation);
    if (indentation != decoder.indentationPattern()) {
        decoder.throwError(
            ConfErrorCategory::Indentation, "The indentation pattern does not match the one on the previous line."_el);
    }
    EL_YIELD_FROM(expectListRow(decoder, expandPlaceholders));
}

/// Read a complete continued list, retaining row context during syntax recovery.
/// @param decoder The decoder at the first bullet, after its indentation.
/// @param expandPlaceholders Whether ordinary text allows placeholder expansion.
/// @return The list's tokens.
/// @throws ConfError For strict parsing or unrecoverable failures.
/// @tested{LexerStandardValueListTest SyntaxParserTest LexerRecoveryTest}
auto expectMultiLineValueList(TokenDecoder &decoder, const bool expandPlaceholders) -> TokenGenerator {
    auto first = Recovery{decoder};
    EL_YIELD_FROM(first.tokens(expectListRow(decoder, expandPlaceholders)));
    while (decoder.character() == CharClass::Spacing) {
        auto finished = false;
        auto row = Recovery{decoder};
        EL_YIELD_FROM(row.tokens(expectListContinuation(decoder, expandPlaceholders, finished)));
        if (finished) {
            co_return;
        }
    }
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
            EL_YIELD_FROM(expectMultiLineValue(decoder, tokenType, expandPlaceholders));
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
