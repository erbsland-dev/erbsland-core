// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../TestHelper.hpp"

#include <erbsland/re/impl/text/Category.hpp>
#include <erbsland/re/impl/text/Character.hpp>
#include <erbsland/re/StdFormatForRegEx.hpp>

using namespace el::re;
using el::text::Char;
using impl::Category;

TESTED_TARGETS(Category)
TAGS(Text Categories)
class RegexpSpecialCategoriesTest final : public UNITTEST_SUBCLASS(re_test::TestHelper) {
public:
    void testDigitAsciiVsUnicodeExamples() {
        using V = Category::Value;

        // ASCII digits must match both ASCII and Unicode digit classes
        for (char32_t cp = U'0'; cp <= U'9'; ++cp) {
            const Char c{cp};
            REQUIRE(Category{V::DigitAscii}.contains(c));
            REQUIRE(Category{V::DigitUnicode}.contains(c));
        }

        // Arabic-Indic digit four U+0664: Unicode digit only
        {
            const Char c{0x0664};
            REQUIRE(Category{V::DigitUnicode}.contains(c));
            REQUIRE_FALSE(Category{V::DigitAscii}.contains(c));
        }

        // Fullwidth digit nine U+FF19: Unicode digit only
        {
            const Char c{0xFF19};
            REQUIRE(Category{V::DigitUnicode}.contains(c));
            REQUIRE_FALSE(Category{V::DigitAscii}.contains(c));
        }

        // Roman Numeral Nine U+2168 is Nl, not Nd -> not a digit class
        {
            const Char c{0x2168};
            REQUIRE_FALSE(Category{V::DigitUnicode}.contains(c));
            REQUIRE_FALSE(Category{V::DigitAscii}.contains(c));
        }
    }

    void testWordAsciiVsUnicodeExamples() {
        using V = Category::Value;

        // ASCII letters, digits and underscore are in both \w variants
        for (char32_t cp : {U'A', U'Z', U'a', U'z', U'0', U'9', U'_'}) {
            const Char c{cp};
            REQUIRE(Category{V::WordAscii}.contains(c));
            REQUIRE(Category{V::WordUnicode}.contains(c));
        }

        // Hyphen-minus is not part of \w
        {
            const Char c{U'-'};
            REQUIRE_FALSE(Category{V::WordAscii}.contains(c));
            REQUIRE_FALSE(Category{V::WordUnicode}.contains(c));
        }

        // Non-ASCII letters are only in the Unicode \w
        for (char32_t cp : std::array<char32_t, 4>{U'Ö', static_cast<char32_t>(0x0416) /* Ж */, U'π', U'汉'}) {
            const Char c{cp};
            REQUIRE(Category{V::WordUnicode}.contains(c));
            REQUIRE_FALSE(Category{V::WordAscii}.contains(c));
        }

        // Combining marks alone are not included in WordUnicode (spec is L + Nd + '_')
        {
            const Char c{0x0301}; // COMBINING ACUTE ACCENT
            REQUIRE_FALSE(Category{V::WordUnicode}.contains(c));
            REQUIRE_FALSE(Category{V::WordAscii}.contains(c));
        }
    }

    void testSpaceAsciiVsUnicodeSets() {
        using V = Category::Value;

        // Space and TAB are considered space in both ASCII and Unicode variants
        for (char32_t cp : {U' ', U'\t'}) {
            const Char c{cp};
            REQUIRE(Category{V::SpaceAscii}.contains(c));
            REQUIRE(Category{V::SpaceUnicode}.contains(c));
            // TAB is explicitly included in all Space... categories
            REQUIRE(Category{V::SpaceAsciiDotAll}.contains(c));
            REQUIRE(Category{V::SpaceUnicodeDotAll}.contains(c));
        }

        // CR, VT, FF are ASCII vertical controls: not SpaceAscii/SpaceUnicode, but part of DotAll and vertical sets
        for (char32_t cp : {U'\r', U'\v', U'\f'}) {
            const Char c{cp};
            runWithContext(
                SOURCE_LOCATION(),
                [&]() {
                    REQUIRE_FALSE(Category{V::SpaceAscii}.contains(c));
                    REQUIRE_FALSE(Category{V::SpaceUnicode}.contains(c));
                    REQUIRE(Category{V::SpaceAsciiDotAll}.contains(c));
                    REQUIRE(Category{V::SpaceUnicodeDotAll}.contains(c));
                    REQUIRE_FALSE(Category{V::HorizontalSpaceUnicode}.contains(c));
                    REQUIRE(Category{V::VerticalSpaceAscii}.contains(c));
                    REQUIRE(Category{V::VerticalSpaceUnicode}.contains(c));
                },
                [&]() { return std::format("cp=U+{:04X}", static_cast<unsigned int>(cp)); });
        }

        // LF: not SpaceAscii (unless DotAll), not SpaceUnicode; but part of DotAll variants
        {
            const Char lf{U'\n'};
            REQUIRE_FALSE(Category{V::SpaceAscii}.contains(lf));
            REQUIRE_FALSE(Category{V::SpaceUnicode}.contains(lf));
            REQUIRE(Category{V::SpaceAsciiDotAll}.contains(lf));
            REQUIRE(Category{V::SpaceUnicodeDotAll}.contains(lf));
        }

        // NO-BREAK SPACE is a Unicode Zs and marked as HorizontalSpaceAscii by design;
        // due to shared ASCII-flag bit across Space-categories, it also satisfies SpaceAscii.
        {
            const Char nbsp{0x00A0};
            REQUIRE(Category{V::HorizontalSpaceAscii}.contains(nbsp));
            REQUIRE(Category{V::HorizontalSpaceUnicode}.contains(nbsp));
            REQUIRE(Category{V::SpaceUnicode}.contains(nbsp));
            REQUIRE(Category{V::SpaceAscii}.contains(nbsp));
        }

        // NEL (U+0085) is a vertical Unicode space; not ASCII space
        {
            const Char nel{0x0085};
            REQUIRE(Category{V::VerticalSpaceUnicode}.contains(nel));
            REQUIRE_FALSE(Category{V::SpaceAscii}.contains(nel));
        }

        // Line/Paragraph separators are vertical Unicode and SpaceUnicodeDotAll
        for (char32_t cp : {0x2028, 0x2029}) { // Zl, Zp
            const Char c{cp};
            REQUIRE(Category{V::VerticalSpaceUnicode}.contains(c));
            REQUIRE(Category{V::SpaceUnicodeDotAll}.contains(c));
            REQUIRE_FALSE(Category{V::SpaceUnicode}.contains(c));
            REQUIRE_FALSE(Category{V::HorizontalSpaceUnicode}.contains(c));
        }
    }

    void testAnyVsAnyDotAll() {
        using V = Category::Value;

        const Char lf{U'\n'};
        REQUIRE_FALSE(Category{V::Any}.contains(lf));
        REQUIRE(Category{V::AnyDotAll}.contains(lf));

        const Char space{U' '};
        REQUIRE(Category{V::Any}.contains(space));
        REQUIRE(Category{V::AnyDotAll}.contains(space));
    }
};
