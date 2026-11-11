// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../src/DigitHistory.hpp"
#include "../../src/DigitViewport.hpp"

#include <erbsland/block/Coordinate.hpp>
#include <erbsland/CpCount.hpp>
#include <erbsland/CpIndex.hpp>
#include <erbsland/CpRange.hpp>
#include <erbsland/ParameterFailure.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::text::literals;
using el::block::Coordinate;

TESTED_TARGETS(DigitHistory DigitViewport)
class DigitHistoryTest final : public el::UnitTest {
public:
    void testRangesAcrossUnequalChunks() {
        auto history = demo::DigitHistory{};
        history.append("141"_el);
        history.append({});
        history.append("59265"_el);
        history.append("3589"_el);
        REQUIRE_EQUAL(history.count(), el::CpCount{12});
        REQUIRE_EQUAL(history.range({el::CpIndex{0}, el::CpCount{12}}), "141592653589"_el);
        REQUIRE_EQUAL(history.range({el::CpIndex{2}, el::CpCount{8}}), "15926535"_el);
        REQUIRE_EQUAL(history.range({el::CpIndex{3}, el::CpCount{5}}), "59265"_el);
        REQUIRE_EQUAL(history.range({el::CpIndex{8}, el::CpCount{100}}), "3589"_el);
        REQUIRE(history.range({el::CpIndex{12}, el::CpCount{1}}).isEmpty());
        REQUIRE(history.range({el::CpIndex{1}, el::CpCount{0}}).isEmpty());
        REQUIRE_EQUAL(history.range({el::CpIndex{}, el::CpCount::infinite()}), "141592653589"_el);
        REQUIRE_THROWS_AS(el::ParameterFailure, history.append("12x3"_el));
        REQUIRE_THROWS_AS(el::ParameterFailure, history.append("١٢٣"_el));
        REQUIRE_EQUAL(history.count(), el::CpCount{12});
    }
    void testEmptyInvalidAndUnboundedRanges() {
        auto history = demo::DigitHistory{};
        REQUIRE(history.range(el::CpRange::all()).isEmpty());
        history.append("14159"_el);
        REQUIRE(history.range(el::CpRange::noRange()).isEmpty());
        REQUIRE(history.range(el::CpRange::empty()).isEmpty());
        REQUIRE(history.range({el::CpIndex{100}, el::CpCount::infinite()}).isEmpty());
        REQUIRE_EQUAL(history.range({el::CpIndex{2}, el::CpCount::infinite()}), "159"_el);
    }
    void testHistoryDoesNotDependOnBatchBoundaries() {
        auto first = demo::DigitHistory{};
        auto second = demo::DigitHistory{};
        first.append("141592653589"_el);
        second.append("1"_el);
        second.append("4159"_el);
        second.append("2653589"_el);
        REQUIRE_EQUAL(first.range({el::CpIndex{}, el::CpCount{12}}), second.range({el::CpIndex{}, el::CpCount{12}}));
    }
    void testNavigationFollowAndResize() {
        auto viewport = demo::DigitViewport{};
        viewport.update(el::CpCount{501}, Coordinate{4});
        REQUIRE_EQUAL(viewport.firstLine(), Coordinate{7});
        viewport.up(Coordinate{3});
        REQUIRE_FALSE(viewport.isFollowing());
        REQUIRE_EQUAL(viewport.firstLine(), Coordinate{4});
        viewport.update(el::CpCount{1000}, Coordinate{4});
        REQUIRE_EQUAL(viewport.firstLine(), Coordinate{4});
        viewport.down(Coordinate{1000});
        REQUIRE_EQUAL(viewport.firstLine(), Coordinate{16});
        viewport.update(el::CpCount{1000}, Coordinate{10});
        REQUIRE_EQUAL(viewport.firstLine(), Coordinate{10});
        viewport.home();
        viewport.up(Coordinate{1});
        REQUIRE_EQUAL(viewport.firstLine(), Coordinate{0});
        viewport.end();
        REQUIRE_FALSE(viewport.isFollowing());
        viewport.follow();
        viewport.update(el::CpCount{1100}, Coordinate{10});
        REQUIRE_EQUAL(viewport.firstLine(), Coordinate{12});
        viewport.update(el::CpCount{0}, Coordinate{0});
        REQUIRE_EQUAL(viewport.firstLine(), Coordinate{0});
        REQUIRE_EQUAL(viewport.visibleLines(), Coordinate{1});
    }
};
