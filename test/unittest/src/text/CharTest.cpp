// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/Char.hpp>
#include <erbsland/text/CharSignal.hpp>
#include <erbsland/text/IntegerBase.hpp>
#include <erbsland/text/LetterCase.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <compare>

using el::text::Char;
using el::text::CharSignal;

TESTED_TARGETS(Char)
class CharTest final : public el::UnitTest {
public:
    void testDefaultAndRawValue() {
        constexpr auto ch = Char{};

        static_assert(ch.isNull());
        static_assert(ch.toRawValue() == U'\0');
        REQUIRE(ch.isNull());
        REQUIRE_EQUAL(ch.toRawValue(), U'\0');
    }

    void testComparisons() {
        constexpr auto a = Char{0x41U};
        constexpr auto b = Char{0x42U};

        static_assert((a <=> b) == std::strong_ordering::less);
        static_assert(a == U'A');
        static_assert(U'A' == a);
        static_assert(U'B' > a);
        REQUIRE(a < b);
        REQUIRE(a == U'A');
        REQUIRE(U'A' == a);
        REQUIRE(U'B' > a);
    }

    void testUnicodeValidity() {
        static_assert(Char::isHighSurrogate(0xD800U));
        static_assert(!Char::isHighSurrogate(0xDC00U));
        static_assert(Char::isLowSurrogate(0xDC00U));
        static_assert(!Char::isLowSurrogate(0xD800U));

        static_assert(Char{0x10FFFF}.isValidUnicode());
        static_assert(!Char{0x110000}.isValidUnicode());
        static_assert(!Char{0xD800}.isValidUnicode());
        static_assert(!Char{0xDFFF}.isValidUnicode());
        static_assert(Char{0xE000}.isValidUnicode());

        REQUIRE(Char::isHighSurrogate(0xD800U));
        REQUIRE_FALSE(Char::isHighSurrogate(0xDC00U));
        REQUIRE(Char::isLowSurrogate(0xDC00U));
        REQUIRE_FALSE(Char::isLowSurrogate(0xD800U));
        REQUIRE(Char{0x10FFFF}.isValidUnicode());
        REQUIRE_FALSE(Char{0x110000}.isValidUnicode());
        REQUIRE_FALSE(Char{0xD800}.isValidUnicode());
        REQUIRE_FALSE(Char{0xDFFF}.isValidUnicode());
        REQUIRE(Char{0xE000}.isValidUnicode());
    }

    void testSignals() {
        using Char = Char;
        using CharSignal = CharSignal;

        constexpr auto endOfData = Char::endOfData();
        constexpr auto noCodePoint = Char::noCodePoint();
        constexpr auto lowestReservedSignal = Char{0xFFFFFF00U};
        constexpr auto belowReservedSignal = Char{0xFFFFFEFFU};

        static_assert(!Char{}.isSignal());
        static_assert(!Char{}.isEndOfData());
        static_assert(!Char{}.isNoCodePoint());
        static_assert(endOfData.toRawValue() == 0xFFFFFFFFU);
        static_assert(noCodePoint.toRawValue() == 0xFFFFFFFEU);
        static_assert(endOfData.isSignal());
        static_assert(noCodePoint.isSignal());
        static_assert(lowestReservedSignal.isSignal());
        static_assert(!belowReservedSignal.isSignal());
        static_assert(endOfData.isEndOfData());
        static_assert(!endOfData.isNoCodePoint());
        static_assert(noCodePoint.isNoCodePoint());
        static_assert(!noCodePoint.isEndOfData());
        static_assert(endOfData == CharSignal::EndOfData);
        static_assert(endOfData != CharSignal::NoCodePoint);
        static_assert(noCodePoint == CharSignal::NoCodePoint);
        static_assert(noCodePoint != CharSignal::EndOfData);
        static_assert(!endOfData.isValidUnicode());
        static_assert(!noCodePoint.isValidUnicode());
        static_assert(Char::fromSignal(CharSignal::EndOfData) == endOfData);
        static_assert(Char::fromSignal(CharSignal::NoCodePoint) == noCodePoint);

        REQUIRE_FALSE(Char{}.isSignal());
        REQUIRE_EQUAL(endOfData.toRawValue(), char32_t{0xFFFFFFFFU});
        REQUIRE_EQUAL(noCodePoint.toRawValue(), char32_t{0xFFFFFFFEU});
        REQUIRE(endOfData.isSignal());
        REQUIRE(noCodePoint.isSignal());
        REQUIRE(lowestReservedSignal.isSignal());
        REQUIRE_FALSE(belowReservedSignal.isSignal());
        REQUIRE(endOfData.isEndOfData());
        REQUIRE_FALSE(endOfData.isNoCodePoint());
        REQUIRE(noCodePoint.isNoCodePoint());
        REQUIRE_FALSE(noCodePoint.isEndOfData());
        REQUIRE(endOfData == CharSignal::EndOfData);
        REQUIRE(endOfData != CharSignal::NoCodePoint);
        REQUIRE(noCodePoint == CharSignal::NoCodePoint);
        REQUIRE(noCodePoint != CharSignal::EndOfData);
        REQUIRE_FALSE(endOfData.isValidUnicode());
        REQUIRE_FALSE(noCodePoint.isValidUnicode());
        REQUIRE_EQUAL(Char::fromSignal(CharSignal::EndOfData), endOfData);
        REQUIRE_EQUAL(Char::fromSignal(CharSignal::NoCodePoint), noCodePoint);
    }

