// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/TestHelper.hpp"

#include <erbsland/cterm/FrameStyle.hpp>
#include <erbsland/cterm/Tile9Style.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <stdexcept>

TESTED_TARGETS(Tile9Style)
class Tile9StyleTest final : public el::UnitTest {
public:
    void testStringConstructorRepeatsTheNineBaseTilesAcrossRectangles() {
        const auto style = Tile9Style{"ABCDEFGHI"_el};

        REQUIRE_EQUAL(style.block(bgeo::BlockRectangle{0, 0, 4, 3}, bgeo::BlockPosition{0, 0}), U'A');
        REQUIRE_EQUAL(style.block(bgeo::BlockRectangle{0, 0, 4, 3}, bgeo::BlockPosition{1, 0}), U'B');
        REQUIRE_EQUAL(style.block(bgeo::BlockRectangle{0, 0, 4, 3}, bgeo::BlockPosition{3, 0}), U'C');
        REQUIRE_EQUAL(style.block(bgeo::BlockRectangle{0, 0, 4, 3}, bgeo::BlockPosition{0, 1}), U'D');
        REQUIRE_EQUAL(style.block(bgeo::BlockRectangle{0, 0, 4, 3}, bgeo::BlockPosition{2, 1}), U'E');
        REQUIRE_EQUAL(style.block(bgeo::BlockRectangle{0, 0, 4, 3}, bgeo::BlockPosition{3, 1}), U'F');
        REQUIRE_EQUAL(style.block(bgeo::BlockRectangle{0, 0, 4, 3}, bgeo::BlockPosition{0, 2}), U'G');
        REQUIRE_EQUAL(style.block(bgeo::BlockRectangle{0, 0, 4, 3}, bgeo::BlockPosition{2, 2}), U'H');
        REQUIRE_EQUAL(style.block(bgeo::BlockRectangle{0, 0, 4, 3}, bgeo::BlockPosition{3, 2}), U'I');
    }

    void testDegenerateRectanglesUseFallbackTilesWhenOnlyNineTilesAreDefined() {
        const auto style = Tile9Style{"ABCDEFGHI"_el};

        REQUIRE_EQUAL(style.block(bgeo::BlockRectangle{0, 0, 4, 1}, bgeo::BlockPosition{0, 0}), U'A');
        REQUIRE_EQUAL(style.block(bgeo::BlockRectangle{0, 0, 4, 1}, bgeo::BlockPosition{1, 0}), U'B');
        REQUIRE_EQUAL(style.block(bgeo::BlockRectangle{0, 0, 4, 1}, bgeo::BlockPosition{3, 0}), U'C');
        REQUIRE_EQUAL(style.block(bgeo::BlockRectangle{0, 0, 1, 4}, bgeo::BlockPosition{0, 0}), U'A');
        REQUIRE_EQUAL(style.block(bgeo::BlockRectangle{0, 0, 1, 4}, bgeo::BlockPosition{0, 1}), U'D');
        REQUIRE_EQUAL(style.block(bgeo::BlockRectangle{0, 0, 1, 3}, bgeo::BlockPosition{0, 2}), U'G');
        REQUIRE_EQUAL(style.block(bgeo::BlockRectangle{0, 0, 1, 1}, bgeo::BlockPosition{0, 0}), U'A');
    }

    void testSixteenTilesProvideExplicitDegenerateVariants() {
        const auto style = Tile9Style{"ABCDEFGHIJKLMNOP"_el};

        REQUIRE_EQUAL(style.block(bgeo::BlockRectangle{0, 0, 4, 1}, bgeo::BlockPosition{0, 0}), U'J');
        REQUIRE_EQUAL(style.block(bgeo::BlockRectangle{0, 0, 4, 1}, bgeo::BlockPosition{1, 0}), U'K');
        REQUIRE_EQUAL(style.block(bgeo::BlockRectangle{0, 0, 4, 1}, bgeo::BlockPosition{3, 0}), U'L');
        REQUIRE_EQUAL(style.block(bgeo::BlockRectangle{0, 0, 1, 4}, bgeo::BlockPosition{0, 0}), U'M');
        REQUIRE_EQUAL(style.block(bgeo::BlockRectangle{0, 0, 1, 1}, bgeo::BlockPosition{0, 0}), U'P');
        REQUIRE_EQUAL(style.block(bgeo::BlockRectangle{0, 0, 1, 4}, bgeo::BlockPosition{0, 3}), U'O');
    }

    void testNamedElementsExposeAllSixteenTiles() {
        const auto style = Tile9Style{"ABCDEFGHIJKLMNOP"_el};

        REQUIRE_EQUAL(style.block(Tile9Style::Element::NorthWest), U'A');
        REQUIRE_EQUAL(style.block(Tile9Style::Element::North), U'B');
        REQUIRE_EQUAL(style.block(Tile9Style::Element::NorthEast), U'C');
        REQUIRE_EQUAL(style.block(Tile9Style::Element::West), U'D');
        REQUIRE_EQUAL(style.block(Tile9Style::Element::Center), U'E');
        REQUIRE_EQUAL(style.block(Tile9Style::Element::East), U'F');
        REQUIRE_EQUAL(style.block(Tile9Style::Element::SouthWest), U'G');
        REQUIRE_EQUAL(style.block(Tile9Style::Element::South), U'H');
        REQUIRE_EQUAL(style.block(Tile9Style::Element::SouthEast), U'I');
        REQUIRE_EQUAL(style.block(Tile9Style::Element::HorizontalWest), U'J');
        REQUIRE_EQUAL(style.block(Tile9Style::Element::HorizontalCenter), U'K');
        REQUIRE_EQUAL(style.block(Tile9Style::Element::HorizontalEast), U'L');
        REQUIRE_EQUAL(style.block(Tile9Style::Element::VerticalNorth), U'M');
        REQUIRE_EQUAL(style.block(Tile9Style::Element::VerticalCenter), U'N');
        REQUIRE_EQUAL(style.block(Tile9Style::Element::VerticalSouth), U'O');
        REQUIRE_EQUAL(style.block(Tile9Style::Element::Single), U'P');
    }

