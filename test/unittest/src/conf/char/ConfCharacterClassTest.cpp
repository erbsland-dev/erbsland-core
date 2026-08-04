// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../ConfTestHelper.hpp"

#include <erbsland/conf/impl/char/CharClass.hpp>
#include <erbsland/conf/impl/char/NamedChars.hpp>

#include <format>
#include <vector>

using namespace el::conf;
using el::conf::impl::CharClass;
namespace text = el::text;
namespace nc = el::conf::impl::nc;

TESTED_TARGETS(CharClass)
class ConfCharacterClassTest final : public UNITTEST_SUBCLASS(ConfTestHelper) {
public:
    void testCharacterClasses() {
        struct TestCase {
            CharClass charClass;
            text::Char matching;
            text::Char nonMatching;
        };
        const auto testCases = std::vector<TestCase>{
            {CharClass::Spacing, nc::tab, nc::newLine},
            {CharClass::LineBreak, nc::newLine, nc::space},
            {CharClass::NameStart, nc::at, nc::underscore},
            {CharClass::Letter, nc::lowercaseA, nc::digit0},
            {CharClass::LetterOrDigit, nc::digit9, nc::underscore},
            {CharClass::DecimalDigit, nc::digit0, nc::uppercaseA},
            {CharClass::HexDigit, nc::uppercaseF, text::Char{U'G'}},
            {CharClass::NameValueSeparator, nc::colon, nc::comma},
            {CharClass::OpeningBracket, nc::doubleQuote, nc::openingSquareBracket},
            {CharClass::SectionStart, nc::minus, nc::plus},
            {CharClass::EndOfLineStart, nc::commentStart, nc::lowercaseA},
            {CharClass::LetterA, nc::uppercaseA, nc::uppercaseB},
            {CharClass::LetterB, nc::lowercaseB, nc::lowercaseA},
            {CharClass::LetterF, nc::uppercaseF, nc::uppercaseE},
            {CharClass::LetterI, nc::uppercaseI, nc::uppercaseA},
            {CharClass::LetterN, nc::lowercaseN, nc::lowercaseA},
            {CharClass::LetterT, nc::uppercaseT, nc::uppercaseA},
            {CharClass::LetterX, nc::lowercaseX, nc::lowercaseA},
            {CharClass::LetterZ, nc::uppercaseZ, nc::uppercaseA},
            {CharClass::NumberStart, nc::plus, nc::lowercaseA},
            {CharClass::TimeStart, nc::lowercaseT, nc::lowercaseA},
            {CharClass::FloatLiteralStart, nc::uppercaseI, nc::digit0},
            {CharClass::ExponentStart, nc::uppercaseE, nc::uppercaseA},
            {CharClass::BinaryDigit, nc::digit1, text::Char{U'2'}},
            {CharClass::PlusOrMinus, nc::minus, nc::asterisk},
            {CharClass::SectionNameStart, nc::doubleQuote, nc::digit0},
            {CharClass::FormatIdentifierChar, nc::underscore, nc::microSign},
            {CharClass::IntegerSuffixChar, nc::microSign, nc::digit0},
            {CharClass::LineBreakOrEnd, text::Char::endOfData(), nc::space},
            {CharClass::ValidAfterValue, nc::valueListSeparator, nc::lowercaseA},
            {CharClass::ValidLang, nc::lowercaseA, text::Char{0x001FU}},
            {CharClass::FilePathSeparator, nc::backslash, nc::colon},
            {CharClass::InvalidWindowsServerName, nc::asterisk, nc::lowercaseA},
        };

        for (const auto &[charClass, matching, nonMatching] : testCases) {
            runWithContext(
                SOURCE_LOCATION(),
                [&]() -> void {
                    REQUIRE_EQUAL(matching, charClass);
                    REQUIRE_EQUAL(charClass, matching);
                    REQUIRE_EQUAL(matching, charClass);
                    REQUIRE_EQUAL(charClass, matching);
                    REQUIRE_NOT_EQUAL(nonMatching, charClass);
                    REQUIRE_NOT_EQUAL(charClass, nonMatching);
                    REQUIRE_NOT_EQUAL(nonMatching, charClass);
                    REQUIRE_NOT_EQUAL(charClass, nonMatching);
                },
                [&]() -> std::string {
                    return std::format(
                        "class {} matching U+{:04X} non-matching U+{:04X}",
                        static_cast<unsigned>(charClass),
                        static_cast<unsigned>(matching.toRawValue()),
                        static_cast<unsigned>(nonMatching.toRawValue()));
                });
        }
    }

