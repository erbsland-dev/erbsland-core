// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/TestHelper.hpp"

#include <erbsland/cterm/GridLayout.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <stdexcept>
#include <vector>

TESTED_TARGETS(GridLayout)
class GridLayoutTest final : public el::UnitTest {
public:
    void testConstructorAndAccessorsStoreCellGeometry() {
        const auto layout = gridLayout({2, 3}, {1, 2});

        REQUIRE_EQUAL(layout.columnCount(), std::size_t{2});
        REQUIRE_EQUAL(layout.rowCount(), std::size_t{2});
        REQUIRE_EQUAL(layout.columnWidth(0), 2);
        REQUIRE_EQUAL(layout.columnWidth(1), 3);
        REQUIRE_EQUAL(layout.rowHeight(0), 1);
        REQUIRE_EQUAL(layout.rowHeight(1), 2);
        REQUIRE_EQUAL(layout.columnWidths(), blockCoordinates({2, 3}));
        REQUIRE_EQUAL(layout.rowHeights(), blockCoordinates({1, 2}));
    }

    void testDefaultBorderDoesNotAddAnyLineCells() {
        const auto layout = gridLayout({2, 3}, {1, 2});
        const auto border = FrameBorder{};

        REQUIRE_EQUAL(layout.size(border), (bgeo::BlockSize{5, 3}));
        REQUIRE_EQUAL(layout.cellRect(1, 1, bgeo::BlockPosition{10, 20}, border), (bgeo::BlockRectangle{12, 21, 3, 2}));
    }

    void testActiveBorderAddsOuterAndSeparatorLineCells() {
        const auto layout = gridLayout({2, 3}, {1, 2});
        const auto border = FrameBorder{FrameStyle::Light};

        REQUIRE_EQUAL(layout.size(border), (bgeo::BlockSize{8, 6}));
        REQUIRE_EQUAL(layout.cellRect(0, 0, bgeo::BlockPosition{10, 20}, border), (bgeo::BlockRectangle{11, 21, 2, 1}));
        REQUIRE_EQUAL(layout.cellRect(1, 1, bgeo::BlockPosition{10, 20}, border), (bgeo::BlockRectangle{14, 23, 3, 2}));
    }

    void testUnsupportedBorderStylesDoNotAddLineCells() {
        const auto layout = gridLayout({2, 3}, {1, 2});
        const auto border = FrameBorder{FrameStyle::FullBlock};

        REQUIRE_EQUAL(layout.size(border), (bgeo::BlockSize{5, 3}));
        REQUIRE_EQUAL(layout.cellRect(1, 1, bgeo::BlockPosition{10, 20}, border), (bgeo::BlockRectangle{12, 21, 3, 2}));
    }

    void testOmittedLinesDoNotContributeToSizeOrCellPositions() {
        const auto layout = gridLayout({2, 3}, {1, 2});
        auto border = FrameBorder{};
        border.set(FrameBorder::Element::VLine, FrameStyle::Light);

        REQUIRE_EQUAL(layout.size(border), (bgeo::BlockSize{6, 3}));
        REQUIRE_EQUAL(layout.cellRect(0, 0, bgeo::BlockPosition{10, 20}, border), (bgeo::BlockRectangle{10, 20, 2, 1}));
        REQUIRE_EQUAL(layout.cellRect(1, 1, bgeo::BlockPosition{10, 20}, border), (bgeo::BlockRectangle{13, 21, 3, 2}));
    }

    void testConstructorRejectsInvalidDimensions() {
        REQUIRE_THROWS_AS(erbsland::err::ParameterError, gridLayout({}, {1}));
        REQUIRE_THROWS_AS(erbsland::err::ParameterError, gridLayout({1}, {}));
        REQUIRE_THROWS_AS(erbsland::err::ParameterError, gridLayout({1, 0}, {1}));
        REQUIRE_THROWS_AS(erbsland::err::ParameterError, gridLayout({1}, {1, -1}));
    }

    void testCellRectRejectsInvalidIndexes() {
        const auto layout = gridLayout({2}, {1});
        const auto border = FrameBorder{};

        REQUIRE_THROWS_AS(erbsland::err::OutOfRangeError, layout.cellRect(1, 0, bgeo::BlockPosition{}, border));
        REQUIRE_THROWS_AS(erbsland::err::OutOfRangeError, layout.cellRect(0, 1, bgeo::BlockPosition{}, border));
    }
};
