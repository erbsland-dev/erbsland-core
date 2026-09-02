// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/block/Position.hpp>
#include <erbsland/block/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <functional>
#include <limits>

TESTED_TARGETS(Position PositionList)
class PositionTest final : public el::UnitTest {
public:
    void testConstructorsAndAccessors() {
        using el::block::Position;
        using el::geometry::Orientation;

        auto position = Position{3, 7};
        REQUIRE_EQUAL(position.x(), 3);
        REQUIRE_EQUAL(position.y(), 7);
        REQUIRE_EQUAL(position.component(Orientation::Horizontal), 3);
        REQUIRE_EQUAL(position.component(Orientation::Vertical), 7);

        position.setX(-4);
        position.setY(9);
        REQUIRE_EQUAL(position, (Position{-4, 9}));
    }

    void testArithmeticAndSaturation() {
        using el::block::Coordinate;
        using el::block::Position;

        const auto maximum = Coordinate::maximum();
        const auto minimum = Coordinate::minimum();

        const auto sum = Position(3, 7) + Position(5, -2);
        const auto difference = Position(3, 7) - Position(5, -2);
        const auto saturated = Position{maximum, minimum} + Position{1, -1};
        REQUIRE_EQUAL(sum, (Position{8, 5}));
        REQUIRE_EQUAL(difference, (Position{-2, 9}));
        REQUIRE_EQUAL(saturated, (Position{maximum, minimum}));
    }

    void testDistanceAndComponentOperations() {
        using el::block::Position;

        const auto position = Position(3, 7);
        const auto other = Position(-2, 10);
        const auto distance = position.distanceTo(other);
        const auto componentMinimum = position.componentMin(other);
        const auto componentMaximum = position.componentMax(other);
        REQUIRE_EQUAL(distance, 8);
        REQUIRE_EQUAL(componentMinimum, (Position{-2, 7}));
        REQUIRE_EQUAL(componentMaximum, (Position{3, 10}));
    }

    void testNeighborHelpers() {
        using el::block::Position;

        const auto cardinal = Position{10, 20}.cardinalFour();
        REQUIRE_EQUAL(cardinal[0], (Position{11, 20}));
        REQUIRE_EQUAL(cardinal[1], (Position{10, 21}));
        REQUIRE_EQUAL(cardinal[2], (Position{9, 20}));
        REQUIRE_EQUAL(cardinal[3], (Position{10, 19}));

        const auto ring = Position{10, 20}.ringEight();
        REQUIRE_EQUAL(ring[0], (Position{11, 20}));
        REQUIRE_EQUAL(ring[7], (Position{11, 19}));
        const auto nonNegativeX = [](const Position position) -> bool { return position.x() >= 0; };
        const auto bitmask = Position{}.cardinalFourBitmask(nonNegativeX);
        REQUIRE_EQUAL(bitmask, 0b1011U);
    }

    void testMinimumMaximumAndHash() {
        using el::block::Coordinate;
        using el::block::Position;

        const auto minimum = Position::minimum();
        const auto maximum = Position::maximum();
        const auto position = Position{3, 4};
        const auto hash = std::hash<Position>{}(position);
        REQUIRE_EQUAL(minimum.x(), Coordinate::minimum());
        REQUIRE_EQUAL(maximum.y(), Coordinate::maximum());
        REQUIRE_EQUAL(hash, position.hash());
    }
};
