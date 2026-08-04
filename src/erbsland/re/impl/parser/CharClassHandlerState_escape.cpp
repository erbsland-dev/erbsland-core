// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CharacterClassState.hpp"

#include "../../../text/StringFormat.hpp"

namespace erbsland::re::impl::parser {

void CharClassHandlerState::handleDigit() {
    readNext(); // consume the letter.
    if (currentFlags().isSet(GroupFlag::Ascii)) {
        addCategory(Category::DigitAscii);
    } else {
        addCategory(Category::DigitUnicode);
    }
}

void CharClassHandlerState::handleWord() {
    readNext(); // consume the letter.
    if (currentFlags().isSet(GroupFlag::Ascii)) {
        addCategory(Category::WordAscii);
    } else {
        addCategory(Category::WordUnicode);
    }
}

void CharClassHandlerState::handleSpace() {
    readNext(); // consume the letter.
    if (currentFlags().isSet(GroupFlag::Ascii)) {
        addCategory(Category::SpaceAscii);
    } else {
        addCategory(Category::SpaceUnicode);
    }
}

void CharClassHandlerState::handleUnicodePropertyName() {
    const auto [ucClass, isNegated] = readUnicodeProperty(_state);
    addCategory(ucClass);
}

void CharClassHandlerState::handleHorizontalSpace() {
    if (!hasFeature(Feature::EscapeHorizontalSpace)) {
        throwParsingError("Horizontal space is not supported"_el);
    }
    readNext();
    if (currentFlags().isSet(GroupFlag::Ascii)) {
        addCategory(Category::HorizontalSpaceAscii);
    } else {
        addCategory(Category::HorizontalSpaceUnicode);
    }
}

void CharClassHandlerState::handleVerticalSpace() {
    if (!hasFeature(Feature::EscapeVerticalSpace)) {
        throwParsingError("Vertical space is not supported"_el);
    }
    readNext();
    if (currentFlags().isSet(GroupFlag::Ascii)) {
        addCategory(Category::VerticalSpaceAscii);
    } else {
        addCategory(Category::VerticalSpaceUnicode);
    }
}

void CharClassHandlerState::handleSingleEscapedCharacter(const text::Char character) {
    readNext();
    processNewLiteral(character);
}

void CharClassHandlerState::handleLegacySingleEscapedCharacter(const text::Char character, const Feature feature) {
    if (!hasFeature(feature)) {
        throwParsingError("This escape sequence is not supported"_el);
    }
    handleSingleEscapedCharacter(character);
}

void CharClassHandlerState::handleQuotedLiteral() {
    if (!hasFeature(Feature::QuotedLiterals)) {
        throwParsingError("Quoted literals are not supported"_el);
    }
    readNext(); // consume the 'Q'
    while (!isAtEnd()) {
        if (currentChar() == U'\\') {
            readNext();
            if (isAtEnd()) {
                break;
            }
            if (currentChar() == U'E') {
                readNext();
                return; // end of literal block.
            }
            addCharacter(U'\\');
        }
        addCharacter(currentChar());
        readNext();
    }
    throwParsingError("Unterminated literal '\\Q...\\E' block"_el);
}

void CharClassHandlerState::throwNegationNotAllowed() {
    const auto character = text::String::fromCharacter(currentChar());
    const auto lowercaseCharacter = text::String::fromCharacter(currentChar().toAsciiLowercase());
    throwParsingError(
        text::StringFormat{"Negated ranges like '\\{0}' are not allowed in character classes '[...]'. "
                           "Negate the class '[^\\{1}]' or join them using alternatives '(?:[...]|\\{0})'"}
            .build(character, lowercaseCharacter));
}

void CharClassHandlerState::throwAnchorNotAllowed() {
    const auto character = text::String::fromCharacter(currentChar());
    throwParsingError(
        text::StringFormat{"Anchors like '\\{0}' are not allowed in character classes '[...]'. "
                           "Join them using alternatives '(?:[...]|\\{0})'"}
            .build(character));
}

void CharClassHandlerState::handleEscapeSequence() {
    readNext(); // consume the backslash
    switch (currentChar().toRawValue()) {
    // Handle escaped character that will be used as they are.
    case U'.':
    case U'\\':
    case U'^':
    case U'$':
    case U'|':
    case U'{':
    case U'}':
    case U'(':
    case U')':
    case U'[':
    case U']':
    case U'+':
    case U'*':
    case U'?':
    case U' ':
    case U'"':
    case U'\'':
    case U'#':
    case U'-':
        handleSingleEscapedCharacter(currentChar());
        break;
    // Special characters
    case U'a':
        handleLegacySingleEscapedCharacter(U'\a', Feature::EscapeBell);
        break;
    case U'c':
        processNewLiteral(readPcreControlCharacter(_state));
        break;
    case U'e':
        handleLegacySingleEscapedCharacter(U'\u001b', Feature::EscapeEscape);
        break;
    case U'f':
        handleLegacySingleEscapedCharacter(U'\f', Feature::EscapeFormFeed);
        break;
    case U'n':
        handleSingleEscapedCharacter(U'\n');
        break;
    case U'r':
        handleSingleEscapedCharacter(U'\r');
        break;
    case U't':
        handleSingleEscapedCharacter(U'\t');
        break;
    case U'o':
        processNewLiteral(readOctalChar(_state));
        break;
    case U'x':
        processNewLiteral(readHexadecimalChar(_state));
        break;
    case U'u':
    case U'U':
        processNewLiteral(readUnicodeChar(_state));
        break;
    // Character ranges
    case U'd':
        handleDigit();
        break;
    case U's':
        handleSpace();
        break;
    case U'w':
        handleWord();
        break;
    case U'h':
        handleHorizontalSpace();
        break;
    case U'v':
        handleVerticalSpace();
        break;
    case U'p':
        handleUnicodePropertyName();
        break;
    case U'Q':
        handleQuotedLiteral();
        break;
    case U'E':
        throwParsingError("Unexpected end quote '\\E' without starting quote '\\Q'"_el);
    case U'D':
    case U'S':
    case U'W':
    case U'H':
    case U'N':
    case U'V':
    case U'P':
        throwNegationNotAllowed();
    case U'A':
    case U'z':
    case U'Z':
    case U'b':
    case U'B':
        throwAnchorNotAllowed();
    default:
        throwParsingError("Unexpected escape sequence in character class"_el);
    }
}

}
