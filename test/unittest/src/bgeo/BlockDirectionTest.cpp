// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/bgeo/BlockDirection.hpp>
#include <erbsland/bgeo/StdFormat.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <functional>

using namespace el::text::literals;

TESTED_TARGETS(BlockDirection)
class BlockDirectionTest final : public el::UnitTest {
public:
    void testContains() {
        using el::bgeo::BlockDirection;

        REQUIRE(BlockDirection{BlockDirection::NorthEast}.contains(BlockDirection::North));
        REQUIRE(BlockDirection{BlockDirection::NorthEast}.contains(BlockDirection::East));
        REQUIRE(BlockDirection{BlockDirection::NorthEast}.contains(BlockDirection::NorthEast));
        REQUIRE_FALSE(BlockDirection{BlockDirection::NorthEast}.contains(BlockDirection::South));
        REQUIRE_FALSE(BlockDirection{BlockDirection::NorthEast}.contains(BlockDirection::SouthEast));
        REQUIRE_FALSE(BlockDirection{BlockDirection::None}.contains(BlockDirection::North));
    }

    void testDeltaConversion() {
        using el::bgeo::BlockDirection;
        using el::bgeo::BlockPosition;

        REQUIRE_EQUAL(BlockDirection{BlockDirection::None}.toDelta(), BlockPosition(0, 0));
        REQUIRE_EQUAL(BlockDirection{BlockDirection::North}.toDelta(), BlockPosition(0, -1));
        REQUIRE_EQUAL(BlockDirection{BlockDirection::NorthEast}.toDelta(), BlockPosition(1, -1));
        REQUIRE_EQUAL(BlockDirection{BlockDirection::East}.toDelta(), BlockPosition(1, 0));
        REQUIRE_EQUAL(BlockDirection{BlockDirection::SouthEast}.toDelta(), BlockPosition(1, 1));
        REQUIRE_EQUAL(BlockDirection{BlockDirection::South}.toDelta(), BlockPosition(0, 1));
        REQUIRE_EQUAL(BlockDirection{BlockDirection::SouthWest}.toDelta(), BlockPosition(-1, 1));
        REQUIRE_EQUAL(BlockDirection{BlockDirection::West}.toDelta(), BlockPosition(-1, 0));
        REQUIRE_EQUAL(BlockDirection{BlockDirection::NorthWest}.toDelta(), BlockPosition(-1, -1));

        REQUIRE_EQUAL(BlockDirection::fromDelta(BlockPosition(10, -20)), BlockDirection::NorthEast);
        REQUIRE_EQUAL(BlockDirection::fromDelta(BlockPosition(0, 0)), BlockDirection::None);
    }

    void testStringConversion() {
        using el::bgeo::BlockDirection;

        REQUIRE_EQUAL(BlockDirection{BlockDirection::North}.toString(), "north"_el);
        REQUIRE_EQUAL(BlockDirection{BlockDirection::NorthEast}.toString(), "north_east"_el);
        REQUIRE_EQUAL(BlockDirection{BlockDirection::SouthWest}.toString(), "south_west"_el);
        REQUIRE_EQUAL(BlockDirection{BlockDirection::None}.toString(), "none"_el);
    }

    void testStringParsingUsesConfigurationIdentifierComparison() {
        using el::bgeo::BlockDirection;

        REQUIRE(BlockDirection::isValidString(""_el));
        REQUIRE(BlockDirection::isValidString("n"_el));
        REQUIRE(BlockDirection::isValidString("North East"_el));
        REQUIRE(BlockDirection::isValidString("north_east"_el));
        REQUIRE(BlockDirection::isValidString("South West"_el));
        REQUIRE_EQUAL(BlockDirection::fromString(""_el), BlockDirection::None);
        REQUIRE_EQUAL(BlockDirection::fromString("North East"_el), BlockDirection::NorthEast);
        REQUIRE_EQUAL(BlockDirection::fromString("South West"_el), BlockDirection::SouthWest);
    }

    void testInvalidStringReturnsNone() {
        using el::bgeo::BlockDirection;

        REQUIRE_FALSE(BlockDirection::isValidString("sideways"_el));
        REQUIRE_EQUAL(BlockDirection::fromString("sideways"_el), BlockDirection::None);
    }

    void testHashSupport() {
        using el::bgeo::BlockDirection;

        REQUIRE_EQUAL(std::hash<BlockDirection>{}(BlockDirection::North), BlockDirection{BlockDirection::North}.hash());
        REQUIRE_NOT_EQUAL(
            std::hash<BlockDirection>{}(BlockDirection::North), std::hash<BlockDirection>{}(BlockDirection::South));
    }
};
