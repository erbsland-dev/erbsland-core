// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/TestHelper.hpp"

#include <erbsland/text/U8EncodingError.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <atomic>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

TESTED_TARGETS(BlockString)
class BlockStringTest final : public UNITTEST_SUBCLASS(TestHelper) {
public:
    void testUtf8StringIsSplitIntoTerminalCharacters() {
        const auto text = BlockString{"äöü"_el};
        REQUIRE_EQUAL(text.length(), BlockCount{3});
        REQUIRE_EQUAL(text[BlockIndex{0}], U'ä');
        REQUIRE_EQUAL(text[BlockIndex{1}], U'ö');
        REQUIRE_EQUAL(text[BlockIndex{2}], U'ü');
    }

    void testUtf32StringIsSplitIntoTerminalCharacters() {
        const auto text = BlockString{U"äöü"_el};
        REQUIRE_EQUAL(text.length(), BlockCount{3});
        REQUIRE_EQUAL(text[BlockIndex{0}], U'ä');
        REQUIRE_EQUAL(text[BlockIndex{1}], U'ö');
        REQUIRE_EQUAL(text[BlockIndex{2}], U'ü');
    }

    void testDisplayWidth() {
        const auto text = BlockString{"A中B"_el};
        REQUIRE_EQUAL(text.displayWidth(), 4);
    }

    void testCountAndIndexOfRespectColorSpecificAndColorAgnosticSearches() {
        auto text = BlockString{};
        text.append(Block{U'A', fg::Red, bg::Black});
        text.append(Block{U'B', fg::Blue, bg::Black});
        text.append(Block{U'A', fg::Green, bg::Black});
        text.append(Block{U"e\u0301"_el});

        REQUIRE_EQUAL(text.count(Block{U'A', fg::Red, bg::Black}), BlockCount{1});
        REQUIRE_EQUAL(text.count(Block{U'A', fg::Green, bg::Black}), BlockCount{1});
        REQUIRE_EQUAL(text.count(Block{U'A', fg::White, bg::Black}), BlockCount{0});
        REQUIRE_EQUAL(text.count(U'A'), BlockCount{2});
        REQUIRE_EQUAL(text.count(U'e'), BlockCount{0});

        REQUIRE_EQUAL(text.indexOf(Block{U'A', fg::Green, bg::Black}), BlockIndex{2});
        REQUIRE_EQUAL(text.indexOf(Block{U'A', fg::Red, bg::Black}, BlockIndex{1}), BlockIndex::noIndex());
        REQUIRE_EQUAL(text.indexOf(U'A'), BlockIndex{0});
        REQUIRE_EQUAL(text.indexOf(U'A', BlockIndex{1}), BlockIndex{2});
        REQUIRE_EQUAL(text.indexOf(U'Z'), BlockIndex::noIndex());
        REQUIRE_EQUAL(text.indexOf(U'B', BlockIndex{99}), BlockIndex::noIndex());
    }

    void testSubstrReturnsRequestedWindowAndClampsBounds() {
        const auto text = BlockString{"AB界D"_el};

        const auto middle = text.slice(BlockRange{BlockIndex{1U}, BlockCount{2U}});
        const auto tail = text.slice(BlockRange{BlockIndex{2U}, BlockCount::infinite()});

        REQUIRE_EQUAL(middle.length(), BlockCount{2});
        REQUIRE_EQUAL(middle[BlockIndex{0}], U'B');
        REQUIRE_EQUAL(middle[BlockIndex{1}], U'界');
        REQUIRE_EQUAL(tail.length(), BlockCount{2});
        REQUIRE_EQUAL(tail[BlockIndex{0}], U'界');
        REQUIRE_EQUAL(tail[BlockIndex{1}], U'D');
        REQUIRE(text.slice(BlockRange{BlockIndex{2U}, BlockCount{0U}}).isEmpty());
        REQUIRE(text.slice(BlockRange{BlockIndex{99U}, BlockCount::infinite()}).isEmpty());
    }

