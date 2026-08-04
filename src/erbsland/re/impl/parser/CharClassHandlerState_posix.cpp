// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CharacterClassState.hpp"

#include "../../../text/Literals.hpp"
#include "../../../text/StringFormat.hpp"
#include "../../../text/StringHashMap.hpp"

namespace erbsland::re::impl::parser {

using namespace text::literals;

void CharClassHandlerState::handlePosixCharacterClass() {
    std::size_t classCount = 1;
    while (currentChar() == U'[') {
        if (isNegated()) {
            throwParsingError(
                "Negated POSIX character classes must not be combined. "_el
                "Use alternatives '(?:a|b)' to combine them");
        }
        readNext();
        expectMore();
        if (classCount == 1 && (hasStartLiteral() || hasContent() || currentChar() != U':')) {
            throwParsingError("Unexpected '[' inside of a character class. Please escape the '[' for clarity"_el);
        }
        readNext();
        expectMore();
        // At this point, it seems we got '[:'
        if (!hasFeature(Feature::PosixClasses)) {
            throwParsingError("POSIX character classes are not supported"_el);
        }
        text::StringEditor posixName;
        // Test for negation
        if (currentChar() == U'^') {
            if (classCount > 1) {
                throwParsingError(
                    "Negated POSIX character classes must not be combined. "_el
                    "Use alternatives '(?:a|b)' to combine them");
            }
            readNext();
            setNegated(true);
        }
        while (!isAtEnd() && currentChar().isAsciiLetter()) {
            posixName.append(currentChar().toAsciiLowercase());
            readNext();
        }
        expectMore();
        if (posixName.isEmpty()) {
            throwParsingError("Expected POSIX character class name after '[:' in character class"_el);
        }
        if (currentChar() == U':') {
            readNext();
            expectMore();
            if (currentChar() == U']') {
                readNext();
                expectMore();
                if (currentChar() == U']' || currentChar() == U'[') {
                    // do not consume this.
                    // It either marks the end of the character class or the start of a new one.
                    processPosixRangesFor(posixName);
                    classCount += 1;
                    continue;
                }
                throwParsingError("You cannot mix POSIX character classes with normal characters"_el);
            }
        }
        throwParsingError("Expected ':]' after POSIX character class name"_el);
    }
}

void CharClassHandlerState::processPosixRangesFor(const text::String &name) {
    // The definitions of the character sets are commonly expected sets for engines like PCRE.
    // They do not follow any formal rule and are only provided for compatibility.
    struct PosixRangeDefinition {
        std::vector<CharRange> ranges{};
        std::vector<Category> categories{};
    };
    static const text::StringHashMap<PosixRangeDefinition> unicodePosixRanges{{
        {"alnum"_el, {.categories = {Category::L, Category::Nl, Category::Nd}}},
        {"alpha"_el, {.categories = {Category::L, Category::Nl}}},
        {"ascii"_el, {.ranges = {CharRange(U'\u0001', U'\u007f')}}},
        {"blank"_el, {.ranges = {CharRange(U'\t')}, .categories = {Category::Zs}}},
        {"cntrl"_el, {.categories = {Category::Cc}}},
        {"digit"_el, {.categories = {Category::Nd}}},
        {"graph"_el, {.categories = {Category::L, Category::M, Category::N, Category::P, Category::S}}},
        {"lower"_el, {.categories = {Category::Ll}}},
        {"print"_el, {.categories = {Category::L, Category::M, Category::N, Category::P, Category::S, Category::Z}}},
        {"punct"_el,
            {.ranges =
                    {CharRange(U'$'),
                        CharRange(U'+'),
                        CharRange(U'<', U'>'),
                        CharRange(U'^'),
                        CharRange(U'`'),
                        CharRange(U'|'),
                        CharRange(U'~')},
                .categories = {Category::P}}},
        {"space"_el, {.ranges = {CharRange(U'\u0009', U'\u000d')}, .categories = {Category::Z}}},
        {"upper"_el, {.categories = {Category::Lu}}},
        {"word"_el, {.categories = {Category::L, Category::Nl, Category::Nd, Category::Pc}}},
        {"xdigit"_el, {.ranges = {CharRange(U'0', U'9'), CharRange(U'a', U'f'), CharRange(U'A', U'F')}}},
    }};
    static const text::StringHashMap<PosixRangeDefinition> asciiPosixRanges{{
        {"alnum"_el, {.ranges = {CharRange(U'0', U'9'), CharRange(U'A', U'Z'), CharRange(U'a', U'z')}}},
        {"alpha"_el, {.ranges = {CharRange(U'A', U'Z'), CharRange(U'a', U'z')}}},
        {"ascii"_el, {.ranges = {CharRange(U'\u0001', U'\u007f')}}},
        {"blank"_el, {.ranges = {CharRange(U' '), CharRange(U'\t')}}},
        {"cntrl"_el, {.ranges = {CharRange(U'\u0001', U'\u001f'), CharRange(U'\u007f')}}},
        {"digit"_el, {.ranges = {CharRange(U'0', U'9')}}},
        {"graph"_el, {.ranges = {CharRange(U'\u0021', U'\u007e')}}},
        {"lower"_el, {.ranges = {CharRange(U'a', U'z')}}},
        {"print"_el, {.ranges = {CharRange(U'\u0020', U'\u007e')}}},
        {"punct"_el,
            {.ranges = {CharRange(U'!', U'/'), CharRange(U':', U'@'), CharRange('[', U'`'), CharRange('{', U'~')}}},
        {"space"_el, {.ranges = {CharRange(U'\u0009', U'\u000d'), CharRange(U'\u0020')}}},
        {"upper"_el, {.ranges = {CharRange(U'A', U'Z')}}},
        {"word"_el, {.ranges = {CharRange(U'_'), CharRange(U'a', U'z'), CharRange(U'A', U'Z'), CharRange(U'0', U'9')}}},
        {"xdigit"_el, {.ranges = {CharRange(U'0', U'9'), CharRange(U'a', U'f'), CharRange(U'A', U'F')}}},
    }};
    const auto &posixRanges = currentFlags().isSet(GroupFlag::Ascii) ? asciiPosixRanges : unicodePosixRanges;
    if (const auto definition = posixRanges.get(name); definition.has_value()) {
        for (const auto &range : definition->ranges) {
            addRange(range.first(), range.last());
        }
        for (const auto &category : definition->categories) {
            addCategory(category);
        }
    } else {
        throwParsingError(text::StringFormat{"Unknown POSIX character class name '{}'"}.build(name));
    }
}

}
