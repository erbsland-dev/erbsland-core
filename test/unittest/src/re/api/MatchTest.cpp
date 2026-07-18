// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../engine/MockStringMatch.hpp"
#include "../StringHelper.hpp"

#include <erbsland/err/ParameterError.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::re;

TESTED_TARGETS(Match)
TAGS(Api Captures)
class MatchTest final : public el::UnitTest {
public:
    void testGroupCountAndHasGroupIndex() {
        constexpr auto text = "abc123xyz"_el;
        CaptureGroupList groups;
        groups.emplace_back(0, CaptureRange{0, 6}, String{});
        groups.emplace_back(1, CaptureRange{0, 3}, "letters"_el);
        groups.emplace_back(2, CaptureRange{3, 6}, "digits"_el);
        groups.emplace_back(3, CaptureRange{6, 6}, "empty"_el);
        const auto match = std::make_shared<MockStringMatch>(std::move(groups), text);

        REQUIRE_EQUAL(match->groupCount(), 4U);

        REQUIRE(match->hasGroupIndex(0));
        REQUIRE(match->hasGroupIndex(1));
        REQUIRE(match->hasGroupIndex(2));
        REQUIRE(match->hasGroupIndex(3));
        REQUIRE_FALSE(match->hasGroupIndex(4));
        REQUIRE_FALSE(match->hasGroupIndex(100));
    }

    void testHasGroupNameIsCaseInsensitiveAndIgnoresEmptyNames() {
        constexpr auto text = "abc123xyz"_el;
        CaptureGroupList groups;
        groups.emplace_back(0, CaptureRange{0, 6}, String{});
        groups.emplace_back(1, CaptureRange{0, 3}, "Letters"_el);
        groups.emplace_back(2, CaptureRange{3, 6}, "digits"_el);
        groups.emplace_back(3, CaptureRange{6, 6}, String{});
        const auto match = std::make_shared<MockStringMatch>(std::move(groups), text);

        REQUIRE(match->hasGroupName("letters"_el));
        REQUIRE(match->hasGroupName("LETTERS"_el));
        REQUIRE(match->hasGroupName("Digits"_el));
        REQUIRE_EQUAL(match->content("LETTERS"_el), "abc"_el);
        REQUIRE_EQUAL(match->group("DIGITS"_el).index(), 2U);
        REQUIRE_FALSE(match->hasGroupName("missing"_el));
        REQUIRE_FALSE(match->hasGroupName(String{}));
    }

    void testContentBeginEndRangeAndGroupAccessByIndexAndName() {
        constexpr auto text = "abc123xyz"_el;
        CaptureGroupList groups;
        groups.emplace_back(0, CaptureRange{0, 6}, String{});
        groups.emplace_back(1, CaptureRange{0, 3}, "letters"_el);
        groups.emplace_back(2, CaptureRange{3, 6}, "digits"_el);
        groups.emplace_back(3, CaptureRange{6, 6}, "empty"_el);
        const auto match = std::make_shared<MockStringMatch>(std::move(groups), text);

        // content()
        REQUIRE_EQUAL(match->content(), "abc123"_el);
        REQUIRE_EQUAL(match->content(1), "abc"_el);
        REQUIRE_EQUAL(match->content("letters"_el), "abc"_el);
        REQUIRE_EQUAL(match->content(2), "123"_el);
        REQUIRE_EQUAL(match->content("digits"_el), "123"_el);
        REQUIRE(match->content(3).isEmpty());
        REQUIRE(match->content("empty"_el).isEmpty());

        // begin()/end() (group 0)
        REQUIRE_EQUAL(match->begin(), 0U);
        REQUIRE_EQUAL(match->end(), 6U);

        // begin()/end() (by index)
        REQUIRE_EQUAL(match->begin(2), 3U);
        REQUIRE_EQUAL(match->end(2), 6U);

        // begin()/end() (by name)
        REQUIRE_EQUAL(match->begin("digits"_el), 3U);
        REQUIRE_EQUAL(match->end("digits"_el), 6U);

        // range() matches begin/end
        REQUIRE_EQUAL(match->range().begin(), match->begin());
        REQUIRE_EQUAL(match->range().end(), match->end());
        REQUIRE_EQUAL(match->range(2).begin(), match->begin(2));
        REQUIRE_EQUAL(match->range(2).end(), match->end(2));
        REQUIRE_EQUAL(match->range("digits"_el).begin(), match->begin("digits"_el));
        REQUIRE_EQUAL(match->range("digits"_el).end(), match->end("digits"_el));

        // group()
        REQUIRE_EQUAL(match->group().index(), 0U);
        REQUIRE_EQUAL(match->group().range(), match->range());
        REQUIRE(match->group().name().isEmpty());

        REQUIRE_EQUAL(match->group(2).index(), 2U);
        REQUIRE_EQUAL(match->group(2).name(), "digits"_el);
        REQUIRE_EQUAL(match->group(2).range(), match->range(2));

        REQUIRE_EQUAL(match->group("digits"_el).index(), 2U);
        REQUIRE_EQUAL(match->group("digits"_el).range(), match->range("digits"_el));
    }

