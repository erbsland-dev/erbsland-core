// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/TestHelper.hpp"

#include <erbsland/unittest/UnitTest.hpp>

TESTED_TARGETS(FrameDrawOptions)
class FrameDrawOptionsTest final : public el::UnitTest {
public:
    void testDefaultOptionsExposeTheExpectedDefaults() {
        const auto &options = FrameDrawOptions::defaultOptions();

        REQUIRE_EQUAL(options.frameColor().sequenceLength(), static_cast<std::size_t>(1));
        REQUIRE_EQUAL(options.frameColor().color(0), Color{});
        REQUIRE_EQUAL(options.fillColor().sequenceLength(), static_cast<std::size_t>(1));
        REQUIRE_EQUAL(options.fillColor().color(0), Color{});
        REQUIRE(options.fillBlock().isEmpty());
        REQUIRE_EQUAL(options.style(), FrameStyle::Light);
        REQUIRE_FALSE(options.block16Style());
        REQUIRE_FALSE(options.tile9Style());
        REQUIRE(options.combinationStyle());
        REQUIRE_EQUAL(options.animationOffset(), static_cast<std::size_t>(0));
        REQUIRE_EQUAL(options.frameColorMode(), FrameColorMode::OneColor);
        REQUIRE_EQUAL(options.fillColorMode(), FrameColorMode::OneColor);
        REQUIRE_EQUAL(&FrameDrawOptions::defaultOptions(), &options);
    }

    void testFrameAndFillColorSettersReplaceTheConfiguredSequences() {
        auto options = FrameDrawOptions{
            ColorSequence{
                Color{fg::Red, bg::Black},
                Color{fg::Blue, bg::Black},
            },
            FrameColorMode::HorizontalStripes};

        REQUIRE_EQUAL(options.frameColor().sequenceLength(), static_cast<std::size_t>(2));
        REQUIRE_EQUAL(options.frameColor().color(0), Color(fg::Red, bg::Black));
        REQUIRE_EQUAL(options.frameColor().color(1), Color(fg::Blue, bg::Black));
        REQUIRE_EQUAL(options.frameColorMode(), FrameColorMode::HorizontalStripes);

        options.setFrameColor(Color{fg::Yellow, bg::Blue});
        REQUIRE_EQUAL(options.frameColor().color(0), Color(fg::Yellow, bg::Blue));

        options.setFrameColor({});
        REQUIRE_EQUAL(options.frameColor().color(0), Color{});

        options.setFrameColor(Foreground{fg::Green});
        REQUIRE_EQUAL(options.frameColor().color(0), Color(fg::Green, bg::Inherited));

        options.setFrameColor(fg::Magenta);
        REQUIRE_EQUAL(options.frameColor().color(0), Color(fg::Magenta, bg::Inherited));

        options.setFrameColor(Foreground{fg::Cyan}, Background{bg::Black});
        REQUIRE_EQUAL(options.frameColor().color(0), Color(fg::Cyan, bg::Black));

        options.setFrameColor(Background{bg::Yellow});
        REQUIRE_EQUAL(options.frameColor().color(0), Color(fg::Inherited, bg::Yellow));

        options.setFrameColor(bg::Blue);
        REQUIRE_EQUAL(options.frameColor().color(0), Color(fg::Inherited, bg::Blue));

        options.setFrameColorSequence(
            ColorSequence{Color{fg::BrightWhite, bg::Red}}, FrameColorMode::BackwardDiagonalStripes);
        REQUIRE_EQUAL(options.frameColor().color(0), Color(fg::BrightWhite, bg::Red));
        REQUIRE_EQUAL(options.frameColorMode(), FrameColorMode::BackwardDiagonalStripes);

        options.setFillColor(Color{fg::Yellow, bg::Blue});
        REQUIRE_EQUAL(options.fillColor().color(0), Color(fg::Yellow, bg::Blue));

        options.setFillColor({});
        REQUIRE_EQUAL(options.fillColor().color(0), Color{});

        options.setFillColor(Foreground{fg::Green});
        REQUIRE_EQUAL(options.fillColor().color(0), Color(fg::Green, bg::Inherited));

        options.setFillColor(fg::Magenta);
        REQUIRE_EQUAL(options.fillColor().color(0), Color(fg::Magenta, bg::Inherited));

        options.setFillColor(Foreground{fg::Cyan}, Background{bg::Black});
        REQUIRE_EQUAL(options.fillColor().color(0), Color(fg::Cyan, bg::Black));

        options.setFillColor(Background{bg::Yellow});
        REQUIRE_EQUAL(options.fillColor().color(0), Color(fg::Inherited, bg::Yellow));

        options.setFillColor(bg::Blue);
        REQUIRE_EQUAL(options.fillColor().color(0), Color(fg::Inherited, bg::Blue));

        options.setFillColorSequence(
            ColorSequence{Color{fg::BrightWhite, bg::Red}}, FrameColorMode::ForwardDiagonalStripes);
        REQUIRE_EQUAL(options.fillColor().color(0), Color(fg::BrightWhite, bg::Red));
        REQUIRE_EQUAL(options.fillColorMode(), FrameColorMode::ForwardDiagonalStripes);
    }

    void testStylePointersAndAnimationSettingsCanBeConfigured() {
        auto options = FrameDrawOptions{};
        const auto block16Style = Block16Style::lightFrame();
        const auto tile9Style = Tile9Style::outerHalfBlockFrame();
        const auto combinationStyle = BlockCombinationStyle::colorOverlay();
        const auto expectedFillBlock = Block{U'#', fg::Green, bg::Black};

        options.setFillBlock(expectedFillBlock);
        options.setStyle(FrameStyle::Double);
        options.setBlock16Style(block16Style);
        options.setTile9Style(tile9Style);
        options.setCombinationStyle(combinationStyle);
        options.setAnimationOffset(7);
        options.setFrameColorMode(FrameColorMode::ChasingBorderCW);
        options.setFillColorMode(FrameColorMode::VerticalStripes);

        REQUIRE_EQUAL(options.fillBlock(), expectedFillBlock);
        REQUIRE_EQUAL(options.style(), FrameStyle::Double);
        REQUIRE_EQUAL(options.block16Style(), block16Style);
        REQUIRE_EQUAL(options.tile9Style(), tile9Style);
        REQUIRE_EQUAL(options.combinationStyle(), combinationStyle);
        REQUIRE_EQUAL(options.animationOffset(), static_cast<std::size_t>(7));
        REQUIRE_EQUAL(options.frameColorMode(), FrameColorMode::ChasingBorderCW);
        REQUIRE_EQUAL(options.fillColorMode(), FrameColorMode::VerticalStripes);
    }
};
