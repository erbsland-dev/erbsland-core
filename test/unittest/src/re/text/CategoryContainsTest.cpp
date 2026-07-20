// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../TestHelper.hpp"

#include <erbsland/re/impl/text/Category.hpp>
#include <erbsland/re/impl/text/Character.hpp>
#include <erbsland/re/StdFormatForRegEx.hpp>
#include <erbsland/unittest/FileHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>

using namespace el::re;
namespace fh = erbsland::unittest::fh;
using el::text::Char;
using impl::Category;

TESTED_TARGETS(Category)
TAGS(Text Categories)
class CategoryContainsTest final : public UNITTEST_SUBCLASS(re_test::TestHelper) {
private:
    [[nodiscard]] static auto tokenToValue(const std::string_view token) -> Category::Value {
        using V = Category::Value;
        static const std::unordered_map<std::string_view, V> map{
            {"Cc", V::Control},
            {"Cf", V::Format},
            {"Cn", V::Unassigned},
            {"Co", V::PrivateUse},
            {"Cs", V::Surrogate},
            {"Ll", V::LowercaseLetter},
            {"Lm", V::ModifierLetter},
            {"Lo", V::OtherLetter},
            {"Lt", V::TitlecaseLetter},
            {"Lu", V::UppercaseLetter},
            {"Mc", V::SpacingMark},
            {"Me", V::EnclosingMark},
            {"Mn", V::NonspacingMark},
            {"Nd", V::DecimalNumber},
            {"Nl", V::LetterNumber},
            {"No", V::OtherNumber},
            {"Pc", V::ConnectorPunctuation},
            {"Pd", V::DashPunctuation},
            {"Pe", V::ClosePunctuation},
            {"Pf", V::FinalPunctuation},
            {"Pi", V::InitialPunctuation},
            {"Po", V::OtherPunctuation},
            {"Ps", V::OpenPunctuation},
            {"Sc", V::CurrencySymbol},
            {"Sk", V::ModifierSymbol},
            {"Sm", V::MathSymbol},
            {"So", V::OtherSymbol},
            {"Zl", V::LineSeparator},
            {"Zp", V::ParagraphSeparator},
            {"Zs", V::SpaceSeparator},
            {"DigitUnicode", V::DigitUnicode},
            {"DigitAscii", V::DigitAscii},
            {"WordUnicode", V::WordUnicode},
            {"WordAscii", V::WordAscii},
            {"SpaceUnicode", V::SpaceUnicode},
            {"SpaceAscii", V::SpaceAscii},
            {"SpaceUnicodeDotAll", V::SpaceUnicodeDotAll},
            {"SpaceAsciiDotAll", V::SpaceAsciiDotAll},
            {"HorizontalSpaceUnicode", V::HorizontalSpaceUnicode},
            {"HorizontalSpaceAscii", V::HorizontalSpaceAscii},
            {"VerticalSpaceUnicode", V::VerticalSpaceUnicode},
            {"VerticalSpaceAscii", V::VerticalSpaceAscii},
            {"Any", V::Any},
            {"AnyDotAll", V::AnyDotAll},
        };
        if (const auto it = map.find(token); it != map.end()) {
            return it->second;
        }
        throw std::runtime_error("Unknown Category token in mask expression: " + std::string{token});
    }

