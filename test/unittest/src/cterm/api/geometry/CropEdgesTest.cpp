// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/TestHelper.hpp"

#include <erbsland/cterm/CropEdges.hpp>
#include <erbsland/unittest/UnitTest.hpp>

TESTED_TARGETS(CropEdges)
class CropEdgesTest final : public el::UnitTest {
public:
    void testDefaultConstructionHasNoEdgesSet() {
        const CropEdges cropEdges;

        REQUIRE_EQUAL(cropEdges.flags().count(), 0U);
        REQUIRE_FALSE(cropEdges.isSet(block::Direction::None));
        REQUIRE_FALSE(cropEdges.isSet(block::Direction::North));
        REQUIRE_FALSE(cropEdges.isSet(block::Direction::NorthEast));
        REQUIRE_FALSE(cropEdges.isSet(block::Direction::East));
        REQUIRE_FALSE(cropEdges.isSet(block::Direction::SouthEast));
        REQUIRE_FALSE(cropEdges.isSet(block::Direction::South));
        REQUIRE_FALSE(cropEdges.isSet(block::Direction::SouthWest));
        REQUIRE_FALSE(cropEdges.isSet(block::Direction::West));
        REQUIRE_FALSE(cropEdges.isSet(block::Direction::NorthWest));
    }

    void testSetClearAndResetManageFlags() {
        CropEdges cropEdges;

        cropEdges.set(block::Direction::North);
        cropEdges.set(block::Direction::SouthWest);
        cropEdges.set(block::Direction::None);

        REQUIRE(cropEdges.isSet(block::Direction::North));
        REQUIRE(cropEdges.isSet(block::Direction::SouthWest));
        REQUIRE_EQUAL(cropEdges.flags().count(), 2U);

        cropEdges.clear(block::Direction::North);
        cropEdges.clear(block::Direction::None);

        REQUIRE_FALSE(cropEdges.isSet(block::Direction::North));
        REQUIRE(cropEdges.isSet(block::Direction::SouthWest));

        cropEdges.reset();

        REQUIRE_EQUAL(cropEdges.flags().count(), 0U);
    }

    void testComparisonOperatorsReflectFlagState() {
        CropEdges left;
        CropEdges right;

        left.set(block::Direction::East);
        right.set(block::Direction::East);

        REQUIRE_EQUAL(left, right);

        right.set(block::Direction::SouthEast);

        REQUIRE_NOT_EQUAL(left, right);
    }

    void testFromViewSetsEdgesAndCornersForClippedContent() {
        const auto cropEdges = CropEdges::fromView(block::Rectangle(2, 3, 4, 3), block::Rectangle(0, 0, 8, 8));

        REQUIRE(cropEdges.isSet(block::Direction::North));
        REQUIRE(cropEdges.isSet(block::Direction::NorthEast));
        REQUIRE(cropEdges.isSet(block::Direction::East));
        REQUIRE(cropEdges.isSet(block::Direction::SouthEast));
        REQUIRE(cropEdges.isSet(block::Direction::South));
        REQUIRE(cropEdges.isSet(block::Direction::SouthWest));
        REQUIRE(cropEdges.isSet(block::Direction::West));
        REQUIRE(cropEdges.isSet(block::Direction::NorthWest));

        REQUIRE_EQUAL(
            CropEdges::fromView(block::Rectangle(2, 3, 4, 3), block::Rectangle(2, 3, 4, 3)).flags().count(), 0U);
    }

    void testEdgeForViewReturnsExactDirectionsForCroppedFramePositions() {
        const auto viewRect = block::Rectangle(10, 20, 4, 3);
        const auto cropEdges = CropEdges::fromView(viewRect, block::Rectangle(7, 18, 10, 6));

        REQUIRE_EQUAL(cropEdges.edgeForView(block::Position(10, 20), viewRect), block::Direction::NorthWest);
        REQUIRE_EQUAL(cropEdges.edgeForView(block::Position(11, 20), viewRect), block::Direction::North);
        REQUIRE_EQUAL(cropEdges.edgeForView(block::Position(13, 20), viewRect), block::Direction::NorthEast);
        REQUIRE_EQUAL(cropEdges.edgeForView(block::Position(13, 21), viewRect), block::Direction::East);
        REQUIRE_EQUAL(cropEdges.edgeForView(block::Position(13, 22), viewRect), block::Direction::SouthEast);
        REQUIRE_EQUAL(cropEdges.edgeForView(block::Position(12, 22), viewRect), block::Direction::South);
        REQUIRE_EQUAL(cropEdges.edgeForView(block::Position(10, 22), viewRect), block::Direction::SouthWest);
        REQUIRE_EQUAL(cropEdges.edgeForView(block::Position(10, 21), viewRect), block::Direction::West);
        REQUIRE_EQUAL(cropEdges.edgeForView(block::Position(11, 21), viewRect), block::Direction::None);
    }

    void testEdgeForViewFallsBackToTheMatchingSingleEdgeAtCorners() {
        CropEdges cropEdges;
        const auto viewRect = block::Rectangle(10, 20, 4, 3);

        cropEdges.set(block::Direction::East);

        REQUIRE_EQUAL(cropEdges.edgeForView(block::Position(13, 20), viewRect), block::Direction::East);
        REQUIRE_EQUAL(cropEdges.edgeForView(block::Position(13, 22), viewRect), block::Direction::East);

        cropEdges.reset();
        cropEdges.set(block::Direction::South);

        REQUIRE_EQUAL(cropEdges.edgeForView(block::Position(13, 22), viewRect), block::Direction::South);
        REQUIRE_EQUAL(cropEdges.edgeForView(block::Position(10, 22), viewRect), block::Direction::South);
    }
};