    void testIndexedAccessAndAtRespectCroppedBounds() {
        const auto text = BlockString{"012345"_el};
        const auto cropped = text.slice(BlockRange{BlockIndex{2U}, BlockCount{3U}});

        REQUIRE_EQUAL(cropped[BlockIndex{0}], U'2');
        REQUIRE_EQUAL(cropped[BlockIndex{2}], U'4');
        REQUIRE(cropped[BlockIndex{3}].isEmpty());
        REQUIRE_THROWS_AS(erbsland::err::OutOfRangeError, cropped.at(BlockIndex{3}));
    }

    void testMutableIndexedAccessIgnoresOutOfBounds() {
        auto text = BlockString{"ABC"_el}.slice(BlockRange{BlockIndex{1U}, BlockCount{2U}});

        text[BlockIndex{99}] = Block{U'X'};

        REQUIRE_EQUAL(render(text), std::string{"BC"});
        REQUIRE(text[BlockIndex{99}].isEmpty());
    }

    void testTrimmedUsesDefaultWhitespaceCharacters() {
        const auto text = BlockString{"\n\t alpha \n"_el};
        const auto trimmed = text.trimmed();

        REQUIRE_EQUAL(trimmed.length(), BlockCount{5});
        REQUIRE_EQUAL(trimmed[BlockIndex{0}], U'a');
        REQUIRE_EQUAL(trimmed[BlockIndex{4}], U'a');
    }

    void testTrimRemovesNonWhitespaceCharactersFromBothEndsOnly() {
        auto text = BlockString{"xyalpha yx"_el};

        text.trim(erbsland::text::CharSet{"xy"_el});

        REQUIRE_EQUAL(render(text), std::string{"alpha "});
    }

    void testTrimWorksOnCroppedStringsAndKeepsSharedCopiesIndependent() {
        const auto source = BlockString{"xx  alpha  yy"_el};
        auto cropped = source.slice(BlockRange{BlockIndex{2U}, BlockCount{9U}});
        const auto sharedCropped = cropped;

        const auto trimmed = cropped.trimmed();
        cropped.trim();

        REQUIRE_EQUAL(render(source), std::string{"xx  alpha  yy"});
        REQUIRE_EQUAL(render(sharedCropped), std::string{"  alpha  "});
        REQUIRE_EQUAL(render(trimmed), std::string{"alpha"});
        REQUIRE_EQUAL(render(cropped), std::string{"alpha"});
    }

    void testNormalizedUsesDefaultWhitespaceCharacters() {
        const auto text = BlockString{" \t alpha\n\nbeta  "_el};

        REQUIRE_EQUAL(render(text.normalized()), std::string{"alpha beta"});
    }

    void testNormalizeReplacesNonWhitespaceRuns() {
        auto text = BlockString{"--alpha..beta--"_el};

        text.normalize(erbsland::text::CharSet{"-."_el}, Block{U'|'});

        REQUIRE_EQUAL(render(text), std::string{"alpha|beta"});
    }

    void testNormalizeWorksOnCroppedStringsAndKeepsSharedCopiesIndependent() {
        const auto source = BlockString{"xx  alpha\t beta  yy"_el};
        auto cropped = source.slice(BlockRange{BlockIndex{2U}, BlockCount{15U}});
        const auto sharedCropped = cropped;

        const auto normalized = cropped.normalized();
        cropped.normalize();

        REQUIRE_EQUAL(render(source), std::string{"xx  alpha\t beta  yy"});
        REQUIRE_EQUAL(render(sharedCropped), std::string{"  alpha\t beta  "});
        REQUIRE_EQUAL(render(normalized), std::string{"alpha beta"});
        REQUIRE_EQUAL(render(cropped), std::string{"alpha beta"});
    }

    void testNormalizeAppliesFirstMatchedCharacterStyleToSeparator() {
        auto text = BlockString{};
        text.append(Block{U'A', fg::White, bg::Black});
        text.append(Block{U' ', fg::Red, bg::Blue});
        text.append(Block{U'\t', fg::Green, bg::Cyan});
        text.append(Block{U'B', fg::White, bg::Black});

        const auto normalized =
            text.normalized(erbsland::text::CharSet{" \t"_el}, Block{U'_', fg::Inherited, bg::Yellow});

        REQUIRE_EQUAL(render(normalized), std::string{"A_B"});
        REQUIRE_EQUAL(normalized[BlockIndex{1}], U'_');
        REQUIRE_EQUAL(normalized[BlockIndex{1}].color(), Color(fg::Red, bg::Yellow));
    }

