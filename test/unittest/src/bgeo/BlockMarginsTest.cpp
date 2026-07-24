// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/bgeo/BlockMargins.hpp>
#include <erbsland/bgeo/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

TESTED_TARGETS(BlockMargins)
class BlockMarginsTest final : public el::UnitTest {
public:
    void testConstructorsAndAccessors() {
        using el::bgeo::BlockMargins;

        REQUIRE_EQUAL(BlockMargins{5}, (BlockMargins{5, 5, 5, 5}));
        REQUIRE_EQUAL((BlockMargins{2, 7}), (BlockMargins{7, 2, 7, 2}));

        auto margins = BlockMargins{1, 2, 3, 4};
        REQUIRE_EQUAL(margins.top(), 1);
        REQUIRE_EQUAL(margins.right(), 2);
        REQUIRE_EQUAL(margins.bottom(), 3);
        REQUIRE_EQUAL(margins.left(), 4);

        margins.setTop(5);
        margins.setRight(6);
        margins.setBottom(7);
        margins.setLeft(8);
        REQUIRE_EQUAL(margins, (BlockMargins{5, 6, 7, 8}));
    }

    void testSideAndOrientationHelpers() {
        using el::bgeo::BlockMargins;
        using el::bgeo::Orientation;

        const auto margins = BlockMargins{1, 2, 3, 4};

        REQUIRE_EQUAL(margins[BlockMargins::Side::Top], 1);
        REQUIRE_EQUAL(margins[BlockMargins::Side::Right], 2);
        REQUIRE_EQUAL(margins[BlockMargins::Side::Bottom], 3);
        REQUIRE_EQUAL(margins[BlockMargins::Side::Left], 4);
        REQUIRE_EQUAL(margins.leading(Orientation::Horizontal), 4);
        REQUIRE_EQUAL(margins.trailing(Orientation::Horizontal), 2);
        REQUIRE_EQUAL(margins.leading(Orientation::Vertical), 1);
        REQUIRE_EQUAL(margins.trailing(Orientation::Vertical), 3);
        REQUIRE_EQUAL(margins.crossLeading(Orientation::Horizontal), 1);
        REQUIRE_EQUAL(margins.crossTrailing(Orientation::Horizontal), 3);
    }

    void testExtentsSpacingAndDeltas() {
        using el::bgeo::BlockMargins;
        using el::bgeo::BlockSize;
        using el::bgeo::Orientation;

        const auto margins = BlockMargins{-3, 4, 5, 6};

        REQUIRE_EQUAL(margins.horizontal(), (BlockMargins{0, 4, 0, 6}));
        REQUIRE_EQUAL(margins.vertical(), (BlockMargins{-3, 0, 5, 0}));
        REQUIRE_EQUAL(margins.horizontalExtent(), 10);
        REQUIRE_EQUAL(margins.verticalExtent(), 5);
        REQUIRE_EQUAL(margins.extent(), (BlockSize{10, 5}));
        REQUIRE_EQUAL(margins.spacing(), (BlockSize{6, 5}));
        REQUIRE_EQUAL(margins.extent(Orientation::Horizontal), 10);
        REQUIRE_EQUAL(margins.spacing(Orientation::Vertical), 5);
        REQUIRE_EQUAL(margins.horizontalDelta(), 10);
        REQUIRE_EQUAL(margins.verticalDelta(), 2);
    }

    void testExpansionAndLimiting() {
        using el::bgeo::BlockMargins;
        using el::bgeo::Orientation;

        auto margins = BlockMargins{1, 5, -2, 4};
        margins.expandTo(BlockMargins{3, 2, 8, 1});
        REQUIRE_EQUAL(margins, (BlockMargins{3, 5, 8, 4}));

        margins.limitTo(BlockMargins{2, 4, 6, 9}, Orientation::Horizontal);
        REQUIRE_EQUAL(margins, (BlockMargins{3, 4, 8, 4}));

        REQUIRE_EQUAL((BlockMargins{-1, 2, -3, 4}.expandedPositive()), (BlockMargins{0, 2, 0, 4}));
        REQUIRE_EQUAL((BlockMargins{1, 5, 3, 7}.limitedWith(BlockMargins{4, 2, 9, 6})), (BlockMargins{1, 2, 3, 6}));
    }

    void testUnaryNegation() {
        using el::bgeo::BlockMargins;

        REQUIRE_EQUAL(-BlockMargins(1, 2, 3, 4), (BlockMargins{-1, -2, -3, -4}));
    }
};
