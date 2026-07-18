// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/BufferTestHelper.hpp"

#include <erbsland/cterm/CursorBuffer.hpp>
#include <erbsland/unittest/UnitTest.hpp>

TESTED_TARGETS(CursorBuffer)
class CursorBufferTest final : public UNITTEST_SUBCLASS(BufferTestHelper) {
public:
    void testDefaultConstructionCreatesAn80x25Buffer() {
        const auto buffer = CursorBuffer{};

        REQUIRE_EQUAL(buffer.size(), (bgeo::BlockSize{80, 25}));
        REQUIRE_EQUAL(buffer.color(), Color::reset());
        REQUIRE_EQUAL(buffer.fillChar(), Block::space());
    }

    void testConstructorUsesFillCharForTheInitialBufferContents() {
        const auto fillChar = Block{U'.', fg::BrightBlack, bg::Black};
        const auto buffer = CursorBuffer{
            bgeo::BlockSize{3, 2}, CursorBuffer::OverflowMode::Shift, CursorBuffer::cMaximumSize, fillChar};

        REQUIRE_EQUAL(buffer.fillChar(), fillChar);
        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{0, 0}), fillChar);
        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{2, 1}), fillChar);
    }

    void testColorSettersResolveInheritedColorsAndApplyToWrittenCharacters() {
        auto buffer = CursorBuffer{bgeo::BlockSize{3, 1}};
        const auto expectedAttributes = BlockAttributes::reset();

        buffer.setColor(Color{fg::Inherited, bg::Inherited});
        REQUIRE_EQUAL(buffer.color(), Color::reset());

        buffer.setForeground(fg::Red);
        buffer.setBackground(bg::Blue);
        buffer.write(Block{U'X', fg::Inherited, bg::Inherited});

        REQUIRE_EQUAL(buffer.color(), Color(fg::Red, bg::Blue));
        REQUIRE_EQUAL(
            buffer.get(bgeo::BlockPosition{0, 0}), (Block{U'X', Color{fg::Red, bg::Blue}, expectedAttributes}));
    }

    void testBlockAttributesResolveAgainstTheWriterStateAndCanBeOverwrittenExplicitly() {
        auto buffer = CursorBuffer{bgeo::BlockSize{3, 1}};
        auto bold = BlockAttributes{};
        bold.setBold(true);
        buffer.setBlockAttributes(bold);

        buffer.write(Block{U'A'});

        auto explicitNoBold = BlockAttributes{};
        explicitNoBold.setBold(false);
        explicitNoBold.setUnderline(true);
        buffer.write(Block{U'B', Color{}, explicitNoBold});

        REQUIRE(buffer.blockAttributes().isBold());
        REQUIRE(buffer.supportedBlockAttributes() == BlockAttributes::all());
        REQUIRE(buffer.get(bgeo::BlockPosition{0, 0}).attributes().isBold());
        REQUIRE_FALSE(buffer.get(bgeo::BlockPosition{1, 0}).attributes().isBold());
        REQUIRE(buffer.get(bgeo::BlockPosition{1, 0}).attributes().isUnderline());
    }

    void testWritingAStringResolvesInheritedStyleAgainstTheTrackedWriterStateForEachCharacter() {
        auto buffer = CursorBuffer{bgeo::BlockSize{2, 1}};
        auto underline = BlockAttributes{};
        underline.setUnderline(true);
        buffer.setStyle(BlockStyle{Color{fg::Red, bg::Blue}, underline});

        auto text = BlockStringEditor{};
        text += Block{U'A', Color{fg::Green, bg::Inherited}, BlockAttributes{}.withFlag(BlockAttributes::Bold, true)};
        text += Block{U'B'};
        buffer.write(text);

        REQUIRE_EQUAL(buffer.color(), Color(fg::Red, bg::Blue));
        REQUIRE(buffer.blockAttributes().isUnderline());
        REQUIRE_FALSE(buffer.blockAttributes().isBold());
        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{0, 0}).color(), Color(fg::Green, bg::Blue));
        REQUIRE(buffer.get(bgeo::BlockPosition{0, 0}).attributes().isBold());
        REQUIRE(buffer.get(bgeo::BlockPosition{0, 0}).attributes().isUnderline());
        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{1, 0}).color(), Color(fg::Red, bg::Blue));
        REQUIRE_FALSE(buffer.get(bgeo::BlockPosition{1, 0}).attributes().isBold());
        REQUIRE(buffer.get(bgeo::BlockPosition{1, 0}).attributes().isUnderline());
    }

    void testWritingAStringUsesTheSameInheritedStyleResolution() {
        auto buffer = CursorBuffer{bgeo::BlockSize{2, 1}};
        buffer.setColor(Color{fg::Green, bg::Blue});
        const auto source = BlockStringEditor{"AB"_el};

        buffer.write(BlockString{source}.slice(BlockRange{BlockIndex{0U}, BlockCount{2U}}));

        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{0, 0}).color(), Color(fg::Green, bg::Blue));
        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{1, 0}).color(), Color(fg::Green, bg::Blue));
    }

    void testPrintAcceptsCharacterAttributesAndAppliesThemToFollowingCharacters() {
        auto buffer = CursorBuffer{bgeo::BlockSize{3, 1}};
        auto bold = BlockAttributes{};
        bold.setBold(true);
        auto noBold = BlockAttributes{};
        noBold.setBold(false);

        buffer.print(bold, "A"_el, noBold, "B"_el);

        REQUIRE(buffer.get(bgeo::BlockPosition{0, 0}).attributes().isBold());
        REQUIRE_FALSE(buffer.get(bgeo::BlockPosition{1, 0}).attributes().isBold());
    }

    void testSetStyleAndPrintBlockStyleUpdateTheTrackedWriterState() {
        auto buffer = CursorBuffer{bgeo::BlockSize{3, 1}};
        auto emphasis = BlockAttributes{};
        emphasis.setBold(true);

        buffer.setStyle(BlockStyle{Color{fg::Red, bg::Blue}, emphasis});
        buffer.write(Block{U'A'});
        buffer.print(
            BlockStyle{Color{fg::Inherited, bg::Green}, BlockAttributes{}.withFlag(BlockAttributes::Bold, false)},
            "B"_el);

        REQUIRE_EQUAL(buffer.color(), Color(fg::Red, bg::Green));
        REQUIRE_FALSE(buffer.blockAttributes().isBold());
        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{0, 0}).color(), Color(fg::Red, bg::Blue));
        REQUIRE(buffer.get(bgeo::BlockPosition{0, 0}).attributes().isBold());
        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{1, 0}).color(), Color(fg::Red, bg::Green));
        REQUIRE_FALSE(buffer.get(bgeo::BlockPosition{1, 0}).attributes().isBold());
    }

    void testWideCharactersPreserveAttributesInTheContinuationCell() {
        auto buffer = CursorBuffer{bgeo::BlockSize{3, 1}};
        auto attributes = BlockAttributes{};
        attributes.setItalic(true);

        buffer.write(Block{U'界', Color{fg::Red, bg::Black}, attributes});

        REQUIRE(buffer.get(bgeo::BlockPosition{0, 0}).attributes().isItalic());
        REQUIRE(buffer.get(bgeo::BlockPosition{1, 0}).isEmpty());
        REQUIRE(buffer.get(bgeo::BlockPosition{1, 0}).attributes().isItalic());
    }

    void testFillCharSetterControlsClearScreenAndResetsTheCursor() {
        auto buffer = CursorBuffer{bgeo::BlockSize{3, 2}};
        const auto fillChar = Block{U'.', fg::BrightBlack, bg::Black};

        buffer.setFillChar(fillChar);
        buffer.write(BlockStringEditor{"ABC"_el});
        buffer.writeLineBreak();
        buffer.write(Block{U'D'});

        buffer.clearScreen();
        buffer.write(Block{U'X'});

        REQUIRE_EQUAL(buffer.fillChar(), fillChar);
        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{1, 0}), fillChar);
        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{0, 1}), fillChar);
        requireRowsEqual(buffer, {"X..", "..."});
    }

    void testFillCharSetterRejectsCharactersThatDoNotOccupyExactlyOneCell() {
        auto buffer = CursorBuffer{bgeo::BlockSize{3, 2}};

        REQUIRE_THROWS_AS(erbsland::err::ParameterError, buffer.setFillChar(Block{U'\n'}));
        REQUIRE_THROWS_AS(erbsland::err::ParameterError, buffer.setFillChar(Block{U'界'}));
    }

    void testMoveCursorClampsToTheBufferArea() {
        auto buffer = CursorBuffer{bgeo::BlockSize{3, 2}};

        buffer.moveTo(bgeo::BlockPosition{99, 99});
        buffer.write(Block{U'X'});

        REQUIRE(buffer.get(bgeo::BlockPosition{2, 1}) == U'X');
    }

    void testMoveCursorIgnoresExtremeAbsoluteMoves() {
        auto buffer = CursorBuffer{bgeo::BlockSize{3, 2}};

        buffer.moveCursor(bgeo::BlockPosition{10'001, 0}, MoveMode::Absolute);
        buffer.write(Block{U'X'});

        REQUIRE(buffer.get(bgeo::BlockPosition{0, 0}) == U'X');
    }

    void testDeferredWrapMovesTheNextCharacterToTheFollowingLine() {
        auto buffer = CursorBuffer{bgeo::BlockSize{2, 2}};

        buffer.write(BlockStringEditor{"AB"_el});
        buffer.write(Block{U'C'});

        requireRowsEqual(buffer, {"AB", "C "});
    }

    void testExplicitLineBreakClearsDeferredWrap() {
        auto buffer = CursorBuffer{bgeo::BlockSize{3, 3}};

        buffer.write(BlockStringEditor{"ABC"_el});
        buffer.writeLineBreak();
        buffer.write(Block{U'D'});

        requireRowsEqual(buffer, {"ABC", "D  ", "   "});
    }

    void testMovingTheCursorClearsDeferredWrap() {
        auto buffer = CursorBuffer{bgeo::BlockSize{3, 2}};

        buffer.write(BlockStringEditor{"ABC"_el});
        buffer.moveTo(bgeo::BlockPosition{1, 1});
        buffer.write(Block{U'D'});

        requireRowsEqual(buffer, {"ABC", " D "});
    }

    void testAutoWrapCanBeDisabledAtTheRightMargin() {
        auto buffer = CursorBuffer{bgeo::BlockSize{2, 2}};

        buffer.moveTo(bgeo::BlockPosition{1, 0});
        buffer.setAutoWrap(false);
        buffer.write(Block{U'A'});
        buffer.write(Block{U'B'});

        requireRowsEqual(buffer, {" B", "  "});
    }

    void testWideCharactersWrapBeforeWritingAtTheLastColumn() {
        auto buffer = CursorBuffer{bgeo::BlockSize{3, 2}};

        buffer.moveTo(bgeo::BlockPosition{2, 0});
        buffer.write(Block{U'界'});

        REQUIRE(buffer.get(bgeo::BlockPosition{0, 1}) == U'界');
        REQUIRE(buffer.get(bgeo::BlockPosition{1, 1}).isEmpty());
        requireRowsEqual(buffer, {"   ", "界  "});
    }

    void testWideCharactersEndingAtTheRightMarginDeferTheNextWrap() {
        auto buffer = CursorBuffer{bgeo::BlockSize{3, 2}};

        buffer.moveTo(bgeo::BlockPosition{1, 0});
        buffer.write(Block{U'界'});
        buffer.write(Block{U'X'});

        REQUIRE(buffer.get(bgeo::BlockPosition{1, 0}) == U'界');
        REQUIRE(buffer.get(bgeo::BlockPosition{2, 0}).isEmpty());
        REQUIRE(buffer.get(bgeo::BlockPosition{0, 1}) == U'X');
    }

    void testWritingAnotherBufferMovesToTheNextLineForEachSourceRow() {
        auto buffer = CursorBuffer{bgeo::BlockSize{3, 3}};
        auto source = Buffer{bgeo::BlockSize{2, 2}};
        source.set(bgeo::BlockPosition{0, 0}, Block{U'A'});
        source.set(bgeo::BlockPosition{1, 0}, Block{U'B'});
        source.set(bgeo::BlockPosition{0, 1}, Block{U'C'});
        source.set(bgeo::BlockPosition{1, 1}, Block{U'D'});

        buffer.write(source);

        requireRowsEqual(buffer, {"AB ", "CD ", "   "});
    }

    void testShiftOverflowScrollsTheBufferUpUsingTheConfiguredFillChar() {
        auto buffer = CursorBuffer{
            bgeo::BlockSize{3, 2}, CursorBuffer::OverflowMode::Shift, CursorBuffer::cMaximumSize, Block{U'.'}};

        buffer.write(Block{U'A'});
        buffer.writeLineBreak();
        buffer.write(Block{U'B'});
        buffer.writeLineBreak();
        buffer.write(Block{U'C'});

        requireRowsEqual(buffer, {"B..", "C.."});
    }

    void testWrapOverflowMovesBackToTheFirstLine() {
        auto buffer = CursorBuffer{bgeo::BlockSize{3, 2}, CursorBuffer::OverflowMode::Wrap};

        buffer.write(Block{U'A'});
        buffer.writeLineBreak();
        buffer.write(Block{U'B'});
        buffer.writeLineBreak();
        buffer.write(Block{U'C'});

        requireRowsEqual(buffer, {"C  ", "B  "});
    }

    void testExpandThenShiftGrowsVerticallyUsingTheConfiguredFillChar() {
        auto buffer = CursorBuffer{
            bgeo::BlockSize{3, 2}, CursorBuffer::OverflowMode::ExpandThenShift, bgeo::BlockSize{3, 3}, Block{U'.'}};

        buffer.write(Block{U'A'});
        buffer.writeLineBreak();
        buffer.write(Block{U'B'});
        buffer.writeLineBreak();
        buffer.write(Block{U'C'});

        REQUIRE_EQUAL(buffer.size(), (bgeo::BlockSize{3, 3}));
        requireRowsEqual(buffer, {"A..", "B..", "C.."});

        buffer.writeLineBreak();
        buffer.write(Block{U'D'});

        requireRowsEqual(buffer, {"B..", "C..", "D.."});
    }

    void testExpandThenWrapGrowsVerticallyUsingTheConfiguredFillChar() {
        auto buffer = CursorBuffer{
            bgeo::BlockSize{3, 2}, CursorBuffer::OverflowMode::ExpandThenWrap, bgeo::BlockSize{3, 3}, Block{U'.'}};

        buffer.write(Block{U'A'});
        buffer.writeLineBreak();
        buffer.write(Block{U'B'});
        buffer.writeLineBreak();
        buffer.write(Block{U'C'});

        REQUIRE_EQUAL(buffer.size(), (bgeo::BlockSize{3, 3}));
        requireRowsEqual(buffer, {"A..", "B..", "C.."});

        buffer.writeLineBreak();
        buffer.write(Block{U'D'});

        requireRowsEqual(buffer, {"D..", "B..", "C.."});
    }

    void testPrintParagraphWritesWrappedLinesAndReturnsTheRenderedLineCount() {
        auto buffer = CursorBuffer{bgeo::BlockSize{4, 4}};

        const auto lineCount = buffer.printParagraph(BlockStringEditor{"AB CD"_el});

        REQUIRE_EQUAL(lineCount, 2);
        requireRowsEqual(buffer, {"AB  ", "CD  ", "    ", "    "});
    }

    void testPrintParagraphLeavesTheRemainingCellsUntouchedWithoutRightFill() {
        auto buffer = CursorBuffer{bgeo::BlockSize{4, 4}};
        fillBufferFromRows(buffer, {"1234", "5678", "ABCD", "EFGH"});

        const auto lineCount = buffer.printParagraph(BlockStringEditor{"AB CD"_el});

        REQUIRE_EQUAL(lineCount, 2);
        requireRowsEqual(buffer, {"AB34", "CD78", "ABCD", "EFGH"});
    }

    void testPrintParagraphStillFillsTheRightSideWhenBackgroundModeRequiresIt() {
        auto buffer = CursorBuffer{bgeo::BlockSize{4, 4}};
        fillBufferFromRows(buffer, {"1234", "5678", "ABCD", "EFGH"});
        buffer.setBackground(bg::Blue);
        auto options = ParagraphOptions{};
        options.setBackgroundMode(ParagraphBackgroundMode::FullRight);

        const auto lineCount = buffer.printParagraph(BlockStringEditor{"AB CD"_el}, options);

        REQUIRE_EQUAL(lineCount, 2);
        requireRowsEqual(buffer, {"AB  ", "CD  ", "ABCD", "EFGH"});
        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{2, 0}).color().bg(), bg::Blue);
        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{2, 1}).color().bg(), bg::Blue);
    }

    void testPrintParagraphSupportsLogStyleWrapMarkersAndEllipsis() {
        auto buffer = CursorBuffer{bgeo::BlockSize{18, 5}};
        buffer.setColor(Color{fg::BrightBlue, bg::Black});
        auto options = ParagraphOptions{};
        options.setWrappedLineIndent(4);
        options.setLineBreakEndMark(BlockStringEditor{">"_el});
        options.setLineBreakStartMark(BlockStringEditor{"< "_el});
        options.setMaximumLineWraps(2);
        options.setParagraphEllipsisMark(BlockStringEditor{"(...)"_el});

        const auto lineCount =
            buffer.printParagraph(BlockStringEditor{"alpha beta gamma delta epsilon zeta eta theta iota"_el}, options);

        REQUIRE_EQUAL(lineCount, 3);
        REQUIRE(buffer.get(bgeo::BlockPosition{17, 0}) == U'>');
        REQUIRE(buffer.get(bgeo::BlockPosition{4, 1}) == U'<');
        REQUIRE(buffer.get(bgeo::BlockPosition{4, 2}) == U'<');
        auto hasEllipsis = false;
        for (auto x = bgeo::BlockCoordinate{0}; x < buffer.size().width(); ++x) {
            if (buffer.get(bgeo::BlockPosition{x, bgeo::BlockCoordinate{2}}) == U'(') {
                hasEllipsis = true;
            }
        }
        REQUIRE(hasEllipsis);
    }

    void testPrintParagraphCanIndentWrappedContinuationLinesForLogDetails() {
        auto buffer = CursorBuffer{bgeo::BlockSize{18, 4}};
        buffer.setColor(Color{fg::White, bg::Black});
        auto options = ParagraphOptions{};
        options.setFirstLineIndent(4);
        options.setWrappedLineIndent(4);

        const auto lineCount = buffer.printParagraph(BlockStringEditor{"detail line wraps here"_el}, options);

        REQUIRE_EQUAL(lineCount, 2);
        REQUIRE(buffer.get(bgeo::BlockPosition{4, 0}) == U'd');
        REQUIRE(buffer.get(bgeo::BlockPosition{4, 1}) == U'w');
        for (auto x = bgeo::BlockCoordinate{0}; x < 4; ++x) {
            REQUIRE(buffer.get(bgeo::BlockPosition{x, bgeo::BlockCoordinate{0}}) == U' ');
            REQUIRE(buffer.get(bgeo::BlockPosition{x, bgeo::BlockCoordinate{1}}) == U' ');
        }
    }

    void testPrintParagraphHonorsHorizontalMargins() {
        auto buffer = CursorBuffer{bgeo::BlockSize{6, 4}};
        auto options = ParagraphOptions{};
        options.setMargins(bgeo::BlockMargins{1, 0});

        const auto lineCount = buffer.printParagraph(BlockStringEditor{"AB CD"_el}, options);

        REQUIRE_EQUAL(lineCount, 2);
        requireRowsEqual(buffer, {" AB   ", " CD   ", "      ", "      "});
    }

    void testPrintParagraphPreservesExistingRowsWhileExpandingTheBufferHeight() {
        auto buffer = CursorBuffer{
            bgeo::BlockSize{4, 1}, CursorBuffer::OverflowMode::ExpandThenShift, bgeo::BlockSize{4, 4}, Block{U'.'}};

        const auto lineCount = buffer.printParagraph(BlockStringEditor{"AB CD EF"_el});

        REQUIRE_EQUAL(lineCount, 3);
        REQUIRE_EQUAL(buffer.size(), (bgeo::BlockSize{4, 4}));
        requireRowsEqual(buffer, {"AB..", "CD..", "EF..", "...."});
    }

    void testExpandThenShiftPreservesVisibleRowsEvenWhenTheRowMapWasChangedBeforehand() {
        auto buffer = CursorBuffer{
            bgeo::BlockSize{3, 2}, CursorBuffer::OverflowMode::ExpandThenShift, bgeo::BlockSize{3, 4}, Block{U'.'}};

        buffer.write(Block{U'A'});
        buffer.writeLineBreak();
        buffer.write(Block{U'B'});
        buffer.eraseRows(blockCoordinate(0), Block{U'.'}, 1);
        buffer.moveTo(bgeo::BlockPosition{0, 1});

        buffer.writeLineBreak();
        buffer.write(Block{U'C'});

        REQUIRE_EQUAL(buffer.size(), (bgeo::BlockSize{3, 3}));
        requireRowsEqual(buffer, {"B..", "...", "C.."});
    }
};
