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

        const auto noneDelta = BlockDirection{BlockDirection::None}.toDelta();
        const auto northDelta = BlockDirection{BlockDirection::North}.toDelta();
        const auto northEastDelta = BlockDirection{BlockDirection::NorthEast}.toDelta();
        const auto eastDelta = BlockDirection{BlockDirection::East}.toDelta();
        const auto southEastDelta = BlockDirection{BlockDirection::SouthEast}.toDelta();
        const auto southDelta = BlockDirection{BlockDirection::South}.toDelta();
        const auto southWestDelta = BlockDirection{BlockDirection::SouthWest}.toDelta();
        const auto westDelta = BlockDirection{BlockDirection::West}.toDelta();
        const auto northWestDelta = BlockDirection{BlockDirection::NorthWest}.toDelta();
        const auto diagonal = BlockDirection::fromDelta(BlockPosition(10, -20));
        const auto origin = BlockDirection::fromDelta(BlockPosition(0, 0));
        REQUIRE_EQUAL(noneDelta, BlockPosition(0, 0));
        REQUIRE_EQUAL(northDelta, BlockPosition(0, -1));
        REQUIRE_EQUAL(northEastDelta, BlockPosition(1, -1));
        REQUIRE_EQUAL(eastDelta, BlockPosition(1, 0));
        REQUIRE_EQUAL(southEastDelta, BlockPosition(1, 1));
        REQUIRE_EQUAL(southDelta, BlockPosition(0, 1));
        REQUIRE_EQUAL(southWestDelta, BlockPosition(-1, 1));
        REQUIRE_EQUAL(westDelta, BlockPosition(-1, 0));
        REQUIRE_EQUAL(northWestDelta, BlockPosition(-1, -1));
        REQUIRE_EQUAL(diagonal, BlockDirection::NorthEast);
        REQUIRE_EQUAL(origin, BlockDirection::None);
    }

    void testStringConversion() {
        using el::bgeo::BlockDirection;

        const auto northText = BlockDirection{BlockDirection::North}.toString();
        const auto northEastText = BlockDirection{BlockDirection::NorthEast}.toString();
        const auto southWestText = BlockDirection{BlockDirection::SouthWest}.toString();
        const auto noneText = BlockDirection{BlockDirection::None}.toString();
        REQUIRE_EQUAL(northText, "north"_el);
        REQUIRE_EQUAL(northEastText, "north_east"_el);
        REQUIRE_EQUAL(southWestText, "south_west"_el);
        REQUIRE_EQUAL(noneText, "none"_el);
    }

    void testStringParsingUsesConfigurationIdentifierComparison() {
        using el::bgeo::BlockDirection;

        REQUIRE(BlockDirection::isValidString(""_el));
        REQUIRE(BlockDirection::isValidString("n"_el));
        REQUIRE(BlockDirection::isValidString("North East"_el));
        REQUIRE(BlockDirection::isValidString("north_east"_el));
        REQUIRE(BlockDirection::isValidString("South West"_el));
        const auto emptyDirection = BlockDirection::fromString(""_el);
        const auto northEastDirection = BlockDirection::fromString("North East"_el);
        const auto southWestDirection = BlockDirection::fromString("South West"_el);
        REQUIRE_EQUAL(emptyDirection, BlockDirection::None);
        REQUIRE_EQUAL(northEastDirection, BlockDirection::NorthEast);
        REQUIRE_EQUAL(southWestDirection, BlockDirection::SouthWest);
    }

    void testInvalidStringReturnsNone() {
        using el::bgeo::BlockDirection;

        REQUIRE_FALSE(BlockDirection::isValidString("sideways"_el));
        const auto direction = BlockDirection::fromString("sideways"_el);
        REQUIRE_EQUAL(direction, BlockDirection::None);
    }

    void testHashSupport() {
        using el::bgeo::BlockDirection;

        const auto northHash = std::hash<BlockDirection>{}(BlockDirection::North);
        const auto southHash = std::hash<BlockDirection>{}(BlockDirection::South);
        const auto expectedNorthHash = BlockDirection{BlockDirection::North}.hash();
        REQUIRE_EQUAL(northHash, expectedNorthHash);
        REQUIRE_NOT_EQUAL(northHash, southHash);
    }
};