    void testReplaceCharacterCoversSingleMultiAndEmptyReplacement() {
        auto text = BlockString{"ABCDE"_el};

        text.replace(BlockRange{BlockIndex{1}, BlockCount{1}}, Block{U'X'});
        REQUIRE_EQUAL(render(text), std::string{"AXCDE"});

        text.replace(BlockRange{BlockIndex{2}, BlockCount{2}}, Block{U'Y'});
        REQUIRE_EQUAL(render(text), std::string{"AXYE"});

        text.replace(BlockRange{BlockIndex{1}, BlockCount{1}}, Block{});
        REQUIRE_EQUAL(render(text), std::string{"AYE"});
    }

    void testReplaceStringViewCoversShorterLongerAndSameSizeReplacement() {
        auto text = BlockString{"ABCDE"_el};

        text.replace(BlockRange{BlockIndex{1}, BlockCount{2}}, BlockStringView{BlockString{"x"_el}});
        REQUIRE_EQUAL(render(text), std::string{"AxDE"});

        text.replace(BlockRange{BlockIndex{2}, BlockCount{1}}, BlockStringView{BlockString{"YZ"_el}});
        REQUIRE_EQUAL(render(text), std::string{"AxYZE"});

        text.replace(BlockRange{BlockIndex{1}, BlockCount{2}}, BlockStringView{BlockString{"12"_el}});
        REQUIRE_EQUAL(render(text), std::string{"A12ZE"});
    }

    void testReplaceStringViewSameSizeCopiesEveryChangedCharacter() {
        auto text = BlockString{"ABCDE"_el};

        text.replace(BlockRange{BlockIndex{1}, BlockCount{3}}, BlockStringView{BlockString{"xyz"_el}});

        REQUIRE_EQUAL(render(text), std::string{"AxyzE"});
    }

    void testReplaceHandlesOutOfBoundsAndCroppedStrings() {
        const auto source = BlockString{"0123456789"_el};
        auto cropped = source.slice(BlockRange{BlockIndex{2U}, BlockCount{5U}});
        const auto sharedCropped = cropped;

        cropped.replace(BlockRange{BlockIndex{9}, BlockCount{1}}, Block{U'X'});
        REQUIRE_EQUAL(render(cropped), std::string{"23456"});

        cropped.replace(BlockRange{BlockIndex{1}, BlockCount{3}}, BlockStringView{BlockString{"XY"_el}});
        REQUIRE_EQUAL(render(cropped), std::string{"2XY6"});

        cropped.replace(BlockRange{BlockIndex{2}, BlockCount{99}}, Block{U'Z'});
        REQUIRE_EQUAL(render(cropped), std::string{"2XZ"});
        REQUIRE_EQUAL(render(source), std::string{"0123456789"});
        REQUIRE_EQUAL(render(sharedCropped), std::string{"23456"});
    }

    void testRemoveHandlesMiddleTailOutOfBoundsAndCroppedStrings() {
        auto text = BlockString{"ABCDE"_el};

        text.remove(BlockRange{BlockIndex{1}, BlockCount{2}});
        REQUIRE_EQUAL(render(text), std::string{"ADE"});

        text.remove(BlockRange{BlockIndex{1}, BlockCount{99}});
        REQUIRE_EQUAL(render(text), std::string{"A"});

        text.remove(BlockRange{BlockIndex{9}, BlockCount{1}});
        REQUIRE_EQUAL(render(text), std::string{"A"});

        const auto source = BlockString{"0123456789"_el};
        auto cropped = source.slice(BlockRange{BlockIndex{2U}, BlockCount{5U}});
        const auto sharedCropped = cropped;

        cropped.remove(BlockRange{BlockIndex{1}, BlockCount{2}});
        REQUIRE_EQUAL(render(cropped), std::string{"256"});

        cropped.remove(BlockRange{BlockIndex{2}, BlockCount{99}});
        REQUIRE_EQUAL(render(cropped), std::string{"25"});
        REQUIRE_EQUAL(render(source), std::string{"0123456789"});
        REQUIRE_EQUAL(render(sharedCropped), std::string{"23456"});
    }

