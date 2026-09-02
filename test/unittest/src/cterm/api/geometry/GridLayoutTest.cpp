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

        REQUIRE_EQUAL(layout.size(border), (block::Size{5, 3}));
        REQUIRE_EQUAL(layout.cellRect(1, 1, block::Position{10, 20}, border), (block::Rectangle{12, 21, 3, 2}));
    }

    void testActiveBorderAddsOuterAndSeparatorLineCells() {
        const auto layout = gridLayout({2, 3}, {1, 2});
        const auto border = FrameBorder{FrameStyle::Light};

        REQUIRE_EQUAL(layout.size(border), (block::Size{8, 6}));
        REQUIRE_EQUAL(layout.cellRect(0, 0, block::Position{10, 20}, border), (block::Rectangle{11, 21, 2, 1}));
        REQUIRE_EQUAL(layout.cellRect(1, 1, block::Position{10, 20}, border), (block::Rectangle{14, 23, 3, 2}));
    }

    void testUnsupportedBorderStylesDoNotAddLineCells() {
        const auto layout = gridLayout({2, 3}, {1, 2});
        const auto border = FrameBorder{FrameStyle::FullBlock};

        REQUIRE_EQUAL(layout.size(border), (block::Size{5, 3}));
        REQUIRE_EQUAL(layout.cellRect(1, 1, block::Position{10, 20}, border), (block::Rectangle{12, 21, 3, 2}));
    }

    void testOmittedLinesDoNotContributeToSizeOrCellPositions() {
        const auto layout = gridLayout({2, 3}, {1, 2});
        auto border = FrameBorder{};
        border.set(FrameBorder::Element::VLine, FrameStyle::Light);

        REQUIRE_EQUAL(layout.size(border), (block::Size{6, 3}));
        REQUIRE_EQUAL(layout.cellRect(0, 0, block::Position{10, 20}, border), (block::Rectangle{10, 20, 2, 1}));
        REQUIRE_EQUAL(layout.cellRect(1, 1, block::Position{10, 20}, border), (block::Rectangle{13, 21, 3, 2}));
    }

    void testConstructorRejectsInvalidDimensions() {
        REQUIRE_THROWS_AS(erbsland::err::ParameterError, gridLayout({}, {1}));
        REQUIRE_THROWS_AS(erbsland::err::ParameterError, gridLayout({1}, {}));
        REQUIRE_THROWS_AS(erbsland::err::ParameterError, gridLayout({1, 0}, {1}));
        REQUIRE_THROWS_AS(erbsland::err::ParameterError, gridLayout({1}, {1, -1}));
    }

    void testConstructorReportsTheParameterName() {
        try {
            static_cast<void>(gridLayout({}, {1}));
            REQUIRE(false);
        } catch (const erbsland::err::ParameterError &error) {
            REQUIRE_EQUAL(error.toString(), "The size list must not be empty. (parameter: columnWidths)"_el);
        }
    }

    void testCellRectRejectsInvalidIndexes() {
        const auto layout = gridLayout({2}, {1});
        const auto border = FrameBorder{};

        REQUIRE_THROWS_AS(erbsland::err::OutOfRangeError, layout.cellRect(1, 0, block::Position{}, border));
        REQUIRE_THROWS_AS(erbsland::err::OutOfRangeError, layout.cellRect(0, 1, block::Position{}, border));
    }
};
