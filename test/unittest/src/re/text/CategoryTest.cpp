// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/ParameterError.hpp>
#include <erbsland/re/impl/text/Category.hpp>
#include <erbsland/re/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <bit>
#include <string>
#include <unordered_set>

using namespace el::re;
using namespace el::text::literals;
using impl::Category;

TESTED_TARGETS(Category)
TAGS(Text Categories)
class CategoryTest final : public el::UnitTest {
public:
    void testDefaultAndValueAndOperators() {
        // Default constructed Category uses the None value
        Category def{};
        REQUIRE_EQUAL(def.raw(), Category::None);

        // Equality / inequality operators
        Category otherDef{};
        REQUIRE(def == otherDef);
        REQUIRE_FALSE(def != otherDef);

        Category letter{Category::Letter};
        Category number{Category::Number};
        REQUIRE(letter != number);
        REQUIRE_FALSE(letter == number);
    }

    void testFromStringKnownAndUnknown() {
        // Long Unicode general category names
        auto letterLong = Category::fromString("letter"_el);
        REQUIRE_EQUAL(letterLong.raw(), Category::Letter);

        auto otherLetterLong = Category::fromString("otherletter"_el);
        REQUIRE_EQUAL(otherLetterLong.raw(), Category::OtherLetter);

        // Short one and two letter Unicode categories
        auto letterShort = Category::fromString("l"_el);
        REQUIRE_EQUAL(letterShort.raw(), Category::Letter);

        auto uppercaseShort = Category::fromString("lu"_el);
        REQUIRE_EQUAL(uppercaseShort.raw(), Category::UppercaseLetter);

        // Unknown names fall back to None
        auto unknown = Category::fromString("doesnotexist"_el);
        REQUIRE_EQUAL(unknown.raw(), Category::None);

        auto empty = Category::fromString(""_el);
        REQUIRE_EQUAL(empty.raw(), Category::None);
    }

    void testFromUnprocessedString() {
        // Valid long name
        REQUIRE_EQUAL(Category::fromUnprocessedString("Letter"_el).raw(), Category::Letter);

        // Valid short name
        REQUIRE_EQUAL(Category::fromUnprocessedString("Lu"_el).raw(), Category::UppercaseLetter);

        // Normalized name (case folding and underscore removal)
        REQUIRE_EQUAL(Category::fromUnprocessedString("Other_Letter"_el).raw(), Category::OtherLetter);
        REQUIRE_EQUAL(Category::fromUnprocessedString("uppercase_letter"_el).raw(), Category::UppercaseLetter);

        // Invalid characters (triggers line 47 in Category.cpp)
        REQUIRE_THROWS_AS(el::err::ParameterError, Category::fromUnprocessedString("letter123"_el));
        REQUIRE_THROWS_AS(el::err::ParameterError, Category::fromUnprocessedString("letter!"_el));
        REQUIRE_THROWS_AS(el::err::ParameterError, Category::fromUnprocessedString("letter-name"_el));

        // Valid characters but unknown category name (triggers line 53 in Category.cpp)
        REQUIRE_THROWS_AS(el::err::ParameterError, Category::fromUnprocessedString("unknown_category"_el));
    }

