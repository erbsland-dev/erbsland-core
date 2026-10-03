// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/ByteCount.hpp>
#include <erbsland/ByteIndex.hpp>
#include <erbsland/CpCount.hpp>
#include <erbsland/CpIndex.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/StdFormat.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringConverter.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringPattern.hpp>
#include <erbsland/U16Count.hpp>
#include <erbsland/U16Index.hpp>
#include <erbsland/U16String.hpp>
#include <erbsland/U16StringEditor.hpp>
#include <erbsland/U32String.hpp>
#include <erbsland/U32StringEditor.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <string>
#include <utility>

using namespace el::text::pattern;

using namespace el::text;
using namespace el::text::literals;
using namespace el::unit;

TESTED_TARGETS(StringPattern StringPatternKind)
class StringPatternTest final : public el::UnitTest {
    template <typename... Args>
    static constexpr bool cAcceptsGlob = requires(const Args &...elements) { StringPattern::fromGlob(elements...); };
    template <typename... Args>
    static constexpr bool cAcceptsDivider =
        requires(const Args &...elements) { StringPattern::fromDivider(elements...); };
    template <typename T>
    static constexpr bool cCanTrim = requires(const StringPattern &pattern, T &text) { pattern.trim(text); };

    static_assert(cAcceptsGlob<Text, Star, OneChar, Star, Range, Set>);
    static_assert(cAcceptsGlob<>);
    static_assert(!cAcceptsGlob<Divider>);
    static_assert(!cAcceptsGlob<Text, Star, Divider>);
    static_assert(cAcceptsDivider<Text, Divider, OneChar, Range, Set>);
    static_assert(!cAcceptsDivider<Star>);
    static_assert(!cAcceptsDivider<Text, Divider, Star>);
    static_assert(!cCanTrim<U8String> && !cCanTrim<U16String> && !cCanTrim<U32String>);
    static_assert(cCanTrim<U8StringEditor> && cCanTrim<U16StringEditor> && cCanTrim<U32StringEditor>);

    using Factory = StringPattern (*)(const AnyString &);

public:
    void testPatternKinds() {
        REQUIRE_EQUAL(StringPattern::fromDivider().kind(), StringPatternKind::Divider);
        REQUIRE_EQUAL(StringPattern::fromDivider(""_el).kind(), StringPatternKind::Divider);
        REQUIRE_EQUAL(StringPattern::fromDivider(u"a*"_el).kind(), StringPatternKind::Divider);
        REQUIRE_EQUAL(StringPattern::fromDivider(U"a*"_el).kind(), StringPatternKind::Divider);
        REQUIRE_EQUAL(StringPattern::fromDivider(Text{U"a"}, Divider{}).kind(), StringPatternKind::Divider);
        REQUIRE_EQUAL(StringPattern::fromGlob(""_el).kind(), StringPatternKind::Glob);
        REQUIRE_EQUAL(StringPattern::fromGlob(u"a*"_el).kind(), StringPatternKind::Glob);
        REQUIRE_EQUAL(StringPattern::fromGlob(U"a*"_el).kind(), StringPatternKind::Glob);
        auto original = StringPattern::fromGlob("a*b*"_el);
        const auto copied = original;
        const auto moved = std::move(original);
        REQUIRE_EQUAL(copied.kind(), StringPatternKind::Glob);
        REQUIRE_EQUAL(moved.kind(), StringPatternKind::Glob);
        REQUIRE_EQUAL(original.kind(), StringPatternKind::Divider);
        REQUIRE_FALSE(original.matches("ab"_el));
        auto assigned = StringPattern::fromDivider("prefix"_el);
        assigned = copied;
        REQUIRE_EQUAL(assigned.kind(), StringPatternKind::Glob);
        REQUIRE(assigned.matches("ab"_el));
    }

