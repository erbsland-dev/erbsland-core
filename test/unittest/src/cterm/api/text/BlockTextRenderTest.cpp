// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/BufferTestHelper.hpp"

#include <erbsland/unittest/UnitTest.hpp>

TESTED_TARGETS(Buffer)
class BlockTextRenderTest final : public UNITTEST_SUBCLASS(BufferTestHelper) {
public:
    void testBufferRendersAlignedText() {
        auto buffer = Buffer{bgeo::BlockSize{8, 3}};
        buffer.drawBlockText(
            "Hi"_el, bgeo::BlockRectangle{0, 0, 8, 3}, bgeo::Alignment::Center, Color{fg::Yellow, bg::Black});
        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{3, 1}), U'H');
        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{4, 1}), U'i');
        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{3, 1}).color(), Color(fg::Yellow, bg::Black));
    }

    void testBufferRendersNewParagraphsWithSingleParagraphSpacing() {
        auto text = BlockText{BlockStringEditor{"A\nB"_el}, bgeo::BlockRectangle{0, 0, 1, 2}, bgeo::Alignment::TopLeft};
        auto buffer = Buffer{bgeo::BlockSize{1, 2}};
        buffer.drawBlockText(text);
        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{0, 0}), U'A');
        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{0, 1}), U'B');
    }

    void testBufferDoesNotInsertSpacingBetweenWrappedLines() {
        auto text =
            BlockText{BlockStringEditor{"AA BB"_el}, bgeo::BlockRectangle{0, 0, 2, 2}, bgeo::Alignment::TopLeft};
        text.setParagraphSpacing(ParagraphSpacing::DoubleLine);
        auto buffer = Buffer{bgeo::BlockSize{2, 2}};
        buffer.drawBlockText(text);
        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{0, 0}), U'A');
        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{1, 0}), U'A');
        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{0, 1}), U'B');
        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{1, 1}), U'B');
    }

    void testBufferRendersDoubleParagraphSpacingWithVerticalAlignment() {
        auto text = BlockText{BlockStringEditor{"A\nB"_el}, bgeo::BlockRectangle{0, 0, 3, 5}, bgeo::Alignment::Center};
        text.setParagraphSpacing(ParagraphSpacing::DoubleLine);
        auto buffer = Buffer{bgeo::BlockSize{3, 5}};
        buffer.drawBlockText(text);
        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{1, 1}), U'A');
        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{1, 3}), U'B');
        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{1, 2}), U' ');
    }

    void testBufferKeepsCharacterColorsIfNoColorSequenceIsDefined() {
        auto text = BlockStringEditor{};
        text.append(Block{U'A', fg::Red, bg::Black});
        text.append(Block{U'B', fg::Green, bg::Black});
        auto buffer = Buffer{bgeo::BlockSize{4, 1}};
        buffer.drawBlockText(BlockText{text, bgeo::BlockRectangle{0, 0, 4, 1}, bgeo::Alignment::TopLeft});
        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{0, 0}).color(), Color(fg::Red, bg::Black));
        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{1, 0}).color(), Color(fg::Green, bg::Black));
    }

    void testBufferUsesExplicitAnimationCycleForFlashAnimation() {
        auto buffer = Buffer{bgeo::BlockSize{3, 1}};
        auto text = BlockText{BlockStringEditor{"ABC"_el}, bgeo::BlockRectangle{0, 0, 3, 1}, bgeo::Alignment::TopLeft};
        text.setColorSequence(
            ColorSequence{Color{fg::Red, bg::Black}, Color{fg::Green, bg::Black}, Color{fg::Blue, bg::Black}});
        text.setAnimation(BlockTextAnimation::ColorDiagonal);
        buffer.drawBlockText(text, 1);
        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{0, 0}).color(), Color(fg::Green, bg::Black));
        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{1, 0}).color(), Color(fg::Blue, bg::Black));
        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{2, 0}).color(), Color(fg::Red, bg::Black));
    }

    void testBufferKeepsCharacterColorPartsOverTextColor() {
        auto text = BlockStringEditor{};
        text.append(Block{U'A', fg::Red, bg::Inherited});
        text.append(Block{U'B', fg::Inherited, bg::Blue});
        auto renderedText = BlockText{text, bgeo::BlockRectangle{0, 0, 2, 1}, bgeo::Alignment::TopLeft};
        renderedText.setColor(Color{fg::Green, bg::Yellow});
        auto buffer = Buffer{bgeo::BlockSize{2, 1}};
        buffer.drawBlockText(renderedText);
        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{0, 0}).color(), Color(fg::Red, bg::Yellow));
        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{1, 0}).color(), Color(fg::Green, bg::Blue));
    }

    void testBufferUsesDefaultAsExplicitTextReset() {
        auto text = BlockStringEditor{};
        text.append(Block{U'A', fg::Default, bg::Inherited});
        auto renderedText = BlockText{text, bgeo::BlockRectangle{0, 0, 1, 1}, bgeo::Alignment::TopLeft};
        renderedText.setColor(Color{fg::Green, bg::Yellow});
        auto buffer = Buffer{bgeo::BlockSize{1, 1}};
        buffer.drawBlockText(renderedText);

        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{0, 0}).color(), Color(fg::Default, bg::Yellow));
    }

    void testBufferRespectsParagraphMarginsWhenRenderingText() {
        auto text = BlockText{BlockStringEditor{"A"_el}, bgeo::BlockRectangle{0, 0, 5, 3}, bgeo::Alignment::TopLeft};
        text.setMargins(bgeo::BlockMargins{1});
        auto buffer = Buffer{bgeo::BlockSize{5, 3}};

        buffer.drawBlockText(text);

        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{1, 1}), U'A');
        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{0, 0}), U' ');
        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{4, 2}), U' ');
    }

    void testBufferRendersBitmapFontText() {
        auto font = std::make_shared<Font>(2);
        font->addGlyph("A"_el, FontGlyph{std::vector<uint64_t>{0b11U, 0b11U}});
        auto text = BlockText{BlockStringEditor{"A"_el}, bgeo::BlockRectangle{0, 0, 2, 1}, bgeo::Alignment::TopLeft};
        text.setColor(Color{fg::BrightWhite, bg::Black});
        text.setFont(font);
        auto buffer = Buffer{bgeo::BlockSize{2, 1}};
        buffer.drawBlockText(text);
        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{0, 0}), U'█');
        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{0, 0}).color(), Color(fg::BrightWhite, bg::Black));
    }

    void testBufferRendersParagraphIndentAndWrapMarks() {
        auto text =
            BlockText{BlockStringEditor{"AA BB CC"_el}, bgeo::BlockRectangle{0, 0, 6, 2}, bgeo::Alignment::TopLeft};
        text.setWrappedLineIndent(3);
        text.setLineBreakStartMark(BlockStringEditor{">"_el});
        text.setLineBreakEndMark(BlockStringEditor{"<"_el});
        auto buffer = Buffer{bgeo::BlockSize{6, 2}};

        buffer.drawBlockText(text);

        requireRowsEqual(buffer, {"AA BB<", "   >CC"});
    }

    void testBufferUsesTabStopsForParagraphRendering() {
        auto text = BlockText{BlockStringEditor{"A\tB"_el}, bgeo::BlockRectangle{0, 0, 5, 1}, bgeo::Alignment::TopLeft};
        text.setTabStops({4});
        auto buffer = Buffer{bgeo::BlockSize{5, 1}};

        buffer.drawBlockText(text);

        requireRowsEqual(buffer, {"A   B"});
    }

    void testBufferBreaksAtNonAdvancingTabStopsWhenRequested() {
        auto text = BlockText{
            BlockStringEditor{"Heading\ttext"_el}, bgeo::BlockRectangle{0, 0, 12, 2}, bgeo::Alignment::TopLeft};
        text.setWrappedLineIndent(6);
        text.setLineBreakEndMark(BlockStringEditor{"<"_el});
        text.setTabStops({6});
        text.setTabOverflowBehavior(TabOverflowBehavior::LineBreak);
        auto buffer = Buffer{bgeo::BlockSize{12, 2}};

        buffer.drawBlockText(text);

        requireRowsEqual(buffer, {"Heading    <", "      text  "});
    }

    void testBufferReplacesNonAdvancingTabsWithSpacesByDefault() {
        auto text = BlockText{
            BlockStringEditor{"Heading\ttext"_el}, bgeo::BlockRectangle{0, 0, 12, 1}, bgeo::Alignment::TopLeft};
        text.setTabStops({6});
        auto buffer = Buffer{bgeo::BlockSize{12, 1}};

        buffer.drawBlockText(text);

        requireRowsEqual(buffer, {"Heading text"});
    }

    void testBufferBreaksWhenTabStopsAreExhaustedAndLineBreakIsRequested() {
        auto text =
            BlockText{BlockStringEditor{"A\tB\tC"_el}, bgeo::BlockRectangle{0, 0, 5, 2}, bgeo::Alignment::TopLeft};
        text.setWrappedLineIndent(2);
        text.setLineBreakEndMark(BlockStringEditor{"<"_el});
        text.setTabStops({2});
        text.setTabOverflowBehavior(TabOverflowBehavior::LineBreak);
        auto buffer = Buffer{bgeo::BlockSize{5, 2}};

        buffer.drawBlockText(text);

        requireRowsEqual(buffer, {"A B <", "  C  "});
    }

    void testBufferUsesWordSeparatorsAsCollapsedSpacing() {
        auto text = BlockText{BlockStringEditor{"A..B"_el}, bgeo::BlockRectangle{0, 0, 3, 1}, bgeo::Alignment::TopLeft};
        text.setWordSeparators(U"."_el);
        auto buffer = Buffer{bgeo::BlockSize{3, 1}};

        buffer.drawBlockText(text);

        requireRowsEqual(buffer, {"A B"});
    }

    void testBufferTreatsTabsAsCollapsedWordSeparatorsForCenteredParagraphs() {
        auto text = BlockText{BlockStringEditor{"A\tB"_el}, bgeo::BlockRectangle{0, 0, 5, 1}, bgeo::Alignment::Center};
        auto buffer = Buffer{bgeo::BlockSize{5, 1}};

        buffer.drawBlockText(text);

        requireRowsEqual(buffer, {" A B "});
    }

    void testBufferSplitsLongWordsUsingTheConfiguredWordBreakMark() {
        auto text =
            BlockText{BlockStringEditor{"ABCDEFG"_el}, bgeo::BlockRectangle{0, 0, 5, 2}, bgeo::Alignment::TopLeft};
        auto buffer = Buffer{bgeo::BlockSize{5, 2}};

        buffer.drawBlockText(text);

        requireRowsEqual(buffer, {"ABCD-", "EFG  "});
    }

    void testBufferUsesParagraphEllipsisAfterMaximumWraps() {
        auto text = BlockText{
            BlockStringEditor{"AA BB CC DD EE"_el}, bgeo::BlockRectangle{0, 0, 5, 2}, bgeo::Alignment::TopLeft};
        text.setMaximumLineWraps(1);
        auto buffer = Buffer{bgeo::BlockSize{5, 2}};

        buffer.drawBlockText(text);

        requireRowsEqual(buffer, {"AA BB", "CC…  "});
    }

    void testBufferUsesParagraphBackgroundModesForWrappedParagraphs() {
        auto text =
            BlockText{BlockStringEditor{"AAAA BBBB"_el}, bgeo::BlockRectangle{0, 0, 6, 2}, bgeo::Alignment::TopLeft};
        text.setColor(Color{fg::White, bg::Red});
        text.setWrappedLineIndent(2);
        text.setBackgroundMode(ParagraphBackgroundMode::WrappedBoth);
        auto buffer = Buffer{bgeo::BlockSize{6, 2}, Block{U' ', fg::White, bg::Blue}};

        buffer.drawBlockText(text);

        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{4, 0}).color(), Color(fg::White, bg::Red));
        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{5, 0}).color(), Color(fg::White, bg::Red));
        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{0, 1}).color(), Color(fg::White, bg::Red));
        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{1, 1}).color(), Color(fg::White, bg::Red));
    }

    void testBufferFallsBackToTheLegacySimpleDrawForInvalidParagraphSettings() {
        auto text =
            BlockText{BlockStringEditor{"AA BB"_el}, bgeo::BlockRectangle{0, 0, 2, 2}, bgeo::Alignment::TopLeft};
        text.setLineBreakEndMark(BlockStringEditor{">>"_el});
        text.setOnError(ParagraphOnError::PlainOutput);
        auto buffer = Buffer{bgeo::BlockSize{2, 2}};

        buffer.drawBlockText(text);

        requireRowsEqual(buffer, {"AA", "BB"});
    }

    void testBufferSkipsInvalidParagraphsWhenOnErrorIsEmpty() {
        auto text =
            BlockText{BlockStringEditor{"AA BB"_el}, bgeo::BlockRectangle{0, 0, 2, 2}, bgeo::Alignment::TopLeft};
        text.setLineBreakEndMark(BlockStringEditor{">>"_el});
        text.setOnError(ParagraphOnError::Empty);
        auto buffer = Buffer{bgeo::BlockSize{2, 2}, Block{U'X', fg::White, bg::Blue}};

        buffer.drawBlockText(text);

        requireRowsEqual(buffer, {"XX", "XX"});
    }
};
