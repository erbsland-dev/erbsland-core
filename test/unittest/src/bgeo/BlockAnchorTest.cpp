// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/bgeo/BlockAnchor.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <concepts>
#include <functional>

TESTED_TARGETS(BlockAnchor BlockAnchorFlag BlockAnchorFlags)
class BlockAnchorTest final : public el::UnitTest {
public:
    void testCompileTimeContracts() {
        using el::bgeo::BlockAnchor;
        using el::bgeo::BlockAnchorFlag;
        using el::bgeo::BlockAnchorFlags;

        static_assert(std::equality_comparable<BlockAnchor>);
        static_assert(std::same_as<decltype(BlockAnchor{}.toRawValue()), BlockAnchorFlags>);
        static_assert(noexcept(BlockAnchor{}));
        static_assert(noexcept(BlockAnchor{BlockAnchorFlag::Center}));
        static_assert(noexcept(BlockAnchor{}.isLeft()));
        static_assert(noexcept(BlockAnchor{}.horizontal()));
        static_assert(noexcept(BlockAnchor{}.toRawValue()));

        static_assert(BlockAnchor{} == BlockAnchor::TopLeft);
        static_assert(BlockAnchor::Center.isHorizontalCenter());
        static_assert(BlockAnchor::Center.isVerticalCenter());
        static_assert(BlockAnchor::BottomRight.isRight());
        static_assert(BlockAnchor::BottomRight.isBottom());
        static_assert(BlockAnchor::Center.horizontal() == BlockAnchor::HCenter);
        static_assert(BlockAnchor::Center.vertical() == BlockAnchor::VCenter);
        static_assert(
            BlockAnchor{BlockAnchorFlags{BlockAnchorFlag::Left, BlockAnchorFlag::Right}} == BlockAnchor::Left);
        static_assert(BlockAnchor{BlockAnchorFlags{BlockAnchorFlag::Top, BlockAnchorFlag::Bottom}} == BlockAnchor::Top);
        static_assert(BlockAnchor{BlockAnchorFlags::fromRawValue(0xffU)} == BlockAnchor::TopLeft);
        static_assert(BlockAnchor{BlockAnchorFlag::None}.toRawValue().isEmpty());
    }

    void testConstantsAndComposition() {
        using el::bgeo::BlockAnchor;
        using el::bgeo::BlockAnchorFlag;

        REQUIRE_EQUAL(BlockAnchor::TopLeft.toRawValue(), BlockAnchorFlag::TopLeft);
        REQUIRE_EQUAL(BlockAnchor::Center.toRawValue(), BlockAnchorFlag::Center);
        REQUIRE_EQUAL(BlockAnchor::BottomRight.toRawValue(), BlockAnchorFlag::BottomRight);
        REQUIRE_EQUAL(BlockAnchor::Top | BlockAnchor::Left, BlockAnchor::TopLeft);
        REQUIRE_EQUAL(BlockAnchor::VCenter | BlockAnchor::HCenter, BlockAnchor::Center);
        REQUIRE_EQUAL(BlockAnchor::Bottom | BlockAnchor::Right, BlockAnchor::BottomRight);
    }

    void testExclusiveNormalizationAndComponents() {
        using el::bgeo::BlockAnchor;
        using el::bgeo::BlockAnchorFlag;
        using el::bgeo::BlockAnchorFlags;

        const auto conflicting =
            BlockAnchor{BlockAnchorFlags{BlockAnchorFlag::Right, BlockAnchorFlag::HCenter, BlockAnchorFlag::Bottom}};

        REQUIRE_EQUAL(conflicting, BlockAnchor::BottomCenter);
        REQUIRE_EQUAL(BlockAnchor::BottomRight.horizontal(), BlockAnchor::Right);
        REQUIRE_EQUAL(BlockAnchor::BottomRight.vertical(), BlockAnchor::Bottom);
        REQUIRE(BlockAnchor::Center.isHorizontalCenter());
        REQUIRE(BlockAnchor::Center.isVerticalCenter());
        REQUIRE_FALSE(BlockAnchor::Center.isLeft());
        REQUIRE_FALSE(BlockAnchor::Center.isTop());
    }

    void testHashSupport() {
        using el::bgeo::BlockAnchor;

        REQUIRE_EQUAL(std::hash<BlockAnchor>{}(BlockAnchor::Center), BlockAnchor::Center.hash());
        REQUIRE_NOT_EQUAL(
            std::hash<BlockAnchor>{}(BlockAnchor::TopLeft), std::hash<BlockAnchor>{}(BlockAnchor::Center));
    }
};
