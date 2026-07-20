// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/bgeo/Alignment.hpp>
#include <erbsland/bgeo/BlockPosition.hpp>
#include <erbsland/bgeo/StdFormatForBlock.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <concepts>
#include <functional>

TESTED_TARGETS(Alignment AlignmentFlag AlignmentFlags)
class AlignmentTest final : public el::UnitTest {
public:
    void testCompileTimeContracts() {
        using el::bgeo::Alignment;
        using el::bgeo::AlignmentFlag;
        using el::bgeo::AlignmentFlags;

        static_assert(std::equality_comparable<Alignment>);
        static_assert(std::same_as<decltype(Alignment{}.toRawValue()), AlignmentFlags>);
        static_assert(noexcept(Alignment{}));
        static_assert(noexcept(Alignment{AlignmentFlag::Center}));
        static_assert(noexcept(Alignment{}.isLeft()));
        static_assert(noexcept(Alignment{}.horizontal()));
        static_assert(noexcept(Alignment{}.toRawValue()));
        static_assert(noexcept(Alignment{}.hash()));

        static_assert(Alignment{} == Alignment::TopLeft);
        static_assert(Alignment::Center.isHorizontalCenter());
        static_assert(Alignment::Center.isVerticalCenter());
        static_assert(Alignment::BottomRight.isRight());
        static_assert(Alignment::BottomRight.isBottom());
        static_assert(Alignment::Center.horizontal() == Alignment::HCenter);
        static_assert(Alignment::Center.vertical() == Alignment::VCenter);
        static_assert(Alignment{AlignmentFlags{AlignmentFlag::Left, AlignmentFlag::Right}} == Alignment::Left);
        static_assert(Alignment{AlignmentFlags{AlignmentFlag::Top, AlignmentFlag::Bottom}} == Alignment::Top);
        static_assert(Alignment{AlignmentFlags::fromRawValue(0xffU)} == Alignment::TopLeft);
        static_assert(Alignment{AlignmentFlag::None}.toRawValue().isEmpty());
    }

    void testConstants() {
        using el::bgeo::Alignment;
        using el::bgeo::AlignmentFlag;

        REQUIRE_EQUAL(Alignment::Left.toRawValue(), AlignmentFlag::Left);
        REQUIRE_EQUAL(Alignment::HCenter.toRawValue(), AlignmentFlag::HCenter);
        REQUIRE_EQUAL(Alignment::Right.toRawValue(), AlignmentFlag::Right);
        REQUIRE_EQUAL(Alignment::Top.toRawValue(), AlignmentFlag::Top);
        REQUIRE_EQUAL(Alignment::VCenter.toRawValue(), AlignmentFlag::VCenter);
        REQUIRE_EQUAL(Alignment::Bottom.toRawValue(), AlignmentFlag::Bottom);
        REQUIRE_EQUAL(Alignment::TopLeft.toRawValue(), AlignmentFlag::TopLeft);
        REQUIRE_EQUAL(Alignment::TopCenter.toRawValue(), AlignmentFlag::TopCenter);
        REQUIRE_EQUAL(Alignment::TopRight.toRawValue(), AlignmentFlag::TopRight);
        REQUIRE_EQUAL(Alignment::CenterLeft.toRawValue(), AlignmentFlag::CenterLeft);
        REQUIRE_EQUAL(Alignment::Center.toRawValue(), AlignmentFlag::Center);
        REQUIRE_EQUAL(Alignment::CenterRight.toRawValue(), AlignmentFlag::CenterRight);
        REQUIRE_EQUAL(Alignment::BottomLeft.toRawValue(), AlignmentFlag::BottomLeft);
        REQUIRE_EQUAL(Alignment::BottomCenter.toRawValue(), AlignmentFlag::BottomCenter);
        REQUIRE_EQUAL(Alignment::BottomRight.toRawValue(), AlignmentFlag::BottomRight);
    }

