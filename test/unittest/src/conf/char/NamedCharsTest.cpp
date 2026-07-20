// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/conf/impl/char/NamedChars.hpp>
#include <erbsland/unittest/UnitTest.hpp>

namespace nc = el::conf::impl::nc;

TESTED_TARGETS(NamedChars)
class NamedCharsTest final : public el::UnitTest {
public:
    void testNamedCharacters() {
        static_assert(nc::newLine == U'\n');
        static_assert(nc::carriageReturn == U'\r');
        static_assert(nc::space == U' ');
        static_assert(nc::tab == U'\t');
        static_assert(nc::hash == U'#');
        static_assert(nc::at == U'@');
        static_assert(nc::doubleQuote == U'"');
        static_assert(nc::singleQuote == U'\'');
        static_assert(nc::backtick == U'`');
        static_assert(nc::slash == U'/');
        static_assert(nc::lessThan == U'<');
        static_assert(nc::greaterThan == U'>');
        static_assert(nc::underscore == U'_');
        static_assert(nc::dollar == U'$');
        static_assert(nc::lowercaseA == U'a');
        static_assert(nc::lowercaseB == U'b');
        static_assert(nc::lowercaseE == U'e');
        static_assert(nc::lowercaseF == U'f');
        static_assert(nc::lowercaseI == U'i');
        static_assert(nc::lowercaseN == U'n');
        static_assert(nc::lowercaseR == U'r');
        static_assert(nc::lowercaseT == U't');
        static_assert(nc::lowercaseU == U'u');
        static_assert(nc::lowercaseX == U'x');
        static_assert(nc::lowercaseZ == U'z');
        static_assert(nc::uppercaseA == U'A');
        static_assert(nc::uppercaseB == U'B');
        static_assert(nc::uppercaseE == U'E');
        static_assert(nc::uppercaseF == U'F');
        static_assert(nc::uppercaseI == U'I');
        static_assert(nc::uppercaseN == U'N');
        static_assert(nc::uppercaseR == U'R');
        static_assert(nc::uppercaseT == U'T');
        static_assert(nc::uppercaseU == U'U');
        static_assert(nc::uppercaseX == U'X');
        static_assert(nc::uppercaseZ == U'Z');
        static_assert(nc::digit0 == U'0');
        static_assert(nc::digit1 == U'1');
        static_assert(nc::digit9 == U'9');
        static_assert(nc::colon == U':');
        static_assert(nc::equal == U'=');
        static_assert(nc::comma == U',');
        static_assert(nc::fullStop == U'.');
        static_assert(nc::backslash == U'\\');
        static_assert(nc::openingCurlyBracket == U'{');
        static_assert(nc::closingCurlyBracket == U'}');
        static_assert(nc::openingSquareBracket == U'[');
        static_assert(nc::closingSquareBracket == U']');
        static_assert(nc::plus == U'+');
        static_assert(nc::minus == U'-');
        static_assert(nc::asterisk == U'*');
        static_assert(nc::microSign == U'µ');
        static_assert(nc::questionMark == U'?');
        static_assert(nc::pipe == U'|');
    }

    void testLanguageMeanings() {
        static_assert(nc::commentStart == nc::hash);
        static_assert(nc::decimalPoint == nc::fullStop);
        static_assert(nc::timeSeparator == nc::colon);
        static_assert(nc::dateSeparator == nc::minus);
        static_assert(nc::namePathSeparator == nc::fullStop);
        static_assert(nc::valueListSeparator == nc::comma);
        static_assert(nc::digitSeparator == nc::singleQuote);
    }
};
