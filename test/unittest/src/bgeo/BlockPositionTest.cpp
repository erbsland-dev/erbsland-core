// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/bgeo/BlockPosition.hpp>
#include <erbsland/bgeo/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <functional>
#include <limits>

TESTED_TARGETS(BlockPosition BlockPositionList)
class BlockPositionTest final : public el::UnitTest {
public:
    void testConstructorsAndAccessors() {
        using el::bgeo::BlockPosition;
        using el::bgeo::Orientation;

        auto position = BlockPosition{3, 7};
        REQUIRE_EQUAL(position.x(), 3);
        REQUIRE_EQUAL(position.y(), 7);
        REQUIRE_EQUAL(position.coordinate(Orientation::Horizontal), 3);
        REQUIRE_EQUAL(position.coordinate(Orientation::Vertical), 7);

        position.setX(-4);
        position.setY(9);
        REQUIRE_EQUAL(position, (BlockPosition{-4, 9}));
    }

    void testArithmeticAndSaturation() {
        using el::bgeo::BlockCoordinate;
        using el::bgeo::BlockPosition;

        const auto maximum = BlockCoordinate::maximum();
        const auto minimum = BlockCoordinate::minimum();

        const auto sum = BlockPosition(3, 7) + BlockPosition(5, -2);
        const auto difference = BlockPosition(3, 7) - BlockPosition(5, -2);
        const auto saturated = BlockPosition{maximum, minimum} + BlockPosition{1, -1};
        REQUIRE_EQUAL(sum, (BlockPosition{8, 5}));
        REQUIRE_EQUAL(difference, (BlockPosition{-2, 9}));
        REQUIRE_EQUAL(saturated, (BlockPosition{maximum, minimum}));
    }

    void testDistanceAndComponentOperations() {
        using el::bgeo::BlockPosition;

        const auto position = BlockPosition(3, 7);
        const auto other = BlockPosition(-2, 10);
        const auto distance = position.distanceTo(other);
        const auto componentMinimum = position.componentMin(other);
        const auto componentMaximum = position.componentMax(other);
        REQUIRE_EQUAL(distance, 8);
        REQUIRE_EQUAL(componentMinimum, (BlockPosition{-2, 7}));
        REQUIRE_EQUAL(componentMaximum, (BlockPosition{3, 10}));
    }

    void testNeighborHelpers() {
        using el::bgeo::BlockPosition;

        const auto cardinal = BlockPosition{10, 20}.cardinalFour();
        REQUIRE_EQUAL(cardinal[0], (BlockPosition{11, 20}));
        REQUIRE_EQUAL(cardinal[1], (BlockPosition{10, 21}));
        REQUIRE_EQUAL(cardinal[2], (BlockPosition{9, 20}));
        REQUIRE_EQUAL(cardinal[3], (BlockPosition{10, 19}));

        const auto ring = BlockPosition{10, 20}.ringEight();
        REQUIRE_EQUAL(ring[0], (BlockPosition{11, 20}));
        REQUIRE_EQUAL(ring[7], (BlockPosition{11, 19}));
        const auto nonNegativeX = [](const BlockPosition position) -> bool { return position.x() >= 0; };
        const auto bitmask = BlockPosition{}.cardinalFourBitmask(nonNegativeX);
        REQUIRE_EQUAL(bitmask, 0b1011U);
    }

    void testMinimumMaximumAndHash() {
        using el::bgeo::BlockCoordinate;
        using el::bgeo::BlockPosition;

        const auto minimum = BlockPosition::minimum();
        const auto maximum = BlockPosition::maximum();
        const auto position = BlockPosition{3, 4};
        const auto hash = std::hash<BlockPosition>{}(position);
        REQUIRE_EQUAL(minimum.x(), BlockCoordinate::minimum());
        REQUIRE_EQUAL(maximum.y(), BlockCoordinate::maximum());
        REQUIRE_EQUAL(hash, position.hash());
    }
};
