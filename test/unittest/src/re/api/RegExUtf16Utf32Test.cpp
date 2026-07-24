// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../StringHelper.hpp"
#include "../TestHelper.hpp"

#include <erbsland/re/Match.hpp>
#include <erbsland/re/Match16.hpp>
#include <erbsland/re/Match32.hpp>
#include <erbsland/re/RegEx.hpp>
#include <erbsland/re/StdFormat.hpp>
#include <erbsland/text/u16/U16String.hpp>
#include <erbsland/text/u16/U16StringEditor.hpp>
#include <erbsland/text/u32/U32String.hpp>
#include <erbsland/text/u32/U32StringEditor.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::re;

TESTED_TARGETS(RegEx)
TAGS(Api Unicode)
class RegExUtf16Utf32Test final : public UNITTEST_SUBCLASS(re_test::TestHelper) {
public:
    void testCompilePatternsInAllStringWidths() {
        const auto regex8 = RegEx::compile("(A)(😀)(B)"_el);
        const auto regex16 = RegEx::compile(u"(A)(😀)(B)"_el);
        const auto regex32 = RegEx::compile(U"(A)(😀)(B)"_el);

        REQUIRE(regex8->fullMatch("A😀B"_el) != nullptr);
        REQUIRE(regex16->fullMatch("A😀B"_el) != nullptr);
        REQUIRE(regex32->fullMatch("A😀B"_el) != nullptr);
        REQUIRE_EQUAL(regex16->fullMatch("A😀B"_el)->content(2), "😀"_el);
        REQUIRE_EQUAL(regex32->fullMatch("A😀B"_el)->content(2), "😀"_el);
        REQUIRE_EQUAL(regex8->pattern().toString(), "(A)(😀)(B)"_el);
        REQUIRE_EQUAL(regex16->pattern().toString(), "(A)(😀)(B)"_el);
        REQUIRE_EQUAL(regex32->pattern().toString(), "(A)(😀)(B)"_el);
    }

    void testAllMatchingOperationsUtf16() {
        const auto re = RegEx::compile("😀"_el);
        const auto text = el::text::U16String{u"😀x😀"_el};

        REQUIRE(re->match(text) != nullptr);
        REQUIRE(re->fullMatch(el::text::U16String{u"😀"_el}) != nullptr);
        REQUIRE_EQUAL(re->findFirst(el::text::U16String{u"x😀"_el})->content(), u"😀"_el);

        auto generatedCount = std::size_t{};
        for (const auto &match : re->findAll(text)) {
            REQUIRE_EQUAL(match->content(), u"😀"_el);
            ++generatedCount;
        }
        REQUIRE_EQUAL(generatedCount, 2U);
        REQUIRE_EQUAL(re->collectAll(text).size(), 2U);
    }

    void testAllMatchingOperationsUtf32() {
        const auto re = RegEx::compile("😀"_el);
        const auto text = el::text::U32String{U"😀x😀"_el};

        REQUIRE(re->match(text) != nullptr);
        REQUIRE(re->fullMatch(el::text::U32String{U"😀"_el}) != nullptr);
        REQUIRE_EQUAL(re->findFirst(el::text::U32String{U"x😀"_el})->content(), U"😀"_el);

        auto generatedCount = std::size_t{};
        for (const auto &match : re->findAll(text)) {
            REQUIRE_EQUAL(match->content(), U"😀"_el);
            ++generatedCount;
        }
        REQUIRE_EQUAL(generatedCount, 2U);
        REQUIRE_EQUAL(re->collectAll(text).size(), 2U);
    }

    void testFullMatchUtf16AndGroups() {
        const auto re = RegEx::compile("(A)(😀)(B)"_el);
        const auto text = el::text::U16String{u"A\U0001F600B"_el};

        const auto m = re->fullMatch(text);
        REQUIRE(m != nullptr);
        REQUIRE_EQUAL(m->begin(), 0U);
        REQUIRE_EQUAL(m->end(), 4U); // A + surrogate pair + B

        REQUIRE(m->content() == u"A\U0001F600B"_el);
        REQUIRE(m->content(1) == u"A"_el);
        REQUIRE(m->content(2) == u"\U0001F600"_el);
        REQUIRE(m->content(3) == u"B"_el);
    }

    void testFullMatchUtf32AndGroups() {
        const auto re = RegEx::compile("(A)(😀)(B)"_el);
        const auto text = el::text::U32String{U"A\U0001F600B"_el};

        const auto m = re->fullMatch(text);
        REQUIRE(m != nullptr);
        REQUIRE_EQUAL(m->begin(), 0U);
        REQUIRE_EQUAL(m->end(), 3U);

        REQUIRE(m->content() == U"A\U0001F600B"_el);
        REQUIRE(m->content(1) == U"A"_el);
        REQUIRE(m->content(2) == U"\U0001F600"_el);
        REQUIRE(m->content(3) == U"B"_el);
    }

