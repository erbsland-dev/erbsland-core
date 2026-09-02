// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/TerminalTestHelper.hpp"
#include "../../support/TestHelper.hpp"

#include <erbsland/unittest/UnitTest.hpp>

#include <string_view>

TESTED_TARGETS(Terminal)
class TerminalConvenienceTest final : public UNITTEST_SUBCLASS(TerminalTestHelper) {
public:
    void testCustomBackendConstructorClampsTheSizeAndTracksTheSafeMarginFlag() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        auto terminal = Terminal{backend, block::Size{0, 5000}};
        const auto expectedSize = block::Size{1, 2048};

        REQUIRE_EQUAL(terminal.size(), expectedSize);
        REQUIRE(terminal.safeMarginEnabled());

        terminal.setSafeMarginEnabled(false);

        REQUIRE_FALSE(terminal.safeMarginEnabled());
        REQUIRE_EQUAL(terminal.size(), expectedSize);
    }

    void testColorAccessorsAndStringWritesUseTheTrackedTerminalColor() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        auto terminal = createTerminal(backend);
        terminal->setLineBufferEnabled(false);

        terminal->setForeground(Foreground::Inherited);
        terminal->setBackground(Background::Inherited);
        terminal->setForeground(fg::Red);
        terminal->setBackground(bg::Blue);
        terminal->write(BlockStringEditor{"A"_el});
        terminal->write(Block{U'B', fg::Inherited, bg::Green});
        terminal->flush();

        REQUIRE_EQUAL(terminal->color(), Color(fg::Red, bg::Green));
        REQUIRE_EQUAL(backend->output(), std::string{"\x1b[31m\x1b[44mA\x1b[42mB"});
    }

    void testAttributeAccessorsAndStringWritesUseTheTrackedTerminalAttributes() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        auto terminal = createTerminal(backend);
        terminal->setLineBufferEnabled(false);

        terminal->setBold(true);
        terminal->setUnderline(true);
        terminal->write(BlockStringEditor{"A"_el});

        auto attributes = BlockAttributes{};
        attributes.setBold(false);
        terminal->write(Block{U'B', Color{}, attributes});
        terminal->flush();

        REQUIRE(terminal->blockAttributes().isUnderline());
        REQUIRE_FALSE(terminal->blockAttributes().isBold());
        REQUIRE_EQUAL(backend->output(), std::string{"\x1b[1m\x1b[4mA\x1b[22mB"});
    }

    void testPrintAndPrintLineAcceptAllSupportedArgumentKindsInTextMode() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        auto terminal = createTerminal(backend);
        terminal->setOutputMode(Terminal::OutputMode::BlockText);

        auto bold = BlockAttributes{};
        bold.setBold(true);
        const auto style = BlockStyle{Color{fg::Green, bg::Blue}, bold};
        terminal->print(style, Block{U'A'}, BlockStringEditor{"B"_el}, "C"_el, "D"_el, "E"_el);
        terminal->printLine("F"_el);
        terminal->flush();

        REQUIRE_EQUAL(backend->output(), std::string{"ABCDEF\n"});
    }

    void testPrintContextWritesMixedStylesInFullControlMode() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        auto terminal = createTerminal(backend);
        terminal->setLineBufferEnabled(false);

        auto underline = BlockAttributes{};
        underline.setUnderline(true);
        auto noUnderline = BlockAttributes{};
        noUnderline.setUnderline(false);

        terminal->print(
            fg::Red,
            bg::Blue,
            "A"_el,
            BlockStyle{Color{fg::Inherited, bg::Green}, underline},
            Block{U'B', fg::Inherited, bg::Inherited},
            noUnderline,
            "C"_el);
        terminal->flush();

        REQUIRE_EQUAL(terminal->style().color(), Color(fg::Red, bg::Green));
        REQUIRE_FALSE(terminal->style().attributes().isUnderline());
        REQUIRE_EQUAL(backend->output(), std::string{"\x1b[31m\x1b[44mA\x1b[42m\x1b[4mB\x1b[24mC"});
    }

    void testStyleWrappersExposeAndApplyTheTrackedWriterStyle() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        auto terminal = createTerminal(backend);
        terminal->setLineBufferEnabled(false);

        auto emphasis = BlockAttributes{};
        emphasis.setUnderline(true);
        terminal->setStyle(BlockStyle{Color{fg::Red, bg::Blue}, emphasis});
        terminal->print(
            BlockStyle{Color{fg::Inherited, bg::Green}, BlockAttributes{}.withFlag(BlockAttributes::Underline, false)},
            "A"_el);
        terminal->flush();

        REQUIRE_EQUAL(terminal->style().color(), Color(fg::Red, bg::Green));
        REQUIRE_FALSE(terminal->style().attributes().isUnderline());
        REQUIRE_EQUAL(backend->output(), std::string{"\x1b[31;44m\x1b[4m\x1b[42m\x1b[24mA"});
    }

    void testBoldAndDimUseStableResetAndReapplySequences() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        auto terminal = createTerminal(backend);
        terminal->setLineBufferEnabled(false);

        terminal->setBold(true);
        terminal->setDim(true);
        terminal->setBold(false);
        terminal->flush();

        REQUIRE_EQUAL(backend->output(), std::string{"\x1b[1m\x1b[22;1;2m\x1b[22;2m"});
    }

    void testPrintParagraphRendersWrapMarksAndReturnsTheWrittenLineCount() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        auto terminal = createTerminal(backend, block::Size{6, 4});
        terminal->setOutputMode(Terminal::OutputMode::BlockText);
        auto options = ParagraphOptions{};
        options.setWrappedLineIndent(3);
        options.setLineBreakStartMark(BlockStringEditor{">"_el});
        options.setLineBreakEndMark(BlockStringEditor{"<"_el});

        const auto writtenLines = terminal->printParagraph("AA BB CC"_el, options);
        terminal->flush();

        REQUIRE_EQUAL(writtenLines, 2);
        REQUIRE_EQUAL(backend->output(), std::string{"AA BB<\n   >CC\n"});
    }

    void testPrintParagraphUsesWrappedLineIndentForSpecialTabStops() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        auto terminal = createTerminal(backend, block::Size{8, 4});
        terminal->setOutputMode(Terminal::OutputMode::BlockText);
        auto options = ParagraphOptions{};
        options.setWrappedLineIndent(4);
        options.setTabStops({ParagraphOptions::cTabWrappedLineIndent});

        const auto writtenLines = terminal->printParagraph("AA\tBB CC"_el, options);
        terminal->flush();

        REQUIRE_EQUAL(writtenLines, 2);
        REQUIRE_EQUAL(backend->output(), std::string{"AA  BB\n    CC\n"});
    }

    void testPrintParagraphKeepsMixedSeparatorAndTabTokensInOrder() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        auto terminal = createTerminal(backend, block::Size{6, 4});
        terminal->setOutputMode(Terminal::OutputMode::BlockText);
        auto options = ParagraphOptions{};
        options.setWordSeparators(U" ,"_el);
        options.setTabStops({4});

        const auto writtenLines = terminal->printParagraph("AA,\tBBBB CC"_el, options);
        terminal->flush();

        REQUIRE_EQUAL(writtenLines, 3);
        REQUIRE_EQUAL(backend->output(), std::string{"AA\n    B-\nBBB CC\n"});
    }

    void testPrintParagraphBreaksAtNonAdvancingTabStopsWhenRequested() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        auto terminal = createTerminal(backend, block::Size{12, 4});
        terminal->setOutputMode(Terminal::OutputMode::BlockText);
        auto options = ParagraphOptions{};
        options.setWrappedLineIndent(6);
        options.setLineBreakEndMark(BlockStringEditor{"<"_el});
        options.setTabStops({6});
        options.setTabOverflowBehavior(TabOverflowBehavior::LineBreak);

        const auto writtenLines = terminal->printParagraph("Heading\ttext"_el, options);
        terminal->flush();

        REQUIRE_EQUAL(writtenLines, 2);
        REQUIRE_EQUAL(backend->output(), std::string{"Heading    <\n      text\n"});
    }

    void testPrintParagraphReplacesNonAdvancingTabsWithSpacesByDefault() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        auto terminal = createTerminal(backend, block::Size{12, 4});
        terminal->setOutputMode(Terminal::OutputMode::BlockText);
        auto options = ParagraphOptions{};
        options.setTabStops({6});

        const auto writtenLines = terminal->printParagraph("Heading\ttext"_el, options);
        terminal->flush();

        REQUIRE_EQUAL(writtenLines, 1);
        REQUIRE_EQUAL(backend->output(), std::string{"Heading text\n"});
    }

    void testPrintParagraphReplacesTabsWithSpacesWhenTabStopsAreExhaustedByDefault() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        auto terminal = createTerminal(backend, block::Size{5, 4});
        terminal->setOutputMode(Terminal::OutputMode::BlockText);
        auto options = ParagraphOptions{};
        options.setTabStops({2});

        const auto writtenLines = terminal->printParagraph("A\tB\tC"_el, options);
        terminal->flush();

        REQUIRE_EQUAL(writtenLines, 1);
        REQUIRE_EQUAL(backend->output(), std::string{"A B C\n"});
    }

    void testPrintParagraphTreatsTabsAsCollapsedWordSeparatorsForRightAlignedText() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        auto terminal = createTerminal(backend, block::Size{5, 4});
        terminal->setOutputMode(Terminal::OutputMode::BlockText);
        auto options = ParagraphOptions{geometry::Alignment::Right};

        const auto writtenLines = terminal->printParagraph("A\tB"_el, options);
        terminal->flush();

        REQUIRE_EQUAL(writtenLines, 1);
        REQUIRE_EQUAL(backend->output(), std::string{"  A B\n"});
    }

    void testPrintParagraphUsesTheConfiguredParagraphSpacing() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        auto terminal = createTerminal(backend, block::Size{4, 3});
        terminal->setOutputMode(Terminal::OutputMode::BlockText);
        auto options = ParagraphOptions{};
        options.setParagraphSpacing(ParagraphSpacing::DoubleLine);

        const auto writtenLines = terminal->printParagraph("A"_el, options);
        terminal->flush();

        REQUIRE_EQUAL(writtenLines, 2);
        REQUIRE_EQUAL(backend->output(), std::string{"A\n\n"});
    }

    void testPrintParagraphHonorsHorizontalMargins() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        auto terminal = createTerminal(backend, block::Size{6, 4});
        terminal->setOutputMode(Terminal::OutputMode::BlockText);
        auto options = ParagraphOptions{};
        options.setMargins(block::Margins{1, 0});

        const auto writtenLines = terminal->printParagraph("AB CD"_el, options);
        terminal->flush();

        REQUIRE_EQUAL(writtenLines, 2);
        REQUIRE_EQUAL(backend->output(), std::string{" AB\n CD\n"});
    }

    void testPrintParagraphTreatsNewlinesAsHardBreaksAndResetsWrapCounting() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        auto terminal = createTerminal(backend, block::Size{5, 4});
        terminal->setOutputMode(Terminal::OutputMode::BlockText);
        auto options = ParagraphOptions{};
        options.setMaximumLineWraps(1);

        const auto writtenLines = terminal->printParagraph("AA BB CC DD EE\nFF GG HH II JJ"_el, options);
        terminal->flush();

        REQUIRE_EQUAL(writtenLines, 4);
        REQUIRE_EQUAL(backend->output(), std::string{"AA BB\nCC…\nFF GG\nHH…\n"});
    }

    void testPrintParagraphKeepsRightSidePaddingWhenFullRightBackgroundIsRequested() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        auto terminal = createTerminal(backend, block::Size{4, 4});
        terminal->setOutputMode(Terminal::OutputMode::BlockText);
        auto options = ParagraphOptions{};
        options.setBackgroundMode(ParagraphBackgroundMode::FullRight);

        const auto writtenLines = terminal->printParagraph("AB CD"_el, options);
        terminal->flush();

        REQUIRE_EQUAL(writtenLines, 2);
        REQUIRE_EQUAL(backend->output(), std::string{"AB  \nCD  \n"});
    }

    void testPrintParagraphFallsBackToPlainOutputWhenRequested() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        auto terminal = createTerminal(backend, block::Size{2, 4});
        terminal->setOutputMode(Terminal::OutputMode::BlockText);
        auto options = ParagraphOptions{};
        options.setLineBreakEndMark(BlockStringEditor{">>"_el});
        options.setOnError(ParagraphOnError::PlainOutput);

        const auto writtenLines = terminal->printParagraph("AA BB"_el, options);
        terminal->flush();

        REQUIRE_EQUAL(writtenLines, 3);
        REQUIRE_EQUAL(backend->output(), std::string{"AA BB\n"});
    }

    void testPrintParagraphUsesExactlyTwoLineBreaksForAnEmptyDoubleSpacedParagraph() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        auto terminal = createTerminal(backend, block::Size{4, 3});
        terminal->setOutputMode(Terminal::OutputMode::BlockText);
        auto options = ParagraphOptions{};
        options.setParagraphSpacing(ParagraphSpacing::DoubleLine);

        const auto writtenLines = terminal->printParagraph(""_el, options);
        terminal->flush();

        REQUIRE_EQUAL(writtenLines, 2);
        REQUIRE_EQUAL(backend->output(), std::string{"\n\n"});
    }

    void testPrintParagraphStringOverloadReplacesInvalidUtf8() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        auto terminal = createTerminal(backend, block::Size{3, 2});
        terminal->setOutputMode(Terminal::OutputMode::BlockText);
        const auto text = bytes({0x41, 0xC3, 0x42});

        const auto writtenLines = terminal->printParagraph(erbsland::text::StringEditor{text});
        terminal->flush();

        REQUIRE_EQUAL(writtenLines, 1);
        REQUIRE_EQUAL(backend->output(), std::string{"A�B\n"});
    }
};
