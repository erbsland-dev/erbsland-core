// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/TestHelper.hpp"

#include <erbsland/unittest/UnitTest.hpp>

TESTED_TARGETS(BufferDrawOptions)
class BufferDrawOptionsTest final : public el::UnitTest {
public:
    void testDefaultConstructionUsesPositionTargetAndFullSource() {
        const auto options = BufferDrawOptions{};
        const auto expectedRect = bgeo::BlockRectangle{0, 0, 0, 0};

        REQUIRE_EQUAL(options.targetRect(), expectedRect);
        REQUIRE(options.isTargetPosition());
        REQUIRE_EQUAL(options.sourceRect(), expectedRect);
        REQUIRE(options.useFullSource());
        REQUIRE(options.combinationStyle() == nullptr);
        REQUIRE_FALSE(options.overwriteColors());
    }

    void testPositionConstructorStoresOnlyTheTargetPosition() {
        const auto options = BufferDrawOptions{bgeo::BlockPosition{3, 4}};
        const auto expectedTargetRect = bgeo::BlockRectangle{3, 4, 0, 0};
        const auto expectedSourceRect = bgeo::BlockRectangle{0, 0, 0, 0};

        REQUIRE_EQUAL(options.targetRect(), expectedTargetRect);
        REQUIRE(options.isTargetPosition());
        REQUIRE_EQUAL(options.sourceRect(), expectedSourceRect);
        REQUIRE(options.useFullSource());
    }

    void testRectangleConstructorAndSettersStoreExplicitRectsAndFlags() {
        auto options = BufferDrawOptions{bgeo::BlockRectangle{1, 2, 7, 8}, bgeo::BlockRectangle{4, 5, 2, 3}};
        const auto combinationStyle = BlockCombinationStyle::colorOverlay();
        const auto expectedInitialTargetRect = bgeo::BlockRectangle{1, 2, 7, 8};
        const auto expectedInitialSourceRect = bgeo::BlockRectangle{4, 5, 2, 3};

        REQUIRE_EQUAL(options.targetRect(), expectedInitialTargetRect);
        REQUIRE_FALSE(options.isTargetPosition());
        REQUIRE_EQUAL(options.sourceRect(), expectedInitialSourceRect);
        REQUIRE_FALSE(options.useFullSource());

        options.setTargetRect(bgeo::BlockRectangle{9, 8, 1, 2});
        options.setSourceRect(bgeo::BlockRectangle{6, 5, 4, 3});
        options.setCombinationStyle(combinationStyle);
        options.setOverwriteColors(true);

        REQUIRE_EQUAL(options.targetRect(), (bgeo::BlockRectangle{9, 8, 1, 2}));
        REQUIRE_FALSE(options.isTargetPosition());
        REQUIRE_EQUAL(options.sourceRect(), (bgeo::BlockRectangle{6, 5, 4, 3}));
        REQUIRE_FALSE(options.useFullSource());
        REQUIRE(options.combinationStyle() == combinationStyle);
        REQUIRE(options.overwriteColors());
    }
};