    void testAsciiFastPath() {
        using Char = Char;
        using AsciiCategory = el::text::AsciiCategory;

        static_assert(Char{U'A'}.isAscii());
        static_assert(Char{U'A'}.isAsciiCategory(AsciiCategory::Letter));
        static_assert(Char{U'A'}.isAsciiCategory(AsciiCategory::UppercaseLetter));
        static_assert(!Char{U'A'}.isAsciiCategory(AsciiCategory::LowercaseLetter));
        static_assert(Char{U'9'}.isAsciiCategory(AsciiCategory::Digit));
        static_assert(Char{U'F'}.isAsciiCategory(AsciiCategory::HexDigit));
        static_assert(Char{U'A'}.isAsciiWord());
        static_assert(Char{U'9'}.isAsciiWord());
        static_assert(Char{U'_'}.isAsciiWord());
        static_assert(!Char{U'-'}.isAsciiWord());
        static_assert(!Char{U'ä'}.isAsciiWord());
        static_assert(Char{U'['}.isSpecialRegexCharacter());
        static_assert(Char{U'\\'}.isSpecialRegexCharacter());
        static_assert(!Char{U'A'}.isSpecialRegexCharacter());
        static_assert(Char{U'_'}.isAsciiCategory(AsciiCategory::Punctuation));
        static_assert(Char{U' '}.isAsciiCategory(AsciiCategory::Whitespace));
        static_assert(Char{U'\t'}.isAsciiCategory(AsciiCategory::Blank));
        static_assert(Char{U'\n'}.isAsciiCategory(AsciiCategory::Control));
        static_assert(!Char{U'ä'}.isAscii());
        static_assert(!Char{U'ä'}.isAsciiCategory(AsciiCategory::Letter));

        REQUIRE(Char{U'Z'}.isAsciiCategory(AsciiCategory::Alphanumeric));
        REQUIRE(Char{U'_'}.isAsciiWord());
        REQUIRE_FALSE(Char{U'-'}.isAsciiWord());
        REQUIRE(Char{U'?'}.isSpecialRegexCharacter());
        REQUIRE_FALSE(Char{U'_'}.isSpecialRegexCharacter());
        REQUIRE(Char{U'~'}.isAsciiCategory(AsciiCategory::Punctuation));
        REQUIRE_FALSE(Char{U' '}.isAsciiCategory(AsciiCategory::Alphanumeric));
    }

    void testDigitValues() {
        using Char = Char;
        using IntegerBase = el::text::IntegerBase;
        using LetterCase = el::text::LetterCase;

        static_assert(Char{U'0'}.digitValue().value() == 0U);
        static_assert(Char{U'9'}.digitValue().value() == 9U);
        static_assert(Char{U'a'}.digitValue().value() == 10U);
        static_assert(Char{U'F'}.digitValue().value() == 15U);
        static_assert(!Char{U'_'}.digitValue().has_value());
        static_assert(Char{U'1'}.isDigitValue(IntegerBase::Binary));
        static_assert(!Char{U'2'}.isDigitValue(IntegerBase::Binary));
        static_assert(Char{U'f'}.isDigitValue(IntegerBase::Hexadecimal));
        static_assert(Char::fromDigitValue(15U, LetterCase::Lowercase) == U'f');
        static_assert(Char::fromDigitValue(15U, LetterCase::Uppercase) == U'F');

        REQUIRE_EQUAL(Char{U'8'}.digitValue().value(), 8U);
        REQUIRE_FALSE(Char{U'g'}.isDigitValue(IntegerBase::Hexadecimal));
        REQUIRE_EQUAL(Char::fromDigitValue(3U), Char{U'3'});
    }

