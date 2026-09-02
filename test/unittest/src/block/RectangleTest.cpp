// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/block/AlignedSource.hpp>
#include <erbsland/block/Rectangle.hpp>
#include <erbsland/block/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <functional>
#include <vector>

TESTED_TARGETS(Rectangle RectangleList AlignedSource)
class RectangleTest final : public el::UnitTest {
public:
    void testConstructorsAndAccessors() {
        using el::block::Position;
        using el::block::Rectangle;
        using el::block::Size;

        auto rect = Rectangle{2, 3, 10, 20};

        REQUIRE_EQUAL(rect.pos(), (Position{2, 3}));
        REQUIRE_EQUAL(rect.size(), (Size{10, 20}));
        REQUIRE_EQUAL(rect.x1(), 2);
        REQUIRE_EQUAL(rect.y1(), 3);
        REQUIRE_EQUAL(rect.x2(), 12);
        REQUIRE_EQUAL(rect.y2(), 23);
        REQUIRE_EQUAL(rect.topLeft(), (Position{2, 3}));
        REQUIRE_EQUAL(rect.bottomRight(), (Position{12, 23}));

        rect.setPos(Position{-4, 5});
        rect.setSize(Size{7, 8});
        REQUIRE_EQUAL(rect, (Rectangle{-4, 5, 7, 8}));
        const auto fromPositions = Rectangle{Position{3, 4}, Position{9, 2}};
        REQUIRE_EQUAL(fromPositions, (Rectangle{3, 4, 6, 0}));
    }

    void testAnchorAndAlignment() {
        using el::block::Position;
        using el::block::Rectangle;
        using el::block::Size;
        using el::geometry::Alignment;
        using el::geometry::Anchor;

        const auto rect = Rectangle{10, 20, 11, 7};

        REQUIRE_EQUAL(rect.anchor(Anchor::TopLeft), (Position{10, 20}));
        REQUIRE_EQUAL(rect.anchor(Anchor::Center), (Position{15, 23}));
        REQUIRE_EQUAL(rect.anchor(Anchor::BottomRight), (Position{20, 26}));
        REQUIRE_EQUAL(rect.center(), (Position{15, 23}));
        REQUIRE_EQUAL(rect.alignmentOffset(Size{5, 3}, Alignment::Center), (Position{13, 22}));
        REQUIRE_EQUAL(rect.alignmentOffset(Size{13, 9}, Alignment::BottomRight), (Position{8, 18}));
    }

    void testContainmentOverlapAndFrame() {
        using el::block::Position;
        using el::block::Rectangle;

        const auto rect = Rectangle{10, 20, 4, 3};

        REQUIRE(rect.contains(Position{10, 20}));
        REQUIRE(rect.contains(Position{13, 22}));
        REQUIRE_FALSE(rect.contains(Position{14, 22}));
        REQUIRE(rect.contains(Rectangle{11, 21, 2, 1}));
        REQUIRE_FALSE(rect.contains(Rectangle{13, 21, 2, 1}));
        REQUIRE(rect.overlaps(Rectangle{13, 22, 3, 3}));
        REQUIRE_FALSE(rect.overlaps(Rectangle{14, 22, 3, 3}));
        REQUIRE(rect.isFrame(Position{11, 20}));
        REQUIRE(rect.isFrame(Position{10, 21}));
        REQUIRE_FALSE(rect.isFrame(Position{11, 21}));
    }

    void testMergeAndIntersection() {
        using el::block::Rectangle;

        const auto first = Rectangle{0, 0, 5, 5};
        const auto second = Rectangle{3, 2, 6, 5};

        const auto merged = first | second;
        const auto intersected = first & second;
        const auto disjoint = first & Rectangle{10, 10, 2, 2};
        REQUIRE_EQUAL(merged, (Rectangle{0, 0, 9, 7}));
        REQUIRE_EQUAL(intersected, (Rectangle{3, 2, 2, 3}));
        REQUIRE_EQUAL(disjoint, Rectangle{});
    }

    void testClampHandlesEmptyAxes() {
        using el::block::Position;
        using el::block::Rectangle;

        const auto regular = Rectangle{10, 20, 4, 3}.clamp(Position{99, 1});
        const auto zeroWidth = Rectangle{10, 20, 0, 3}.clamp(Position{99, 99});
        const auto zeroHeight = Rectangle{10, 20, 4, 0}.clamp(Position{99, 99});
        const auto empty = Rectangle{10, 20, 0, 0}.clamp(Position{99, 99});
        REQUIRE_EQUAL(regular, (Position{13, 20}));
        REQUIRE_EQUAL(zeroWidth, (Position{10, 22}));
        REQUIRE_EQUAL(zeroHeight, (Position{13, 20}));
        REQUIRE_EQUAL(empty, (Position{10, 20}));
    }

    void testMarginsAndSubRectangle() {
        using el::block::Margins;
        using el::block::Rectangle;
        using el::block::Size;
        using el::geometry::Anchor;

        const auto rect = Rectangle{10, 20, 20, 10};

        const auto expanded = rect.expandedBy(Margins{1, 2, 3, 4});
        const auto inset = rect.insetBy(Margins{1, 2, 3, 4});
        const auto bottomRight = rect.subRectangle(Anchor::BottomRight, Size{5, 4}, Margins{1});
        const auto center = rect.subRectangle(Anchor::Center, Size{0, 0}, Margins{2});
        REQUIRE_EQUAL(expanded, (Rectangle{6, 19, 26, 14}));
        REQUIRE_EQUAL(inset, (Rectangle{14, 21, 14, 6}));
        REQUIRE_EQUAL(bottomRight, (Rectangle{24, 25, 5, 4}));
        REQUIRE_EQUAL(center, (Rectangle{12, 22, 16, 6}));
    }

