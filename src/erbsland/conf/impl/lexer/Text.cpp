// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Text.hpp"

#include "Core.hpp"
#include "Number.hpp"
#include "ValueMultiLine.hpp"

#include "../char/NamedChars.hpp"
#include "../utilities/YieldMacros.hpp"

#include <cassert>

namespace erbsland::conf::impl::lexer {

using namespace text::literals;

void parseString(
    Decoder &decoder,
    text::StringEditor &target,
    const text::Char terminator,
    const text::Char escapeChar,
    const EscapeFn &escapeFn) {

    while (!decoder.character().isEndOfData()) {
        // Capture ordinary text as one raw UTF-8 span. The decoder still visits and validates every character, but
        // appending the complete span avoids growing the target and encoding each unchanged character separately.
        auto textTransaction = Transaction{decoder};
        while (
            decoder.character() != terminator && decoder.character() != escapeChar &&
            decoder.character() != CharClass::LineBreakOrEnd) {
            decoder.checkForErrorAndThrowIt();
            decoder.next();
        }
        auto capturedText = textTransaction.capturedString();
        textTransaction.commit();
        target.append(capturedText);

        if (decoder.character() == CharClass::LineBreak) {
            decoder.throwSyntaxError("Unexpected line break in text or code-block."_el);
        }
        if (decoder.character() == terminator) {
            decoder.next();
            return;
        }
        if (decoder.character().isEndOfData()) {
            break;
        }
        assert(decoder.character() == escapeChar);
        decoder.next();
        decoder.expectMore("Unexpected end in an escape sequence."_el);
        escapeFn(decoder, target);
    }
    decoder.throwUnexpectedEndOfDataError();
}

auto parseMultiLineString(
    TokenDecoder &decoder, const text::Char escapeChar, EscapeFn escapeFn, const TokenType tokenType)
    -> TokenGenerator {

    // Initial check if the line starts with the end marker, so we avoid creating a transaction and capture string.
    if (!isAtMultiLineEnd(decoder, tokenType)) {
        text::StringEditor decodedText;
        // Carefully consume the text block by block, so we can skip trailing spacing.
        while (!isAtMultiLineEnd(decoder, tokenType)) {
            // Consume anything that is not space, or the end of the line.
            while (decoder.character() != CharClass::Spacing && decoder.character() != CharClass::LineBreakOrEnd) {
                decoder.checkForErrorAndThrowIt();
                if (decoder.character() == escapeChar) {
                    decoder.next();
                    escapeFn(decoder, decodedText);
                } else {
                    decodedText.append(decoder.character());
                    decoder.next();
                }
            }
            // If the line ends here, commit everything consumed so far.
            if (decoder.character() == CharClass::LineBreakOrEnd) {
                break;
            }
            // At this point we are in spacing territory, always expect that we read the trailing space of the line.
            auto trailingSpaceTransaction = Transaction{decoder};
            while (decoder.character() == CharClass::Spacing) {
                decoder.next();
            }
            if (isAtMultiLineEnd(decoder, tokenType)) {
                // If we reached the end of the line, while consuming spaces. We have to roll back this section,
                // as this is the trailing portion that is not part of the actual text.
                trailingSpaceTransaction.rollback();
                break;
            }
            decodedText.append(trailingSpaceTransaction.capturedString());
            trailingSpaceTransaction.commit();
        }
        EL_YIELD_TOKEN(tokenType, std::move(decodedText));
    }
    // Read the end-of-line tokens (may include a comment if at #).
    EL_YIELD_FROM(expectEndOfLine(decoder, ExpectMore::No));
    // Do the check for more data after creating all tokens for the line.
    decoder.expectMore("Unexpected end in a multi-line text, code-block or regular expression."_el);
    co_return;
}

void parseText(Decoder &decoder, text::StringEditor &target) {
    parseString(decoder, target, nc::doubleQuote, nc::backslash, parseTextEscapeSequence);
}

/// Parse the Unicode escape sequence, after `\u` or `\U`.
/// @param decoder The decoder.
/// @param target The target to add the parsed Unicode character.
void parseUnicodeEscapeSequence(Decoder &decoder, text::StringEditor &target) {
    decoder.expectMore("Unexpected end in a Unicode escape sequence."_el);
    text::Char character{};
    if (decoder.character() == nc::openingCurlyBracket) { // dynamic length hex.
        decoder.next();
        decoder.expect(CharClass::HexDigit, "Expected a hex digit after the opening bracket."_el);
        const auto numberResult =
            parseNumber(decoder, text::IntegerBase::Hexadecimal, Sign::Positive, NumberSeparators::No);
        const auto number = numberResult.value();
        const auto digits = numberResult.digitCount();
        decoder.expectMore("Unexpected end in a Unicode escape sequence."_el);
        if (digits > 8) {
            decoder.throwSyntaxError("Hex escape sequence is too long."_el);
        }
        decoder.expectAndNext(nc::closingCurlyBracket, "Expected a closing bracket after the hexadecimal number."_el);
        character = text::Char{static_cast<char32_t>(number)};
    } else if (decoder.character() == CharClass::HexDigit) { // fixed length hex.
        const auto number =
            parseNumber(decoder, text::IntegerBase::Hexadecimal, Sign::Positive, NumberSeparators::No, 4).value();
        decoder.expectMore("Unexpected end in a Unicode escape sequence."_el);
        if (number < 0) {
            decoder.throwSyntaxError("Hex escape sequence requires four digits."_el);
        }
        character = text::Char{static_cast<char32_t>(number)};
    } else {
        decoder.throwSyntaxError("Expected a hex digit or an opening bracket."_el);
    }
    if (!character.isValidUnicode() || character.isNull()) {
        decoder.throwSyntaxError("Invalid unicode value in escape sequence."_el);
    }
    target.append(character);
}

void parseTextEscapeSequence(Decoder &decoder, text::StringEditor &target) {
    // Do end checks before calling `next()`, because of open transactions.
    decoder.expectMore("Unexpected end in an escape sequence."_el);
    if (decoder.character() == CharClass::LineBreak) {
        decoder.throwSyntaxError("Unexpected line break in escape sequence."_el);
    }
    // Get the escaped character first and consume it.
    const auto escapedChar = decoder.character();
    decoder.checkForErrorAndThrowIt();
    decoder.next();
    // Next, decide how to handle the escaped character.
    switch (escapedChar.toRawValue()) {
    case nc::backslash.toRawValue():
        target.append(u8'\\');
        break;
    case nc::doubleQuote.toRawValue():
        target.append(u8'"');
        break;
    case nc::dollar.toRawValue():
        target.append(u8'$');
        break;
    case nc::lowercaseT.toRawValue():
    case nc::uppercaseT.toRawValue():
        target.append(u8'\t');
        break;
    case nc::lowercaseN.toRawValue():
    case nc::uppercaseN.toRawValue():
        target.append(u8'\n');
        break;
    case nc::lowercaseR.toRawValue():
    case nc::uppercaseR.toRawValue():
        target.append(u8'\r');
        break;
    case nc::lowercaseU.toRawValue():
    case nc::uppercaseU.toRawValue():
        parseUnicodeEscapeSequence(decoder, target);
        break;
    default:
        decoder.throwSyntaxError("Unexpected character in escape sequence."_el);
    }
}

void parseRegularExpression(Decoder &decoder, text::StringEditor &target) {
    parseString(decoder, target, nc::slash, nc::backslash, parseRegularExpressionEscapeSequence);
}

void parseRegularExpressionEscapeSequence(Decoder &decoder, text::StringEditor &target) {
    if (decoder.character() == nc::slash) {
        target.append(u8'/');
    } else if (!decoder.character().isError()) {
        target.append(u8'\\');
        target.append(decoder.character());
    } else {
        // This will throw any captured encoding or control-character exception.
        decoder.throwSyntaxError("Unexpected character in escape sequence."_el);
    }
    decoder.next();
}

void parseCode(Decoder &decoder, text::StringEditor &target) {
    parseString(
        decoder,
        target,
        nc::backtick,
        text::Char::null(), // no escape.
        nullptr);
}

}