    void testSafeUnicode() {
        static_assert(!Char{0x001FU}.isSafeUnicode());
        static_assert(Char{0x0020U}.isSafeUnicode());
        static_assert(Char{0x007EU}.isSafeUnicode());
        static_assert(!Char{0x007FU}.isSafeUnicode());
        static_assert(!Char{0x009FU}.isSafeUnicode());
        static_assert(Char{0x00A0U}.isSafeUnicode());
        static_assert(!Char{0x061CU}.isSafeUnicode());
        static_assert(!Char{0x200EU}.isSafeUnicode());
        static_assert(!Char{0x202AU}.isSafeUnicode());
        static_assert(!Char{0x2066U}.isSafeUnicode());
        static_assert(!Char{0x2400U}.isSafeUnicode());
        static_assert(!Char{0xFE00U}.isSafeUnicode());
        static_assert(!Char{0xFFF9U}.isSafeUnicode());
        static_assert(!Char{0xE0000U}.isSafeUnicode());
        static_assert(!Char{0xE0100U}.isSafeUnicode());
        static_assert(Char{0x1F600U}.isSafeUnicode());
        static_assert(!Char{0xD800U}.isSafeUnicode());
        static_assert(!Char::endOfData().isSafeUnicode());

        REQUIRE(Char{U'A'}.isSafeUnicode());
        REQUIRE_FALSE(Char{U'\n'}.isSafeUnicode());
        REQUIRE_FALSE(Char{0x2400U}.isSafeUnicode());
        REQUIRE_FALSE(Char::endOfData().isSafeUnicode());
    }

    void testUnicodeCategoryLookup() {
        using Char = Char;
        using UnicodeCategory = el::text::UnicodeCategory;
        using UnicodeCategoryGroup = el::text::UnicodeCategoryGroup;

        REQUIRE_EQUAL(Char{U'A'}.category(), UnicodeCategory::UppercaseLetter);
        REQUIRE_EQUAL(Char{U'A'}.categoryGroup(), UnicodeCategoryGroup::Letter);
        REQUIRE(Char{U'A'}.isCategory(UnicodeCategory::UppercaseLetter));

        REQUIRE_EQUAL(Char{0x0301U}.category(), UnicodeCategory::NonspacingMark);
        REQUIRE_EQUAL(Char{0x0301U}.categoryGroup(), UnicodeCategoryGroup::Mark);

        REQUIRE_EQUAL(Char{0x0660U}.category(), UnicodeCategory::DecimalNumber);
        REQUIRE_EQUAL(Char{0x0660U}.categoryGroup(), UnicodeCategoryGroup::Number);

        REQUIRE_EQUAL(Char{U'_'}.category(), UnicodeCategory::ConnectorPunctuation);
        REQUIRE(Char{U'_'}.isCategoryGroup(UnicodeCategoryGroup::Punctuation));

        REQUIRE_EQUAL(Char{U'+'}.category(), UnicodeCategory::MathSymbol);
        REQUIRE(Char{U'+'}.isCategoryGroup(UnicodeCategoryGroup::Symbol));

        REQUIRE_EQUAL(Char{U' '}.category(), UnicodeCategory::SpaceSeparator);
        REQUIRE(Char{U' '}.isCategoryGroup(UnicodeCategoryGroup::Separator));

        REQUIRE_EQUAL(Char{U'\n'}.category(), UnicodeCategory::Control);
        REQUIRE_EQUAL(Char{U'\n'}.categoryGroup(), UnicodeCategoryGroup::Other);
        REQUIRE(Char{U'\n'}.isControl());
        REQUIRE(Char{0x0085U}.isControl());
        REQUIRE_FALSE(Char{U'A'}.isControl());
        REQUIRE_FALSE(Char{0x200DU}.isControl());
        REQUIRE_FALSE(Char{0x110000U}.isControl());
        REQUIRE(Char{U'\n'}.isControlOrFormat());
        REQUIRE(Char{0x200DU}.isControlOrFormat());
        REQUIRE_FALSE(Char{U'A'}.isControlOrFormat());
        REQUIRE_FALSE(Char{0x110000U}.isControlOrFormat());

        REQUIRE_EQUAL(Char{0xE000U}.category(), UnicodeCategory::PrivateUse);
        REQUIRE_EQUAL(Char{0x110000U}.category(), UnicodeCategory::Unassigned);
        REQUIRE_EQUAL(Char{0xD800U}.category(), UnicodeCategory::Unassigned);
    }

