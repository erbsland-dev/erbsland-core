// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StdFormat.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/StringPattern.hpp>
#include <erbsland/text/u16/U16String.hpp>
#include <erbsland/text/u16/U16StringEditor.hpp>
#include <erbsland/text/u32/U32String.hpp>
#include <erbsland/text/u32/U32StringEditor.hpp>
#include <erbsland/text/u8/U8String.hpp>
#include <erbsland/text/u8/U8StringEditor.hpp>
#include <erbsland/unit/ByteIndex.hpp>
#include <erbsland/unit/ByteLength.hpp>
#include <erbsland/unit/CpIndex.hpp>
#include <erbsland/unit/CpLength.hpp>
#include <erbsland/unit/U16DataIndex.hpp>
#include <erbsland/unit/U16DataLength.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <string>
#include <utility>

using namespace el::text::pattern;

using namespace el::text;
using namespace el::text::literals;
using namespace el::unit;

TESTED_TARGETS(StringPattern)
class StringPatternTest final : public el::UnitTest {
public:
    void testDefaultPatternNeverMatches() {
        const auto pattern = StringPattern{};

        REQUIRE_FALSE(pattern.matches("anything"_el));
        REQUIRE_EQUAL(pattern.trimmed("anything"_el), "anything"_el);
        REQUIRE_EQUAL(pattern.index("anything"_el), ByteIndex::noIndex());
        REQUIRE_EQUAL(pattern.length("anything"_el), ByteLength::zero());

        const auto [matching, rest] = pattern.split("anything"_el);
        REQUIRE(matching.isEmpty());
        REQUIRE_EQUAL(rest, "anything"_el);
    }

    void testParsedSyntax() {
        const auto pattern = StringPattern{"te??[a-z]*"_el};

        REQUIRE(pattern.matches("textm-tail"_el));
        REQUIRE(pattern.matches(u"texza-suffix"_el));
        REQUIRE(pattern.matches(U"testq"_el));
        REQUIRE_FALSE(pattern.matches("tex1M-tail"_el));
        REQUIRE_FALSE(pattern.matches("textM-tail"_el));

        const auto escaped = StringPattern{"file\\[\\?\\]\\*"_el};
        REQUIRE(escaped.matches("file[?]*.txt"_el));
        REQUIRE_FALSE(escaped.matches("fileA.txt"_el));
    }

    void testInvalidParsedSyntax() {
        REQUIRE_THROWS(StringPattern{"*"_el});
        REQUIRE_THROWS(StringPattern{"a*b*c"_el});
        REQUIRE_THROWS(StringPattern{"a["_el});
        REQUIRE_THROWS(StringPattern{"[]"_el});
        REQUIRE_THROWS(StringPattern{"[z-a]"_el});
        REQUIRE_THROWS(StringPattern{"a\\q"_el});
        REQUIRE_THROWS(StringPattern{"a]"_el});
    }

    void testStaticConstruction() {

        const auto prefix = StringPattern{Text{U"te"}, OneChar{}, Range{U'a', U'z'}, Divider{}};
        const auto setPattern = StringPattern{Text{U"ID"}, Set{{U'a', U'z'}, {U'A', U'Z'}}, OneChar{}};
        const auto u8MatchPattern = StringPattern{Text{U"\u00E9"}, Divider{}};
        const auto u16MatchPattern = StringPattern{Text{U"\U0001F600"}, Divider{}};

        REQUIRE(prefix.matches("text-more"_el));
        REQUIRE_FALSE(prefix.matches("te9T-more"_el));
        REQUIRE(setPattern.matches("IDa7"_el));
        REQUIRE(setPattern.matches("IDZ_"_el));
        REQUIRE_FALSE(setPattern.matches("ID17"_el));
        REQUIRE(u8MatchPattern.matches(u8"\u00E9clair"_el));
        REQUIRE(u16MatchPattern.matches(u"\U0001F600 trail"_el));
    }

