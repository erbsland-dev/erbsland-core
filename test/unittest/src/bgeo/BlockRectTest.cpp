// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/bgeo/BlockAlignedSource.hpp>
#include <erbsland/bgeo/BlockRectangle.hpp>
#include <erbsland/bgeo/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <functional>
#include <vector>

TESTED_TARGETS(BlockRect BlockRectList BlockAlignedSource)
class BlockRectTest final : public el::UnitTest {
public:
    void testConstructorsAndAccessors() {
        using el::bgeo::BlockPosition;
        using el::bgeo::BlockRectangle;
        using el::bgeo::BlockSize;

        auto rect = BlockRectangle{2, 3, 10, 20};

        REQUIRE_EQUAL(rect.pos(), (BlockPosition{2, 3}));
        REQUIRE_EQUAL(rect.size(), (BlockSize{10, 20}));
        REQUIRE_EQUAL(rect.x1(), 2);
        REQUIRE_EQUAL(rect.y1(), 3);
        REQUIRE_EQUAL(rect.x2(), 12);
        REQUIRE_EQUAL(rect.y2(), 23);
        REQUIRE_EQUAL(rect.topLeft(), (BlockPosition{2, 3}));
        REQUIRE_EQUAL(rect.bottomRight(), (BlockPosition{12, 23}));

        rect.setPos(BlockPosition{-4, 5});
        rect.setSize(BlockSize{7, 8});
        REQUIRE_EQUAL(rect, (BlockRectangle{-4, 5, 7, 8}));
        const auto fromPositions = BlockRectangle{BlockPosition{3, 4}, BlockPosition{9, 2}};
        REQUIRE_EQUAL(fromPositions, (BlockRectangle{3, 4, 6, 0}));
    }

    void testAnchorAndAlignment() {
        using el::bgeo::Alignment;
        using el::bgeo::BlockAnchor;
        using el::bgeo::BlockPosition;
        using el::bgeo::BlockRectangle;
        using el::bgeo::BlockSize;

        const auto rect = BlockRectangle{10, 20, 11, 7};

        REQUIRE_EQUAL(rect.anchor(BlockAnchor::TopLeft), (BlockPosition{10, 20}));
        REQUIRE_EQUAL(rect.anchor(BlockAnchor::Center), (BlockPosition{15, 23}));
        REQUIRE_EQUAL(rect.anchor(BlockAnchor::BottomRight), (BlockPosition{20, 26}));
        REQUIRE_EQUAL(rect.center(), (BlockPosition{15, 23}));
        REQUIRE_EQUAL(rect.alignmentOffset(BlockSize{5, 3}, Alignment::Center), (BlockPosition{13, 22}));
        REQUIRE_EQUAL(rect.alignmentOffset(BlockSize{13, 9}, Alignment::BottomRight), (BlockPosition{8, 18}));
    }

    void testContainmentOverlapAndFrame() {
        using el::bgeo::BlockPosition;
        using el::bgeo::BlockRectangle;

        const auto rect = BlockRectangle{10, 20, 4, 3};

        REQUIRE(rect.contains(BlockPosition{10, 20}));
        REQUIRE(rect.contains(BlockPosition{13, 22}));
        REQUIRE_FALSE(rect.contains(BlockPosition{14, 22}));
        REQUIRE(rect.contains(BlockRectangle{11, 21, 2, 1}));
        REQUIRE_FALSE(rect.contains(BlockRectangle{13, 21, 2, 1}));
        REQUIRE(rect.overlaps(BlockRectangle{13, 22, 3, 3}));
        REQUIRE_FALSE(rect.overlaps(BlockRectangle{14, 22, 3, 3}));
        REQUIRE(rect.isFrame(BlockPosition{11, 20}));
        REQUIRE(rect.isFrame(BlockPosition{10, 21}));
        REQUIRE_FALSE(rect.isFrame(BlockPosition{11, 21}));
    }

    void testMergeAndIntersection() {
        using el::bgeo::BlockRectangle;

        const auto first = BlockRectangle{0, 0, 5, 5};
        const auto second = BlockRectangle{3, 2, 6, 5};

        const auto merged = first | second;
        const auto intersected = first & second;
        const auto disjoint = first & BlockRectangle{10, 10, 2, 2};
        REQUIRE_EQUAL(merged, (BlockRectangle{0, 0, 9, 7}));
        REQUIRE_EQUAL(intersected, (BlockRectangle{3, 2, 2, 3}));
        REQUIRE_EQUAL(disjoint, BlockRectangle{});
    }