    void testIndexOfCharacterSetCoversBoundsAndCroppedStrings() {
        const auto text = BlockString{"abc-def"_el};

        REQUIRE_EQUAL(text.indexOf(erbsland::text::CharSet{"-x"_el}), BlockIndex{3});
        REQUIRE_EQUAL(text.indexOf(erbsland::text::CharSet{"-x"_el}, BlockIndex{4}), BlockIndex::noIndex());
        REQUIRE_EQUAL(text.indexOf(erbsland::text::CharSet{"z"_el}), BlockIndex::noIndex());
        REQUIRE_EQUAL(text.indexOf(erbsland::text::CharSet{"-"_el}, BlockIndex{99}), BlockIndex::noIndex());

        const auto cropped = BlockString{"xxabc-yy"_el}.slice(BlockRange{BlockIndex{2U}, BlockCount{4U}});

        REQUIRE_EQUAL(cropped.indexOf(erbsland::text::CharSet{"-x"_el}), BlockIndex{3});
        REQUIRE_EQUAL(cropped.indexOf(erbsland::text::CharSet{"x"_el}), BlockIndex::noIndex());
    }

    void testIndexNotOfCharacterSetCoversBoundsAndCroppedStrings() {
        const auto text = BlockString{"aaabbb"_el};

        REQUIRE_EQUAL(text.indexNotOf(erbsland::text::CharSet{"a"_el}), BlockIndex{3});
        REQUIRE_EQUAL(text.indexNotOf(erbsland::text::CharSet{"ab"_el}), BlockIndex::noIndex());
        REQUIRE_EQUAL(text.indexNotOf(erbsland::text::CharSet{"a"_el}, BlockIndex{99}), BlockIndex::noIndex());

        const auto cropped = BlockString{"xxabc-yy"_el}.slice(BlockRange{BlockIndex{2U}, BlockCount{4U}});

        REQUIRE_EQUAL(cropped.indexNotOf(erbsland::text::CharSet{"abc"_el}), BlockIndex{3});
        REQUIRE_EQUAL(cropped.indexNotOf(erbsland::text::CharSet{"abc-"_el}), BlockIndex::noIndex());
    }

    void testWrapIntoLinesUsesTerminalWidth() {
        const auto text = BlockString{"漢字テスト"_el};
        const auto lines = text.wrapIntoLines(4);
        REQUIRE_EQUAL(lines.size(), std::size_t{3});
        REQUIRE_EQUAL(lines[0][BlockIndex{0}], U'漢');
        REQUIRE_EQUAL(lines[0][BlockIndex{1}], U'字');
        REQUIRE_EQUAL(lines[1][BlockIndex{0}], U'テ');
        REQUIRE_EQUAL(lines[1][BlockIndex{1}], U'ス');
        REQUIRE_EQUAL(lines[2][BlockIndex{0}], U'ト');
    }

    void testWrapIntoLinesUsesSingleParagraphSpacingByDefault() {
        const auto text = BlockString{"alpha beta\ngamma"_el};
        const auto lines = text.wrapIntoLines(6);
        REQUIRE_EQUAL(lines.size(), std::size_t{3});
        REQUIRE_EQUAL(lines[0][BlockIndex{0}], U'a');
        REQUIRE_EQUAL(lines[0][BlockIndex{4}], U'a');
        REQUIRE_EQUAL(lines[1][BlockIndex{0}], U'b');
        REQUIRE_EQUAL(lines[2][BlockIndex{0}], U'g');
    }

    void testWrapIntoLinesAddsExtraSpacingForDoubleParagraphSpacing() {
        const auto text = BlockString{"alpha beta\ngamma"_el};
        const auto lines = text.wrapIntoLines(6, ParagraphSpacing::DoubleLine);

        REQUIRE_EQUAL(lines.size(), std::size_t{4});
        REQUIRE_EQUAL(lines[0][BlockIndex{0}], U'a');
        REQUIRE_EQUAL(lines[1][BlockIndex{0}], U'b');
        REQUIRE(lines[2].isEmpty());
        REQUIRE_EQUAL(lines[3][BlockIndex{0}], U'g');
    }

    void testWrapIntoLinesPreservesExplicitEmptyParagraphs() {
        const auto lines = BlockString{"alpha\n\nbeta"_el}.wrapIntoLines(10);

        REQUIRE_EQUAL(renderLines(lines), std::vector<std::string>({"alpha", "", "beta"}));
    }