    void testToLongAndShortStringMappedValues() {
        // A value that is present in the valueToNameMap
        Category letter{Category::Letter};
        REQUIRE_EQUAL(letter.toLongString(), "Letter"_el);
        REQUIRE_EQUAL(letter.toShortString(), "L"_el);

        Category control{Category::Control};
        REQUIRE_EQUAL(control.toLongString(), "Control"_el);
        REQUIRE_EQUAL(control.toShortString(), "Cc"_el);

        // RegExp specific classes have dedicated name mappings as well
        Category reUnicodeDigit{Category::DigitUnicode};
        Category reAsciiDigit{Category::DigitAscii};
        REQUIRE_EQUAL(reUnicodeDigit.toLongString(), "DigitUnicode"_el);
        REQUIRE_EQUAL(reAsciiDigit.toLongString(), "DigitAscii"_el);
        REQUIRE_EQUAL(reUnicodeDigit.toShortString(), "DigitUnicode"_el);
        REQUIRE_EQUAL(reAsciiDigit.toShortString(), "DigitAscii"_el);

        Category reUnicodeWord{Category::WordUnicode};
        Category reAsciiWord{Category::WordAscii};
        REQUIRE_EQUAL(reUnicodeWord.toLongString(), "WordUnicode"_el);
        REQUIRE_EQUAL(reAsciiWord.toLongString(), "WordAscii"_el);
        REQUIRE_EQUAL(reUnicodeWord.toShortString(), "WordUnicode"_el);
        REQUIRE_EQUAL(reAsciiWord.toShortString(), "WordAscii"_el);

        Category reUnicodeSpace{Category::SpaceUnicode};
        Category reAsciiSpace{Category::SpaceAscii};
        REQUIRE_EQUAL(reUnicodeSpace.toLongString(), "SpaceUnicode"_el);
        REQUIRE_EQUAL(reAsciiSpace.toLongString(), "SpaceAscii"_el);
        REQUIRE_EQUAL(reUnicodeSpace.toShortString(), "SpaceUnicode"_el);
        REQUIRE_EQUAL(reAsciiSpace.toShortString(), "SpaceAscii"_el);

        Category reUnicodeSpaceDotAll{Category::SpaceUnicodeDotAll};
        Category reAsciiSpaceDotAll{Category::SpaceAsciiDotAll};
        REQUIRE_EQUAL(reUnicodeSpaceDotAll.toLongString(), "SpaceUnicodeDotAll"_el);
        REQUIRE_EQUAL(reAsciiSpaceDotAll.toLongString(), "SpaceAsciiDotAll"_el);
        REQUIRE_EQUAL(reUnicodeSpaceDotAll.toShortString(), "SpaceUnicodeDotAll"_el);
        REQUIRE_EQUAL(reAsciiSpaceDotAll.toShortString(), "SpaceAsciiDotAll"_el);

        Category reHorizUnicode{Category::HorizontalSpaceUnicode};
        Category reHorizAscii{Category::HorizontalSpaceAscii};
        REQUIRE_EQUAL(reHorizUnicode.toLongString(), "HorizontalSpaceUnicode"_el);
        REQUIRE_EQUAL(reHorizAscii.toLongString(), "HorizontalSpaceAscii"_el);
        REQUIRE_EQUAL(reHorizUnicode.toShortString(), "HorizontalSpaceUnicode"_el);
        REQUIRE_EQUAL(reHorizAscii.toShortString(), "HorizontalSpaceAscii"_el);

        Category reVertUnicode{Category::VerticalSpaceUnicode};
        Category reVertAscii{Category::VerticalSpaceAscii};
        REQUIRE_EQUAL(reVertUnicode.toLongString(), "VerticalSpaceUnicode"_el);
        REQUIRE_EQUAL(reVertAscii.toLongString(), "VerticalSpaceAscii"_el);
        REQUIRE_EQUAL(reVertUnicode.toShortString(), "VerticalSpaceUnicode"_el);
        REQUIRE_EQUAL(reVertAscii.toShortString(), "VerticalSpaceAscii"_el);

        Category reAny{Category::Any};
        Category reAnyDotAll{Category::AnyDotAll};
        REQUIRE_EQUAL(reAny.toLongString(), "Any"_el);
        REQUIRE_EQUAL(reAnyDotAll.toLongString(), "AnyDotAll"_el);
        REQUIRE_EQUAL(reAny.toShortString(), "Any"_el);
        REQUIRE_EQUAL(reAnyDotAll.toShortString(), "AnyDotAll"_el);
    }

    void testToLongAndShortStringForNone() {
        // The None value is not present in the name map and must return empty strings
        Category none{}; // default is None
        REQUIRE_EQUAL(none.raw(), Category::None);

        const auto &longName = none.toLongString();
        const auto &shortName = none.toShortString();
        REQUIRE_EQUAL(longName, ""_el);
        REQUIRE_EQUAL(shortName, ""_el);
    }

    void testIncludes() {
        REQUIRE(Category{Category::Letter}.includes(Category{Category::Letter}));
        REQUIRE(Category{Category::Letter}.includes(Category{Category::UppercaseLetter}));
        REQUIRE(Category{Category::Letter}.includes(Category{Category::LowercaseLetter}));
        REQUIRE_FALSE(Category{Category::LowercaseLetter}.includes(Category{Category::Letter}));
        REQUIRE(Category{Category::SpaceUnicode}.includes(Category{Category::SpaceAscii}));
        REQUIRE_FALSE(Category{Category::SpaceAscii}.includes(Category{Category::SpaceUnicode}));
    }
};