    void testClampHandlesEmptyAxes() {
        using el::bgeo::BlockPosition;
        using el::bgeo::BlockRectangle;

        const auto regular = BlockRectangle{10, 20, 4, 3}.clamp(BlockPosition{99, 1});
        const auto zeroWidth = BlockRectangle{10, 20, 0, 3}.clamp(BlockPosition{99, 99});
        const auto zeroHeight = BlockRectangle{10, 20, 4, 0}.clamp(BlockPosition{99, 99});
        const auto empty = BlockRectangle{10, 20, 0, 0}.clamp(BlockPosition{99, 99});
        REQUIRE_EQUAL(regular, (BlockPosition{13, 20}));
        REQUIRE_EQUAL(zeroWidth, (BlockPosition{10, 22}));
        REQUIRE_EQUAL(zeroHeight, (BlockPosition{13, 20}));
        REQUIRE_EQUAL(empty, (BlockPosition{10, 20}));
    }

    void testMarginsAndSubRectangle() {
        using el::bgeo::BlockAnchor;
        using el::bgeo::BlockMargins;
        using el::bgeo::BlockRectangle;
        using el::bgeo::BlockSize;

        const auto rect = BlockRectangle{10, 20, 20, 10};

        const auto expanded = rect.expandedBy(BlockMargins{1, 2, 3, 4});
        const auto inset = rect.insetBy(BlockMargins{1, 2, 3, 4});
        const auto bottomRight = rect.subRectangle(BlockAnchor::BottomRight, BlockSize{5, 4}, BlockMargins{1});
        const auto center = rect.subRectangle(BlockAnchor::Center, BlockSize{0, 0}, BlockMargins{2});
        REQUIRE_EQUAL(expanded, (BlockRectangle{6, 19, 26, 14}));
        REQUIRE_EQUAL(inset, (BlockRectangle{14, 21, 14, 6}));
        REQUIRE_EQUAL(bottomRight, (BlockRectangle{24, 25, 5, 4}));
        REQUIRE_EQUAL(center, (BlockRectangle{12, 22, 16, 6}));
    }

    void testAlignedSource() {
        using el::bgeo::Alignment;
        using el::bgeo::BlockAlignedSource;
        using el::bgeo::BlockRectangle;

        const auto target = BlockRectangle{10, 20, 6, 4};

        const auto centered = target.alignedSource(BlockRectangle{0, 0, 4, 2}, Alignment::Center);
        const auto bottomRight = target.alignedSource(BlockRectangle{100, 200, 10, 8}, Alignment::BottomRight);
        REQUIRE_EQUAL(centered, (BlockAlignedSource{BlockRectangle{11, 21, 4, 2}, BlockRectangle{0, 0, 4, 2}}));
        REQUIRE_EQUAL(bottomRight, (BlockAlignedSource{BlockRectangle{10, 20, 6, 4}, BlockRectangle{104, 204, 6, 4}}));
    }

    void testFrameIndexDirectionAndIteration() {
        using el::bgeo::BlockDirection;
        using el::bgeo::BlockPosition;
        using el::bgeo::BlockRectangle;

        const auto rect = BlockRectangle{10, 20, 4, 3};

        REQUIRE_EQUAL(rect.frameIndex(BlockPosition{10, 20}), 0);
        REQUIRE_EQUAL(rect.frameIndex(BlockPosition{13, 20}), 3);
        REQUIRE_EQUAL(rect.frameIndex(BlockPosition{13, 21}), 4);
        REQUIRE_EQUAL(rect.frameIndex(BlockPosition{13, 22}), 5);
        REQUIRE_EQUAL(rect.frameIndex(BlockPosition{10, 21}), 9);
        REQUIRE_EQUAL(rect.frameIndex(BlockPosition{11, 21}), -1);
        REQUIRE_EQUAL(rect.frameDirection(BlockPosition{10, 20}), BlockDirection::NorthWest);
        REQUIRE_EQUAL(rect.frameDirection(BlockPosition{13, 21}), BlockDirection::East);
        const auto columnFrameIndex = BlockRectangle{0, 0, 1, 5}.frameIndex(BlockPosition{0, 4});
        REQUIRE_EQUAL(columnFrameIndex, 4);

        auto visited = std::vector<BlockPosition>{};
        rect.forEachInFrame([&](BlockPosition position, int index) -> void {
            REQUIRE_EQUAL(rect.frameIndex(position), index);
            visited.push_back(position);
        });
        REQUIRE_EQUAL(visited.size(), 10U);
        REQUIRE_EQUAL(visited.front(), (BlockPosition{10, 20}));
        REQUIRE_EQUAL(visited.back(), (BlockPosition{10, 21}));
    }