    void testSplitLinesPreservesEmptyLinesWithoutTrailingEmptyLine() {
        const auto lines = BlockString{"alpha\n\nbeta\n"_el}.splitLines();

        REQUIRE_EQUAL(lines.size(), std::size_t{3});
        REQUIRE_EQUAL(lines[0].length(), BlockCount{5});
        REQUIRE_EQUAL(lines[0][BlockIndex{0}], U'a');
        REQUIRE(lines[1].isEmpty());
        REQUIRE_EQUAL(lines[2].length(), BlockCount{4});
        REQUIRE_EQUAL(lines[2][BlockIndex{0}], U'b');
    }

    void testSplitLinesHandlesEmptyLeadingAndSingleNewlineStrings() {
        REQUIRE(BlockString{}.splitLines().empty());

        const auto leadingEmptyLine = BlockString{"\nalpha"_el}.splitLines();
        REQUIRE_EQUAL(leadingEmptyLine.size(), std::size_t{2});
        REQUIRE(leadingEmptyLine[0].isEmpty());
        REQUIRE_EQUAL(leadingEmptyLine[1][BlockIndex{0}], U'a');

        const auto singleNewline = BlockString{"\n"_el}.splitLines();
        REQUIRE_EQUAL(singleNewline.size(), std::size_t{1});
        REQUIRE(singleNewline[0].isEmpty());
    }

    void testWrapIntoLinesPreservesColoredSpacingBetweenWords() {
        auto text = BlockString{};
        text.append(Block{U'A', fg::Red, bg::Black});
        text.append(Block{U' ', fg::Inherited, bg::Blue});
        text.append(Block{U' ', fg::Inherited, bg::Cyan});
        text.append(Block{U'B', fg::Green, bg::Black});

        const auto lines = text.wrapIntoLines(8);

        REQUIRE_EQUAL(lines.size(), std::size_t{1});
        REQUIRE_EQUAL(lines[0].length(), BlockCount{4});
        REQUIRE_EQUAL(lines[0][BlockIndex{1}].color(), Color(fg::Inherited, bg::Blue));
        REQUIRE_EQUAL(lines[0][BlockIndex{2}].color(), Color(fg::Inherited, bg::Cyan));
    }

    void testWrapIntoLinesTrimsOuterSpacingAndKeepsInternalSpacingThatFits() {
        const auto lines = BlockString{"  alpha   beta  "_el}.wrapIntoLines(12);

        REQUIRE_EQUAL(renderLines(lines), std::vector<std::string>({"alpha   beta"}));
    }

    void testWrapIntoLinesSplitsOversizedWordsIntoStandaloneChunks() {
        const auto lines = BlockString{"supercalifragilistic test"_el}.wrapIntoLines(5);

        REQUIRE_EQUAL(renderLines(lines), std::vector<std::string>({"super", "calif", "ragil", "istic", "test"}));
    }

    void testWrapIntoLinesSplitsWideWordsAtCharacterBoundaries() {
        const auto lines = BlockString{"漢字abcde"_el}.wrapIntoLines(5);

        REQUIRE_EQUAL(renderLines(lines), std::vector<std::string>({"漢字a", "bcde"}));
    }

    void testWrapIntoLinesDoesNotDuplicateWordsBetweenTokens() {
        const auto lines =
            BlockString{"Text Gallery  |  alignment, wrapping, wide characters, and bitmap fonts"_el}.wrapIntoLines(
                200);

        REQUIRE_EQUAL(
            renderLines(lines),
            std::vector<std::string>({"Text Gallery  |  alignment, wrapping, wide characters, and bitmap fonts"}));
    }

    void testTerminalLinesCountsWrappedAndExplicitLineBreaks() {
        REQUIRE_EQUAL(BlockString{}.terminalLines(4), 0);
        REQUIRE_EQUAL(BlockString{"AA BB"_el}.terminalLines(2), 3);
        REQUIRE_EQUAL(BlockString{"AA\nBB"_el}.terminalLines(2), 3);
        REQUIRE_EQUAL(BlockString{"AA\n"_el}.terminalLines(2), 2);
    }

