// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/bgeo/BlockSize.hpp>
#include <erbsland/bgeo/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <limits>
#include <vector>

TESTED_TARGETS(BlockSize)
class BlockSizeTest final : public el::UnitTest {
public:
    void testConstructorsAndNonNegativeInvariant() {
        using el::bgeo::BlockPosition;
        using el::bgeo::BlockSize;
        using el::bgeo::Orientation;

        auto size = BlockSize{-4, 8};
        REQUIRE_EQUAL(size.width(), 0);
        REQUIRE_EQUAL(size.height(), 8);

        size.setWidth(12);
        size.setHeight(-5);
        REQUIRE_EQUAL(size, (BlockSize{12, 0}));
        REQUIRE_EQUAL(size.coordinate(Orientation::Horizontal), 12);
        REQUIRE_EQUAL(size.coordinate(Orientation::Vertical), 0);
        const auto fromPositions = BlockSize{BlockPosition{-2, 10}, BlockPosition{5, 4}};
        REQUIRE_EQUAL(fromPositions, (BlockSize{7, 6}));
    }

    void testAnchorsAndAlignmentOffsets() {
        using el::bgeo::Alignment;
        using el::bgeo::BlockAnchor;
        using el::bgeo::BlockPosition;
        using el::bgeo::BlockSize;

        const auto size = BlockSize{11, 7};

        REQUIRE_EQUAL(size.anchor(BlockAnchor::TopLeft), (BlockPosition{0, 0}));
        REQUIRE_EQUAL(size.anchor(BlockAnchor::Center), (BlockPosition{5, 3}));
        REQUIRE_EQUAL(size.anchor(BlockAnchor::BottomRight), (BlockPosition{10, 6}));
        const auto emptyAnchor = BlockSize{}.anchor(BlockAnchor::BottomRight);
        REQUIRE_EQUAL(emptyAnchor, (BlockPosition{0, 0}));
        REQUIRE_EQUAL(size.alignmentOffset(BlockSize{5, 3}, Alignment::Center), (BlockPosition{3, 2}));
        REQUIRE_EQUAL(size.alignmentOffset(BlockSize{13, 9}, Alignment::BottomRight), (BlockPosition{-2, -2}));
    }

    void testFitsContainsClampAndIndex() {
        using el::bgeo::BlockPosition;
        using el::bgeo::BlockSize;

        const auto size = BlockSize{4, 3};

        REQUIRE(size.fitsInto(BlockSize{4, 5}));
        REQUIRE_FALSE(BlockSize{5, 3}.fitsInto(size));
        REQUIRE(size.contains(BlockPosition{3, 2}));
        REQUIRE_FALSE(size.contains(BlockPosition{4, 2}));
        REQUIRE_EQUAL(size.clamp(BlockPosition{-5, 7}), (BlockPosition{0, 2}));
        const auto zeroWidthClamp = BlockSize{0, 3}.clamp(BlockPosition{5, 2});
        const auto zeroHeightClamp = BlockSize{4, 0}.clamp(BlockPosition{2, 5});
        REQUIRE_EQUAL(zeroWidthClamp, (BlockPosition{0, 2}));
        REQUIRE_EQUAL(zeroHeightClamp, (BlockPosition{2, 0}));
        REQUIRE_EQUAL(size.index(BlockPosition{2, 1}), 6U);
    }

    void testConstraintOperationsPreserveInvariants() {
        using el::bgeo::BlockCoordinate;
        using el::bgeo::BlockSize;
        using el::bgeo::Orientation;

        const auto maximum = BlockCoordinate::maximum();

        const auto sum = BlockSize{2, 3} + BlockSize{4, 5};
        const auto difference = BlockSize{2, 3} - BlockSize{4, 1};
        const auto saturated = BlockSize{maximum, maximum} + BlockSize{1, 1};
        REQUIRE_EQUAL(sum, (BlockSize{6, 8}));
        REQUIRE_EQUAL(difference, (BlockSize{0, 2}));
        REQUIRE_EQUAL(saturated, (BlockSize{maximum, maximum}));

        auto size = BlockSize{2, 3};
        size.subtract(BlockSize{7, 8}, Orientation::Horizontal);
        REQUIRE_EQUAL(size, (BlockSize{0, 3}));
        size.subtract(BlockSize{7, 8}, Orientation::Vertical);
        REQUIRE_EQUAL(size, (BlockSize{0, 0}));

        const auto expanded = BlockSize{2, 9}.expandedWith(BlockSize{5, 4});
        const auto limited = BlockSize{2, 9}.limitedWith(BlockSize{5, 4});
        const auto clamped = BlockSize{7, 3}.clampTo(BlockSize{2, 2}, BlockSize{5, 4});
        REQUIRE_EQUAL(expanded, (BlockSize{5, 9}));
        REQUIRE_EQUAL(limited, (BlockSize{2, 4}));
        REQUIRE_EQUAL(clamped, (BlockSize{5, 3}));
    }

    void testTransforms() {
        using el::bgeo::BlockPosition;
        using el::bgeo::BlockSize;
        using el::bgeo::Orientation;
        using el::bgeo::Symmetry;

        const auto size = BlockSize{4, 3};

        REQUIRE_EQUAL(size.rotateCCW(BlockPosition{1, 0}, 0), (BlockPosition{1, 0}));
        REQUIRE_EQUAL(size.rotateCCW(BlockPosition{1, 0}, 1), (BlockPosition{0, 2}));
        REQUIRE_EQUAL(size.rotateCCW(BlockPosition{1, 0}, 2), (BlockPosition{2, 2}));
        REQUIRE_EQUAL(size.rotateCCW(BlockPosition{1, 0}, -1), (BlockPosition{2, 1}));
        REQUIRE_EQUAL(size.rotateCCW(BlockPosition{5, -1}, 1), (BlockPosition{-1, -2}));

        REQUIRE_EQUAL(size.mirror(BlockPosition{1, 2}, Orientation::Horizontal), (BlockPosition{2, 2}));
        REQUIRE_EQUAL(size.mirror(BlockPosition{1, 2}, Orientation::Vertical), (BlockPosition{1, 0}));

        REQUIRE_EQUAL(size.transform(BlockPosition{1, 2}, Symmetry::Identity), (BlockPosition{1, 2}));
        REQUIRE_EQUAL(size.transform(BlockPosition{1, 2}, Symmetry::Rotate90), (BlockPosition{2, 2}));
        REQUIRE_EQUAL(size.transform(BlockPosition{1, 2}, Symmetry::MirrorDiagonal), (BlockPosition{2, 1}));
        REQUIRE_EQUAL(size.transform(BlockPosition{1, 2}, Symmetry::MirrorAntiDiagonal), (BlockPosition{0, 2}));
    }

    void testForEach() {
        using el::bgeo::BlockPosition;
        using el::bgeo::BlockSize;

        auto visited = std::vector<BlockPosition>{};
        BlockSize{2, 2}.forEach([&](BlockPosition position) -> void { visited.push_back(position); });

        REQUIRE_EQUAL(visited.size(), 4U);
        REQUIRE_EQUAL(visited[0], (BlockPosition{0, 0}));
        REQUIRE_EQUAL(visited[3], (BlockPosition{1, 1}));
    }
};