    void testUnicodeCaseMapping() {
        using Char = Char;

        REQUIRE_EQUAL(Char{U'A'}.caseFolded(), Char{U'a'});
        REQUIRE_EQUAL(Char{U'A'}.toLowercase(), Char{U'a'});
        REQUIRE_EQUAL(Char{U'a'}.toUppercase(), Char{U'A'});
        REQUIRE_EQUAL(Char::caseFolded(Char{U'A'}), Char{U'a'});
        REQUIRE_EQUAL(Char::toLowercase(Char{U'A'}), Char{U'a'});
        REQUIRE_EQUAL(Char::toUppercase(Char{U'a'}), Char{U'A'});

        REQUIRE_EQUAL(Char{0x00C4U}.caseFolded(), Char{0x00E4U});
        REQUIRE_EQUAL(Char{0x00C4U}.toLowercase(), Char{0x00E4U});
        REQUIRE_EQUAL(Char{0x00E4U}.toUppercase(), Char{0x00C4U});

        REQUIRE_EQUAL(Char{0x03A3U}.caseFolded(), Char{0x03C3U});
        REQUIRE_EQUAL(Char{0x03A3U}.toLowercase(), Char{0x03C3U});
        REQUIRE_EQUAL(Char{0x03C2U}.caseFolded(), Char{0x03C3U});
        REQUIRE_EQUAL(Char{0x03C2U}.toUppercase(), Char{0x03A3U});

        REQUIRE_EQUAL(Char{0x212AU}.caseFolded(), Char{U'k'});
        REQUIRE_EQUAL(Char{0x212AU}.toLowercase(), Char{U'k'});
        REQUIRE_EQUAL(Char{0x00B5U}.caseFolded(), Char{0x03BCU});
        REQUIRE_EQUAL(Char{0x00B5U}.toUppercase(), Char{0x039CU});

        REQUIRE_EQUAL(Char{0x110000U}.caseFolded(), Char{0x110000U});
        REQUIRE_EQUAL(Char{0xD800U}.toLowercase(), Char{0xD800U});
        REQUIRE_EQUAL(Char{0xD800U}.toUppercase(), Char{0xD800U});
    }

    void testDisplayWidth() {
        REQUIRE_EQUAL(Char{U'A'}.displayWidth(), 1);
        REQUIRE_EQUAL(Char{U'界'}.displayWidth(), 2);
        REQUIRE_EQUAL(Char{0x1F600U}.displayWidth(), 2);
        REQUIRE_EQUAL(Char{0x0301U}.displayWidth(), 0);
        REQUIRE_EQUAL(Char{U'\n'}.displayWidth(), 0);
        REQUIRE_EQUAL(Char{0x0085U}.displayWidth(), 0);
        REQUIRE_EQUAL(Char{0x110000U}.displayWidth(), 0);
        REQUIRE_EQUAL(Char::endOfData().displayWidth(), 0);
        REQUIRE_EQUAL(Char::noCodePoint().displayWidth(), 0);
    }

    void testAsciiCaseMapping() {
        using Char = Char;
        using Signal = CharSignal;

        static_assert(Char{U'A'}.toAsciiLowercase() == U'a');
        static_assert(Char{U'Z'}.toAsciiLowercase() == U'z');
        static_assert(Char{U'a'}.toAsciiUppercase() == U'A');
        static_assert(Char{U'z'}.toAsciiUppercase() == U'Z');
        static_assert(Char::toAsciiLowercase(Char{U'A'}) == U'a');
        static_assert(Char::toAsciiUppercase(Char{U'a'}) == U'A');
        static_assert(Char{U'0'}.toAsciiLowercase() == U'0');
        static_assert(Char{U'!'}.toAsciiUppercase() == U'!');
        static_assert(Char::null().toAsciiLowercase().isNull());
        static_assert(Char::replacement().toAsciiUppercase().isReplacement());
        static_assert(Char::fromSignal(Signal::EndOfData).toAsciiLowercase().isEndOfData());
        static_assert(Char::fromSignal(Signal::NoCodePoint).toAsciiUppercase().isNoCodePoint());

        REQUIRE_EQUAL(Char{0x00C4U}.toAsciiLowercase(), Char{0x00C4U});
        REQUIRE_EQUAL(Char{0x00E4U}.toAsciiUppercase(), Char{0x00E4U});
        REQUIRE_EQUAL(Char{0x00C4U}.toLowercase(), Char{0x00E4U});
        REQUIRE_EQUAL(Char{0x00E4U}.toUppercase(), Char{0x00C4U});
    }

    void testReplacementCharacter() {
        const auto replacement = Char::replacement();

        REQUIRE(replacement.isReplacement());
        REQUIRE_EQUAL(replacement.toRawValue(), char32_t{0xFFFD});
    }
};
