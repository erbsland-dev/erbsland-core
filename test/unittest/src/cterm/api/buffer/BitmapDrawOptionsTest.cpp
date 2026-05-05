// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/TestHelper.hpp"

#include <erbsland/unittest/UnitTest.hpp>

TESTED_TARGETS(BitmapDrawOptions)
class BitmapDrawOptionsTest final : public el::UnitTest {
public:
    void testDefaultOptionsExposeTheExpectedDefaults() {
        const auto &options = BitmapDrawOptions::defaultOptions();

        REQUIRE_EQUAL(options.color().sequenceLength(), static_cast<std::size_t>(1));
        REQUIRE_EQUAL(options.color().color(0), Color(fg::Default, bg::Default));
        REQUIRE_EQUAL(options.colorMode(), BitmapColorMode::OneColor);
        REQUIRE_EQUAL(options.colorAnimationOffset(), static_cast<std::size_t>(0));
        REQUIRE(options.block16Style() == nullptr);
        REQUIRE(options.combinationStyle() == nullptr);
        REQUIRE_EQUAL(options.fullBlock(), U'█');
        REQUIRE_EQUAL(options.doubleBlocks().length(), BlockCount{2U});
        REQUIRE_EQUAL(options.doubleBlocks()[BlockIndex{0U}], U'█');
        REQUIRE_EQUAL(options.doubleBlocks()[BlockIndex{1U}], U'█');
        REQUIRE_EQUAL(options.halfBlocks().length(), BlockCount{16U});
        REQUIRE_EQUAL(options.halfBlocks()[BlockIndex{0U}], U' ');
        REQUIRE_EQUAL(options.halfBlocks()[BlockIndex{15U}], U'█');
        REQUIRE_EQUAL(options.scaleMode(), BitmapScaleMode::HalfBlock);
        REQUIRE(&BitmapDrawOptions::defaultOptions() == &options);
    }

    void testColorSequenceAndSettersReplaceTheConfiguredBaseColors() {
        auto options = BitmapDrawOptions{
            ColorSequence{
                Color{fg::Red, bg::Black},
                Color{fg::Blue, bg::Black},
            },
            BitmapColorMode::VerticalStripes};

        REQUIRE_EQUAL(options.color().sequenceLength(), static_cast<std::size_t>(2));
        REQUIRE_EQUAL(options.color().color(0), Color(fg::Red, bg::Black));
        REQUIRE_EQUAL(options.color().color(1), Color(fg::Blue, bg::Black));
        REQUIRE_EQUAL(options.colorMode(), BitmapColorMode::VerticalStripes);

        options.setColorMode(BitmapColorMode::HorizontalStripes);
        options.setColorAnimationOffset(5);
        REQUIRE_EQUAL(options.colorMode(), BitmapColorMode::HorizontalStripes);
        REQUIRE_EQUAL(options.colorAnimationOffset(), static_cast<std::size_t>(5));

        options.setColor(Color{fg::Yellow, bg::Blue});
        REQUIRE_EQUAL(options.color().sequenceLength(), static_cast<std::size_t>(1));
        REQUIRE_EQUAL(options.color().color(0), Color(fg::Yellow, bg::Blue));

        options.setColor({});
        REQUIRE_EQUAL(options.color().color(0), Color{});

        options.setColor(Foreground{fg::Green});
        REQUIRE_EQUAL(options.color().color(0), Color(fg::Green, bg::Inherited));

        options.setColor(fg::Magenta);
        REQUIRE_EQUAL(options.color().color(0), Color(fg::Magenta, bg::Inherited));

        options.setColor(Foreground{fg::Cyan}, Background{bg::Black});
        REQUIRE_EQUAL(options.color().color(0), Color(fg::Cyan, bg::Black));

        options.setColor(Background{bg::Yellow});
        REQUIRE_EQUAL(options.color().color(0), Color(fg::Inherited, bg::Yellow));

        options.setColor(bg::Blue);
        REQUIRE_EQUAL(options.color().color(0), Color(fg::Inherited, bg::Blue));

        options.setColorSequence(
            ColorSequence{Color{fg::BrightWhite, bg::Red}}, BitmapColorMode::BackwardDiagonalStripes);
        REQUIRE_EQUAL(options.color().sequenceLength(), static_cast<std::size_t>(1));
        REQUIRE_EQUAL(options.color().color(0), Color(fg::BrightWhite, bg::Red));
        REQUIRE_EQUAL(options.colorMode(), BitmapColorMode::BackwardDiagonalStripes);
    }

    void testStylePointersAndConfiguredBlocksCanBeAccessed() {
        auto options = BitmapDrawOptions{};
        const auto block16Style = Block16Style::lightFrame();
        const auto combinationStyle = BlockCombinationStyle::colorOverlay();

        options.setBlock16Style(block16Style);
        options.setCombinationStyle(combinationStyle);
        options.setScaleMode(BitmapScaleMode::DoubleBlock);
        options.setFullBlock(Block{U'#', fg::Green, bg::Black});
        options.setDoubleBlocks(BlockString{"[]"_el});
        options.setHalfBlocks(BlockString{"abcdefghijklmnop"_el});

        REQUIRE(options.block16Style() == block16Style);
        REQUIRE(options.combinationStyle() == combinationStyle);
        REQUIRE_EQUAL(options.scaleMode(), BitmapScaleMode::DoubleBlock);
        REQUIRE_EQUAL(options.fullBlock(), U'#');
        REQUIRE_EQUAL(options.fullBlock().color(), Color(fg::Green, bg::Black));
        REQUIRE_EQUAL(options.doubleBlocks().length(), BlockCount{2U});
        REQUIRE_EQUAL(options.doubleBlocks()[BlockIndex{0U}], U'[');
        REQUIRE_EQUAL(options.doubleBlocks()[BlockIndex{1U}], U']');
        REQUIRE_EQUAL(options.halfBlocks().length(), BlockCount{16U});
        REQUIRE_EQUAL(options.halfBlocks()[BlockIndex{0U}], U'a');
        REQUIRE_EQUAL(options.halfBlocks()[BlockIndex{15U}], U'p');
    }

    void testBlockValidationRejectsInvalidWidthsAndCounts() {
        auto options = BitmapDrawOptions{};

        REQUIRE_THROWS_AS(std::invalid_argument, options.setFullBlock(Block{U'界'}));
        REQUIRE_THROWS_AS(std::invalid_argument, options.setDoubleBlocks(BlockString{"X"_el}));
        REQUIRE_THROWS_AS(std::invalid_argument, options.setDoubleBlocks(BlockString{U"界X"_el}));
        REQUIRE_THROWS_AS(std::invalid_argument, options.setHalfBlocks(BlockString{"short"_el}));
        REQUIRE_THROWS_AS(std::invalid_argument, options.setHalfBlocks(BlockString{U"abcdefghijklmno界"_el}));
    }
};