    void testStaticConstructionLimits() {

        const auto maximumElements = StringPattern{Text{U"abcdefghijklmnop"}};
        REQUIRE(maximumElements.matches("abcdefghijklmnop-tail"_el));
        REQUIRE_THROWS(StringPattern{Text{U"abcdefghijklmnopq"}});

        const auto firstRanges =
            Set{{U'a', U'a'},
                {U'b', U'b'},
                {U'c', U'c'},
                {U'd', U'd'},
                {U'e', U'e'},
                {U'f', U'f'},
                {U'g', U'g'},
                {U'h', U'h'}};
        const auto secondRanges =
            Set{{U'i', U'i'},
                {U'j', U'j'},
                {U'k', U'k'},
                {U'l', U'l'},
                {U'm', U'm'},
                {U'n', U'n'},
                {U'o', U'o'},
                {U'p', U'p'}};
        const auto maximumRanges = StringPattern{firstRanges, secondRanges};
        REQUIRE(maximumRanges.matches("ai"_el));
        REQUIRE(maximumRanges.matches("hp"_el));
        REQUIRE_FALSE(maximumRanges.matches("qq"_el));
        REQUIRE_THROWS(StringPattern{firstRanges, secondRanges, Range{U'q', U'q'}});
    }

    void testParsedPatternsUseDynamicStorage() {
        const auto pattern = StringPattern{"abcdefghijklmnopq"_el};

        REQUIRE(pattern.matches("abcdefghijklmnopq-tail"_el));
        REQUIRE_EQUAL(pattern.index("abcdefghijklmnopq-tail"_el), ByteIndex{17U});
    }

    void testCopiesShareImmutablePatternBehavior() {
        auto parsedOriginal = StringPattern{"ab*xy"_el};
        const auto parsedCopy = parsedOriginal;
        const auto parsedMoved = StringPattern{std::move(parsedOriginal)};

        REQUIRE(parsedCopy.matches("ab-middle-xy"_el));
        REQUIRE(parsedMoved.matches("ab-middle-xy"_el));

        auto typedOriginal = StringPattern{Text{U"ID"}, Range{U'0', U'9'}, Divider{}};
        const auto typedCopy = typedOriginal;
        const auto typedMoved = StringPattern{std::move(typedOriginal)};

        REQUIRE(typedCopy.matches("ID7-tail"_el));
        REQUIRE(typedMoved.matches("ID7-tail"_el));
        REQUIRE_FALSE(typedCopy.matches("IDA-tail"_el));
        REQUIRE_FALSE(typedMoved.matches("IDA-tail"_el));
    }

    void testFrontPatternOperations() {
        const auto pattern = StringPattern{"abc"_el};
        const auto text = U8String{"abcdef"_el};

        REQUIRE(pattern.matches(text));
        REQUIRE_EQUAL(pattern.trimmed(text), "def"_el);
        REQUIRE_EQUAL(pattern.index(text), ByteIndex{3U});
        REQUIRE_EQUAL(pattern.length(text), ByteLength{3U});

        const auto [matching, rest] = pattern.split(text);
        REQUIRE_EQUAL(matching, "abc"_el);
        REQUIRE_EQUAL(rest, "def"_el);
    }

    void testDividerOperations() {
        const auto front = StringPattern{"abc*"_el};
        const auto back = StringPattern{"*abc"_el};
        const auto both = StringPattern{"abc*xyz"_el};
        const auto text = U8String{"abcdefxyz"_el};
        const auto backText = U8String{"defabc"_el};

        REQUIRE_EQUAL(front.trimmed(text), "defxyz"_el);
        REQUIRE_EQUAL(front.index(text), ByteIndex{3U});
        REQUIRE_EQUAL(front.length(text), ByteLength{3U});

        REQUIRE_EQUAL(back.trimmed(backText), "def"_el);
        REQUIRE_EQUAL(back.index(backText), ByteIndex{3U});
        REQUIRE_EQUAL(back.length(backText), ByteLength{3U});

        REQUIRE_EQUAL(both.trimmed(text), "def"_el);
        REQUIRE_EQUAL(both.index(text), ByteIndex{3U});
        REQUIRE_EQUAL(both.length(text), ByteLength{3U});

        const auto [frontMatch, frontRest] = front.split(text);
        REQUIRE_EQUAL(frontMatch, "abc"_el);
        REQUIRE_EQUAL(frontRest, "defxyz"_el);

        const auto [backRest, backMatch] = back.split(backText);
        REQUIRE_EQUAL(backRest, "def"_el);
        REQUIRE_EQUAL(backMatch, "abc"_el);

        const auto [bothMatch, bothRest] = both.split(text);
        REQUIRE_EQUAL(bothMatch, "abc"_el);
        REQUIRE_EQUAL(bothRest, "def"_el);
        REQUIRE_FALSE(both.matches("abcxy"_el));
        REQUIRE(both.matches("abcxyz"_el));
        REQUIRE_EQUAL(both.trimmed("abcxyz"_el), ""_el);
    }