    void testGlobExtractionAcrossWidths() {
        for (const auto factory : std::array<Factory, 2>{StringPattern::fromGlob, StringPattern::fromPosixGlob}) {
            WITH_CONTEXT((verifyGlobExtraction<U8String, U8StringEditor>(
                "ä😀"_el, factory("ä*"_el), factory("z*"_el), factory(""_el))));
            WITH_CONTEXT((verifyGlobExtraction<U16String, U16StringEditor>(
                u"ä😀"_el, factory("ä*"_el), factory("z*"_el), factory(""_el))));
            WITH_CONTEXT((verifyGlobExtraction<U32String, U32StringEditor>(
                U"ä😀"_el, factory("ä*"_el), factory("z*"_el), factory(""_el))));
        }
    }

    void testDividerExtractionAcrossWidths() {
        WITH_CONTEXT((verifyDividerExtraction<U8String, U8StringEditor>("ä-middle-😀"_el, "ä"_el, "-middle-"_el)));
        WITH_CONTEXT((verifyDividerExtraction<U16String, U16StringEditor>(u"ä-middle-😀"_el, u"ä"_el, u"-middle-"_el)));
        WITH_CONTEXT((verifyDividerExtraction<U32String, U32StringEditor>(U"ä-middle-😀"_el, U"ä"_el, U"-middle-"_el)));
    }

    void testStaticGlobExtractionAcrossWidths() {
        const auto pattern = StringPattern::fromGlob(Text{U"ä"}, Star{});
        const auto mismatch = StringPattern::fromGlob(Text{U"z"}, Star{});
        const auto empty = StringPattern::fromGlob();
        WITH_CONTEXT((verifyGlobExtraction<U8String, U8StringEditor>("ä😀"_el, pattern, mismatch, empty)));
        WITH_CONTEXT((verifyGlobExtraction<U16String, U16StringEditor>(u"ä😀"_el, pattern, mismatch, empty)));
        WITH_CONTEXT((verifyGlobExtraction<U32String, U32StringEditor>(U"ä😀"_el, pattern, mismatch, empty)));
    }

    void testAnyStringFactories() {
        for (const auto &source : std::array<AnyString, 3>{"ä*😀"_el, u"ä*😀"_el, U"ä*😀"_el}) {
            for (
                const auto factory : std::array<Factory, 3>{
                    StringPattern::fromDivider, StringPattern::fromGlob, StringPattern::fromPosixGlob}) {
                const auto pattern = factory(source);
                REQUIRE(pattern.matches("ä/😀"_el));
                REQUIRE(pattern.matches(u"ä/😀"_el));
                REQUIRE(pattern.matches(U"ä/😀"_el));
                REQUIRE_FALSE(pattern.matches("ä/x"_el));
            }
        }
        REQUIRE_FALSE(StringPattern::fromDivider(AnyString{}).matches(""_el));
        REQUIRE(StringPattern::fromGlob(AnyString{}).matches(""_el));
        REQUIRE(StringPattern::fromPosixGlob(AnyString{}).matches(""_el));
    }

    void testGlobDialectDifferences() {
        REQUIRE(StringPattern::fromGlob("[^a-c]*"_el).matches("z/path"_el));
        REQUIRE_FALSE(StringPattern::fromGlob("[^a-c]*"_el).matches("b/path"_el));
        REQUIRE(StringPattern::fromGlob("[!a-c]"_el).matches("!"_el));
        REQUIRE_FALSE(StringPattern::fromGlob("[!a-c]"_el).matches("z"_el));
        REQUIRE(StringPattern::fromGlob(R"(file\[\?\]\*\\)"_el).matches(R"(file[?]*\)"_el));
        REQUIRE(StringPattern::fromGlob("[^]"_el).matches("😀"_el));
        REQUIRE_FALSE(StringPattern::fromGlob("[^^]"_el).matches("^"_el));
        REQUIRE(StringPattern::fromGlob("[a^]"_el).matches("^"_el));
        REQUIRE(StringPattern::fromGlob("[ä-ö]"_el).matches("é"_el));
        REQUIRE(StringPattern::fromGlob("[-a]"_el).matches("-"_el));
        REQUIRE(StringPattern::fromGlob("[a-]"_el).matches("-"_el));
        for (
            const auto &invalid :
            {"a["_el, "[]"_el, "[z-a]"_el, "[a-a]"_el, "a]"_el, R"(a\q)"_el, R"(a\)"_el, R"([a\q])"_el}) {
            REQUIRE_THROWS(StringPattern::fromGlob(invalid));
            REQUIRE_THROWS(StringPattern::fromDivider(invalid));
        }
    }

