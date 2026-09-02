// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/Char.hpp>
#include <erbsland/text/fuzzy/Matcher.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringList.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::text::literals;

TESTED_TARGETS(text::fuzzy::Match text::fuzzy::Matcher)
class FuzzyMatcherTest final : public el::UnitTest {
public:
    void testEditOperationsAndScores() {
        const auto matches =
            el::text::fuzzy::Matcher{"config"_el}
                .setMaximumDistance(el::unit::CpLength{2U})
                .findMatches(el::text::StringList{"confic"_el, "config"_el, "confg"_el, "conf"_el, "other"_el});

        REQUIRE_EQUAL(matches.count(), el::unit::ItemCount{4U});
        REQUIRE(matches.get(el::unit::ItemIndex{0U}).text() == "config"_el);
        REQUIRE_EQUAL(matches.get(el::unit::ItemIndex{0U}).distance(), el::unit::CpLength::zero());
        REQUIRE(matches.get(el::unit::ItemIndex{1U}).text() == "confic"_el);
        REQUIRE_EQUAL(matches.get(el::unit::ItemIndex{1U}).distance(), el::unit::CpLength::one());
        REQUIRE(matches.get(el::unit::ItemIndex{2U}).text() == "confg"_el);
        REQUIRE(matches.get(el::unit::ItemIndex{3U}).text() == "conf"_el);
    }

    void testTranspositionAndUnicodeCodePoints() {
        auto matches = el::text::fuzzy::Matcher{"server"_el}
                           .setMaximumDistance(el::unit::CpLength::one())
                           .findMatches(el::text::StringList{"sevrer"_el});
        REQUIRE_EQUAL(matches.count(), el::unit::ItemCount::one());
        REQUIRE_EQUAL(matches.first().distance(), el::unit::CpLength::one());

        matches = el::text::fuzzy::Matcher{u8"grüße"_el}
                      .setMaximumDistance(el::unit::CpLength::one())
                      .findMatches(el::text::StringList{u8"grüβe"_el});
        REQUIRE_EQUAL(matches.count(), el::unit::ItemCount::one());
        REQUIRE_EQUAL(matches.first().distance(), el::unit::CpLength::one());
    }

    void testComparisonDeduplicationAndLimits() {
        const auto matches = el::text::fuzzy::Matcher{"CONFIG"_el}
                                 .setComparisonFn(el::text::Char::compareAsciiFolded)
                                 .setMaximumDistance(el::unit::CpLength{2U})
                                 .setMaximumResults(el::unit::ItemCount{2U})
                                 .findMatches(el::text::StringList{"config"_el, "CONFIG"_el, "confic"_el, "conf"_el});

        REQUIRE_EQUAL(matches.count(), el::unit::ItemCount{2U});
        REQUIRE(matches.first().text() == "config"_el);
        REQUIRE_EQUAL(matches.first().distance(), el::unit::CpLength::zero());
        REQUIRE(matches.last().text() == "confic"_el);
    }

    void testUnboundedDefaultsAndStableTies() {
        const auto matcher = el::text::fuzzy::Matcher{"cat"_el};
        REQUIRE(matcher.maximumDistance().isInfinite());
        REQUIRE(matcher.maximumResults().isInfinite());
        REQUIRE(matcher.comparisonFn() == nullptr);

        const auto matches = matcher.findMatches(el::text::StringList{"bat"_el, "cut"_el, "cats"_el});
        REQUIRE_EQUAL(matches.count(), el::unit::ItemCount{3U});
        REQUIRE(matches.get(el::unit::ItemIndex{0U}).text() == "bat"_el);
        REQUIRE(matches.get(el::unit::ItemIndex{1U}).text() == "cut"_el);
        REQUIRE(matches.get(el::unit::ItemIndex{2U}).text() == "cats"_el);
    }
};