    [[nodiscard]] static auto parseMaskExpression(const std::string &expression) -> Category::Mask {
        Category::Mask mask = 0;
        std::istringstream stream{expression};
        std::string token;
        while (stream >> token) {
            if (token != "|") {
                mask |= static_cast<Category::Mask>(tokenToValue(token));
            }
        }
        return mask;
    }

public:
    /// Test `Category::contains()` against a large, data-driven sample
    /// built from the Unicode Character Database.
    void testContainsAgainstUnicodeData() {
        // All primary character category values; aliases (C, L, ...) share
        // the same underlying value and are therefore not listed here
        // individually.
        static constexpr std::array allValues{
            Category::None,
            Category::Other,
            Category::Letter,
            Category::Mark,
            Category::Number,
            Category::Punctuation,
            Category::Symbol,
            Category::Separator,
            Category::Control,
            Category::Format,
            Category::Unassigned,
            Category::PrivateUse,
            Category::Surrogate,
            Category::LowercaseLetter,
            Category::ModifierLetter,
            Category::OtherLetter,
            Category::TitlecaseLetter,
            Category::UppercaseLetter,
            Category::SpacingMark,
            Category::EnclosingMark,
            Category::NonspacingMark,
            Category::DecimalNumber,
            Category::LetterNumber,
            Category::OtherNumber,
            Category::ConnectorPunctuation,
            Category::DashPunctuation,
            Category::ClosePunctuation,
            Category::FinalPunctuation,
            Category::InitialPunctuation,
            Category::OtherPunctuation,
            Category::OpenPunctuation,
            Category::CurrencySymbol,
            Category::ModifierSymbol,
            Category::MathSymbol,
            Category::OtherSymbol,
            Category::LineSeparator,
            Category::ParagraphSeparator,
            Category::SpaceSeparator,
            Category::DigitUnicode,
            Category::WordUnicode,
            Category::SpaceUnicode,
            Category::DigitAscii,
            Category::WordAscii,
            Category::SpaceAscii,
        };

        const auto lines = fh::readDataLines("data/re/CategoryContainsTestData.txt");
        std::size_t lineNumber = 0;
        for (const auto &line : lines) {
            ++lineNumber;
            if (line.empty() || line[0] == '#') {
                continue; // comment or empty line
            }

            const auto sepPos = line.find(';');
            REQUIRE_NOT_EQUAL(sepPos, std::string::npos);

            const auto codePointHex = line.substr(0, sepPos);
            const auto maskExpr = line.substr(sepPos + 1);

            const auto codePoint = static_cast<char32_t>(std::stoul(codePointHex, nullptr, 16));

            const auto expectedMask = parseMaskExpression(maskExpr);
            const Char character{codePoint};

            // Add rich context for failures: include line number and
            // code point in the diagnostic message.
            runWithContext(
                SOURCE_LOCATION(),
                [&]() {
                    for (const auto value : allValues) {
                        const bool actual = Category{value}.contains(character);

                        if (!character.isValidUnicode()) {
                            // For invalid Unicode scalar values `contains()`
                            // must *always* be false regardless of the
                            // underlying Unicode general category.
                            REQUIRE_FALSE(actual);
                        } else {
                            const Category::Mask valueMask = static_cast<Category::Mask>(value);
                            const bool expected = (expectedMask & valueMask) == valueMask;

                            // Use REQUIRE to trigger a failure immediately if the
                            // behavior does not match the expected mask.
                            REQUIRE_EQUAL(actual, expected);
                        }
                    }
                },
                [&]() {
                    return std::format(
                        "CategoryContainsTestData.txt line {} (code point U+{:06X})",
                        lineNumber,
                        static_cast<unsigned int>(codePoint));
                });
        }
    }

    /// Explicit tests for invalid characters (surrogates and values
    /// beyond the Unicode scalar range). `Category::contains()` must
    /// always return `false` for these, regardless of the character
    /// class.
    void testContainsWithInvalidCharacters() {
        constexpr std::array<char32_t, 6> invalidCodePoints{
            0xD800U,     // start of surrogate range
            0xDFFFU,     // end of surrogate range
            0xDC00U,     // middle of surrogate range
            0x110000U,   // just beyond the maximum scalar value
            0x1FFFFFU,   // well beyond Unicode range
            0xFFFFFFFFU, // max 32-bit value
        };

        static constexpr std::array classesToTest{
            Category::Other,
            Category::Letter,
            Category::Number,
            Category::Separator,
            Category::DigitUnicode,
            Category::WordUnicode,
            Category::SpaceUnicode,
            Category::SpaceUnicodeDotAll,
            Category::HorizontalSpaceUnicode,
            Category::VerticalSpaceUnicode,
            Category::Any,
            Category::AnyDotAll,
            Category::DigitAscii,
            Category::WordAscii,
            Category::SpaceAscii,
            Category::SpaceAsciiDotAll,
            Category::HorizontalSpaceAscii,
            Category::VerticalSpaceAscii,
        };

        for (const auto &cp : invalidCodePoints) {
            const Char character{cp};
            runWithContext(
                SOURCE_LOCATION(),
                [&]() {
                    for (const auto value : classesToTest) {
                        REQUIRE_FALSE(Category{value}.contains(character));
                    }
                },
                [&]() { return std::format("invalid code point U+{:08X}", static_cast<unsigned int>(cp)); });
        }
    }