    void testNaturalTextSizeMeasuresExplicitLines() {
        REQUIRE_EQUAL(BlockString{}.naturalBlockTextSize(), (bgeo::BlockSize{1, 1}));
        REQUIRE_EQUAL(BlockString{"A界\nBC"_el}.naturalBlockTextSize(), (bgeo::BlockSize{3, 2}));
        REQUIRE_EQUAL(BlockString{"AB\n\nC\n"_el}.naturalBlockTextSize(), (bgeo::BlockSize{2, 3}));
    }

    void testWrappedTextHeightUsesParagraphLayoutAndMargins() {
        auto options = BlockTextOptions{};
        options.setMargins(bgeo::BlockMargins{1, 0, 2, 0});

        REQUIRE_EQUAL(BlockString{"alpha beta gamma"_el}.wrappedBlockTextHeight(blockCoordinate(10), options), 5);
    }

    void testWrappedTextHeightUsesPlainOutputFallbackForInvalidParagraphSettings() {
        auto options = BlockTextOptions{};
        options.setLineBreakEndMark(BlockString{">>"_el});
        options.setOnError(ParagraphOnError::PlainOutput);

        REQUIRE_EQUAL(BlockString{"AA BB"_el}.wrappedBlockTextHeight(blockCoordinate(2), options), 2);

        options.setOnError(ParagraphOnError::Empty);
        REQUIRE_EQUAL(BlockString{"AA BB"_el}.wrappedBlockTextHeight(blockCoordinate(2), options), 0);
    }

    void testCombiningCharactersStayInOneCell() {
        const auto text = BlockString{erbsland::text::String{bytes({0x65, 0xCC, 0x81})}};
        REQUIRE_EQUAL(text.length(), BlockCount{1});
        REQUIRE_EQUAL(text.displayWidth(), 1);
        REQUIRE_EQUAL(text[BlockIndex{0}].toString(), erbsland::text::String{bytes({0x65, 0xCC, 0x81})});
    }

    void testControlCodesAreFilteredExceptTabAndNewline() {
        const auto text = BlockString{"A\r\x01\t\nB"_el};

        REQUIRE_EQUAL(text.length(), BlockCount{4});
        REQUIRE_EQUAL(text[BlockIndex{0}], U'A');
        REQUIRE_EQUAL(text[BlockIndex{1}].toString(), "\t"_el);
        REQUIRE_EQUAL(text[BlockIndex{2}].toString(), "\n"_el);
        REQUIRE_EQUAL(text[BlockIndex{3}], U'B');
    }

    void testInvalidUtf8Fails() {
        REQUIRE_THROWS_AS(
            erbsland::text::U8EncodingError,
            (BlockString{erbsland::text::String{bytes({0xC3})}, erbsland::text::EncodingErrorMode::Throw}));
    }

    void testUtf8StringCanIgnoreEncodingErrorMode() {
        const auto text =
            BlockString{erbsland::text::String{bytes({0x41, 0xC3, 0x42})}, erbsland::text::EncodingErrorMode::Ignore};

        REQUIRE_EQUAL(text.length(), BlockCount{2});
        REQUIRE_EQUAL(text[BlockIndex{0}], U'A');
        REQUIRE_EQUAL(text[BlockIndex{1}], U'B');
    }

    void testUtf8StringCanReplaceEncodingErrorModeDeterministically() {
        const auto text = BlockString{
            erbsland::text::String{bytes({0x41, 0xE2, 0x28, 0xA1, 0x42})}, erbsland::text::EncodingErrorMode::Replace};

        REQUIRE_EQUAL(text.length(), BlockCount{5});
        REQUIRE_EQUAL(text[BlockIndex{0}], U'A');
        REQUIRE_EQUAL(text[BlockIndex{1}], U'\uFFFD');
        REQUIRE_EQUAL(text[BlockIndex{2}], U'(');
        REQUIRE_EQUAL(text[BlockIndex{3}], U'\uFFFD');
        REQUIRE_EQUAL(text[BlockIndex{4}], U'B');
    }

    void testStyledUtf8StringCanReplaceEncodingErrorMode() {
        auto attributes = BlockAttributes{};
        attributes.setBold(true);
        const auto style = BlockStyle{Color{fg::Red, bg::Blue}, attributes};
        const auto text =
            BlockString{erbsland::text::String{bytes({0xC3})}, style, erbsland::text::EncodingErrorMode::Replace};

        REQUIRE_EQUAL(text.length(), BlockCount{1});
        REQUIRE_EQUAL(text[BlockIndex{0}], U'\uFFFD');
        REQUIRE_EQUAL(text[BlockIndex{0}].style(), style);
    }