    void testGridCellsAndValidation() {
        using el::bgeo::BlockRectangle;

        const auto rectangle = BlockRectangle{0, 0, 11, 5};
        const auto cells = rectangle.gridCells(2, 3, 1, 1);

        REQUIRE_EQUAL(cells.size(), 6U);
        REQUIRE_EQUAL(cells[0], (BlockRectangle{0, 0, 3, 2}));
        REQUIRE_EQUAL(cells[1], (BlockRectangle{4, 0, 3, 2}));
        REQUIRE_EQUAL(cells[2], (BlockRectangle{8, 0, 3, 2}));
        REQUIRE_EQUAL(cells[3], (BlockRectangle{0, 3, 3, 2}));
        const auto smallRectangle = BlockRectangle{0, 0, 3, 3};
        REQUIRE_THROWS(smallRectangle.gridCells(0, 1));
        REQUIRE_THROWS(smallRectangle.gridCells(1, 0));
        REQUIRE_THROWS(smallRectangle.gridCells(2, 2, 2, 2));
        REQUIRE_THROWS(smallRectangle.gridCells(1, 1, -1, 0));
    }

    void testTransforms() {
        using el::bgeo::BlockPosition;
        using el::bgeo::BlockRectangle;
        using el::bgeo::Orientation;
        using el::bgeo::Symmetry;

        const auto rect = BlockRectangle{10, 20, 4, 3};

        REQUIRE_EQUAL(rect.rotateCCW(BlockPosition{11, 20}, 1), (BlockPosition{10, 22}));
        REQUIRE_EQUAL(rect.rotateCCW(BlockPosition{11, 20}, -1), (BlockPosition{12, 21}));
        REQUIRE_EQUAL(rect.mirror(BlockPosition{11, 22}, Orientation::Horizontal), (BlockPosition{12, 22}));
        REQUIRE_EQUAL(rect.mirror(BlockPosition{11, 22}, Orientation::Vertical), (BlockPosition{11, 20}));

        REQUIRE_EQUAL(rect.transform(BlockPosition{11, 20}, Symmetry::Rotate180), (BlockPosition{12, 22}));
        REQUIRE_EQUAL(rect.transform(BlockPosition{11, 20}, Symmetry::MirrorDiagonal), (BlockPosition{10, 21}));
        REQUIRE_EQUAL(rect.transform(BlockPosition{11, 20}, Symmetry::MirrorAntiDiagonal), (BlockPosition{12, 22}));
    }

    void testForEachBoundsAndHash() {
        using el::bgeo::BlockPosition;
        using el::bgeo::BlockPositionList;
        using el::bgeo::BlockRectangle;

        auto visited = std::vector<BlockPosition>{};
        const auto rect = BlockRectangle{2, 3, 2, 2};
        rect.forEach([&](const BlockPosition position) -> void { visited.push_back(position); });

        REQUIRE_EQUAL(visited.size(), 4U);
        REQUIRE_EQUAL(visited[0], (BlockPosition{2, 3}));
        REQUIRE_EQUAL(visited[3], (BlockPosition{3, 4}));
        const auto bounds = BlockRectangle::bounds(BlockPositionList{{3, 5}, {-2, 4}, {10, 9}});
        const auto emptyBounds = BlockRectangle::bounds(BlockPositionList{});
        const auto hashRect = BlockRectangle{2, 3, 4, 5};
        const auto hash = std::hash<BlockRectangle>{}(hashRect);
        REQUIRE_EQUAL(bounds, (BlockRectangle{-2, 4, 13, 6}));
        REQUIRE_EQUAL(emptyBounds, BlockRectangle{});
        REQUIRE_EQUAL(hash, hashRect.hash());
    }
};
