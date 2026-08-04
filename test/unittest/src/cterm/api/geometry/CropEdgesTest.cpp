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
        REQUIRE_FALSE(cropEdges.isSet(bgeo::BlockDirection::None));
        REQUIRE_FALSE(cropEdges.isSet(bgeo::BlockDirection::North));
        REQUIRE_FALSE(cropEdges.isSet(bgeo::BlockDirection::NorthEast));
        REQUIRE_FALSE(cropEdges.isSet(bgeo::BlockDirection::East));
        REQUIRE_FALSE(cropEdges.isSet(bgeo::BlockDirection::SouthEast));
        REQUIRE_FALSE(cropEdges.isSet(bgeo::BlockDirection::South));
        REQUIRE_FALSE(cropEdges.isSet(bgeo::BlockDirection::SouthWest));
        REQUIRE_FALSE(cropEdges.isSet(bgeo::BlockDirection::West));
        REQUIRE_FALSE(cropEdges.isSet(bgeo::BlockDirection::NorthWest));
    }

    void testSetClearAndResetManageFlags() {
        CropEdges cropEdges;

        cropEdges.set(bgeo::BlockDirection::North);
        cropEdges.set(bgeo::BlockDirection::SouthWest);
        cropEdges.set(bgeo::BlockDirection::None);

        REQUIRE(cropEdges.isSet(bgeo::BlockDirection::North));
        REQUIRE(cropEdges.isSet(bgeo::BlockDirection::SouthWest));
        REQUIRE_EQUAL(cropEdges.flags().count(), 2U);

        cropEdges.clear(bgeo::BlockDirection::North);
        cropEdges.clear(bgeo::BlockDirection::None);

        REQUIRE_FALSE(cropEdges.isSet(bgeo::BlockDirection::North));
        REQUIRE(cropEdges.isSet(bgeo::BlockDirection::SouthWest));

        cropEdges.reset();

        REQUIRE_EQUAL(cropEdges.flags().count(), 0U);
    }

    void testComparisonOperatorsReflectFlagState() {
        CropEdges left;
        CropEdges right;

        left.set(bgeo::BlockDirection::East);
        right.set(bgeo::BlockDirection::East);

        REQUIRE_EQUAL(left, right);

        right.set(bgeo::BlockDirection::SouthEast);

        REQUIRE_NOT_EQUAL(left, right);
    }

    void testFromViewSetsEdgesAndCornersForClippedContent() {
        const auto cropEdges = CropEdges::fromView(bgeo::BlockRectangle(2, 3, 4, 3), bgeo::BlockRectangle(0, 0, 8, 8));

        REQUIRE(cropEdges.isSet(bgeo::BlockDirection::North));
        REQUIRE(cropEdges.isSet(bgeo::BlockDirection::NorthEast));
        REQUIRE(cropEdges.isSet(bgeo::BlockDirection::East));
        REQUIRE(cropEdges.isSet(bgeo::BlockDirection::SouthEast));
        REQUIRE(cropEdges.isSet(bgeo::BlockDirection::South));
        REQUIRE(cropEdges.isSet(bgeo::BlockDirection::SouthWest));
        REQUIRE(cropEdges.isSet(bgeo::BlockDirection::West));
        REQUIRE(cropEdges.isSet(bgeo::BlockDirection::NorthWest));

        REQUIRE_EQUAL(
            CropEdges::fromView(bgeo::BlockRectangle(2, 3, 4, 3), bgeo::BlockRectangle(2, 3, 4, 3)).flags().count(),
            0U);
    }

    void testEdgeForViewReturnsExactDirectionsForCroppedFramePositions() {
        const auto viewRect = bgeo::BlockRectangle(10, 20, 4, 3);
        const auto cropEdges = CropEdges::fromView(viewRect, bgeo::BlockRectangle(7, 18, 10, 6));

        REQUIRE_EQUAL(cropEdges.edgeForView(bgeo::BlockPosition(10, 20), viewRect), bgeo::BlockDirection::NorthWest);
        REQUIRE_EQUAL(cropEdges.edgeForView(bgeo::BlockPosition(11, 20), viewRect), bgeo::BlockDirection::North);
        REQUIRE_EQUAL(cropEdges.edgeForView(bgeo::BlockPosition(13, 20), viewRect), bgeo::BlockDirection::NorthEast);
        REQUIRE_EQUAL(cropEdges.edgeForView(bgeo::BlockPosition(13, 21), viewRect), bgeo::BlockDirection::East);
        REQUIRE_EQUAL(cropEdges.edgeForView(bgeo::BlockPosition(13, 22), viewRect), bgeo::BlockDirection::SouthEast);
        REQUIRE_EQUAL(cropEdges.edgeForView(bgeo::BlockPosition(12, 22), viewRect), bgeo::BlockDirection::South);
        REQUIRE_EQUAL(cropEdges.edgeForView(bgeo::BlockPosition(10, 22), viewRect), bgeo::BlockDirection::SouthWest);
        REQUIRE_EQUAL(cropEdges.edgeForView(bgeo::BlockPosition(10, 21), viewRect), bgeo::BlockDirection::West);
        REQUIRE_EQUAL(cropEdges.edgeForView(bgeo::BlockPosition(11, 21), viewRect), bgeo::BlockDirection::None);
    }

    void testEdgeForViewFallsBackToTheMatchingSingleEdgeAtCorners() {
        CropEdges cropEdges;
        const auto viewRect = bgeo::BlockRectangle(10, 20, 4, 3);

        cropEdges.set(bgeo::BlockDirection::East);

        REQUIRE_EQUAL(cropEdges.edgeForView(bgeo::BlockPosition(13, 20), viewRect), bgeo::BlockDirection::East);
        REQUIRE_EQUAL(cropEdges.edgeForView(bgeo::BlockPosition(13, 22), viewRect), bgeo::BlockDirection::East);

        cropEdges.reset();
        cropEdges.set(bgeo::BlockDirection::South);

        REQUIRE_EQUAL(cropEdges.edgeForView(bgeo::BlockPosition(13, 22), viewRect), bgeo::BlockDirection::South);
        REQUIRE_EQUAL(cropEdges.edgeForView(bgeo::BlockPosition(10, 22), viewRect), bgeo::BlockDirection::South);
    }
};
