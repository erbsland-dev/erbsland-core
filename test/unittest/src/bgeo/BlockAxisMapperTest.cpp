// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/bgeo/BlockAxisMapper.hpp>
#include <erbsland/bgeo/StdFormatForBlock.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::bgeo;

using el::bgeo::BlockAxisMapper;
using el::bgeo::BlockPosition;
using el::bgeo::BlockSize;
using el::bgeo::Orientation;

static_assert(BlockAxisMapper{Orientation::Horizontal}.size(3, 7) == BlockSize{3, 7});
static_assert(BlockAxisMapper{Orientation::Vertical}.size(3, 7) == BlockSize{7, 3});
static_assert(BlockAxisMapper{Orientation::Horizontal}.position(3, 7) == BlockPosition{3, 7});
static_assert(BlockAxisMapper{Orientation::Vertical}.position(3, 7) == BlockPosition{7, 3});

TESTED_TARGETS(BlockAxisMapper)
class BlockAxisMapperTest final : public el::UnitTest {
public:
    void testHorizontalOrientationMapsMainAxisToWidthAndX() {

        const auto mapper = BlockAxisMapper{Orientation::Horizontal};

        REQUIRE_EQUAL(mapper.size(12, 5), BlockSize(12, 5));
        REQUIRE_EQUAL(mapper.position(7, 2), BlockPosition(7, 2));
        REQUIRE_EQUAL(mapper.position(7), BlockPosition(7, 0));
        REQUIRE_EQUAL(mapper.horizontalValue(11, 22), 11);
        REQUIRE_EQUAL(mapper.verticalValue(11, 22), 22);
    }

    void testVerticalOrientationMapsMainAxisToHeightAndY() {

        const auto mapper = BlockAxisMapper{Orientation::Vertical};

        REQUIRE_EQUAL(mapper.size(12, 5), BlockSize(5, 12));
        REQUIRE_EQUAL(mapper.position(7, 2), BlockPosition(2, 7));
        REQUIRE_EQUAL(mapper.position(7), BlockPosition(0, 7));
        REQUIRE_EQUAL(mapper.horizontalValue(11, 22), 22);
        REQUIRE_EQUAL(mapper.verticalValue(11, 22), 11);
    }
};