    void testFindAllUtf16ViewEmojiTwice() {
        const auto re = RegEx::compile("😀"_el);
        const auto text = el::text::U16String{u"\U0001F600\U0001F600"_el};

        const auto matches = re->collectAll(text);
        REQUIRE_EQUAL(matches.size(), 2U);

        REQUIRE_EQUAL(matches[0]->begin(), 0U);
        REQUIRE_EQUAL(matches[0]->end(), 2U);
        REQUIRE_EQUAL(matches[1]->begin(), 2U);
        REQUIRE_EQUAL(matches[1]->end(), 4U);
    }

    void testFindAllUtf32ViewEmojiTwice() {
        const auto re = RegEx::compile("😀"_el);
        const auto text = el::text::U32String{U"\U0001F600\U0001F600"_el};

        const auto matches = re->collectAll(text);
        REQUIRE_EQUAL(matches.size(), 2U);

        REQUIRE_EQUAL(matches[0]->begin(), 0U);
        REQUIRE_EQUAL(matches[0]->end(), 1U);
        REQUIRE_EQUAL(matches[1]->begin(), 1U);
        REQUIRE_EQUAL(matches[1]->end(), 2U);
    }

    void testUtf8MatchAndGeneratorOwnTemporarySubject() {
        const auto re = RegEx::compile("😀"_el);
        const auto match = re->findFirst(String{StringEditor{"x😀y"_el}});
        REQUIRE(match != nullptr);
        REQUIRE_EQUAL(match->content(), "😀"_el);

        auto generator = re->findAll(String{StringEditor{"😀x😀"_el}});
        auto count = std::size_t{};
        for (const auto &generatedMatch : generator) {
            REQUIRE_EQUAL(generatedMatch->content(), "😀"_el);
            ++count;
        }
        REQUIRE_EQUAL(count, 2U);
    }

    void testUtf16AndUtf32MatchesOwnTemporarySubjects() {
        const auto re = RegEx::compile("😀"_el);
        const auto match16 = re->findFirst(el::text::U16String{el::text::U16StringEditor{u"x😀y"_el}});
        const auto match32 = re->findFirst(el::text::U32String{el::text::U32StringEditor{U"x😀y"_el}});

        REQUIRE(match16 != nullptr);
        REQUIRE(match32 != nullptr);
        REQUIRE_EQUAL(match16->content(), u"😀"_el);
        REQUIRE_EQUAL(match32->content(), U"😀"_el);
    }

    void testMalformedUtf8SubjectBecomesReplacementIncludingCrlfPeek() {
        const auto re = RegEx::compile(el::text::U32String{U"\r�"_el}, Flags{Flag::CRLF});
        const auto malformed = re_test::string_helper::bytesToString({'\r', 0xFFU});
        REQUIRE(re->fullMatch(String{malformed}) != nullptr);
    }

    void testMalformedUtf16SubjectBecomesReplacement() {
        const auto re = RegEx::compile(el::text::U32String{U"a�"_el});
        REQUIRE(re->fullMatch(el::text::U16String{u"a\xD800"_el}) != nullptr);
    }

    void testMalformedUtf32SubjectBecomesReplacement() {
        const auto re = RegEx::compile(el::text::U32String{U"a�"_el});
        REQUIRE(re->fullMatch(el::text::U32String{U"a\xD800"_el}) != nullptr);
    }

    void testMalformedPatternBecomesReplacement() {
        const auto malformed = re_test::string_helper::bytesToString({'a', 0xFFU});
        const auto re = RegEx::compile(String{malformed});
        REQUIRE(re->fullMatch(el::text::U32String{U"a�"_el}) != nullptr);
        REQUIRE_EQUAL(re->pattern(), "a�"_el);
    }

    void testMalformedUtf16AndUtf32PatternsBecomeReplacement() {
        const auto regex16 = RegEx::compile(el::text::U16String{u"a\xD800"_el});
        const auto regex32 = RegEx::compile(el::text::U32String{U"a\xD800"_el});

        REQUIRE(regex16->fullMatch(el::text::U32String{U"a�"_el}) != nullptr);
        REQUIRE(regex32->fullMatch(el::text::U32String{U"a�"_el}) != nullptr);
        REQUIRE_EQUAL(regex16->pattern(), "a�"_el);
        REQUIRE_EQUAL(regex32->pattern(), "a�"_el);
    }
};
