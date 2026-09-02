// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/geometry/Anchor.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <concepts>
#include <functional>

TESTED_TARGETS(Anchor AnchorFlag AnchorFlags)
class AnchorTest final : public el::UnitTest {
public:
    void testCompileTimeContracts() {
        using el::geometry::Anchor;
        using el::geometry::AnchorFlag;
        using el::geometry::AnchorFlags;

        static_assert(std::equality_comparable<Anchor>);
        static_assert(std::same_as<decltype(Anchor{}.toRawValue()), AnchorFlags>);
        static_assert(noexcept(Anchor{}));
        static_assert(noexcept(Anchor{AnchorFlag::Center}));
        static_assert(noexcept(Anchor{}.isLeft()));
        static_assert(noexcept(Anchor{}.horizontal()));
        static_assert(noexcept(Anchor{}.toRawValue()));

        static_assert(Anchor{} == Anchor::TopLeft);
        static_assert(Anchor::Center.isHorizontalCenter());
        static_assert(Anchor::Center.isVerticalCenter());
        static_assert(Anchor::BottomRight.isRight());
        static_assert(Anchor::BottomRight.isBottom());
        static_assert(Anchor::Center.horizontal() == Anchor::HCenter);
        static_assert(Anchor::Center.vertical() == Anchor::VCenter);
        static_assert(Anchor{AnchorFlags{AnchorFlag::Left, AnchorFlag::Right}} == Anchor::Left);
        static_assert(Anchor{AnchorFlags{AnchorFlag::Top, AnchorFlag::Bottom}} == Anchor::Top);
        static_assert(Anchor{AnchorFlags::fromRawValue(0xffU)} == Anchor::TopLeft);
        static_assert(Anchor{AnchorFlag::None}.toRawValue().isEmpty());
    }

    void testConstantsAndComposition() {
        using el::geometry::Anchor;
        using el::geometry::AnchorFlag;

        REQUIRE_EQUAL(Anchor::TopLeft.toRawValue(), AnchorFlag::TopLeft);
        REQUIRE_EQUAL(Anchor::Center.toRawValue(), AnchorFlag::Center);
        REQUIRE_EQUAL(Anchor::BottomRight.toRawValue(), AnchorFlag::BottomRight);
        const auto topLeft = Anchor::Top | Anchor::Left;
        const auto center = Anchor::VCenter | Anchor::HCenter;
        const auto bottomRight = Anchor::Bottom | Anchor::Right;
        REQUIRE_EQUAL(topLeft, Anchor::TopLeft);
        REQUIRE_EQUAL(center, Anchor::Center);
        REQUIRE_EQUAL(bottomRight, Anchor::BottomRight);
    }

    void testExclusiveNormalizationAndComponents() {
        using el::geometry::Anchor;
        using el::geometry::AnchorFlag;
        using el::geometry::AnchorFlags;

        const auto conflicting = Anchor{AnchorFlags{AnchorFlag::Right, AnchorFlag::HCenter, AnchorFlag::Bottom}};

        REQUIRE_EQUAL(conflicting, Anchor::BottomCenter);
        const auto horizontal = Anchor::BottomRight.horizontal();
        const auto vertical = Anchor::BottomRight.vertical();
        REQUIRE_EQUAL(horizontal, Anchor::Right);
        REQUIRE_EQUAL(vertical, Anchor::Bottom);
        REQUIRE(Anchor::Center.isHorizontalCenter());
        REQUIRE(Anchor::Center.isVerticalCenter());
        REQUIRE_FALSE(Anchor::Center.isLeft());
        REQUIRE_FALSE(Anchor::Center.isTop());
    }

    void testHashSupport() {
        using el::geometry::Anchor;

        const auto centerHash = std::hash<Anchor>{}(Anchor::Center);
        const auto topLeftHash = std::hash<Anchor>{}(Anchor::TopLeft);
        REQUIRE_EQUAL(centerHash, Anchor::Center.hash());
        REQUIRE_NOT_EQUAL(topLeftHash, centerHash);
    }
};
