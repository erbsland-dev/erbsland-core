// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/BufferTestHelper.hpp"

#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <memory>
#include <string>
#include <vector>

TESTED_TARGETS(Buffer)
class BufferTest final : public UNITTEST_SUBCLASS(BufferTestHelper) {
public:
    void testConstructorAndFill() {
        Buffer buffer(block::Size(2, 3));
        REQUIRE_EQUAL(buffer.size(), block::Size(2, 3));
        buffer.size().forEach([&](const block::Position pos) { REQUIRE_EQUAL(buffer.get(pos), U' '); });
    }

    void testFillConstructor() {
        const Block fill{"X"_el, fg(fg::Red), bg(bg::Blue)};
        Buffer buffer(block::Size(2, 2));
        buffer.fill(fill);
        buffer.size().forEach([&](const block::Position pos) {
            const auto &block = buffer.get(pos);
            REQUIRE_EQUAL(block, U'X');
            REQUIRE_EQUAL(block.color(), Color(fg::Red, bg::Blue));
        });
    }

    void testDefaultAndFillConstructorsInitializeCellsAndValidateSize() {
        const auto fill = Block{U'X', fg::Yellow, bg::Blue};

        const auto placeholder = Buffer{};
        REQUIRE_EQUAL(placeholder.size(), block::Size(1, 1));
        REQUIRE_EQUAL(placeholder.get(block::Position{0, 0}), U' ');

        const auto filled = Buffer{block::Size{2, 2}, fill};
        filled.size().forEach([&](const block::Position pos) -> void {
            REQUIRE_EQUAL(filled.get(pos), U'X');
            REQUIRE_EQUAL(filled.get(pos).color(), Color(fg::Yellow, bg::Blue));
        });

        REQUIRE_THROWS_AS(erbsland::err::ParameterError, Buffer(block::Size{0, 1}));
        REQUIRE_THROWS_AS(erbsland::err::ParameterError, Buffer(block::Size{1, 0}));
        REQUIRE_THROWS_AS(erbsland::err::ParameterError, Buffer(block::Size{10'001, 1}));
    }

    void testResizeWithPreserveContentKeepsContentAndFillsExpandedCells() {
        auto buffer = Buffer{block::Size{2, 2}};
        buffer.set(block::Position{0, 0}, Block{U'A'});
        buffer.set(block::Position{1, 0}, Block{U'B'});
        buffer.set(block::Position{0, 1}, Block{U'C'});
        buffer.set(block::Position{1, 1}, Block{U'D'});

        buffer.resize(block::Size{3, 3}, BufferResizeMode::PreserveContent, Block{U'.'});

        REQUIRE_EQUAL(buffer.size(), block::Size(3, 3));
        requireRowsEqual(buffer, {"AB.", "CD.", "..."});
    }

    void testResizeWithPreserveContentCropsToTheNewRectangle() {
        auto buffer = Buffer{block::Size{3, 2}};
        buffer.set(block::Position{0, 0}, Block{U'A'});
        buffer.set(block::Position{1, 0}, Block{U'B'});
        buffer.set(block::Position{2, 0}, Block{U'C'});
        buffer.set(block::Position{0, 1}, Block{U'D'});
        buffer.set(block::Position{1, 1}, Block{U'E'});
        buffer.set(block::Position{2, 1}, Block{U'F'});

        buffer.resize(block::Size{2, 1}, BufferResizeMode::PreserveContent, Block{U'.'});

        REQUIRE_EQUAL(buffer.size(), block::Size(2, 1));
        requireRowsEqual(buffer, {"AB"});
    }

    void testResizeWithPreserveContentShrinkingWidthFillsRowsAddedByHeightExpansion() {
        auto buffer = Buffer{block::Size{3, 2}};
        buffer.set(block::Position{0, 0}, Block{U'A'});
        buffer.set(block::Position{1, 0}, Block{U'B'});
        buffer.set(block::Position{2, 0}, Block{U'C'});
        buffer.set(block::Position{0, 1}, Block{U'D'});
        buffer.set(block::Position{1, 1}, Block{U'E'});
        buffer.set(block::Position{2, 1}, Block{U'F'});

        buffer.resize(block::Size{2, 3}, BufferResizeMode::PreserveContent, Block{U'.'});

        REQUIRE_EQUAL(buffer.size(), block::Size(2, 3));
        requireRowsEqual(buffer, {"AB", "DE", ".."});
    }

    void testResizeWithFastModeFillsExpandedTailWithExplicitCharacter() {
        auto buffer = Buffer{block::Size{2, 1}};
        buffer.set(block::Position{0, 0}, Block{U'A'});
        buffer.set(block::Position{1, 0}, Block{U'B'});

        buffer.resize(block::Size{4, 1}, BufferResizeMode::Fast, Block{U'.'});

        REQUIRE_EQUAL(buffer.size(), block::Size(4, 1));
        requireRowsEqual(buffer, {"AB.."});
    }

    void testResizeRejectsInvalidSizes() {
        auto buffer = Buffer{block::Size{2, 2}};

        REQUIRE_THROWS_AS(erbsland::err::ParameterError, buffer.resize(block::Size{0, 2}));
        REQUIRE_THROWS_AS(erbsland::err::ParameterError, buffer.resize(block::Size{2, 0}));
        REQUIRE_THROWS_AS(erbsland::err::ParameterError, buffer.resize(block::Size{10'001, 1}));
    }

    void testSetAndGet() {
        Buffer buffer(block::Size(2, 2));
        Block block{U'A', fg::Green, bg::Black};
        buffer.set(block::Position(1, 0), block);
        REQUIRE_EQUAL(buffer.get(block::Position(1, 0)), U'A');
        REQUIRE_EQUAL(buffer.get(block::Position(1, 0)).color(), Color(fg::Green, bg::Black));
        REQUIRE_EQUAL(buffer.get(block::Position(0, 0)), U' ');
        buffer.set(block::Position(5, 5), Block{"B"_el, fg(fg::Blue), bg(bg::Red)});
        REQUIRE_EQUAL(buffer.get(block::Position(0, 1)), U' ');
    }

    void testSetWideCharacterWithLvalueFillsTheFollowingCell() {
        Buffer buffer(block::Size(4, 1));
        auto attributes = BlockAttributes{};
        attributes.setItalic(true);
        const auto block = Block{U'界', Color{fg::Green, bg::Blue}, attributes};

        buffer.set(block::Position(1, 0), block);

        REQUIRE_EQUAL(buffer.get(block::Position(1, 0)), U'界');
        REQUIRE_EQUAL(buffer.get(block::Position(1, 0)).color(), Color(fg::Green, bg::Blue));
        REQUIRE(buffer.get(block::Position(1, 0)).attributes().isItalic());
        REQUIRE(buffer.get(block::Position(2, 0)).isEmpty());
        REQUIRE_EQUAL(buffer.get(block::Position(2, 0)).color(), Color(fg::Green, bg::Blue));
        REQUIRE(buffer.get(block::Position(2, 0)).attributes().isItalic());
        REQUIRE_EQUAL(buffer.get(block::Position(3, 0)), U' ');
        REQUIRE_EQUAL(buffer.get(block::Position(3, 0)).color(), Color{});
    }

    void testSetWideCharacterWithRvalueFillsTheFollowingCell() {
        Buffer buffer(block::Size(4, 1));

        buffer.set(block::Position(0, 0), Block{U'界', fg::Yellow, bg::Black});

        REQUIRE_EQUAL(buffer.get(block::Position(0, 0)), U'界');
        REQUIRE_EQUAL(buffer.get(block::Position(0, 0)).color(), Color(fg::Yellow, bg::Black));
        REQUIRE(buffer.get(block::Position(1, 0)).isEmpty());
        REQUIRE_EQUAL(buffer.get(block::Position(1, 0)).color(), Color(fg::Yellow, bg::Black));
        REQUIRE_EQUAL(buffer.get(block::Position(2, 0)), U' ');
        REQUIRE_EQUAL(buffer.get(block::Position(2, 0)).color(), Color{});
    }

    void testSetWideCharacterAtTheRightEdgeIsIgnored() {
        auto buffer = Buffer{block::Size{2, 1}};
        buffer.fill(Block{U'.'});

        buffer.set(block::Position{1, 0}, Block{U'界', fg::Yellow, bg::Blue});

        requireRowsEqual(buffer, {".."});
        REQUIRE_EQUAL(buffer.get(block::Position{0, 0}), U'.');
        REQUIRE_EQUAL(buffer.get(block::Position{1, 0}), U'.');
    }

    void testSetStringWritesMultipleLinesAndSkipsZeroWidthCharacters() {
        auto buffer = Buffer{block::Size{4, 2}, Block{U'.'}};
        auto text = BlockStringEditor{};
        text.append(Block{U'界', fg::Red, bg::Black});
        text.append(Block{});
        text.append(Block{U'A', fg::Green, bg::Black});
        text.append(Block{U'\n'});
        text.append(Block{U'B', fg::Blue, bg::Black});

        buffer.set(block::Position{0, 0}, text);

        REQUIRE_EQUAL(buffer.get(block::Position{0, 0}), U'界');
        REQUIRE_EQUAL(buffer.get(block::Position{0, 0}).color(), Color(fg::Red, bg::Black));
        REQUIRE(buffer.get(block::Position{1, 0}).isEmpty());
        REQUIRE_EQUAL(buffer.get(block::Position{1, 0}).color(), Color(fg::Red, bg::Black));
        REQUIRE_EQUAL(buffer.get(block::Position{2, 0}), U'A');
        REQUIRE_EQUAL(buffer.get(block::Position{2, 0}).color(), Color(fg::Green, bg::Black));
        REQUIRE_EQUAL(buffer.get(block::Position{0, 1}), U'B');
        REQUIRE_EQUAL(buffer.get(block::Position{0, 1}).color(), Color(fg::Blue, bg::Black));
        REQUIRE_EQUAL(buffer.get(block::Position{3, 1}), U'.');
    }

    void testFromLinesFactoriesBuildBuffersFromPlainAndWideText() {
        const auto lines = BlockStringLines{BlockStringEditor{"ab"_el}, BlockStringEditor{}, BlockStringEditor{"c"_el}};
        const auto fromLines = Buffer::fromLines(lines);

        REQUIRE_EQUAL(fromLines.size(), block::Size(2, 3));
        requireRowsEqual(fromLines, {"ab", "  ", "c "});

        const auto sourceText = BlockStringEditor{"x界\nA!"_el};
        const auto fromText = Buffer::fromLinesInString(sourceText.slice(BlockRange{BlockIndex{1U}, BlockCount{3U}}));

        REQUIRE_EQUAL(fromText.size(), block::Size(2, 2));
        REQUIRE_EQUAL(fromText.get(block::Position{0, 0}), U'界');
        REQUIRE(fromText.get(block::Position{1, 0}).isEmpty());
        REQUIRE_EQUAL(fromText.get(block::Position{1, 0}).color(), Color{});
        requireRowsEqual(fromText, {"界 ", "A "});
    }

    void testFromLinesFactoriesRejectEmptyInput() {
        REQUIRE_THROWS_AS(erbsland::err::ParameterError, Buffer::fromLines(BlockStringLines{}));
        REQUIRE_THROWS_AS(erbsland::err::ParameterError, Buffer::fromLinesInString(BlockString{}));
    }

    void testCloneCreatesAnIndependentWritableCopy() {
        auto buffer = Buffer{block::Size{2, 1}};
        buffer.set(block::Position{0, 0}, Block{U'A'});

        const auto cloned = buffer.clone();

        REQUIRE_NOT_EQUAL(cloned, nullptr);
        cloned->set(block::Position{1, 0}, Block{U'B', fg::Red, bg::Black});

        REQUIRE_EQUAL(buffer.get(block::Position{0, 0}), U'A');
        REQUIRE_EQUAL(buffer.get(block::Position{1, 0}), U' ');
        REQUIRE_EQUAL(cloned->get(block::Position{0, 0}), U'A');
        REQUIRE_EQUAL(cloned->get(block::Position{1, 0}), U'B');
        REQUIRE_EQUAL(cloned->get(block::Position{1, 0}).color(), Color(fg::Red, bg::Black));
    }

    void testSetAndResizeFromCopiesBufferInstances() {
        auto source = Buffer{block::Size{2, 2}};
        source.set(block::Position{0, 0}, Block{U'A'});
        source.set(block::Position{1, 0}, Block{U'B'});
        source.set(block::Position{0, 1}, Block{U'C'});
        source.set(block::Position{1, 1}, Block{U'D'});
        auto target = Buffer{block::Size{1, 1}};

        target.setAndResizeFrom(source);

        REQUIRE_EQUAL(target.size(), block::Size(2, 2));
        requireRowsEqual(target, {"AB", "CD"});
    }

    void testSetAndResizeFromAlsoSupportsReadableViews() {
        auto source = Buffer{block::Size{3, 2}};
        source.set(block::Position{0, 0}, Block{U'A'});
        source.set(block::Position{1, 0}, Block{U'B'});
        source.set(block::Position{2, 0}, Block{U'C'});
        source.set(block::Position{0, 1}, Block{U'D'});
        source.set(block::Position{1, 1}, Block{U'E'});
        source.set(block::Position{2, 1}, Block{U'F'});
        const auto view = BufferConstRefView{source, block::Rectangle{1, 0, 2, 2}};
        auto target = Buffer{block::Size{1, 1}};

        target.setAndResizeFrom(view);

        REQUIRE_EQUAL(target.size(), block::Size(2, 2));
        requireRowsEqual(target, {"BC", "EF"});
    }

    void testToMaskMatchesCharactersFromStringAndCanInvertTheSelection() {
        auto buffer = Buffer{block::Size{3, 2}};
        buffer.set(block::Position{0, 0}, Block{U'A', fg::Red, bg::Black});
        buffer.set(block::Position{1, 0}, Block{U'B', fg::Blue, bg::Black});
        buffer.set(block::Position{2, 0}, Block{U'C', fg::Green, bg::Black});
        buffer.set(block::Position{0, 1}, Block{U'B', fg::Yellow, bg::Black});
        buffer.set(block::Position{1, 1}, Block{U' ', fg::White, bg::Black});
        buffer.set(block::Position{2, 1}, Block{U'A', fg::Magenta, bg::Black});

        const auto mask = buffer.toMask(erbsland::text::CharSet{"AB"_el});
        const auto invertedMask = buffer.toMask(erbsland::text::CharSet{"AB"_el}, true);

        REQUIRE(mask.pixel(block::Position{0, 0}));
        REQUIRE(mask.pixel(block::Position{1, 0}));
        REQUIRE_FALSE(mask.pixel(block::Position{2, 0}));
        REQUIRE(mask.pixel(block::Position{0, 1}));
        REQUIRE_FALSE(mask.pixel(block::Position{1, 1}));
        REQUIRE(mask.pixel(block::Position{2, 1}));

        REQUIRE_FALSE(invertedMask.pixel(block::Position{0, 0}));
        REQUIRE_FALSE(invertedMask.pixel(block::Position{1, 0}));
        REQUIRE(invertedMask.pixel(block::Position{2, 0}));
        REQUIRE_FALSE(invertedMask.pixel(block::Position{0, 1}));
        REQUIRE(invertedMask.pixel(block::Position{1, 1}));
        REQUIRE_FALSE(invertedMask.pixel(block::Position{2, 1}));
    }

    void testToMaskMatchesCharactersFromInitializerList() {
        auto buffer = Buffer{block::Size{2, 2}};
        buffer.set(block::Position{0, 0}, Block{U'X'});
        buffer.set(block::Position{1, 0}, Block{U' '});
        buffer.set(block::Position{0, 1}, Block{U'Y'});
        buffer.set(block::Position{1, 1}, Block{U'Z'});

        const auto mask = buffer.toMask({U'Y', U'Z'});

        REQUIRE_FALSE(mask.pixel(block::Position{0, 0}));
        REQUIRE_FALSE(mask.pixel(block::Position{1, 0}));
        REQUIRE(mask.pixel(block::Position{0, 1}));
        REQUIRE(mask.pixel(block::Position{1, 1}));
    }

    void testToMaskWithEmptyCharacterSetsReturnsAClearedBitmap() {
        auto buffer = Buffer{block::Size{2, 2}};
        buffer.set(block::Position{0, 0}, Block{U'X'});
        buffer.set(block::Position{1, 1}, Block{U'Y'});

        const auto stringMask = buffer.toMask(erbsland::text::CharSet{});
        const auto listMask = buffer.toMask({});

        REQUIRE_EQUAL(stringMask.size(), block::Size(2, 2));
        REQUIRE_FALSE(stringMask.pixel(block::Position{0, 0}));
        REQUIRE_FALSE(stringMask.pixel(block::Position{1, 0}));
        REQUIRE_FALSE(stringMask.pixel(block::Position{0, 1}));
        REQUIRE_FALSE(stringMask.pixel(block::Position{1, 1}));

        REQUIRE_EQUAL(listMask.size(), block::Size(2, 2));
        REQUIRE_FALSE(listMask.pixel(block::Position{0, 0}));
        REQUIRE_FALSE(listMask.pixel(block::Position{1, 0}));
        REQUIRE_FALSE(listMask.pixel(block::Position{0, 1}));
        REQUIRE_FALSE(listMask.pixel(block::Position{1, 1}));
    }

    void testFillRectangleAppliesCombinationStyle() {
        Buffer buffer(block::Size(3, 3));
        buffer.fill(Block{U'A', fg::Green, bg::Blue});

        buffer.fill(
            block::Rectangle{1, 1, 1, 1},
            Block{U'X', fg::Inherited, bg::Yellow},
            BlockCombinationStyle::colorOverlay());

        REQUIRE_EQUAL(buffer.get(block::Position(1, 1)), U'X');
        REQUIRE_EQUAL(buffer.get(block::Position(1, 1)).color(), Color(fg::Green, bg::Yellow));
        REQUIRE_EQUAL(buffer.get(block::Position(0, 0)), U'A');
        REQUIRE_EQUAL(buffer.get(block::Position(0, 0)).color(), Color(fg::Green, bg::Blue));
    }

    void testFillRectangleWithTile9StyleRepeatsEdgesAndCenter() {
        Buffer buffer(block::Size(4, 3));

        buffer.fill(block::Rectangle{0, 0, 4, 3}, Tile9Style::create("ABCDEFGHI"_el));

        requireRowsEqual(buffer, {"ABBC", "DEEF", "GHHI"});
    }

    void testFillRectangleWithTile9StyleAppliesBaseColorBeforeTileColor() {
        auto tiles = std::array<Block, 9>{};
        tiles[0] = Block{U'A', fg::Red, bg::Inherited};
        tiles[1] = Block{U'B'};
        tiles[2] = Block{U'C'};
        tiles[3] = Block{U'D'};
        tiles[4] = Block{U'E'};
        tiles[5] = Block{U'F'};
        tiles[6] = Block{U'G'};
        tiles[7] = Block{U'H'};
        tiles[8] = Block{U'I'};
        const auto style = std::make_shared<Tile9Style>(tiles);
        Buffer buffer(block::Size(2, 2));

        buffer.fill(block::Rectangle{0, 0, 2, 2}, style, Color{fg::Blue, bg::Black});

        REQUIRE_EQUAL(buffer.get(block::Position{0, 0}).color(), Color(fg::Red, bg::Black));
        REQUIRE_EQUAL(buffer.get(block::Position{1, 0}).color(), Color(fg::Blue, bg::Black));
    }

    void testDrawFrameWithCharacterMarksTheWholePerimeter() {
        Buffer buffer(block::Size(3, 3));

        buffer.drawFrame(block::Rectangle{0, 0, 3, 3}, Block{U'#'});

        requireRowsEqual(buffer, {"###", "# #", "###"});
    }

    void testDrawFrameWithStyleRendersExpectedBoxCharacters() {
        Buffer buffer(block::Size(4, 3));

        buffer.drawFrame(block::Rectangle{0, 0, 4, 3}, FrameStyle::LightWithRoundedCorners);

        requireRowsEqual(buffer, {"╭──╮", "│  │", "╰──╯"});
    }

    void testDrawFrameWithNoneStyleDrawsColoredSpacesOnThePerimeter() {
        Buffer buffer(block::Size(3, 3));
        buffer.fill(Block{U'X', fg::Green, bg::Blue});

        buffer.drawFrame(block::Rectangle{0, 0, 3, 3}, FrameStyle::None, Color{fg::Red, bg::Black});

        requireRowsEqual(buffer, {"   ", " X ", "   "});
        REQUIRE_EQUAL(buffer.get(block::Position{0, 0}).color(), Color(fg::Red, bg::Black));
        REQUIRE_EQUAL(buffer.get(block::Position{1, 1}).color(), Color(fg::Green, bg::Blue));
    }

    void testDrawFrameWithTile9StyleOmitsTheCenterTile() {
        Buffer buffer(block::Size(4, 3));

        buffer.drawFrame(block::Rectangle{0, 0, 4, 3}, Tile9Style::create("ABCDEFGHI"_el));

        requireRowsEqual(buffer, {"ABBC", "D  F", "GHHI"});
    }

    void testDrawFrameWithNewPredefinedStylesRendersExpectedShapes() {
        {
            Buffer buffer(block::Size(4, 3));
            buffer.drawFrame(block::Rectangle{0, 0, 4, 3}, FrameStyle::LightDoubleDash);
            requireRowsEqual(buffer, {"┌╌╌┐", "╎  ╎", "└╌╌┘"});
        }
        {
            Buffer buffer(block::Size(4, 3));
            buffer.drawFrame(block::Rectangle{0, 0, 4, 3}, FrameStyle::HeavyQuadrupleDash);
            requireRowsEqual(buffer, {"┏┉┉┓", "┋  ┋", "┗┉┉┛"});
        }
        {
            Buffer buffer(block::Size(4, 3));
            buffer.drawFrame(block::Rectangle{0, 0, 4, 3}, FrameStyle::FullBlock);
            requireRowsEqual(buffer, {"████", "█  █", "████"});
        }
        {
            Buffer buffer(block::Size(4, 3));
            buffer.drawFrame(block::Rectangle{0, 0, 4, 3}, FrameStyle::FullBlockWithChamfer);
            requireRowsEqual(buffer, {"◢██◣", "█  █", "◥██◤"});
        }
        {
            Buffer buffer(block::Size(4, 3));
            buffer.drawFrame(block::Rectangle{0, 0, 4, 3}, FrameStyle::OuterHalfBlock);
            requireRowsEqual(buffer, {"▛▀▀▜", "▌  ▐", "▙▄▄▟"});
        }
        {
            Buffer buffer(block::Size(4, 3));
            buffer.drawFrame(block::Rectangle{0, 0, 4, 3}, FrameStyle::InnerHalfBlock);
            requireRowsEqual(buffer, {"▗▄▄▖", "▐  ▌", "▝▀▀▘"});
        }
    }

    void testDrawFrameWithTile9StylesHandlesDegenerateRectangles() {
        {
            Buffer buffer(block::Size(1, 1));
            buffer.drawFrame(block::Rectangle{0, 0, 1, 1}, FrameStyle::OuterHalfBlock);
            requireRowsEqual(buffer, {"█"});
        }
        {
            Buffer buffer(block::Size(3, 1));
            buffer.drawFrame(block::Rectangle{0, 0, 3, 1}, FrameStyle::OuterHalfBlock);
            requireRowsEqual(buffer, {"███"});
        }
        {
            Buffer buffer(block::Size(1, 3));
            buffer.drawFrame(block::Rectangle{0, 0, 1, 3}, FrameStyle::InnerHalfBlock);
            requireRowsEqual(buffer, {"█", "█", "█"});
        }
    }

    void testDrawFrameWithStyleAppliesBaseFrameColorBeforeTileColor() {
        auto tiles = std::array<Block, 16>{};
        tiles[3] = Block{U'┌', fg::Red, bg::Inherited};
        tiles[5] = Block{U'─'};
        tiles[6] = Block{U'┐'};
        tiles[9] = Block{U'└'};
        tiles[10] = Block{U'│'};
        tiles[12] = Block{U'┘'};
        const auto style = std::make_shared<Block16Style>(tiles);
        Buffer buffer(block::Size(3, 3));

        buffer.drawFrame(block::Rectangle{0, 0, 3, 3}, style, BlockCombinationStylePtr{}, Color{fg::Blue, bg::Black});

        REQUIRE_EQUAL(buffer.get(block::Position(0, 0)).color(), Color(fg::Red, bg::Black));
        REQUIRE_EQUAL(buffer.get(block::Position(1, 0)).color(), Color(fg::Blue, bg::Black));
    }

    void testDrawFilledFrameWithTile9FrameStyleFillsTheInterior() {
        Buffer buffer(block::Size(4, 3));

        buffer.drawFilledFrame(block::Rectangle{0, 0, 4, 3}, FrameStyle::OuterHalfBlock, Block{U'.'});

        requireRowsEqual(buffer, {"▛▀▀▜", "▌..▐", "▙▄▄▟"});
    }

    void testDrawFilledFrameWithStyleAppliesCombinationStyleToFill() {
        Buffer buffer(block::Size(3, 3));
        buffer.fill(Block{U'A', fg::Green, bg::Blue});

        buffer.drawFilledFrame(
            block::Rectangle{0, 0, 3, 3},
            Block16Style::lightFrame(),
            Block{U' ', fg::Inherited, bg::Yellow},
            BlockCombinationStyle::colorOverlay());

        REQUIRE_EQUAL(buffer.get(block::Position(1, 1)), U' ');
        REQUIRE_EQUAL(buffer.get(block::Position(1, 1)).color(), Color(fg::Green, bg::Yellow));
    }

    void testDrawFrameWithOptionsAppliesFrameAndFillColorOverlayOrder() {
        Buffer buffer(block::Size(3, 3));
        buffer.fill(Block{U' ', fg::Green, bg::Blue});
        auto options = FrameDrawOptions{};
        options.setFrameColor(Color{fg::Red, bg::Inherited});
        options.setFillColor(Color{fg::Inherited, bg::Yellow});
        options.setFillBlock(Block{U'.', fg::White, bg::Inherited});

        buffer.drawFrame(block::Rectangle{0, 0, 3, 3}, options);

        requireRowsEqual(buffer, {"┌─┐", "│.│", "└─┘"});
        REQUIRE_EQUAL(buffer.get(block::Position{0, 0}).color(), Color(fg::Red, bg::Blue));
        REQUIRE_EQUAL(buffer.get(block::Position{1, 1}).color(), Color(fg::White, bg::Yellow));
    }

    void testDrawFrameWithOptionsUsesStripeModesAcrossFrameAndFill() {
        Buffer buffer(block::Size(4, 3));
        buffer.fill(Block{U' ', fg::White, bg::Black});
        auto options = FrameDrawOptions{};
        options.setFrameColorSequence(
            ColorSequence{
                Color{fg::Red, bg::Inherited},
                Color{fg::Blue, bg::Inherited},
            },
            FrameColorMode::VerticalStripes);
        options.setFillColorSequence(
            ColorSequence{
                Color{fg::Inherited, bg::Yellow},
                Color{fg::Inherited, bg::Magenta},
            },
            FrameColorMode::HorizontalStripes);
        options.setFillBlock(Block{U' '});

        buffer.drawFrame(block::Rectangle{0, 0, 4, 3}, options);

        REQUIRE_EQUAL(buffer.get(block::Position{0, 0}).color(), Color(fg::Red, bg::Black));
        REQUIRE_EQUAL(buffer.get(block::Position{1, 0}).color(), Color(fg::Blue, bg::Black));
        REQUIRE_EQUAL(buffer.get(block::Position{0, 1}).color(), Color(fg::Red, bg::Black));
        REQUIRE_EQUAL(buffer.get(block::Position{1, 1}).color(), Color(fg::Blue, bg::Magenta));
        REQUIRE_EQUAL(buffer.get(block::Position{2, 1}).color(), Color(fg::Red, bg::Magenta));
    }

    void testDrawFrameWithOptionsPrefersCustomStylesAndTile9Fill() {
        auto customTiles = std::array<Block, 16>{};
        customTiles.fill(Block{U'X'});
        const auto customStyle = std::make_shared<Block16Style>(customTiles);
        auto options = FrameDrawOptions{};
        options.setStyle(FrameStyle::LightWithRoundedCorners);
        options.setBlock16Style(customStyle);

        {
            Buffer buffer(block::Size(4, 3));
            buffer.drawFrame(block::Rectangle{0, 0, 4, 3}, options);
            requireRowsEqual(buffer, {"XXXX", "X  X", "XXXX"});
        }
        {
            Buffer buffer(block::Size(4, 3));
            options.setTile9Style(Tile9Style::create("ABCDEFGHI"_el));
            options.setFillBlock(Block{U'.'});
            buffer.drawFrame(block::Rectangle{0, 0, 4, 3}, options);
            requireRowsEqual(buffer, {"ABBC", "DEEF", "GHHI"});
        }
    }

    void testDrawFrameWithOptionsChasingBorderModesAnimateClockwiseAndCounterClockwise() {
        const auto colors = ColorSequence{
            Color{fg::Red, bg::Black},
            Color{fg::Green, bg::Black},
            Color{fg::Blue, bg::Black},
        };

        {
            Buffer buffer(block::Size(4, 4));
            auto options = FrameDrawOptions{};
            options.setFrameColorSequence(colors, FrameColorMode::ChasingBorderCW);

            buffer.drawFrame(block::Rectangle{0, 0, 4, 4}, options, 1);

            REQUIRE_EQUAL(buffer.get(block::Position{0, 0}).color(), Color(fg::Blue, bg::Black));
            REQUIRE_EQUAL(buffer.get(block::Position{1, 0}).color(), Color(fg::Red, bg::Black));
            REQUIRE_EQUAL(buffer.get(block::Position{2, 0}).color(), Color(fg::Green, bg::Black));
            REQUIRE_EQUAL(buffer.get(block::Position{3, 1}).color(), Color(fg::Red, bg::Black));
            REQUIRE_EQUAL(buffer.get(block::Position{0, 1}).color(), Color(fg::Green, bg::Black));
        }
        {
            Buffer buffer(block::Size(4, 4));
            auto options = FrameDrawOptions{};
            options.setFrameColorSequence(colors, FrameColorMode::ChasingBorderCCW);

            buffer.drawFrame(block::Rectangle{0, 0, 4, 4}, options, 1);

            REQUIRE_EQUAL(buffer.get(block::Position{0, 0}).color(), Color(fg::Green, bg::Black));
            REQUIRE_EQUAL(buffer.get(block::Position{1, 0}).color(), Color(fg::Blue, bg::Black));
            REQUIRE_EQUAL(buffer.get(block::Position{2, 0}).color(), Color(fg::Red, bg::Black));
            REQUIRE_EQUAL(buffer.get(block::Position{3, 1}).color(), Color(fg::Blue, bg::Black));
            REQUIRE_EQUAL(buffer.get(block::Position{0, 1}).color(), Color(fg::Red, bg::Black));
        }
    }

    void testDrawGridLayoutWithDefaultBorderDrawsNothing() {
        Buffer buffer(block::Size(5, 3));
        buffer.fill(Block{U'.'});

        buffer.drawGridLayout(block::Position{}, gridLayout({2, 3}, {1, 2}), FrameBorder{});

        requireRowsEqual(buffer, {".....", ".....", "....."});
    }

    void testDrawGridLayoutWithLightBorderRendersATable() {
        Buffer buffer(block::Size(19, 7));

        buffer.drawGridLayout(block::Position{}, gridLayout({5, 5, 5}, {1, 1, 1}), FrameBorder{FrameStyle::Light});

        requireRowsEqual(
            buffer,
            {
                "┌─────┬─────┬─────┐",
                "│     │     │     │",
                "├─────┼─────┼─────┤",
                "│     │     │     │",
                "├─────┼─────┼─────┤",
                "│     │     │     │",
                "└─────┴─────┴─────┘",
            });
    }

    void testDrawGridLayoutPreservesDashedLineSegments() {
        Buffer buffer(block::Size(7, 3));

        buffer.drawGridLayout(block::Position{}, gridLayout({2, 2}, {1}), FrameBorder{FrameStyle::LightDoubleDash});

        requireRowsEqual(buffer, {"┌╌╌┬╌╌┐", "╎  ╎  ╎", "└╌╌┴╌╌┘"});
    }

    void testDrawGridLayoutUsesStraightSegmentsAtMissingOuterCorners() {
        {
            Buffer buffer(block::Size(7, 4));
            auto border = FrameBorder{};
            border.set(FrameBorder::Element::Top, FrameStyle::Light);

            buffer.drawGridLayout(block::Position{}, gridLayout({7}, {3}), border);

            requireRowsEqual(buffer, {"───────", "       ", "       ", "       "});
        }
        {
            Buffer buffer(block::Size(3, 3));
            auto border = FrameBorder{};
            border.set(FrameBorder::Element::Left, FrameStyle::Light);

            buffer.drawGridLayout(block::Position{}, gridLayout({2}, {3}), border);

            requireRowsEqual(buffer, {"│  ", "│  ", "│  "});
        }
    }

    void testDrawGridLayoutCombinesSeparatorCrossings() {
        Buffer buffer(block::Size(5, 3));

        auto border = FrameBorder{};
        border.set(FrameBorder::Element::HLine, FrameStyle::Light);
        border.set(FrameBorder::Element::VLine, FrameStyle::Light);
        buffer.drawGridLayout(block::Position{}, gridLayout({2, 2}, {1, 1}), border);

        requireRowsEqual(buffer, {"  │  ", "──┼──", "  │  "});
    }

    void testDrawGridLayoutDrawsRoundedOuterCornersWithSquareInternalJoins() {
        Buffer buffer(block::Size(7, 5));

        buffer.drawGridLayout(
            block::Position{}, gridLayout({2, 2}, {1, 1}), FrameBorder{FrameStyle::LightWithRoundedCorners});

        requireRowsEqual(
            buffer,
            {
                "╭──┬──╮",
                "│  │  │",
                "├──┼──┤",
                "│  │  │",
                "╰──┴──╯",
            });
    }

    void testDrawGridLayoutUsesSquareCornerForMixedRoundedAndLightStyles() {
        Buffer buffer(block::Size(4, 3));
        auto border = FrameBorder{};
        border.set(FrameBorderElement::Top, FrameStyle::LightWithRoundedCorners);
        border.set(FrameBorderElement::Left, FrameStyle::Light);

        buffer.drawGridLayout(block::Position{}, gridLayout({3}, {2}), border);

        requireRowsEqual(buffer, {"┌───", "│   ", "│   "});
    }

    void testDrawGridLayoutAppliesElementColors() {
        Buffer buffer(block::Size(5, 3));
        auto border = FrameBorder{};
        border.set(FrameBorder::Element::HLine, FrameStyle::Light, Color{fg::Red, bg::Black});
        border.set(FrameBorder::Element::VLine, FrameStyle::Light, Color{fg::Blue, bg::Black});

        buffer.drawGridLayout(block::Position{}, gridLayout({2, 2}, {1, 1}), border);

        REQUIRE_EQUAL(buffer.get(block::Position{0, 1}).color(), Color(fg::Red, bg::Black));
        REQUIRE_EQUAL(buffer.get(block::Position{2, 0}).color(), Color(fg::Blue, bg::Black));
        REQUIRE_EQUAL(buffer.get(block::Position{2, 1}).color(), Color(fg::Red, bg::Black));
    }

    void testDrawBitmapHalfBlocksUseQuadMaskAndBaseColor() {
        Buffer buffer(block::Size(1, 1));
        auto bitmap = Bitmap{block::Size{2, 2}};
        bitmap.setPixel(block::Position{0, 0}, true);
        bitmap.setPixel(block::Position{1, 1}, true);
        auto options = BitmapDrawOptions{};
        options.setColor(Color{fg::BrightGreen, bg::Blue});

        buffer.drawBitmap(bitmap, block::Position{0, 0}, options);

        REQUIRE_EQUAL(buffer.get(block::Position{0, 0}), U'▚');
        REQUIRE_EQUAL(buffer.get(block::Position{0, 0}).color(), Color(fg::BrightGreen, bg::Blue));
    }

    void testDrawBitmapFullBlocksOnlyAffectSetPixels() {
        Buffer buffer(block::Size(3, 2));
        buffer.fill(Block{U' ', fg::White, bg::Black});
        auto bitmap = Bitmap{block::Size{2, 2}};
        bitmap.setPixel(block::Position{1, 0}, true);
        auto options = BitmapDrawOptions{};
        options.setScaleMode(BitmapScaleMode::FullBlock);
        options.setColor(Color{fg::BrightYellow, bg::Blue});

        buffer.drawBitmap(bitmap, block::Position{0, 0}, options);

        requireRowsEqual(buffer, {" █ ", "   "});
        REQUIRE_EQUAL(buffer.get(block::Position{1, 0}).color(), Color(fg::BrightYellow, bg::Blue));
        REQUIRE_EQUAL(buffer.get(block::Position{0, 0}).color(), Color(fg::White, bg::Black));
    }

    void testDrawBitmapDoubleBlocksUseBothConfiguredCharacters() {
        Buffer buffer(block::Size(2, 1));
        auto bitmap = Bitmap{block::Size{1, 1}};
        bitmap.setPixel(block::Position{0, 0}, true);
        auto options = BitmapDrawOptions{};
        options.setScaleMode(BitmapScaleMode::DoubleBlock);
        options.setDoubleBlocks(BlockStringEditor{"[]"_el});
        options.setColor(Color{fg::BrightCyan, bg::Black});

        buffer.drawBitmap(bitmap, block::Position{0, 0}, options);

        requireRowsEqual(buffer, {"[]"});
        REQUIRE_EQUAL(buffer.get(block::Position{0, 0}).color(), Color(fg::BrightCyan, bg::Black));
        REQUIRE_EQUAL(buffer.get(block::Position{1, 0}).color(), Color(fg::BrightCyan, bg::Black));
    }

    void testDrawBitmapRectCroppingUsesAlignment() {
        Buffer buffer(block::Size(2, 1));
        auto bitmap = Bitmap{block::Size{4, 1}};
        bitmap.setPixel(block::Position{0, 0}, true);
        bitmap.setPixel(block::Position{3, 0}, true);
        auto options = BitmapDrawOptions{};
        options.setScaleMode(BitmapScaleMode::FullBlock);

        buffer.drawBitmap(bitmap, block::Rectangle{0, 0, 2, 1}, geometry::Alignment::Right, options);

        requireRowsEqual(buffer, {" █"});
    }

    void testDrawBitmapBlock16StyleUsesNeighborConnections() {
        Buffer buffer(block::Size(3, 1));
        auto bitmap = Bitmap{block::Size{3, 1}};
        bitmap.setPixel(block::Position{0, 0}, true);
        bitmap.setPixel(block::Position{1, 0}, true);
        bitmap.setPixel(block::Position{2, 0}, true);
        auto options = BitmapDrawOptions{};
        options.setBlock16Style(Block16Style::lightFrame());

        buffer.drawBitmap(bitmap, block::Position{0, 0}, options);

        requireRowsEqual(buffer, {"╶─╴"});
    }

    void testDrawBitmapVerticalStripesUseBitmapPixelsForDoubleBlocks() {
        Buffer buffer(block::Size(4, 1));
        auto bitmap = Bitmap{block::Size{2, 1}};
        bitmap.setPixel(block::Position{0, 0}, true);
        bitmap.setPixel(block::Position{1, 0}, true);
        auto options = BitmapDrawOptions{};
        options.setScaleMode(BitmapScaleMode::DoubleBlock);
        options.setColorSequence(
            ColorSequence{
                Color{fg::Red, bg::Black},
                Color{fg::Blue, bg::Black},
            },
            BitmapColorMode::VerticalStripes);

        buffer.drawBitmap(bitmap, block::Position{0, 0}, options);

        REQUIRE_EQUAL(buffer.get(block::Position{0, 0}).color(), Color(fg::Red, bg::Black));
        REQUIRE_EQUAL(buffer.get(block::Position{1, 0}).color(), Color(fg::Red, bg::Black));
        REQUIRE_EQUAL(buffer.get(block::Position{2, 0}).color(), Color(fg::Blue, bg::Black));
        REQUIRE_EQUAL(buffer.get(block::Position{3, 0}).color(), Color(fg::Blue, bg::Black));
    }

    void testDrawBitmapCanInheritExistingBufferColor() {
        Buffer buffer(block::Size(1, 1));
        buffer.fill(Block{U' ', fg::BrightWhite, bg::Magenta});
        auto bitmap = Bitmap{block::Size{1, 1}};
        bitmap.setPixel(block::Position{0, 0}, true);
        auto options = BitmapDrawOptions{};
        options.setScaleMode(BitmapScaleMode::FullBlock);
        options.setColorSequence(ColorSequence{});

        buffer.drawBitmap(bitmap, block::Position{0, 0}, options);

        REQUIRE_EQUAL(buffer.get(block::Position{0, 0}), U'█');
        REQUIRE_EQUAL(buffer.get(block::Position{0, 0}).color(), Color(fg::BrightWhite, bg::Magenta));
    }

    void testDrawBitmapBackwardDiagonalStripesWrapNegativeIndexes() {
        Buffer buffer(block::Size(2, 1));
        auto bitmap = Bitmap{block::Size{2, 1}};
        bitmap.setPixel(block::Position{0, 0}, true);
        bitmap.setPixel(block::Position{1, 0}, true);
        auto options = BitmapDrawOptions{};
        options.setScaleMode(BitmapScaleMode::FullBlock);
        options.setColorSequence(
            ColorSequence{
                Color{fg::Red, bg::Black},
                Color{fg::Blue, bg::Black},
            },
            BitmapColorMode::BackwardDiagonalStripes);

        buffer.drawBitmap(bitmap, block::Position{0, 0}, options);

        REQUIRE_EQUAL(buffer.get(block::Position{0, 0}).color(), Color(fg::Red, bg::Black));
        REQUIRE_EQUAL(buffer.get(block::Position{1, 0}).color(), Color(fg::Blue, bg::Black));
    }

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
    void testDrawTextOverloadUsesTextTypes() {
        auto buffer = Buffer{block::Size{5, 3}};

        buffer.drawBlockText(
            "Hi"_el, geometry::Alignment::Center, block::Rectangle{0, 0, 5, 3}, Color{fg::Yellow, bg::Blue});

        requireRowsEqual(buffer, {"     ", " Hi  ", "     "});
        REQUIRE_EQUAL(buffer.get(block::Position{1, 1}).color(), Color(fg::Yellow, bg::Blue));
        REQUIRE_EQUAL(buffer.get(block::Position{2, 1}).color(), Color(fg::Yellow, bg::Blue));
    }
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
};