    void testStaticGlobs() {
        const auto typed = StringPattern::fromGlob(
            Text{U"ä"}, Star{}, Text{U"ID"}, Range{U'0', U'9'}, Star{}, Set{{U'a', U'c'}}, OneChar{}, Text{U"😀"});
        const auto parsed = StringPattern::fromGlob("ä*ID[0-9]*[a-c]?😀"_el);
        REQUIRE_EQUAL(typed.kind(), StringPatternKind::Glob);
        for (
            const auto &text :
            {"äID3b!😀"_el, "ä/ID3/more/c?😀"_el, "äIDxc?😀"_el, "äID3d?😀"_el, "xäID3b!😀"_el, "äID3b!😀x"_el}) {
            REQUIRE_EQUAL(typed.matches(text), parsed.matches(text));
        }
        REQUIRE(typed.matches(u"ä/ID3/more/c?😀"_el));
        REQUIRE(typed.matches(U"ä/ID3/more/c?😀"_el));
        REQUIRE(typed.trimmed("äID3b!😀"_el).isEmpty());
        REQUIRE(StringPattern::fromGlob().matches(""_el));
        REQUIRE_FALSE(StringPattern::fromGlob().matches("x"_el));
        REQUIRE(StringPattern::fromGlob(Star{}).matches(""_el));
        REQUIRE(StringPattern::fromGlob(Star{}, Star{}).matches(".hidden/path\nnext"_el));
        REQUIRE(StringPattern::fromGlob(Text{U"*?[]\\"}).matches(R"(*?[]\)"_el));
        REQUIRE_FALSE(StringPattern::fromGlob(Text{U"abc"}).matches("abcdef"_el));
        REQUIRE_THROWS(StringPattern::fromGlob(Set{}));

        const auto maximum = StringPattern::fromGlob(Text{U"abcdefghijklmno"}, Star{}, Star{});
        REQUIRE(maximum.matches("abcdefghijklmnopq"_el));
        REQUIRE_THROWS(StringPattern::fromGlob(Text{U"abcdefghijklmnop"}, Star{}));
        REQUIRE_THROWS(StringPattern::fromGlob(Text{U"abcdefghijklmnopq"}));
        const auto set =
            Set{{U'a', U'b'},
                {U'c', U'd'},
                {U'e', U'f'},
                {U'g', U'h'},
                {U'i', U'j'},
                {U'k', U'l'},
                {U'm', U'n'},
                {U'o', U'p'}};
        REQUIRE(StringPattern::fromGlob(set, Star{}, set).matches("a/b"_el));
        REQUIRE_THROWS(StringPattern::fromGlob(set, set, Range{U'a', U'z'}));
    }

    void testPosixGlobKindsAndCopies() {
        for (const auto &source : std::array<AnyString, 3>{"a*b*"_el, u"a*b*"_el, U"a*b*"_el}) {
            auto original = StringPattern::fromPosixGlob(source);
            const auto copied = original;
            const auto moved = std::move(original);
            REQUIRE_EQUAL(copied.kind(), StringPatternKind::PosixGlob);
            REQUIRE_EQUAL(moved.kind(), StringPatternKind::PosixGlob);
            REQUIRE_EQUAL(original.kind(), StringPatternKind::Divider);
            REQUIRE_FALSE(original.matches("ab"_el));
            auto assigned = StringPattern::fromDivider();
            assigned = copied;
            REQUIRE_EQUAL(assigned.kind(), StringPatternKind::PosixGlob);
            REQUIRE(assigned.matches("ab"_el));
            assigned = StringPattern::fromGlob(Text{U"a"}, Star{}, Text{U"b"}, Star{});
            REQUIRE_EQUAL(assigned.kind(), StringPatternKind::Glob);
            REQUIRE(assigned.matches("ab"_el));
        }
    }

