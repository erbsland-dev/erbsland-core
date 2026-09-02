// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/TestHelper.hpp"

#include <erbsland/unittest/UnitTest.hpp>

TESTED_TARGETS(BufferDrawOptions)
class BufferDrawOptionsTest final : public el::UnitTest {
public:
    void testDefaultConstructionUsesPositionTargetAndFullSource() {
        const auto options = BufferDrawOptions{};
        const auto expectedRect = block::Rectangle{0, 0, 0, 0};

        REQUIRE_EQUAL(options.targetRect(), expectedRect);
        REQUIRE(options.isTargetPosition());
        REQUIRE_EQUAL(options.sourceRect(), expectedRect);
        REQUIRE(options.useFullSource());
        REQUIRE_EQUAL(options.combinationStyle(), nullptr);
        REQUIRE_FALSE(options.overwriteColors());
    }

    void testPositionConstructorStoresOnlyTheTargetPosition() {
        const auto options = BufferDrawOptions{block::Position{3, 4}};
        const auto expectedTargetRect = block::Rectangle{3, 4, 0, 0};
        const auto expectedSourceRect = block::Rectangle{0, 0, 0, 0};

        REQUIRE_EQUAL(options.targetRect(), expectedTargetRect);
        REQUIRE(options.isTargetPosition());
        REQUIRE_EQUAL(options.sourceRect(), expectedSourceRect);
        REQUIRE(options.useFullSource());
    }

    void testRectangleConstructorAndSettersStoreExplicitRectsAndFlags() {
        auto options = BufferDrawOptions{block::Rectangle{1, 2, 7, 8}, block::Rectangle{4, 5, 2, 3}};
        const auto combinationStyle = BlockCombinationStyle::colorOverlay();
        const auto expectedInitialTargetRect = block::Rectangle{1, 2, 7, 8};
        const auto expectedInitialSourceRect = block::Rectangle{4, 5, 2, 3};

        REQUIRE_EQUAL(options.targetRect(), expectedInitialTargetRect);
        REQUIRE_FALSE(options.isTargetPosition());
        REQUIRE_EQUAL(options.sourceRect(), expectedInitialSourceRect);
        REQUIRE_FALSE(options.useFullSource());

        options.setTargetRect(block::Rectangle{9, 8, 1, 2});
        options.setSourceRect(block::Rectangle{6, 5, 4, 3});
        options.setCombinationStyle(combinationStyle);
        options.setOverwriteColors(true);

        REQUIRE_EQUAL(options.targetRect(), (block::Rectangle{9, 8, 1, 2}));
        REQUIRE_FALSE(options.isTargetPosition());
        REQUIRE_EQUAL(options.sourceRect(), (block::Rectangle{6, 5, 4, 3}));
        REQUIRE_FALSE(options.useFullSource());
        REQUIRE_EQUAL(options.combinationStyle(), combinationStyle);
        REQUIRE(options.overwriteColors());
    }
};