    void testValidLanguageBoundaries() {
        static_assert(nc::lowercaseA == CharClass::Letter);
        static_assert(CharClass::Letter == nc::lowercaseA);
        static_assert(nc::digit0 != CharClass::Letter);
        static_assert(CharClass::Letter != nc::digit0);

        REQUIRE_EQUAL(nc::tab, CharClass::ValidLang);
        REQUIRE_EQUAL(nc::newLine, CharClass::ValidLang);
        REQUIRE_EQUAL(nc::carriageReturn, CharClass::ValidLang);
        REQUIRE_NOT_EQUAL(text::Char{0x0000U}, CharClass::ValidLang);
        REQUIRE_NOT_EQUAL(text::Char{0x001FU}, CharClass::ValidLang);
        REQUIRE_EQUAL(text::Char{0x0020U}, CharClass::ValidLang);
        REQUIRE_EQUAL(text::Char{0x007EU}, CharClass::ValidLang);
        REQUIRE_NOT_EQUAL(text::Char{0x007FU}, CharClass::ValidLang);
        REQUIRE_NOT_EQUAL(text::Char{0x00A0U}, CharClass::ValidLang);
        REQUIRE_EQUAL(text::Char{0x00A1U}, CharClass::ValidLang);
        REQUIRE_NOT_EQUAL(text::Char::endOfData(), CharClass::ValidLang);
        REQUIRE_NOT_EQUAL(text::Char::noCodePoint(), CharClass::ValidLang);
        REQUIRE_NOT_EQUAL(text::Char::error(), CharClass::ValidLang);
        REQUIRE_NOT_EQUAL(text::Char::byteOrderMark(), CharClass::ValidLang);
        REQUIRE_NOT_EQUAL(text::Char{0xFEFFU}, CharClass::ValidLang);
        REQUIRE_NOT_EQUAL(text::Char{0xD800U}, CharClass::ValidLang);
        REQUIRE_NOT_EQUAL(text::Char{0xDFFFU}, CharClass::ValidLang);
        REQUIRE_NOT_EQUAL(text::Char{0x110000U}, CharClass::ValidLang);
        REQUIRE_EQUAL(text::Char{0x10FFFFU}, CharClass::ValidLang);
    }

    void testWindowsServerNameBoundaries() {
        REQUIRE_EQUAL(text::Char{0x001FU}, CharClass::InvalidWindowsServerName);
        REQUIRE_NOT_EQUAL(text::Char{0x0020U}, CharClass::InvalidWindowsServerName);
        REQUIRE_NOT_EQUAL(text::Char{0x007FU}, CharClass::InvalidWindowsServerName);
        REQUIRE_EQUAL(text::Char{0x0080U}, CharClass::InvalidWindowsServerName);
        REQUIRE_EQUAL(nc::asterisk, CharClass::InvalidWindowsServerName);
        REQUIRE_EQUAL(nc::questionMark, CharClass::InvalidWindowsServerName);
        REQUIRE_EQUAL(nc::pipe, CharClass::InvalidWindowsServerName);
        REQUIRE_EQUAL(nc::doubleQuote, CharClass::InvalidWindowsServerName);
        REQUIRE_EQUAL(nc::lessThan, CharClass::InvalidWindowsServerName);
        REQUIRE_NOT_EQUAL(nc::greaterThan, CharClass::InvalidWindowsServerName);
    }

    void testInvalidClassValue() { REQUIRE_NOT_EQUAL(nc::lowercaseA, static_cast<CharClass>(0xFFU)); }
};
