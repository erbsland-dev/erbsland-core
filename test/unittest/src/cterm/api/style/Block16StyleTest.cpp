// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/TestHelper.hpp"

#include <erbsland/cterm/Block16Style.hpp>
#include <erbsland/cterm/FrameStyle.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <stdexcept>

TESTED_TARGETS(Block16Style)
class Block16StyleTest final : public el::UnitTest {
public:
    void testStringConstructorSplitsExactlySixteenTerminalCharacters() {
        const auto style = Block16Style{"0123456789ABCDEF"_el};

        REQUIRE_EQUAL(style.block(0), U'0');
        REQUIRE_EQUAL(style.block(10), U'A');
        REQUIRE_EQUAL(style.block(15), U'F');
    }

    void testUtf32StringConstructorSplitsExactlySixteenTerminalCharacters() {
        const auto style = Block16Style{U"0123456789ABCDEF"_el};

        REQUIRE_EQUAL(style.block(0), U'0');
        REQUIRE_EQUAL(style.block(10), U'A');
        REQUIRE_EQUAL(style.block(15), U'F');
    }

    void testStringConstructorRejectsInvalidTileCounts() {
        REQUIRE_THROWS_AS(erbsland::err::ParameterError, Block16Style{"123"_el});
        REQUIRE_THROWS_AS(erbsland::err::ParameterError, Block16Style{"0123456789ABCDEFG"_el});
    }

    void testCreateFactoryBuildsSharedStyles() {
        const auto utf8Style = Block16Style::create("0123456789ABCDEF"_el);
        const auto utf32Style = Block16Style::create(U"0123456789ABCDEF"_el);

        REQUIRE(utf8Style != nullptr);
        REQUIRE(utf32Style != nullptr);
        REQUIRE_EQUAL(utf8Style->block(1), U'1');
        REQUIRE_EQUAL(utf32Style->block(15), U'F');
    }

    void testBlockReturnsConfiguredTilesAndDefaultsForInvalidIndexes() {
        const auto style = Block16Style{std::array<Block, 16>{
            Block{U'0'},
            Block{U'1'},
            Block{U'2'},
            Block{U'3'},
            Block{U'4'},
            Block{U'5'},
            Block{U'6'},
            Block{U'7'},
            Block{U'8'},
            Block{U'9'},
            Block{U'A'},
            Block{U'B'},
            Block{U'C'},
            Block{U'D'},
            Block{U'E'},
            Block{U'F'},
        }};

        REQUIRE_EQUAL(style.block(0), U'0');
        REQUIRE_EQUAL(style.block(5), U'5');
        REQUIRE_EQUAL(style.block(15), U'F');
        REQUIRE(style.block(16).isEmpty());
    }

    void testPredefinedStylesExposeExpectedTiles() {
        REQUIRE_EQUAL(Block16Style::lightFrame()->block(3), U'┌');
        REQUIRE_EQUAL(Block16Style::lightDoubleDashFrame()->block(5), U'╌');
        REQUIRE_EQUAL(Block16Style::lightTripleDashFrame()->block(5), U'┄');
        REQUIRE_EQUAL(Block16Style::lightQuadrupleDashFrame()->block(5), U'┈');
        REQUIRE_EQUAL(Block16Style::lightRoundedFrame()->block(3), U'╭');
        REQUIRE_EQUAL(Block16Style::heavyFrame()->block(15), U'╋');
        REQUIRE_EQUAL(Block16Style::heavyDoubleDashFrame()->block(10), U'╏');
        REQUIRE_EQUAL(Block16Style::heavyTripleDashFrame()->block(5), U'┅');
        REQUIRE_EQUAL(Block16Style::heavyQuadrupleDashFrame()->block(5), U'┉');
        REQUIRE_EQUAL(Block16Style::doubleFrame()->block(10), U'║');
        REQUIRE_EQUAL(Block16Style::fullBlockFrame()->block(15), U'█');
        REQUIRE_EQUAL(Block16Style::fullBlockWithChamferFrame()->block(3), U'◢');
        REQUIRE_EQUAL(Block16Style::noneFrame()->block(15), U' ');
    }

    void testForStyleMapsKnownAndUnknownEnumValues() {
        REQUIRE_EQUAL(Block16Style::forStyle(FrameStyle::Light)->block(5), U'─');
        REQUIRE_EQUAL(Block16Style::forStyle(FrameStyle::LightDoubleDash)->block(5), U'╌');
        REQUIRE_EQUAL(Block16Style::forStyle(FrameStyle::LightTripleDash)->block(5), U'┄');
        REQUIRE_EQUAL(Block16Style::forStyle(FrameStyle::LightQuadrupleDash)->block(5), U'┈');
        REQUIRE_EQUAL(Block16Style::forStyle(FrameStyle::Heavy)->block(5), U'━');
        REQUIRE_EQUAL(Block16Style::forStyle(FrameStyle::HeavyDoubleDash)->block(5), U'╍');
        REQUIRE_EQUAL(Block16Style::forStyle(FrameStyle::HeavyTripleDash)->block(5), U'┅');
        REQUIRE_EQUAL(Block16Style::forStyle(FrameStyle::HeavyQuadrupleDash)->block(5), U'┉');
        REQUIRE_EQUAL(Block16Style::forStyle(FrameStyle::Double)->block(5), U'═');
        REQUIRE_EQUAL(Block16Style::forStyle(FrameStyle::FullBlock)->block(5), U'█');
        REQUIRE_EQUAL(Block16Style::forStyle(FrameStyle::FullBlockWithChamfer)->block(3), U'◢');
        REQUIRE(Block16Style::forStyle(FrameStyle::OuterHalfBlock) == nullptr);
        REQUIRE(Block16Style::forStyle(FrameStyle::InnerHalfBlock) == nullptr);
        REQUIRE_EQUAL(Block16Style::forStyle(FrameStyle::LightWithRoundedCorners)->block(12), U'╯');
        REQUIRE_EQUAL(Block16Style::forStyle(FrameStyle::None)->block(15), U' ');
        REQUIRE_EQUAL(Block16Style::forStyle(static_cast<FrameStyle>(255))->block(3), U'┌');
    }
};