    void testConstruction() {
        using el::bgeo::Alignment;
        using el::bgeo::AlignmentFlag;
        using el::bgeo::AlignmentFlags;

        REQUIRE(Alignment{} == Alignment::TopLeft);
        REQUIRE(Alignment{AlignmentFlag::Center} == Alignment::Center);
        REQUIRE(Alignment{AlignmentFlags{AlignmentFlag::Right, AlignmentFlag::Bottom}} == Alignment::BottomRight);
        REQUIRE(Alignment{AlignmentFlag::None}.toRawValue().isEmpty());
        REQUIRE(Alignment{AlignmentFlag::Left}.vertical().toRawValue().isEmpty());
        REQUIRE(Alignment{AlignmentFlag::Top}.horizontal().toRawValue().isEmpty());
    }

    void testExclusiveNormalization() {
        using el::bgeo::Alignment;
        using el::bgeo::AlignmentFlag;
        using el::bgeo::AlignmentFlags;

        REQUIRE(
            Alignment{AlignmentFlags{AlignmentFlag::Left, AlignmentFlag::HCenter, AlignmentFlag::Right}} ==
            Alignment::Left);
        REQUIRE(Alignment{AlignmentFlags{AlignmentFlag::HCenter, AlignmentFlag::Right}} == Alignment::HCenter);
        REQUIRE(
            Alignment{AlignmentFlags{AlignmentFlag::Top, AlignmentFlag::VCenter, AlignmentFlag::Bottom}} ==
            Alignment::Top);
        REQUIRE(Alignment{AlignmentFlags{AlignmentFlag::VCenter, AlignmentFlag::Bottom}} == Alignment::VCenter);
        REQUIRE(
            Alignment{AlignmentFlags{AlignmentFlag::Right, AlignmentFlag::Bottom, AlignmentFlag::VCenter}} ==
            Alignment::CenterRight);
    }

    void testAccessorsAndFilters() {
        using el::bgeo::Alignment;
        using el::bgeo::AlignmentFlag;

        const auto alignment = Alignment::BottomCenter;
        REQUIRE_FALSE(alignment.isLeft());
        REQUIRE(alignment.isHorizontalCenter());
        REQUIRE_FALSE(alignment.isRight());
        REQUIRE_FALSE(alignment.isTop());
        REQUIRE_FALSE(alignment.isVerticalCenter());
        REQUIRE(alignment.isBottom());
        REQUIRE(alignment.horizontal() == Alignment::HCenter);
        REQUIRE(alignment.vertical() == Alignment::Bottom);

        const auto verticalOnly = Alignment{AlignmentFlag::VCenter};
        REQUIRE_FALSE(verticalOnly.isLeft());
        REQUIRE_FALSE(verticalOnly.isHorizontalCenter());
        REQUIRE_FALSE(verticalOnly.isRight());
        REQUIRE(verticalOnly.isVerticalCenter());
    }

    void testHashSupport() {
        using el::bgeo::Alignment;

        REQUIRE(std::hash<Alignment>{}(Alignment::Center) == Alignment::Center.hash());
        REQUIRE(std::hash<Alignment>{}(Alignment::Center) == std::hash<Alignment>{}(Alignment::Center));
    }

    void testOffsetHelpers() {
        using el::bgeo::Alignment;

        REQUIRE_EQUAL(Alignment::Left.horizontalOffset(10, 4), 0);
        REQUIRE_EQUAL(Alignment::HCenter.horizontalOffset(10, 4), 3);
        REQUIRE_EQUAL(Alignment::Right.horizontalOffset(10, 4), 6);
        REQUIRE_EQUAL(Alignment::Top.verticalOffset(8, 2), 0);
        REQUIRE_EQUAL(Alignment::VCenter.verticalOffset(8, 2), 3);
        REQUIRE_EQUAL(Alignment::Bottom.verticalOffset(8, 2), 6);
        REQUIRE_EQUAL(Alignment::Center.horizontalOffset(4, 10), -3);
        REQUIRE_EQUAL(Alignment::BottomRight.verticalOffset(2, 8), -6);
    }
};