    void testFullStringGlobs() {
        for (const auto factory : std::array<Factory, 2>{StringPattern::fromGlob, StringPattern::fromPosixGlob}) {
            const auto pattern = factory("src/*/file?.[ch]pp"_el);
            REQUIRE(pattern.matches("src/a/b/file1.hpp"_el));
            REQUIRE(pattern.matches(u"src/ä/file😀.cpp"_el));
            REQUIRE(pattern.matches(U"src/ä/file😀.cpp"_el));
            REQUIRE_FALSE(pattern.matches("src/a/file11.hpp"_el));
            REQUIRE_FALSE(pattern.matches("src/a/file1.hpp.bak"_el));
            REQUIRE_FALSE(pattern.matches("other/src/a/file1.hpp"_el));
            REQUIRE(factory("*a*b*c*"_el).matches("aaabbbccc"_el));
            REQUIRE_FALSE(factory("*a*b*c"_el).matches("acb"_el));
            REQUIRE(factory("**"_el).matches(""_el));
            REQUIRE(factory("*"_el).matches(".hidden/path\nnext"_el));
            REQUIRE(factory(""_el).matches(""_el));
            REQUIRE_FALSE(factory(""_el).matches("a"_el));
            REQUIRE_FALSE(factory("A"_el).matches("a"_el));
            REQUIRE_FALSE(factory("?"_el).matches(""_el));
            REQUIRE(factory(u"ä*😀"_el).matches("ä/😀"_el));
            REQUIRE(factory(U"ä*😀"_el).matches(u"ä/😀"_el));
        }
    }
    void testPosixGlobBracketClassesAndLiterals() {
        REQUIRE(StringPattern::fromPosixGlob("[!a-c]*"_el).matches("z/path"_el));
        REQUIRE_FALSE(StringPattern::fromPosixGlob("[!a-c]*"_el).matches("b/path"_el));
        REQUIRE(StringPattern::fromPosixGlob("[[]"_el).matches("["_el));
        REQUIRE(StringPattern::fromPosixGlob("[]]"_el).matches("]"_el));
        REQUIRE(StringPattern::fromPosixGlob("[!]]"_el).matches("["_el));
        REQUIRE_FALSE(StringPattern::fromPosixGlob("[!]]"_el).matches("]"_el));
        REQUIRE(StringPattern::fromPosixGlob("x["_el).matches("x["_el));
        REQUIRE(StringPattern::fromPosixGlob("[a*?"_el).matches("[abcd"_el));
        REQUIRE(StringPattern::fromPosixGlob("[!a?"_el).matches("[!ab"_el));
        REQUIRE(StringPattern::fromPosixGlob("[a-b][c-]"_el).matches("a-"_el));
        REQUIRE(StringPattern::fromPosixGlob("[a-b][c-]"_el).matches("bc"_el));
        REQUIRE_FALSE(StringPattern::fromPosixGlob("[a-b][c-]"_el).matches("bd"_el));
        REQUIRE(StringPattern::fromPosixGlob("[]"_el).matches("[]"_el));
        REQUIRE(StringPattern::fromPosixGlob("[!]"_el).matches("[!]"_el));
        REQUIRE(StringPattern::fromPosixGlob("[-a]"_el).matches("-"_el));
        REQUIRE(StringPattern::fromPosixGlob("[a-]"_el).matches("-"_el));
        REQUIRE_FALSE(StringPattern::fromPosixGlob("[z-a]"_el).matches("z"_el));
        REQUIRE(StringPattern::fromPosixGlob("[!z-a]"_el).matches("z"_el));
        REQUIRE(StringPattern::fromPosixGlob("[^a]"_el).matches("^"_el));
        REQUIRE_FALSE(StringPattern::fromPosixGlob("[^a]"_el).matches("b"_el));
        REQUIRE(StringPattern::fromPosixGlob("[ä-ö]"_el).matches("é"_el));
        REQUIRE(StringPattern::fromPosixGlob(R"(a\b)"_el).matches(R"(a\b)"_el));
    }
    void testGlobMatchingBoundaries() {
        const auto pattern = StringPattern::fromGlob("*ä?"_el);
        const auto text = "xä😀"_el;
        REQUIRE_EQUAL(pattern.length(text), text.length());
        REQUIRE_EQUAL(pattern.index(text), text.indexAt(StringSide::Back));
        REQUIRE(pattern.trimmed(text).isEmpty());
        const auto [matched, remainder] = pattern.split(text);
        REQUIRE_EQUAL(matched, text);
        REQUIRE(remainder.isEmpty());
        auto editable = U16StringEditor{u"xä😀"_el};
        REQUIRE(pattern.trim(editable));
        REQUIRE(editable.isEmpty());
        REQUIRE_EQUAL(pattern.trimmed("no match"_el), "no match"_el);
        // The Divider factory matches prefixes; only the glob factory anchors both ends.
        REQUIRE(StringPattern::fromDivider("abc"_el).matches("abcdef"_el));
        REQUIRE_FALSE(StringPattern::fromGlob("abc"_el).matches("abcdef"_el));
    }
    void testEmptyDividerNeverMatches() {
        const auto pattern = StringPattern::fromDivider();

        REQUIRE_FALSE(pattern.matches("anything"_el));
        REQUIRE_EQUAL(pattern.trimmed("anything"_el), "anything"_el);
        REQUIRE_EQUAL(pattern.index("anything"_el), ByteIndex::noIndex());
        REQUIRE_EQUAL(pattern.length("anything"_el), ByteCount::zero());

        const auto [matching, rest] = pattern.split("anything"_el);
        REQUIRE(matching.isEmpty());
        REQUIRE_EQUAL(rest, "anything"_el);
    }

