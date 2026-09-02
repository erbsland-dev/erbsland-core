// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/TestHelper.hpp"

#include <erbsland/unittest/UnitTest.hpp>

TESTED_TARGETS(BlockUnit BlockIndex BlockCount BlockRange)
class BlockUnitTest final : public el::UnitTest {
public:
    void testIndexAndCountExposeTypedPositions() {
        const auto index = BlockIndex{3U};
        const auto count = BlockCount{5U};

        REQUIRE_EQUAL(index.toSizeT(), std::size_t{3});
        REQUIRE_EQUAL(count.toSizeT(), std::size_t{5});
        REQUIRE(BlockIndex::noIndex().isNoIndex());
    }

    void testRangeEndIndexAndClampingUseBlockUnits() {
        const auto unchanged = BlockRange{BlockIndex{2U}, BlockCount{3U}}.clampedTo(BlockCount{10U});
        const auto shortened = BlockRange{BlockIndex{2U}, BlockCount{99U}}.clampedTo(BlockCount{6U});
        const auto emptyTail = BlockRange{BlockIndex{9U}, BlockCount{1U}}.clampedTo(BlockCount{4U});

        REQUIRE_EQUAL(unchanged.index(), BlockIndex{2U});
        REQUIRE_EQUAL(unchanged.length(), BlockCount{3U});
        REQUIRE_EQUAL(unchanged.endIndex(), BlockIndex{5U});
        REQUIRE_EQUAL(shortened.length(), BlockCount{4U});
        REQUIRE_EQUAL(emptyTail.index(), BlockIndex{4U});
        REQUIRE_EQUAL(emptyTail.length(), BlockCount{0U});
    }

    void testAllRangeClampsToTheAvailableBlockCount() {
        const auto clamped = BlockRange::all().clampedTo(BlockCount{7U});

        REQUIRE_EQUAL(clamped.index(), BlockIndex{0U});
        REQUIRE_EQUAL(clamped.length(), BlockCount{7U});
    }
};
