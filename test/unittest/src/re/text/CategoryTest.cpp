// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/Char.hpp>
#include <erbsland/CharRange.hpp>
#include <erbsland/LoopStatus.hpp>
#include <erbsland/NotFoundError.hpp>
#include <erbsland/re/impl/text/Category.hpp>
#include <erbsland/re/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <barrier>
#include <bit>
#include <future>
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
        REQUIRE_EQUAL(def, otherDef);
        REQUIRE_EQUAL(def, otherDef);

        Category letter{Category::Letter};
        Category number{Category::Number};
        REQUIRE_NOT_EQUAL(letter, number);
        REQUIRE_NOT_EQUAL(letter, number);
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
        REQUIRE_THROWS_AS(el::err::NotFoundError, Category::fromUnprocessedString("letter123"_el));
        REQUIRE_THROWS_AS(el::err::NotFoundError, Category::fromUnprocessedString("letter!"_el));
        REQUIRE_THROWS_AS(el::err::NotFoundError, Category::fromUnprocessedString("letter-name"_el));

        // Valid characters but unknown category name (triggers line 53 in Category.cpp)
        REQUIRE_THROWS_AS(el::err::NotFoundError, Category::fromUnprocessedString("unknown_category"_el));
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

    void testCharacterSetCopiesAcrossThreads() {
        static constexpr auto cCategories = std::array{
            Category::Letter,
            Category::DecimalNumber,
            Category::DigitUnicode,
            Category::WordUnicode,
            Category::WordAscii,
            Category::SpaceUnicode,
            Category::SpaceAscii,
            Category::SpaceUnicodeDotAll,
            Category::SpaceAsciiDotAll,
            Category::HorizontalSpaceUnicode,
            Category::HorizontalSpaceAscii,
            Category::VerticalSpaceUnicode,
            Category::Any};
        auto start = std::barrier{4};
        auto workers = std::array<std::future<bool>, 4>{};
        for (auto &worker : workers) {
            worker = std::async(std::launch::async, [&start]() -> bool {
                start.arrive_and_wait();
                for (const auto value : cCategories) {
                    const auto category = Category{value};
                    const auto original = category.characterSet();
                    auto modified = category.characterSet();
                    auto firstCharacter = el::Char{};
                    original.forEach([&](const el::CharRange range) -> el::LoopStatus {
                        firstCharacter = range.from();
                        return el::LoopStatus::Stop;
                    });
                    modified.remove(firstCharacter);
                    if (!original.contains(firstCharacter) || modified.contains(firstCharacter) ||
                        category.characterSet() != original) {
                        return false;
                    }
                }
                return true;
            });
        }
        for (auto &worker : workers) {
            REQUIRE(worker.get());
        }
        REQUIRE_EQUAL(
            Category{Category::DecimalNumber}.characterSet(), Category{Category::DigitUnicode}.characterSet());
        REQUIRE_EQUAL(
            Category{Category::SpaceUnicode}.characterSet(), Category{Category::HorizontalSpaceUnicode}.characterSet());
        REQUIRE_EQUAL(
            Category{Category::SpaceAscii}.characterSet(), Category{Category::HorizontalSpaceAscii}.characterSet());
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