    void testParsedSyntax() {
        const auto pattern = StringPattern::fromDivider("te??[a-z]*"_el);

        REQUIRE(pattern.matches("textm-tail"_el));
        REQUIRE(pattern.matches(u"texza-suffix"_el));
        REQUIRE(pattern.matches(U"testq"_el));
        REQUIRE_FALSE(pattern.matches("tex1M-tail"_el));
        REQUIRE_FALSE(pattern.matches("textM-tail"_el));

        const auto escaped = StringPattern::fromDivider("file\\[\\?\\]\\*"_el);
        REQUIRE(escaped.matches("file[?]*.txt"_el));
        REQUIRE_FALSE(escaped.matches("fileA.txt"_el));
    }

    void testNegatedBracketSets() {
        const auto pattern = StringPattern::fromDivider("[^a-z]"_el);
        REQUIRE(pattern.matches("7"_el));
        REQUIRE(pattern.matches(u"\u00e4"_el));
        REQUIRE(pattern.matches(U"\U0001F600"_el));
        REQUIRE_FALSE(pattern.matches("a"_el));
        REQUIRE_FALSE(pattern.matches("z"_el));
        REQUIRE(StringPattern::fromDivider(u"[^a-z]"_el).matches("7"_el));
        REQUIRE(StringPattern::fromDivider(U"[^a-z]"_el).matches("7"_el));
        REQUIRE(StringPattern::fromDivider("[a^]"_el).matches("^"_el));
        REQUIRE_FALSE(StringPattern::fromDivider("[^^]"_el).matches("^"_el));
        REQUIRE(StringPattern::fromDivider("[^]"_el).matches("^"_el));
        REQUIRE_FALSE(StringPattern::fromDivider("[^-a-z]"_el).matches("-"_el));
        REQUIRE_THROWS(StringPattern::fromDivider("[^z-a]"_el));
    }

    void testInvalidParsedSyntax() {
        REQUIRE_THROWS(StringPattern::fromDivider("*"_el));
        REQUIRE_THROWS(StringPattern::fromDivider("a*b*c"_el));
        REQUIRE_THROWS(StringPattern::fromDivider("a["_el));
        REQUIRE_THROWS(StringPattern::fromDivider("[]"_el));
        REQUIRE_THROWS(StringPattern::fromDivider("[z-a]"_el));
        REQUIRE_THROWS(StringPattern::fromDivider("a\\q"_el));
        REQUIRE_THROWS(StringPattern::fromDivider("a]"_el));
    }