    /// Tests for the special category ReDotAll: it must match every valid Unicode scalar value.
    void testReDotAll() {
        using V = Category::Value;

        // Sample a variety of valid code points, including controls and high code points
        static constexpr std::array<char32_t, 8> valid{
            U'\0',  // NUL
            U'\n',  // LF
            U' ',   // space
            U'A',   // letter
            0x07FF, // BMP edge
            0x2028, // Zl
            0x2029, // Zp
            0x1F600 // 😀 emoji
        };
        for (auto cp : valid) {
            REQUIRE(Category{V::AnyDotAll}.contains(Char{cp}));
        }

        // Invalid Unicode must not match
        constexpr std::array<char32_t, 4> invalid{0xD800U, 0xDFFFU, 0x110000U, 0xFFFFFFFFU};
        for (auto cp : invalid) {
            REQUIRE_FALSE(Category{V::AnyDotAll}.contains(Char{cp}));
        }
    }

    void testHorizontalAndVerticalSpaceCategories() {
        using V = Category::Value;

        // ASCII horizontal space (TAB)
        {
            const Char tab{U'\t'};
            REQUIRE(Category{V::HorizontalSpaceAscii}.contains(tab));
            REQUIRE(Category{V::HorizontalSpaceUnicode}.contains(tab));
            REQUIRE_FALSE(Category{V::VerticalSpaceAscii}.contains(tab));
            REQUIRE_FALSE(Category{V::VerticalSpaceUnicode}.contains(tab));
        }

        // ASCII vertical space (LF)
        {
            const Char lf{U'\n'};
            REQUIRE(Category{V::VerticalSpaceAscii}.contains(lf));
            REQUIRE(Category{V::VerticalSpaceUnicode}.contains(lf));
            REQUIRE_FALSE(Category{V::HorizontalSpaceAscii}.contains(lf));
            REQUIRE_FALSE(Category{V::HorizontalSpaceUnicode}.contains(lf));
        }

        // Unicode horizontal space (NO-BREAK SPACE)
        {
            const Char nbsp{0x00A0U};
            REQUIRE(Category{V::HorizontalSpaceUnicode}.contains(nbsp));
            REQUIRE(Category{V::HorizontalSpaceAscii}.contains(nbsp));
        }

        // Unicode vertical space (LINE SEPARATOR)
        {
            const Char lineSep{0x2028U};
            REQUIRE(Category{V::VerticalSpaceUnicode}.contains(lineSep));
            REQUIRE_FALSE(Category{V::VerticalSpaceAscii}.contains(lineSep));
        }
    }

    void testSpaceAndAnyDotAllBehavior() {
        using V = Category::Value;

        const Char lf{U'\n'};

        // Dot (Any) must not match ASCII vertical space; DotAll must.
        REQUIRE_FALSE(Category{V::Any}.contains(lf));
        REQUIRE(Category{V::AnyDotAll}.contains(lf));

        REQUIRE_FALSE(Category{V::SpaceAscii}.contains(lf));
        REQUIRE(Category{V::SpaceAsciiDotAll}.contains(lf));
    }
};
