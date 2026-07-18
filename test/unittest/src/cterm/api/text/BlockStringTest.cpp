// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/TestHelper.hpp"

#include <erbsland/cterm/BlockString.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <string>
#include <type_traits>
#include <utility>
#include <vector>

static_assert(std::is_constructible_v<BlockString, const el::text::String &, BlockStyle>);
static_assert(std::is_constructible_v<BlockString, const el::text::U32String &, BlockStyle>);
static_assert(std::is_same_v<decltype(std::declval<const BlockString &>().trimmed()), BlockString>);
static_assert(std::is_same_v<decltype(BlockString::fromLines({el::text::String{"line"_el}})), BlockString>);
static_assert(std::is_same_v<decltype(std::declval<const BlockStringEditor &>().trimmed()), BlockStringEditor>);

TESTED_TARGETS(BlockString)
class BlockStringTest final : public el::UnitTest {
public:
    void testEditorImplicitlyConvertsToString() {
        auto text = BlockStringEditor{};
        text.append(Block{U'A', fg::Red, bg::Black});
        text.append(Block{U'B', fg::Green, bg::Black});

        const BlockString string = text;

        REQUIRE_EQUAL(string.length(), BlockCount{2U});
        REQUIRE_EQUAL(string[BlockIndex{0U}], U'A');
        REQUIRE_EQUAL(string[BlockIndex{1U}], U'B');
        REQUIRE_EQUAL(string[BlockIndex{0U}].color(), Color(fg::Red, bg::Black));
        REQUIRE_EQUAL(string[BlockIndex{1U}].color(), Color(fg::Green, bg::Black));
    }

    void testStringCanBeExplicitlyConstructedFromAView() {
        const auto source = BlockStringEditor{"AB界D"_el};
        const auto view = BlockString{source}.slice(BlockRange{BlockIndex{1U}, BlockCount{2U}});
        const auto copy = BlockStringEditor{view};

        REQUIRE_EQUAL(render(copy), std::string{"B界"});
        REQUIRE_EQUAL(copy.length(), BlockCount{2U});
    }

    void testSubstrAndTrimmedReturnViews() {
        const auto source = BlockStringEditor{"  alpha beta  "_el};
        const auto trimmed = BlockString{source}.trimmed(erbsland::text::CharSet{" "_el});
        const auto middle = trimmed.slice(BlockRange{BlockIndex{2U}, BlockCount{5U}});

        REQUIRE_EQUAL(render(trimmed), std::string{"alpha beta"});
        REQUIRE_EQUAL(render(middle), std::string{"pha b"});
    }

    void testIndexedAccessAndAtRespectCroppedBounds() {
        const auto source = BlockStringEditor{"012345"_el};
        const auto view = BlockString{source}.slice(BlockRange{BlockIndex{2U}, BlockCount{3U}});

        REQUIRE_EQUAL(view[BlockIndex{0U}], U'2');
        REQUIRE_EQUAL(view[BlockIndex{2U}], U'4');
        REQUIRE(view[BlockIndex{3U}].isEmpty());
        REQUIRE_THROWS_AS(erbsland::err::OutOfRangeError, view.at(BlockIndex{3U}));
    }

    void testTrimmedUsesDefaultWhitespaceCharacters() {
        const auto source = BlockStringEditor{"xx \talpha\n yy"_el};
        const auto view = BlockString{source}.slice(BlockRange{BlockIndex{2U}, BlockCount{9U}});

        REQUIRE_EQUAL(render(view), std::string{" \talpha\n "});
        REQUIRE_EQUAL(render(view.trimmed()), std::string{"alpha"});
    }

    void testTrimmedRemovesNonWhitespaceCharactersFromCroppedView() {
        const auto source = BlockStringEditor{"00xyalpha yx11"_el};
        const auto view = BlockString{source}.slice(BlockRange{BlockIndex{2U}, BlockCount{10U}});

        REQUIRE_EQUAL(render(view.trimmed(erbsland::text::CharSet{"xy"_el})), std::string{"alpha "});
    }

    void testIndexOfCharacterSetCoversBoundsAndCroppedViews() {
        const auto source = BlockStringEditor{"xxabc-yy"_el};
        const auto view = BlockString{source}.slice(BlockRange{BlockIndex{2U}, BlockCount{4U}});

        REQUIRE_EQUAL(view.indexOf(erbsland::text::CharSet{"-x"_el}), BlockIndex{3U});
        REQUIRE_EQUAL(view.indexOf(erbsland::text::CharSet{"-x"_el}, BlockIndex{4U}), BlockIndex::noIndex());
        REQUIRE_EQUAL(view.indexOf(erbsland::text::CharSet{"x"_el}), BlockIndex::noIndex());
        REQUIRE_EQUAL(view.indexOf(erbsland::text::CharSet{"-"_el}, BlockIndex{99U}), BlockIndex::noIndex());
    }

    void testIndexNotOfCharacterSetCoversBoundsAndCroppedViews() {
        const auto source = BlockStringEditor{"xxabc-yy"_el};
        const auto view = BlockString{source}.slice(BlockRange{BlockIndex{2U}, BlockCount{4U}});

        REQUIRE_EQUAL(view.indexNotOf(erbsland::text::CharSet{"abc"_el}), BlockIndex{3U});
        REQUIRE_EQUAL(view.indexNotOf(erbsland::text::CharSet{"abc-"_el}), BlockIndex::noIndex());
        REQUIRE_EQUAL(view.indexNotOf(erbsland::text::CharSet{"a"_el}, BlockIndex{99U}), BlockIndex::noIndex());
    }

