// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Section.hpp"

#include "Core.hpp"
#include "Name.hpp"

#include "../char/NamedChars.hpp"
#include "../utilities/YieldMacros.hpp"

namespace erbsland::conf::impl::lexer {

using namespace text::literals;

auto expectSection(TokenDecoder &decoder) -> TokenGenerator {
    bool isListSection = false;
    // First, parse the opening bracket for the section.
    while (decoder.character() == nc::minus) { // Skip any number of '-' in front of the section.
        decoder.next();
    }
    if (decoder.character() == nc::asterisk) { // If there is an asterisk, this is a list section header.
        decoder.next();
        isListSection = true;
    }
    // At this point, only an open square bracket is valid.
    decoder.expectAndNext(nc::openingSquareBracket, "Expected an opening square bracket, but got something else."_el);
    if (isListSection) {
        EL_YIELD_TOKEN(TokenType::SectionListOpen);
    } else {
        EL_YIELD_TOKEN(TokenType::SectionMapOpen);
    }
    // Spacing inside the section brackets is allowed.
    EL_YIELD_OPTIONAL(scanForSpacing(decoder));
    // Relative paths start with a path separator.
    if (decoder.character() == nc::namePathSeparator) {
        decoder.next();
        EL_YIELD_TOKEN(TokenType::NamePathSeparator);
        EL_YIELD_OPTIONAL(scanForSpacing(decoder));
    }
    // Read as many names as we get (the parser will handle the logic).
    while (decoder.character() == CharClass::SectionNameStart) {
        if (decoder.character() == CharClass::Letter) {
            EL_YIELD(expectRegularOrMetaNameToken(decoder));
        } else {
            EL_YIELD(expectTextName(decoder));
        }
        EL_YIELD_OPTIONAL(scanForSpacing(decoder));
        if (decoder.character() != nc::namePathSeparator) {
            break;
        }
        decoder.next();
        EL_YIELD_TOKEN(TokenType::NamePathSeparator);
        EL_YIELD_OPTIONAL(scanForSpacing(decoder));
    }
    // At this point, we expect the closing square bracket.
    decoder.expectAndNext(nc::closingSquareBracket, "Expected a closing square bracket, but got something else."_el);
    // For a list section, accept an asterisk.
    if (decoder.character() == nc::asterisk) {
        if (isListSection) {
            decoder.next();
        } else {
            decoder.throwSyntaxError("A map section cannot have an asterisk after the closing square bracket."_el);
        }
    }

    // Accept any number of minus chars.
    while (decoder.character() == nc::minus) {
        decoder.next();
    }
    if (isListSection) {
        EL_YIELD_TOKEN(TokenType::SectionListClose);
    } else {
        EL_YIELD_TOKEN(TokenType::SectionMapClose);
    }
    // At this point, the line must end.
    decoder.expect(CharClass::EndOfLineStart, "Expected end of line after section, but got something else."_el);
    EL_YIELD_FROM(expectEndOfLine(decoder, ExpectMore::No));
    co_return;
}

}
