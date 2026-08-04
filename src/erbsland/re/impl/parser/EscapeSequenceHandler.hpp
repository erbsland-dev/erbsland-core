// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AnchorAndSpecialHandler.hpp"
#include "EscapeSequences.hpp"
#include "ParserState.hpp"
#include "RegularCharHandler.hpp"

namespace erbsland::re::impl::parser {

/// Handle special characters like `\n`.
inline void handleSpecialCharacter(ParserState &state, const text::Char character) {
    state.readNext();
    addRegularChar(state, character);
}

/// Handle legacy special characters like `\a`
inline void handleLegacySpecialCharacter(ParserState &state, const text::Char character, const Feature feature) {
    if (!state.hasFeature(feature)) {
        state.throwParsingError("This escape sequence is not supported"_el);
    }
    handleSpecialCharacter(state, character);
}

/// Handle character ranges based on a character category.
inline void handleOneCharRange(ParserState &state, const Category category, const bool isNegated) {
    state.readNext();
    auto newNode = state.createNode(node_data::CharacterCategory{category, isNegated});
    state.addNode(handleQuantifier(state, newNode));
}

/// Handle character ranges based on two character categories.
inline void handleOneCharRange(
    ParserState &state, const Category category1, const Category category2, const bool isNegated) {

    state.readNext();
    auto newNode = state.createNode(node_data::CharacterCategory{std::vector{{category1, category2}}, isNegated});
    state.addNode(handleQuantifier(state, newNode));
}

/// Handle unicode property syntax using shared reader
inline void handleUnicodeCharClass(ParserState &state, bool /*isNegatedIgnored*/) {
    const auto [characterClass, isNegated] = readUnicodeProperty(state);
    auto newNode = state.createNode(node_data::CharacterCategory{characterClass, isNegated});
    state.addNode(handleQuantifier(state, newNode));
}

/// Handle octal character
inline void handleOctalChar(ParserState &state) {
    addRegularChar(state, readOctalChar(state));
}

/// Handle hexadecimal character
inline void handleHexadecimalChar(ParserState &state) {
    addRegularChar(state, readHexadecimalChar(state));
}

/// Handle unicode character.
inline void handleUnicodeChar(ParserState &state) {
    addRegularChar(state, readUnicodeChar(state));
}

/// Parse a digit-class escape sequence.
inline void handleDigit(ParserState &state, const bool isNegated) {
    if (state.currentFlags().isSet(GroupFlag::Ascii)) {
        handleOneCharRange(state, Category::DigitAscii, isNegated);
    } else {
        handleOneCharRange(state, Category::DigitUnicode, isNegated);
    }
}

/// Parse a word-class escape sequence.
inline void handleWord(ParserState &state, const bool isNegated) {
    if (state.currentFlags().isSet(GroupFlag::Ascii)) {
        handleOneCharRange(state, Category::WordAscii, isNegated);
    } else {
        handleOneCharRange(state, Category::WordUnicode, isNegated);
    }
}

/// Parse a whitespace-class escape sequence.
inline void handleSpace(ParserState &state, const bool isNegated) {
    if (state.currentFlags().isSet(GroupFlag::Ascii)) {
        if (state.currentFlags().isSet(GroupFlag::DotAll)) {
            handleOneCharRange(state, Category::SpaceAsciiDotAll, isNegated);
        } else {
            handleOneCharRange(state, Category::SpaceAscii, isNegated);
        }
    } else {
        if (state.currentFlags().isSet(GroupFlag::DotAll)) {
            handleOneCharRange(state, Category::SpaceUnicodeDotAll, isNegated);
        } else {
            handleOneCharRange(state, Category::SpaceUnicode, isNegated);
        }
    }
}

/// Parse a horizontal-whitespace escape sequence.
inline void handleHorizontalWhiteSpace(ParserState &state, const bool isNegated) {
    if (!state.hasFeature(Feature::EscapeHorizontalSpace)) {
        state.throwParsingError("Horizontal whitespace is not supported"_el);
    }
    if (state.currentFlags().isSet(GroupFlag::Ascii)) {
        handleOneCharRange(state, Category::HorizontalSpaceAscii, isNegated);
    } else {
        handleOneCharRange(state, Category::HorizontalSpaceUnicode, isNegated);
    }
}

/// Parse a vertical-whitespace escape sequence.
inline void handleVerticalWhiteSpace(ParserState &state, const bool isNegated) {
    if (!state.hasFeature(Feature::EscapeVerticalSpace)) {
        state.throwParsingError("Vertical whitespace is not supported"_el);
    }
    if (state.currentFlags().isSet(GroupFlag::Ascii)) {
        handleOneCharRange(state, Category::VerticalSpaceAscii, isNegated);
    } else {
        handleOneCharRange(state, Category::VerticalSpaceUnicode, isNegated);
    }
}

/// Parse the escape sequence that matches every character except a newline.
inline void handleNotNewline(ParserState &state) {
    state.readNext();
    auto charClass = CharClass{std::vector{CharRange{U'\n'}}};
    charClass.prepareForUse();
    auto newNode = state.createNode(node_data::CharacterClass{charClass, true});
    state.addNode(handleQuantifier(state, newNode));
}

/// Parse a word-boundary escape sequence.
inline void handleWordBoundary(ParserState &state, const bool isNegated) {
    if (isNegated) {
        if (state.currentFlags().isSet(GroupFlag::Ascii)) {
            handleAnchor(state, TextAnchor::NonAsciiWordBoundary);
        } else {
            handleAnchor(state, TextAnchor::NonUnicodeWordBoundary);
        }
    } else {
        if (state.currentFlags().isSet(GroupFlag::Ascii)) {
            handleAnchor(state, TextAnchor::AsciiWordBoundary);
        } else {
            handleAnchor(state, TextAnchor::UnicodeWordBoundary);
        }
    }
}

/// Parse a PCRE control-character escape sequence.
inline void handlePcreControlCharacter(ParserState &state) {
    addRegularChar(state, readPcreControlCharacter(state));
}

/// Parse a quoted literal block escape sequence.
inline void handleLiteralBlock(ParserState &state) {
    if (!state.hasFeature(Feature::QuotedLiterals)) {
        state.throwParsingError("Quoted literals are not supported"_el);
    }
    state.readNext(); // consume the 'Q'
    while (!state.isAtEnd()) {
        if (state.currentChar() == U'\\') {
            state.readNext();
            if (state.isAtEnd()) {
                break;
            }
            if (state.currentChar() == U'E') {
                state.readNext();
                return; // end of literal block.
            }
            addRegularChar(state, U'\\');
        }
        addRegularChar(state, state.readNextAndExchange());
    }
    state.throwParsingError("Unterminated literal '\\Q...\\E' block"_el);
}

/// Handle all escape sequences after an initial backslash.
/// For regular escaped characters, delegate to `handleRegularChar()`.
///
/// Here we derive from the behavior of many regex engines. If an unknown escape sequence is found,
/// stop with a parsing error and do not silently ignore it. This makes parsing the expression safer and
/// is more predictable.
inline void handleEscapeSequence(ParserState &state) {
    state.readNext(); // consume the backslash.
    if (state.isAtEnd()) {
        state.throwParsingError("Unexpected end of the pattern after backslash"_el);
    }
    switch (state.currentChar().toRawValue()) {
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
        addRegularChar(state, state.readNextAndExchange());
        break;
    // Quoting
    case U'E':
        state.throwParsingError("Found a quoting escape end '\\E' without start '\\Q'"_el);
    case U'Q':
        handleLiteralBlock(state);
        break;
    // Special characters
    case U'a':
        handleLegacySpecialCharacter(state, U'\a', Feature::EscapeBell);
        break;
    case U'c':
        handlePcreControlCharacter(state);
        break;
    case U'e':
        handleLegacySpecialCharacter(state, U'\u001b', Feature::EscapeEscape);
        break;
    case U'f':
        handleLegacySpecialCharacter(state, U'\f', Feature::EscapeFormFeed);
        break;
    case U'n':
        handleSpecialCharacter(state, U'\n');
        break;
    case U'r':
        handleSpecialCharacter(state, U'\r');
        break;
    case U't':
        handleSpecialCharacter(state, U'\t');
        break;
    // Warn about backreferences
    case U'0':
        state.throwParsingError("Backreferences are not supported. Use '\\o{...}' for octal characters"_el);
    case U'1':
    case U'2':
    case U'3':
    case U'4':
    case U'5':
    case U'6':
    case U'7':
    case U'8':
    case U'9':
    case U'k':
    case U'K':
    case U'g':
    case U'G':
        state.throwParsingError("Backreferences and subpatterns are not supported"_el);
    // Numerically specified characters
    case U'o':
        handleOctalChar(state);
        break;
    case U'x':
        handleHexadecimalChar(state);
        break;
    case U'u':
    case U'U':
        handleUnicodeChar(state);
        break;
    // Character ranges
    case U'd':
        handleDigit(state, false);
        break;
    case U'D':
        handleDigit(state, true);
        break;
    case U's':
        handleSpace(state, false);
        break;
    case U'S':
        handleSpace(state, true);
        break;
    case U'w':
        handleWord(state, false);
        break;
    case U'W':
        handleWord(state, true);
        break;
    case U'h':
        handleHorizontalWhiteSpace(state, false);
        break;
    case U'H':
        handleHorizontalWhiteSpace(state, true);
        break;
    case U'N':
        handleNotNewline(state);
        break;
    case U'v':
        handleVerticalWhiteSpace(state, false);
        break;
    case U'V':
        handleVerticalWhiteSpace(state, true);
        break;
    case U'p':
        handleUnicodeCharClass(state, false);
        break;
    case U'P':
        handleUnicodeCharClass(state, true);
        break;
    case U'A':
        handleAnchor(state, TextAnchor::Start);
        break;
    case U'z':
        if (!state.hasFeature(Feature::AnchorLowercaseZ)) {
            state.throwParsingError("Lowercase 'z' anchor is not supported"_el);
        }
        handleAnchor(state, TextAnchor::End);
        break;
    case U'Z':
        handleAnchor(state, TextAnchor::End);
        break;
    case U'b':
        handleWordBoundary(state, false);
        break;
    case U'B':
        handleWordBoundary(state, true);
        break;
    default:
        state.throwParsingError("Unknown escape sequence"_el);
    }
}

}
