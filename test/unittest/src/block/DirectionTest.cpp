// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/block/Direction.hpp>
#include <erbsland/block/StdFormat.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <functional>

using namespace el::text::literals;

TESTED_TARGETS(Direction)
class DirectionTest final : public el::UnitTest {
public:
    void testContains() {
        using el::block::Direction;

        REQUIRE(Direction{Direction::NorthEast}.contains(Direction::North));
        REQUIRE(Direction{Direction::NorthEast}.contains(Direction::East));
        REQUIRE(Direction{Direction::NorthEast}.contains(Direction::NorthEast));
        REQUIRE_FALSE(Direction{Direction::NorthEast}.contains(Direction::South));
        REQUIRE_FALSE(Direction{Direction::NorthEast}.contains(Direction::SouthEast));
        REQUIRE_FALSE(Direction{Direction::None}.contains(Direction::North));
    }

    void testDeltaConversion() {
        using el::block::Direction;
        using el::block::Position;

        const auto noneDelta = Direction{Direction::None}.toDelta();
        const auto northDelta = Direction{Direction::North}.toDelta();
        const auto northEastDelta = Direction{Direction::NorthEast}.toDelta();
        const auto eastDelta = Direction{Direction::East}.toDelta();
        const auto southEastDelta = Direction{Direction::SouthEast}.toDelta();
        const auto southDelta = Direction{Direction::South}.toDelta();
        const auto southWestDelta = Direction{Direction::SouthWest}.toDelta();
        const auto westDelta = Direction{Direction::West}.toDelta();
        const auto northWestDelta = Direction{Direction::NorthWest}.toDelta();
        const auto diagonal = Direction::fromDelta(Position(10, -20));
        const auto origin = Direction::fromDelta(Position(0, 0));
        REQUIRE_EQUAL(noneDelta, Position(0, 0));
        REQUIRE_EQUAL(northDelta, Position(0, -1));
        REQUIRE_EQUAL(northEastDelta, Position(1, -1));
        REQUIRE_EQUAL(eastDelta, Position(1, 0));
        REQUIRE_EQUAL(southEastDelta, Position(1, 1));
        REQUIRE_EQUAL(southDelta, Position(0, 1));
        REQUIRE_EQUAL(southWestDelta, Position(-1, 1));
        REQUIRE_EQUAL(westDelta, Position(-1, 0));
        REQUIRE_EQUAL(northWestDelta, Position(-1, -1));
        REQUIRE_EQUAL(diagonal, Direction::NorthEast);
        REQUIRE_EQUAL(origin, Direction::None);
    }

    void testStringConversion() {
        using el::block::Direction;

        const auto northText = Direction{Direction::North}.toString();
        const auto northEastText = Direction{Direction::NorthEast}.toString();
        const auto southWestText = Direction{Direction::SouthWest}.toString();
        const auto noneText = Direction{Direction::None}.toString();
        REQUIRE_EQUAL(northText, "north"_el);
        REQUIRE_EQUAL(northEastText, "north_east"_el);
        REQUIRE_EQUAL(southWestText, "south_west"_el);
        REQUIRE_EQUAL(noneText, "none"_el);
    }

    void testStringParsingUsesConfigurationIdentifierComparison() {
        using el::block::Direction;

        REQUIRE(Direction::isValidString(""_el));
        REQUIRE(Direction::isValidString("n"_el));
        REQUIRE(Direction::isValidString("North East"_el));
        REQUIRE(Direction::isValidString("north_east"_el));
        REQUIRE(Direction::isValidString("South West"_el));
        const auto emptyDirection = Direction::fromString(""_el);
        const auto northEastDirection = Direction::fromString("North East"_el);
        const auto southWestDirection = Direction::fromString("South West"_el);
        REQUIRE_EQUAL(emptyDirection, Direction::None);
        REQUIRE_EQUAL(northEastDirection, Direction::NorthEast);
        REQUIRE_EQUAL(southWestDirection, Direction::SouthWest);
    }

    void testInvalidStringReturnsNone() {
        using el::block::Direction;

        REQUIRE_FALSE(Direction::isValidString("sideways"_el));
        const auto direction = Direction::fromString("sideways"_el);
        REQUIRE_EQUAL(direction, Direction::None);
    }

    void testHashSupport() {
        using el::block::Direction;

        const auto northHash = std::hash<Direction>{}(Direction::North);
        const auto southHash = std::hash<Direction>{}(Direction::South);
        const auto expectedNorthHash = Direction{Direction::North}.hash();
        REQUIRE_EQUAL(northHash, expectedNorthHash);
        REQUIRE_NOT_EQUAL(northHash, southHash);
    }
};