    void testStaticConstruction() {

        const auto prefix = StringPattern::fromDivider(Text{U"te"}, OneChar{}, Range{U'a', U'z'}, Divider{});
        const auto setPattern = StringPattern::fromDivider(Text{U"ID"}, Set{{U'a', U'z'}, {U'A', U'Z'}}, OneChar{});
        const auto u8MatchPattern = StringPattern::fromDivider(Text{U"\u00E9"}, Divider{});
        const auto u16MatchPattern = StringPattern::fromDivider(Text{U"\U0001F600"}, Divider{});

        REQUIRE(prefix.matches("text-more"_el));
        REQUIRE_FALSE(prefix.matches("te9T-more"_el));
        REQUIRE(setPattern.matches("IDa7"_el));
        REQUIRE(setPattern.matches("IDZ_"_el));
        REQUIRE_FALSE(setPattern.matches("ID17"_el));
        REQUIRE(u8MatchPattern.matches(u8"\u00E9clair"_el));
        REQUIRE(u16MatchPattern.matches(u"\U0001F600 trail"_el));
    }

    void testStaticConstructionLimits() {

        const auto maximumElements = StringPattern::fromDivider(Text{U"abcdefghijklmnop"});
        REQUIRE(maximumElements.matches("abcdefghijklmnop-tail"_el));
        REQUIRE_THROWS(StringPattern::fromDivider(Text{U"abcdefghijklmnopq"}));

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
        const auto maximumRanges = StringPattern::fromDivider(firstRanges, secondRanges);
        REQUIRE(maximumRanges.matches("ai"_el));
        REQUIRE(maximumRanges.matches("hp"_el));
        REQUIRE_FALSE(maximumRanges.matches("qq"_el));
        REQUIRE_THROWS(StringPattern::fromDivider(firstRanges, secondRanges, Range{U'q', U'q'}));
    }

    void testParsedPatternsUseDynamicStorage() {
        const auto pattern = StringPattern::fromDivider("abcdefghijklmnopq"_el);

        REQUIRE(pattern.matches("abcdefghijklmnopq-tail"_el));
        REQUIRE_EQUAL(pattern.index("abcdefghijklmnopq-tail"_el), ByteIndex{17U});
    }

    void testCopiesShareImmutablePatternBehavior() {
        auto parsedOriginal = StringPattern::fromDivider("ab*xy"_el);
        const auto parsedCopy = parsedOriginal;
        const auto parsedMoved = StringPattern{std::move(parsedOriginal)};

        REQUIRE(parsedCopy.matches("ab-middle-xy"_el));
        REQUIRE(parsedMoved.matches("ab-middle-xy"_el));

        auto typedOriginal = StringPattern::fromDivider(Text{U"ID"}, Range{U'0', U'9'}, Divider{});
        const auto typedCopy = typedOriginal;
        const auto typedMoved = StringPattern{std::move(typedOriginal)};

        REQUIRE(typedCopy.matches("ID7-tail"_el));
        REQUIRE(typedMoved.matches("ID7-tail"_el));
        REQUIRE_FALSE(typedCopy.matches("IDA-tail"_el));
        REQUIRE_FALSE(typedMoved.matches("IDA-tail"_el));
    }

    void testFrontPatternOperations() {
        const auto pattern = StringPattern::fromDivider("abc"_el);
        const auto text = String{"abcdef"_el};

        REQUIRE(pattern.matches(text));
        REQUIRE_EQUAL(pattern.trimmed(text), "def"_el);
        REQUIRE_EQUAL(pattern.index(text), ByteIndex{3U});
        REQUIRE_EQUAL(pattern.length(text), ByteCount{3U});

        const auto [matching, rest] = pattern.split(text);
        REQUIRE_EQUAL(matching, "abc"_el);
        REQUIRE_EQUAL(rest, "def"_el);
    }