    void testNamedExtendedElementsFallBackForNineTileStyles() {
        const auto style = Tile9Style{"ABCDEFGHI"_el};

        REQUIRE_EQUAL(style.block(Tile9Style::Element::HorizontalWest), U'A');
        REQUIRE_EQUAL(style.block(Tile9Style::Element::HorizontalCenter), U'B');
        REQUIRE_EQUAL(style.block(Tile9Style::Element::HorizontalEast), U'C');
        REQUIRE_EQUAL(style.block(Tile9Style::Element::VerticalNorth), U'A');
        REQUIRE_EQUAL(style.block(Tile9Style::Element::VerticalCenter), U'D');
        REQUIRE_EQUAL(style.block(Tile9Style::Element::VerticalSouth), U'G');
        REQUIRE_EQUAL(style.block(Tile9Style::Element::Single), U'A');
    }

    void testStringConstructorRejectsInvalidTileCounts() {
        REQUIRE_THROWS_AS(std::invalid_argument, Tile9Style{"12345678"_el});
        REQUIRE_THROWS_AS(std::invalid_argument, Tile9Style{"1234567890"_el});
        REQUIRE_THROWS_AS(std::invalid_argument, Tile9Style{"123456789012345"_el});
        REQUIRE_THROWS_AS(std::invalid_argument, Tile9Style{"12345678901234567"_el});
    }

    void testCreateFactoryBuildsSharedStyles() {
        const auto utf8Style = Tile9Style::create("ABCDEFGHI"_el);
        const auto utf32Style = Tile9Style::create(U"ABCDEFGHIJKLMNOP"_el);

        REQUIRE(utf8Style != nullptr);
        REQUIRE(utf32Style != nullptr);
        REQUIRE_EQUAL(utf8Style->block(bgeo::BlockRectangle{0, 0, 3, 3}, bgeo::BlockPosition{1, 1}), U'E');
        REQUIRE_EQUAL(utf32Style->block(bgeo::BlockRectangle{0, 0, 1, 1}, bgeo::BlockPosition{0, 0}), U'P');
    }

    void testArrayConstructorsKeepConfiguredTiles() {
        const auto tiles9 = std::array<Block, 9>{
            Block{U'0'},
            Block{U'1'},
            Block{U'2'},
            Block{U'3'},
            Block{U'4'},
            Block{U'5'},
            Block{U'6'},
            Block{U'7'},
            Block{U'8'},
        };
        const auto tiles16 = std::array<Block, 16>{
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
            Block{"10"_el},
            Block{"11"_el},
            Block{"12"_el},
            Block{"13"_el},
            Block{"14"_el},
            Block{"15"_el},
        };
        const auto style9 = Tile9Style{tiles9};
        const auto style16 = Tile9Style{tiles16};

        REQUIRE_EQUAL(style9.block(bgeo::BlockRectangle{0, 0, 3, 3}, bgeo::BlockPosition{2, 2}), U'8');
        REQUIRE_EQUAL(
            style16.block(bgeo::BlockRectangle{0, 0, 1, 1}, bgeo::BlockPosition{0, 0}).charStr(),
            tiles16[15].charStr());
    }

    void testPredefinedStylesExposeExpectedTiles() {
        REQUIRE_EQUAL(
            Tile9Style::outerHalfBlockFrame()
                ->block(bgeo::BlockRectangle{0, 0, 4, 3}, bgeo::BlockPosition{0, 0})
                .charStr(),
            "▛"_el);
        REQUIRE_EQUAL(
            Tile9Style::outerHalfBlockFrame()
                ->block(bgeo::BlockRectangle{0, 0, 1, 1}, bgeo::BlockPosition{0, 0})
                .charStr(),
            "█"_el);
        REQUIRE_EQUAL(
            Tile9Style::innerHalfBlockFrame()
                ->block(bgeo::BlockRectangle{0, 0, 4, 3}, bgeo::BlockPosition{1, 0})
                .charStr(),
            "▄"_el);
        REQUIRE_EQUAL(
            Tile9Style::innerHalfBlockFrame()
                ->block(bgeo::BlockRectangle{0, 0, 1, 2}, bgeo::BlockPosition{0, 1})
                .charStr(),
            "█"_el);
    }

    void testForStyleMapsKnownAndUnknownEnumValues() {
        REQUIRE(Tile9Style::forStyle(FrameStyle::OuterHalfBlock) != nullptr);
        REQUIRE(Tile9Style::forStyle(FrameStyle::InnerHalfBlock) != nullptr);
        REQUIRE(Tile9Style::forStyle(FrameStyle::None) == nullptr);
        REQUIRE(Tile9Style::forStyle(FrameStyle::Light) == nullptr);
        REQUIRE(Tile9Style::forStyle(static_cast<FrameStyle>(255)) == nullptr);
    }
};
