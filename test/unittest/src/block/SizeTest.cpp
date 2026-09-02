// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/block/Size.hpp>
#include <erbsland/block/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <limits>
#include <vector>

TESTED_TARGETS(Size)
class SizeTest final : public el::UnitTest {
public:
    void testConstructorsAndNonNegativeInvariant() {
        using el::block::Position;
        using el::block::Size;
        using el::geometry::Orientation;

        auto size = Size{-4, 8};
        REQUIRE_EQUAL(size.width(), 0);
        REQUIRE_EQUAL(size.height(), 8);

        size.setWidth(12);
        size.setHeight(-5);
        REQUIRE_EQUAL(size, (Size{12, 0}));
        REQUIRE_EQUAL(size.component(Orientation::Horizontal), 12);
        REQUIRE_EQUAL(size.component(Orientation::Vertical), 0);
        const auto fromPositions = Size{Position{-2, 10}, Position{5, 4}};
        REQUIRE_EQUAL(fromPositions, (Size{7, 6}));
    }

    void testAnchorsAndAlignmentOffsets() {
        using el::block::Position;
        using el::block::Size;
        using el::geometry::Alignment;
        using el::geometry::Anchor;

        const auto size = Size{11, 7};

        REQUIRE_EQUAL(size.anchor(Anchor::TopLeft), (Position{0, 0}));
        REQUIRE_EQUAL(size.anchor(Anchor::Center), (Position{5, 3}));
        REQUIRE_EQUAL(size.anchor(Anchor::BottomRight), (Position{10, 6}));
        const auto emptyAnchor = Size{}.anchor(Anchor::BottomRight);
        REQUIRE_EQUAL(emptyAnchor, (Position{0, 0}));
        REQUIRE_EQUAL(size.alignmentOffset(Size{5, 3}, Alignment::TopLeft), (Position{0, 0}));
        REQUIRE_EQUAL(size.alignmentOffset(Size{5, 3}, Alignment::Center), (Position{3, 2}));
        REQUIRE_EQUAL(size.alignmentOffset(Size{5, 3}, Alignment::BottomRight), (Position{6, 4}));
        REQUIRE_EQUAL(size.alignmentOffset(Size{13, 9}, Alignment::BottomRight), (Position{-2, -2}));
    }

    void testFitsContainsClampAndIndex() {
        using el::block::Position;
        using el::block::Size;

        const auto size = Size{4, 3};

        REQUIRE(size.fitsInto(Size{4, 5}));
        REQUIRE_FALSE(Size{5, 3}.fitsInto(size));
        REQUIRE(size.contains(Position{3, 2}));
        REQUIRE_FALSE(size.contains(Position{4, 2}));
        REQUIRE_EQUAL(size.clamp(Position{-5, 7}), (Position{0, 2}));
        const auto zeroWidthClamp = Size{0, 3}.clamp(Position{5, 2});
        const auto zeroHeightClamp = Size{4, 0}.clamp(Position{2, 5});
        REQUIRE_EQUAL(zeroWidthClamp, (Position{0, 2}));
        REQUIRE_EQUAL(zeroHeightClamp, (Position{2, 0}));
        REQUIRE_EQUAL(size.index(Position{2, 1}), 6U);
    }

    void testConstraintOperationsPreserveInvariants() {
        using el::block::Coordinate;
        using el::block::Size;
        using el::geometry::Orientation;

        const auto maximum = Coordinate::maximum();

        const auto sum = Size{2, 3} + Size{4, 5};
        const auto difference = Size{2, 3} - Size{4, 1};
        const auto saturated = Size{maximum, maximum} + Size{1, 1};
        REQUIRE_EQUAL(sum, (Size{6, 8}));
        REQUIRE_EQUAL(difference, (Size{0, 2}));
        REQUIRE_EQUAL(saturated, (Size{maximum, maximum}));

        auto size = Size{2, 3};
        size.subtract(Size{7, 8}, Orientation::Horizontal);
        REQUIRE_EQUAL(size, (Size{0, 3}));
        size.subtract(Size{7, 8}, Orientation::Vertical);
        REQUIRE_EQUAL(size, (Size{0, 0}));

        const auto expanded = Size{2, 9}.expandedWith(Size{5, 4});
        const auto limited = Size{2, 9}.limitedWith(Size{5, 4});
        const auto clamped = Size{7, 3}.clampTo(Size{2, 2}, Size{5, 4});
        REQUIRE_EQUAL(expanded, (Size{5, 9}));
        REQUIRE_EQUAL(limited, (Size{2, 4}));
        REQUIRE_EQUAL(clamped, (Size{5, 3}));
    }

    void testTransforms() {
        using el::block::Position;
        using el::block::Size;
        using el::geometry::Orientation;
        using el::geometry::Symmetry;

        const auto size = Size{4, 3};

        REQUIRE_EQUAL(size.rotateCCW(Position{1, 0}, 0), (Position{1, 0}));
        REQUIRE_EQUAL(size.rotateCCW(Position{1, 0}, 1), (Position{0, 2}));
        REQUIRE_EQUAL(size.rotateCCW(Position{1, 0}, 2), (Position{2, 2}));
        REQUIRE_EQUAL(size.rotateCCW(Position{1, 0}, -1), (Position{2, 1}));
        REQUIRE_EQUAL(size.rotateCCW(Position{5, -1}, 1), (Position{-1, -2}));

        REQUIRE_EQUAL(size.mirror(Position{1, 2}, Orientation::Horizontal), (Position{2, 2}));
        REQUIRE_EQUAL(size.mirror(Position{1, 2}, Orientation::Vertical), (Position{1, 0}));

        REQUIRE_EQUAL(size.transform(Position{1, 2}, Symmetry::Identity), (Position{1, 2}));
        REQUIRE_EQUAL(size.transform(Position{1, 2}, Symmetry::Rotate90), (Position{2, 2}));
        REQUIRE_EQUAL(size.transform(Position{1, 2}, Symmetry::MirrorDiagonal), (Position{2, 1}));
        REQUIRE_EQUAL(size.transform(Position{1, 2}, Symmetry::MirrorAntiDiagonal), (Position{0, 2}));
    }

    void testForEach() {
        using el::block::Position;
        using el::block::Size;

        auto visited = std::vector<Position>{};
        Size{2, 2}.forEach([&](Position position) -> void { visited.push_back(position); });

        REQUIRE_EQUAL(visited.size(), 4U);
        REQUIRE_EQUAL(visited[0], (Position{0, 0}));
        REQUIRE_EQUAL(visited[3], (Position{1, 1}));
    }
};