    void testAppendStyledAppendsUtf32TextWithoutChangingExistingCharacters() {
        auto text = BlockString{"A"_el};
        auto attributes = BlockAttributes{};
        attributes.setBold(true);
        const auto style = BlockStyle{Color{fg::BrightGreen, bg::Blue}, attributes};

        text.appendStyled(U"BC"_el, style);

        REQUIRE_EQUAL(text.length(), BlockCount{3});
        REQUIRE_EQUAL(text[BlockIndex{0}], U'A');
        REQUIRE_EQUAL(text[BlockIndex{1}], U'B');
        REQUIRE_EQUAL(text[BlockIndex{2}], U'C');
        REQUIRE_EQUAL(text[BlockIndex{1}].style(), style);
        REQUIRE_EQUAL(text[BlockIndex{2}].style(), style);
    }

    void testStringViewAppendOperatorsReuseReadOnlyRanges() {
        const auto source = BlockString{"ABCDE"_el};
        const auto middle = BlockStringView{source}.slice(BlockRange{BlockIndex{1U}, BlockCount{3U}});

        auto appended = BlockString{"A"_el};
        appended += middle.slice(BlockRange{BlockIndex{0U}, BlockCount{2U}});

        const auto combined = BlockString{"A"_el} + middle.slice(BlockRange{BlockIndex{2U}, BlockCount{1U}});

        REQUIRE_EQUAL(render(appended), std::string{"ABC"});
        REQUIRE_EQUAL(render(combined), std::string{"AD"});
    }

    void testCopyOnWriteKeepsCopiedStringsIndependent() {
        const auto source = BlockString{"alpha"_el};
        auto copy = source;

        copy[BlockIndex{0}] = Block{U'A'};
        copy.append(Block{U'!'});

        REQUIRE_EQUAL(render(source), std::string{"alpha"});
        REQUIRE_EQUAL(render(copy), std::string{"Alpha!"});
    }

    void testSubstrSharesStorageUntilTheSliceIsModified() {
        const auto source = BlockString{"ABCDE"_el};
        auto slice = source.slice(BlockRange{BlockIndex{1U}, BlockCount{3U}});

        slice[BlockIndex{0}] = Block{U'X'};

        REQUIRE_EQUAL(render(source), std::string{"ABCDE"});
        REQUIRE_EQUAL(render(slice), std::string{"XCD"});
    }

    void testCroppedStringDetachesAfterReadOnlyRangeOperations() {
        const auto source = BlockString{"xxABC\nyy"_el};
        auto cropped = source.slice(BlockRange{BlockIndex{2U}, BlockCount{4U}});
        const auto sharedCropped = cropped;

        REQUIRE_EQUAL(cropped.displayWidth(), 3);
        REQUIRE_EQUAL(cropped.count(U'B'), BlockCount{1});
        REQUIRE_EQUAL(cropped.indexOf(U'\n'), BlockIndex{3});
        REQUIRE_EQUAL(render(cropped.slice(BlockRange{BlockIndex{1U}, BlockCount{2U}})), std::string{"BC"});
        REQUIRE_EQUAL(
            render(cropped.croppedToDisplayWidth(blockCoordinate(2), bgeo::Alignment::Left)), std::string{"AB"});
        REQUIRE_EQUAL(renderLines(cropped.splitLines()), std::vector<std::string>({"ABC"}));

        cropped[BlockIndex{1}] = Block{U'Z'};

        REQUIRE_EQUAL(render(source), std::string{"xxABC\nyy"});
        REQUIRE_EQUAL(render(sharedCropped), std::string{"ABC\n"});
        REQUIRE_EQUAL(render(cropped), std::string{"AZC\n"});
    }

    void testMutableIteratorsDetachBeforeWriting() {
        const auto source = BlockString{"ABCD"_el};
        auto copy = source;

        *copy.begin() = Block{U'Z'};

        REQUIRE_EQUAL(render(source), std::string{"ABCD"});
        REQUIRE_EQUAL(render(copy), std::string{"ZBCD"});
    }