    void testSplitWordsReturnsOnlyWordViews() {
        const auto words = BlockString{BlockStringEditor{"  alpha\tbeta\n\ngamma  "_el}}.splitWords();

        REQUIRE_EQUAL(renderWords(words), std::vector<std::string>({"alpha", "beta", "gamma"}));
    }

    void testSplitLinesPreservesEmptyLinesWithoutTrailingEmptyLine() {
        const auto lines = BlockString{BlockStringEditor{"alpha\n\nbeta\n"_el}}.splitLines();

        REQUIRE_EQUAL(renderWords(lines), std::vector<std::string>({"alpha", "", "beta"}));
    }

    void testWrapIntoLinesMaterializesStrings() {
        const auto lines =
            BlockString{BlockStringEditor{"alpha beta\ngamma"_el}}.wrapIntoLines(6, ParagraphSpacing::DoubleLine);

        REQUIRE_EQUAL(renderStringLines(lines), std::vector<std::string>({"alpha", "beta", "", "gamma"}));
    }

    void testTextMeasurementWorksOnViews() {
        const auto source = BlockStringEditor{"A界\nBC"_el};
        const auto view = BlockString{source};
        auto options = BlockTextOptions{};

        REQUIRE_EQUAL(view.naturalBlockTextSize(), (bgeo::BlockSize{3, 2}));
        REQUIRE_EQUAL(view.wrappedBlockTextHeight(blockCoordinate(3), options), 2);
    }

    void testCroppedViewsUseTheSameRangeAlgorithmsAsStrings() {
        const auto source = BlockStringEditor{U"xxA界 e\u0301\nB yy"_el};
        const auto stringRange = source.slice(BlockRange{BlockIndex{2U}, BlockCount{6U}});
        const auto view = BlockString{source}.slice(BlockRange{BlockIndex{2U}, BlockCount{6U}});

        REQUIRE_EQUAL(render(view), render(stringRange));
        REQUIRE_EQUAL(view.displayWidth(), stringRange.displayWidth());
        REQUIRE_EQUAL(view.count(U'e'), stringRange.count(U'e'));
        REQUIRE_EQUAL(view.indexOf(U'\n'), stringRange.indexOf(U'\n'));
        REQUIRE_EQUAL(
            view.indexNotOf(erbsland::text::CharSet{"A界 e"_el}),
            stringRange.indexNotOf(erbsland::text::CharSet{"A界 e"_el}));
        REQUIRE_EQUAL(
            render(view.croppedToDisplayWidth(blockCoordinate(4), bgeo::Alignment::Left)),
            render(stringRange.croppedToDisplayWidth(blockCoordinate(4), bgeo::Alignment::Left)));
        REQUIRE_EQUAL(
            render(view.croppedToDisplayWidth(blockCoordinate(4), bgeo::Alignment::Right)),
            render(stringRange.croppedToDisplayWidth(blockCoordinate(4), bgeo::Alignment::Right)));
        REQUIRE_EQUAL(renderWords(view.splitWords()), std::vector<std::string>({"A界", "é", "B"}));
        REQUIRE_EQUAL(renderWords(view.splitLines()), std::vector<std::string>({"A界 é", "B"}));
        REQUIRE_EQUAL(view.naturalBlockTextSize(), stringRange.naturalBlockTextSize());
    }

    void testEmptyViewRangesStayEmpty() {
        const auto view = BlockString{BlockStringEditor{"xxx"_el}}.trimmed(erbsland::text::CharSet{"x"_el});

        REQUIRE(view.isEmpty());
        REQUIRE(view.slice(BlockRange{BlockIndex{0U}, BlockCount::infinite()}).isEmpty());
        REQUIRE(view.croppedToDisplayWidth(blockCoordinate(3), bgeo::Alignment::Left).isEmpty());
        REQUIRE(view.splitWords().empty());
        REQUIRE(view.splitLines().empty());
    }

    void testMovedFromViewKeepsEmptyInvariant() {
        const auto source = BlockStringEditor{"alpha"_el};
        auto view = BlockString{source};
        const auto moved = std::move(view);

        REQUIRE_EQUAL(render(moved), std::string{"alpha"});
        REQUIRE(view.isEmpty());
        REQUIRE_EQUAL(view.displayWidth(), 0);
        REQUIRE_EQUAL(view.length(), BlockCount{0});

        auto assigned = BlockString{source};
        assigned = std::move(view);

        REQUIRE(assigned.isEmpty());
        REQUIRE(view.isEmpty());
        REQUIRE_EQUAL(view.displayWidth(), 0);
    }

private:
    [[nodiscard]] static auto render(const BlockString text) -> std::string {
        auto result = std::string{};
        for (const auto &character : text) {
            result += blockToStdString(character);
        }
        return result;
    }

    [[nodiscard]] static auto renderWords(const std::vector<BlockString> &words) -> std::vector<std::string> {
        auto result = std::vector<std::string>{};
        result.reserve(words.size());
        for (const auto word : words) {
            result.push_back(render(word));
        }
        return result;
    }

    [[nodiscard]] static auto renderStringLines(const BlockStringLines &lines) -> std::vector<std::string> {
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
