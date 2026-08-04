// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/TestHelper.hpp"

#include <erbsland/cterm/FrameBorder.hpp>
#include <erbsland/unittest/UnitTest.hpp>

TESTED_TARGETS(FrameBorder)
class FrameBorderTest final : public el::UnitTest {
public:
    void testDefaultConstructorSetsAllElementsToNone() {
        const auto border = FrameBorder{};

        REQUIRE_EQUAL(border.style(FrameBorderElement::Top), FrameStyle::None);
        REQUIRE_EQUAL(border.style(FrameBorderElement::Bottom), FrameStyle::None);
        REQUIRE_EQUAL(border.style(FrameBorderElement::Left), FrameStyle::None);
        REQUIRE_EQUAL(border.style(FrameBorderElement::Right), FrameStyle::None);
        REQUIRE_EQUAL(border.style(FrameBorderElement::HLine), FrameStyle::None);
        REQUIRE_EQUAL(border.style(FrameBorderElement::VLine), FrameStyle::None);
    }

    void testStyleConstructorSetsAllElements() {
        const auto border = FrameBorder{FrameStyle::Double, Color{fg::Yellow, bg::Blue}};

        REQUIRE_EQUAL(border.style(FrameBorderElement::Top), FrameStyle::Double);
        REQUIRE_EQUAL(border.color(FrameBorderElement::Top), (Color{fg::Yellow, bg::Blue}));
        REQUIRE_EQUAL(border.style(FrameBorderElement::VLine), FrameStyle::Double);
        REQUIRE_EQUAL(border.color(FrameBorderElement::VLine), (Color{fg::Yellow, bg::Blue}));
    }

    void testSetUpdatesIndividualElementsWithBothElementNames() {
        auto border = FrameBorder{};

        border.set(FrameBorderElement::Top, FrameStyle::Light, Color{fg::Green, bg::Black});
        border.set(FrameBorder::Element::VLine, FrameStyle::Heavy, Color{fg::Red, bg::Blue});

        const auto topBorder = border.border(FrameBorderElement::Top);
        REQUIRE_EQUAL(topBorder.style, FrameStyle::Light);
        REQUIRE_EQUAL(topBorder.color, (Color{fg::Green, bg::Black}));
        REQUIRE_EQUAL(border.style(FrameBorder::Element::VLine), FrameStyle::Heavy);
        REQUIRE_EQUAL(border.color(FrameBorder::Element::VLine), (Color{fg::Red, bg::Blue}));
        REQUIRE_EQUAL(border.style(FrameBorderElement::Bottom), FrameStyle::None);
    }

    void testCornerCharResolvesBasicJoints() {
        const auto none = FrameBorder::Border{};
        const auto light = FrameBorder::Border{.style = FrameStyle::Light};
        const auto rounded = FrameBorder::Border{.style = FrameStyle::LightWithRoundedCorners};

        REQUIRE_EQUAL(FrameBorder::cornerChar(none, none, none, none), U' ');
        REQUIRE_EQUAL(FrameBorder::cornerChar(light, none, none, none), U'╶');
        REQUIRE_EQUAL(FrameBorder::cornerChar(rounded, rounded, none, none), U'╭');
        REQUIRE_EQUAL(FrameBorder::cornerChar(light, rounded, none, none), U'┌');
    }

    void testCornerResolvesAnchorsForLightBorder() {
        const auto border = FrameBorder{FrameStyle::Light};

        REQUIRE_EQUAL(border.corner(bgeo::BlockAnchor::TopLeft), U'┌');
        REQUIRE_EQUAL(border.corner(bgeo::BlockAnchor::TopCenter), U'┬');
        REQUIRE_EQUAL(border.corner(bgeo::BlockAnchor::CenterLeft), U'├');
        REQUIRE_EQUAL(border.corner(bgeo::BlockAnchor::Center), U'┼');
        REQUIRE_EQUAL(border.corner(bgeo::BlockAnchor::BottomRight), U'┘');
    }

    void testCornerResolvesRoundedOuterCornersOnly() {
        const auto border = FrameBorder{FrameStyle::LightWithRoundedCorners};

        REQUIRE_EQUAL(border.corner(bgeo::BlockAnchor::TopLeft), U'╭');
        REQUIRE_EQUAL(border.corner(bgeo::BlockAnchor::TopCenter), U'┬');
        REQUIRE_EQUAL(border.corner(bgeo::BlockAnchor::Center), U'┼');
        REQUIRE_EQUAL(border.corner(bgeo::BlockAnchor::BottomRight), U'╯');
    }

    void testLineStyleDetectionIncludesOnlySupportedGridStyles() {
        REQUIRE(FrameBorder::isLineStyle(FrameStyle::None));
        REQUIRE(FrameBorder::isLineStyle(FrameStyle::LightQuadrupleDash));
        REQUIRE(FrameBorder::isLineStyle(FrameStyle::HeavyTripleDash));
        REQUIRE(FrameBorder::isLineStyle(FrameStyle::Double));
        REQUIRE(FrameBorder::isLineStyle(FrameStyle::LightWithRoundedCorners));
        REQUIRE(!FrameBorder::isLineStyle(FrameStyle::FullBlock));
        REQUIRE(!FrameBorder::isLineStyle(FrameStyle::FullBlockWithChamfer));
        REQUIRE(!FrameBorder::isLineStyle(FrameStyle::OuterHalfBlock));
        REQUIRE(!FrameBorder::isLineStyle(FrameStyle::InnerHalfBlock));
    }
};