    void testDividerOperations() {
        const auto front = StringPattern::fromDivider("abc*"_el);
        const auto back = StringPattern::fromDivider("*abc"_el);
        const auto both = StringPattern::fromDivider("abc*xyz"_el);
        const auto text = String{"abcdefxyz"_el};
        const auto backText = String{"defabc"_el};

        REQUIRE_EQUAL(front.trimmed(text), "defxyz"_el);
        REQUIRE_EQUAL(front.index(text), ByteIndex{3U});
        REQUIRE_EQUAL(front.length(text), ByteCount{3U});

        REQUIRE_EQUAL(back.trimmed(backText), "def"_el);
        REQUIRE_EQUAL(back.index(backText), ByteIndex{3U});
        REQUIRE_EQUAL(back.length(backText), ByteCount{3U});

        REQUIRE_EQUAL(both.trimmed(text), "def"_el);
        REQUIRE_EQUAL(both.index(text), ByteIndex{3U});
        REQUIRE_EQUAL(both.length(text), ByteCount{3U});

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
        const auto u8Pattern = StringPattern::fromDivider(u8"\u00E9?*"_el);
        const auto u8Text = String{u8"\u00E9x-rest"_el};
        REQUIRE(u8Pattern.matches(u8Text));
        REQUIRE_EQUAL(u8Pattern.index(u8Text), ByteIndex{3U});
        REQUIRE_EQUAL(u8Pattern.length(u8Text), ByteCount{3U});

        const auto u16Pattern = StringPattern::fromDivider(u"\U0001F600*"_el);
        const auto u16Text = U16String{u"\U0001F600trail"_el};
        REQUIRE(u16Pattern.matches(u16Text));
        REQUIRE_EQUAL(u16Pattern.index(u16Text), U16Index{2U});
        REQUIRE_EQUAL(u16Pattern.length(u16Text), U16Count{2U});

        const auto u32Pattern = StringPattern::fromDivider(U"\u03BB*"_el);
        const auto u32Text = U32String{U"\u03BBtrail"_el};
        REQUIRE(u32Pattern.matches(u32Text));
        REQUIRE_EQUAL(u32Pattern.index(u32Text), CpIndex{1U});
        REQUIRE_EQUAL(u32Pattern.length(u32Text), CpCount{1U});
    }

    void testShortDividedPatternsDoNotOverlap() {
        const auto oneEachSide = StringPattern::fromDivider("?*?"_el);
        REQUIRE_FALSE(oneEachSide.matches("a"_el));
        REQUIRE(oneEachSide.matches("ab"_el));

        const auto overlappingText = StringPattern::fromDivider("ab*bc"_el);
        REQUIRE_FALSE(overlappingText.matches("abc"_el));
        REQUIRE(overlappingText.matches("abbc"_el));
        REQUIRE(overlappingText.matches("abxbc"_el));

        const auto u8Pattern = StringPattern::fromDivider(u8"\u00E9*\u00E9"_el);
        REQUIRE_FALSE(u8Pattern.matches(u8"\u00E9"_el));
        REQUIRE(u8Pattern.matches(u8"\u00E9\u00E9"_el));

        const auto u16Pattern = StringPattern::fromDivider(u"\U0001F600*\U0001F600"_el);
        REQUIRE_FALSE(u16Pattern.matches(u"\U0001F600"_el));
        REQUIRE(u16Pattern.matches(u"\U0001F600\U0001F600"_el));

        const auto u32Pattern = StringPattern::fromDivider(U"\u03BB*\u03BB"_el);
        REQUIRE_FALSE(u32Pattern.matches(U"\u03BB"_el));
        REQUIRE(u32Pattern.matches(U"\u03BB\u03BB"_el));
    }