    void testNativeLengthsAcrossWidths() {
        const auto u8Pattern = StringPattern{u8"\u00E9?*"_el};
        const auto u8Text = U8String{u8"\u00E9x-rest"_el};
        REQUIRE(u8Pattern.matches(u8Text));
        REQUIRE_EQUAL(u8Pattern.index(u8Text), ByteIndex{3U});
        REQUIRE_EQUAL(u8Pattern.length(u8Text), ByteLength{3U});

        const auto u16Pattern = StringPattern{u"\U0001F600*"_el};
        const auto u16Text = U16String{u"\U0001F600trail"_el};
        REQUIRE(u16Pattern.matches(u16Text));
        REQUIRE_EQUAL(u16Pattern.index(u16Text), U16DataIndex{2U});
        REQUIRE_EQUAL(u16Pattern.length(u16Text), U16DataLength{2U});

        const auto u32Pattern = StringPattern{U"\u03BB*"_el};
        const auto u32Text = U32String{U"\u03BBtrail"_el};
        REQUIRE(u32Pattern.matches(u32Text));
        REQUIRE_EQUAL(u32Pattern.index(u32Text), CpIndex{1U});
        REQUIRE_EQUAL(u32Pattern.length(u32Text), CpLength{1U});
    }

    void testShortDividedPatternsDoNotOverlap() {
        const auto oneEachSide = StringPattern{"?*?"_el};
        REQUIRE_FALSE(oneEachSide.matches("a"_el));
        REQUIRE(oneEachSide.matches("ab"_el));

        const auto overlappingText = StringPattern{"ab*bc"_el};
        REQUIRE_FALSE(overlappingText.matches("abc"_el));
        REQUIRE(overlappingText.matches("abbc"_el));
        REQUIRE(overlappingText.matches("abxbc"_el));

        const auto u8Pattern = StringPattern{u8"\u00E9*\u00E9"_el};
        REQUIRE_FALSE(u8Pattern.matches(u8"\u00E9"_el));
        REQUIRE(u8Pattern.matches(u8"\u00E9\u00E9"_el));

        const auto u16Pattern = StringPattern{u"\U0001F600*\U0001F600"_el};
        REQUIRE_FALSE(u16Pattern.matches(u"\U0001F600"_el));
        REQUIRE(u16Pattern.matches(u"\U0001F600\U0001F600"_el));

        const auto u32Pattern = StringPattern{U"\u03BB*\u03BB"_el};
        REQUIRE_FALSE(u32Pattern.matches(U"\u03BB"_el));
        REQUIRE(u32Pattern.matches(U"\u03BB\u03BB"_el));
    }

    void testTrimMutableText() {
        const auto pattern = StringPattern{"abc*xyz"_el};

        auto view = U8String{"abcdefxyz"_el};
        REQUIRE(pattern.trim(view));
        REQUIRE_EQUAL(view, "def"_el);

        auto text = U8StringEditor{"abcdefxyz"_el};
        REQUIRE(pattern.trim(text));
        REQUIRE_EQUAL(text, "def"_el);

        auto unchanged = U8StringEditor{"abdefxyz"_el};
        REQUIRE_FALSE(pattern.trim(unchanged));
        REQUIRE_EQUAL(unchanged, "abdefxyz"_el);
    }
};