    void testAlignedSource() {
        using el::block::AlignedSource;
        using el::block::Rectangle;
        using el::geometry::Alignment;

        const auto target = Rectangle{10, 20, 6, 4};

        const auto centered = target.alignedSource(Rectangle{0, 0, 4, 2}, Alignment::Center);
        const auto bottomRight = target.alignedSource(Rectangle{100, 200, 10, 8}, Alignment::BottomRight);
        REQUIRE_EQUAL(centered, (AlignedSource{Rectangle{11, 21, 4, 2}, Rectangle{0, 0, 4, 2}}));
        REQUIRE_EQUAL(bottomRight, (AlignedSource{Rectangle{10, 20, 6, 4}, Rectangle{104, 204, 6, 4}}));
    }

    void testFrameIndexDirectionAndIteration() {
        using el::block::Direction;
        using el::block::Position;
        using el::block::Rectangle;

        const auto rect = Rectangle{10, 20, 4, 3};

        REQUIRE_EQUAL(rect.frameIndex(Position{10, 20}), 0);
        REQUIRE_EQUAL(rect.frameIndex(Position{13, 20}), 3);
        REQUIRE_EQUAL(rect.frameIndex(Position{13, 21}), 4);
        REQUIRE_EQUAL(rect.frameIndex(Position{13, 22}), 5);
        REQUIRE_EQUAL(rect.frameIndex(Position{10, 21}), 9);
        REQUIRE_EQUAL(rect.frameIndex(Position{11, 21}), -1);
        REQUIRE_EQUAL(rect.frameDirection(Position{10, 20}), Direction::NorthWest);
        REQUIRE_EQUAL(rect.frameDirection(Position{13, 21}), Direction::East);
        const auto columnFrameIndex = Rectangle{0, 0, 1, 5}.frameIndex(Position{0, 4});
        REQUIRE_EQUAL(columnFrameIndex, 4);

        auto visited = std::vector<Position>{};
        rect.forEachInFrame([&](Position position, int index) -> void {
            REQUIRE_EQUAL(rect.frameIndex(position), index);
            visited.push_back(position);
        });
        REQUIRE_EQUAL(visited.size(), 10U);
        REQUIRE_EQUAL(visited.front(), (Position{10, 20}));
        REQUIRE_EQUAL(visited.back(), (Position{10, 21}));
    }

    void testGridCellsAndValidation() {
        using el::block::Rectangle;

        const auto rectangle = Rectangle{0, 0, 11, 5};
        const auto cells = rectangle.gridCells(2, 3, 1, 1);

        REQUIRE_EQUAL(cells.size(), 6U);
        REQUIRE_EQUAL(cells[0], (Rectangle{0, 0, 3, 2}));
        REQUIRE_EQUAL(cells[1], (Rectangle{4, 0, 3, 2}));
        REQUIRE_EQUAL(cells[2], (Rectangle{8, 0, 3, 2}));
        REQUIRE_EQUAL(cells[3], (Rectangle{0, 3, 3, 2}));
        const auto smallRectangle = Rectangle{0, 0, 3, 3};
        REQUIRE_THROWS(smallRectangle.gridCells(0, 1));
        REQUIRE_THROWS(smallRectangle.gridCells(1, 0));
        REQUIRE_THROWS(smallRectangle.gridCells(2, 2, 2, 2));
        REQUIRE_THROWS(smallRectangle.gridCells(1, 1, -1, 0));
    }

    void testTransforms() {
        using el::block::Position;
        using el::block::Rectangle;
        using el::geometry::Orientation;
        using el::geometry::Symmetry;

        const auto rect = Rectangle{10, 20, 4, 3};

        REQUIRE_EQUAL(rect.rotateCCW(Position{11, 20}, 1), (Position{10, 22}));
        REQUIRE_EQUAL(rect.rotateCCW(Position{11, 20}, -1), (Position{12, 21}));
        REQUIRE_EQUAL(rect.mirror(Position{11, 22}, Orientation::Horizontal), (Position{12, 22}));
        REQUIRE_EQUAL(rect.mirror(Position{11, 22}, Orientation::Vertical), (Position{11, 20}));

        REQUIRE_EQUAL(rect.transform(Position{11, 20}, Symmetry::Rotate180), (Position{12, 22}));
        REQUIRE_EQUAL(rect.transform(Position{11, 20}, Symmetry::MirrorDiagonal), (Position{10, 21}));
        REQUIRE_EQUAL(rect.transform(Position{11, 20}, Symmetry::MirrorAntiDiagonal), (Position{12, 22}));
    }

    void testForEachBoundsAndHash() {
        using el::block::Position;
        using el::block::PositionList;
        using el::block::Rectangle;

        auto visited = std::vector<Position>{};
        const auto rect = Rectangle{2, 3, 2, 2};
        rect.forEach([&](const Position position) -> void { visited.push_back(position); });

        REQUIRE_EQUAL(visited.size(), 4U);
        REQUIRE_EQUAL(visited[0], (Position{2, 3}));
        REQUIRE_EQUAL(visited[3], (Position{3, 4}));
        const auto bounds = Rectangle::bounds(PositionList{{3, 5}, {-2, 4}, {10, 9}});
        const auto emptyBounds = Rectangle::bounds(PositionList{});
        const auto hashRect = Rectangle{2, 3, 4, 5};
        const auto hash = std::hash<Rectangle>{}(hashRect);
        REQUIRE_EQUAL(bounds, (Rectangle{-2, 4, 13, 6}));
        REQUIRE_EQUAL(emptyBounds, Rectangle{});
        REQUIRE_EQUAL(hash, hashRect.hash());
    }
};