    void testRangeAndGroupAccessByIndexAndName() {
        constexpr auto text = "abc123xyz"_el;
        CaptureGroupList groups;
        groups.emplace_back(0, CaptureRange{0, 6}, String{});
        groups.emplace_back(1, CaptureRange{3, 6}, "digits"_el);
        const auto match = std::make_shared<MockStringMatch>(std::move(groups), text);

        REQUIRE_EQUAL(match->range().begin(), 0U);
        REQUIRE_EQUAL(match->range().end(), 6U);
        REQUIRE_EQUAL(match->range(1).begin(), 3U);
        REQUIRE_EQUAL(match->range(1).end(), 6U);
        REQUIRE_EQUAL(match->range("digits"_el).begin(), 3U);
        REQUIRE_EQUAL(match->range("digits"_el).end(), 6U);

        REQUIRE_EQUAL(match->group().index(), 0U);
        REQUIRE_EQUAL(match->group().range().begin(), 0U);
        REQUIRE_EQUAL(match->group().range().end(), 6U);

        REQUIRE_EQUAL(match->group(1).index(), 1U);
        REQUIRE_EQUAL(match->group(1).name(), "digits"_el);
        REQUIRE_EQUAL(match->group("digits"_el).index(), 1U);
        REQUIRE_EQUAL(match->group("digits"_el).range().begin(), 3U);
        REQUIRE_EQUAL(match->group("digits"_el).range().end(), 6U);
    }

    void testRangeAndGroupThrowOnInvalidIndexOrName() {
        constexpr auto text = "abc"_el;
        CaptureGroupList groups;
        groups.emplace_back(0, CaptureRange{0, 3}, String{});
        const auto match = std::make_shared<MockStringMatch>(std::move(groups), text);

        REQUIRE_THROWS_AS(el::err::ParameterError, match->content(1));
        REQUIRE_THROWS_AS(el::err::ParameterError, match->content("missing"_el));
        REQUIRE_THROWS_AS(el::err::ParameterError, match->begin(1));
        REQUIRE_THROWS_AS(el::err::ParameterError, match->begin("missing"_el));
        REQUIRE_THROWS_AS(el::err::ParameterError, match->end(1));
        REQUIRE_THROWS_AS(el::err::ParameterError, match->end("missing"_el));
        REQUIRE_THROWS_AS(el::err::ParameterError, match->range(1));
        REQUIRE_THROWS_AS(el::err::ParameterError, match->group(1));
        REQUIRE_THROWS_AS(el::err::ParameterError, match->range("missing"_el));
        REQUIRE_THROWS_AS(el::err::ParameterError, match->group("missing"_el));
    }
};