    void testTrimMutableText() {
        const auto pattern = StringPattern::fromDivider("abc*xyz"_el);

        auto text = StringEditor{"abcdefxyz"_el};
        REQUIRE(pattern.trim(text));
        REQUIRE_EQUAL(text, "def"_el);

        auto unchanged = StringEditor{"abdefxyz"_el};
        REQUIRE_FALSE(pattern.trim(unchanged));
        REQUIRE_EQUAL(unchanged, "abdefxyz"_el);
    }

private:
    template <typename tString, typename tEditor>
    void verifyDividerExtraction(const tString &text, const tString &prefix, const tString &middle) {
        using Index = decltype(prefix.indexAt(StringSide::Front));
        using Length = decltype(prefix.length());
        const auto pattern = StringPattern::fromDivider("ä*😀"_el);
        REQUIRE(pattern.matches(text));
        REQUIRE_EQUAL(pattern.length(text), prefix.length());
        REQUIRE_EQUAL(pattern.index(text), prefix.indexAt(StringSide::Back));
        REQUIRE_EQUAL(pattern.trimmed(text), middle);
        const auto [matched, remainder] = pattern.split(text);
        REQUIRE_EQUAL(matched, prefix);
        REQUIRE_EQUAL(remainder, middle);
        auto editor = tEditor{text};
        REQUIRE(pattern.trim(editor));
        REQUIRE_EQUAL(tString{editor}, middle);
        const auto mismatch = prefix;
        editor = tEditor{mismatch};
        REQUIRE_FALSE(pattern.matches(mismatch));
        REQUIRE_FALSE(pattern.trim(editor));
        REQUIRE_EQUAL(tString{editor}, mismatch);
        REQUIRE_EQUAL(pattern.trimmed(mismatch), mismatch);
        REQUIRE_EQUAL(pattern.length(mismatch), Length::zero());
        REQUIRE_EQUAL(pattern.index(mismatch), Index::noIndex());
        const auto [unmatched, original] = pattern.split(mismatch);
        REQUIRE(unmatched.isEmpty());
        REQUIRE_EQUAL(original, mismatch);
        REQUIRE_FALSE(pattern.matches(tString{}));
        REQUIRE_FALSE(StringPattern::fromDivider().matches(tString{}));
        REQUIRE_FALSE(StringPattern::fromDivider().matches(text));
    }

    template <typename tString, typename tEditor>
    void verifyGlobExtraction(
        const tString &text,
        const StringPattern &pattern,
        const StringPattern &mismatch,
        const StringPattern &emptyGlob) {
        using Index = decltype(text.indexAt(StringSide::Front));
        using Length = decltype(text.length());
        REQUIRE(pattern.matches(text));
        REQUIRE_EQUAL(pattern.length(text), text.length());
        REQUIRE_EQUAL(pattern.index(text), text.indexAt(StringSide::Back));
        REQUIRE(pattern.trimmed(text).isEmpty());
        const auto [matched, remainder] = pattern.split(text);
        REQUIRE_EQUAL(matched, text);
        REQUIRE(remainder.isEmpty());
        auto editor = tEditor{text};
        REQUIRE(pattern.trim(editor));
        REQUIRE(editor.isEmpty());

        editor = tEditor{text};
        REQUIRE_FALSE(mismatch.matches(text));
        REQUIRE_FALSE(mismatch.trim(editor));
        REQUIRE_EQUAL(tString{editor}, text);
        REQUIRE_EQUAL(mismatch.trimmed(text), text);
        REQUIRE_EQUAL(mismatch.length(text), Length::zero());
        REQUIRE_EQUAL(mismatch.index(text), Index::noIndex());
        const auto [unmatched, original] = mismatch.split(text);
        REQUIRE(unmatched.isEmpty());
        REQUIRE_EQUAL(original, text);

        auto empty = tString{};
        auto emptyEditor = tEditor{};
        REQUIRE(emptyGlob.matches(empty));
        REQUIRE_EQUAL(emptyGlob.length(empty), Length::zero());
        REQUIRE_EQUAL(emptyGlob.index(empty), empty.indexAt(StringSide::Back));
        REQUIRE_FALSE(emptyGlob.trim(emptyEditor));
        REQUIRE(emptyGlob.trimmed(empty).isEmpty());
        const auto [emptyMatch, emptyRest] = emptyGlob.split(empty);
        REQUIRE(emptyMatch.isEmpty());
        REQUIRE(emptyRest.isEmpty());
        REQUIRE_FALSE(StringPattern::fromDivider().matches(empty));
    }
};