    void testReserveAndClearDoNotModifySharedCopies() {
        const auto source = BlockString{"ABCD"_el};
        auto copy = source;

        copy.reserve(BlockCount{64});
        copy.clear();

        REQUIRE_EQUAL(render(source), std::string{"ABCD"});
        REQUIRE(copy.isEmpty());
    }

    void testSharedCopiesSupportConcurrentReads() {
        const auto source = BlockString{"alpha beta gamma"_el};
        auto readFailed = std::atomic<bool>{false};
        auto worker = [&source, &readFailed]() {
            for (auto iteration = 0; iteration < 2'000; ++iteration) {
                const auto copy = source;
                if (copy.displayWidth() != 16 || copy.indexOf(U'b') != BlockIndex{6U} ||
                    copy.slice(BlockRange{BlockIndex{6U}, BlockCount{4U}}).displayWidth() != 4) {
                    readFailed.store(true);
                }
            }
        };

        auto a = std::thread{worker};
        auto b = std::thread{worker};
        auto c = std::thread{worker};

        a.join();
        b.join();
        c.join();

        REQUIRE_FALSE(readFailed.load());
        REQUIRE_EQUAL(render(source), std::string{"alpha beta gamma"});
    }

    void testCopiedStringsCanMutateIndependentlyOnMultipleThreads() {
        const auto source = BlockString{"Base"_el};
        auto left = source;
        auto right = source;

        auto leftDone = std::atomic<bool>{false};
        auto rightDone = std::atomic<bool>{false};

        auto leftWorker = std::thread{[&left, &leftDone]() {
            left.clear();
            left.append("left"_el);
            left[BlockIndex{0}] = Block{U'L'};
            leftDone.store(true);
        }};
        auto rightWorker = std::thread{[&right, &rightDone]() {
            right.clear();
            right.append("right"_el);
            right[BlockIndex{0}] = Block{U'R'};
            rightDone.store(true);
        }};

        leftWorker.join();
        rightWorker.join();

        REQUIRE(leftDone.load());
        REQUIRE(rightDone.load());
        REQUIRE_EQUAL(render(source), std::string{"Base"});
        REQUIRE_EQUAL(render(left), std::string{"Left"});
        REQUIRE_EQUAL(render(right), std::string{"Right"});
    }

    void testTrimmedSubstr() {
        const auto source = BlockString{"XXX  ABC  XXX"_el};
        auto substr = source.slice(BlockRange{BlockIndex{3U}, BlockCount{7U}});
        REQUIRE_EQUAL(substr, BlockString{"  ABC  "_el});
        auto trimmed = substr.trimmed();
        REQUIRE_EQUAL(trimmed, BlockString{"ABC"_el});
    }

    void testNormalizeSpecial() {
        auto source = BlockString{"\nXXX\n\tABC\n\tXXX\t"_el};
        source.normalize();
        REQUIRE_EQUAL(source, BlockString{"XXX ABC XXX"_el});
    }

    void testMovedFromStringKeepsEmptyInvariant() {
        auto source = BlockString{"alpha"_el};
        const auto target = std::move(source);

        REQUIRE_EQUAL(render(target), std::string{"alpha"});
        REQUIRE(source.isEmpty());
        REQUIRE_EQUAL(source.displayWidth(), 0);
        REQUIRE_EQUAL(source.length(), BlockCount{0});

        auto assigned = BlockString{"beta"_el};
        assigned = std::move(source);

        REQUIRE(assigned.isEmpty());
        REQUIRE(source.isEmpty());
        REQUIRE_EQUAL(source.displayWidth(), 0);
    }

private:
    [[nodiscard]] static auto render(const BlockString &text) -> std::string {
        auto result = std::string{};
        for (const auto &character : text) {
            result += blockToStdString(character);
        }
        return result;
    }

    [[nodiscard]] static auto renderLines(const BlockStringLines &lines) -> std::vector<std::string> {
        auto result = std::vector<std::string>{};
        result.reserve(lines.size());
        for (const auto &line : lines) {
            auto text = std::string{};
            for (const auto &character : line) {
                text += blockToStdString(character);
            }
            result.push_back(text);
        }
        return result;
    }
};
