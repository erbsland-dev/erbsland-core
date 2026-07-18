// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/TestHelper.hpp"

#include <erbsland/unittest/UnitTest.hpp>

#include <string>
#include <vector>

TESTED_TARGETS(BlockStringEditor)
class BlockStringConversionTest final : public el::UnitTest {
public:
    void testFromLinesJoinsUtf8AndUtf32LinesAndPreservesColor() {
        const auto utf8 = BlockStringEditor::fromLines({"A"_el, "B"_el}, Color{fg::Red, bg::Blue});
        REQUIRE_EQUAL(render(utf8), std::string{"A\nB"});
        REQUIRE_EQUAL(utf8.length(), BlockCount{3U});
        REQUIRE_EQUAL(utf8[BlockIndex{0U}].color(), Color(fg::Red, bg::Blue));
        REQUIRE_EQUAL(utf8[BlockIndex{1U}], U'\n');
        REQUIRE_EQUAL(utf8[BlockIndex{2U}].color(), Color(fg::Red, bg::Blue));

        const auto utf32 = BlockStringEditor::fromLines({U"ä"_el, U"ö"_el}, Color{fg::Green, bg::Black});
        REQUIRE_EQUAL(render(utf32), std::string{"ä\nö"});
        REQUIRE_EQUAL(utf32.length(), BlockCount{3U});
        REQUIRE_EQUAL(utf32[BlockIndex{0U}].color(), Color(fg::Green, bg::Black));
        REQUIRE_EQUAL(utf32[BlockIndex{2U}].color(), Color(fg::Green, bg::Black));

        REQUIRE(BlockStringEditor::fromLines(std::initializer_list<erbsland::text::String>{}).isEmpty());
        REQUIRE(BlockStringEditor::fromLines(std::initializer_list<erbsland::text::U32String>{}).isEmpty());
    }

    void testFromLinesCanApplyBaseAttributes() {
        auto attributes = BlockAttributes{};
        attributes.setBold(true);
        attributes.setUnderline(true);

        const auto text = BlockStringEditor::fromLines({"A"_el, "B"_el}, Color{fg::Red, bg::Blue}, attributes);

        REQUIRE_EQUAL(render(text), std::string{"A\nB"});
        REQUIRE(text[BlockIndex{0U}].attributes().isBold());
        REQUIRE(text[BlockIndex{0U}].attributes().isUnderline());
        REQUIRE(text[BlockIndex{1U}].attributes().isBold());
        REQUIRE(text[BlockIndex{1U}].attributes().isUnderline());
        REQUIRE(text[BlockIndex{2U}].attributes().isBold());
        REQUIRE(text[BlockIndex{2U}].attributes().isUnderline());
    }

    void testStringAndFromLinesAcceptBlockStyleForUniformFormatting() {
        auto attributes = BlockAttributes{};
        attributes.setItalic(true);
        const auto style = BlockStyle{Color{fg::Green, bg::Black}, attributes};

        const auto text = BlockStringEditor{"AB"_el, style};
        const auto lines = BlockStringEditor::fromLines({"A"_el, "B"_el}, style);

        REQUIRE_EQUAL(text[BlockIndex{0U}].color(), Color(fg::Green, bg::Black));
        REQUIRE(text[BlockIndex{1U}].attributes().isItalic());
        REQUIRE_EQUAL(lines[BlockIndex{0U}].color(), Color(fg::Green, bg::Black));
        REQUIRE(lines[BlockIndex{2U}].attributes().isItalic());
    }

    void testBraceInitializedDefaultAndColorStylesAreUnambiguous() {
        static_assert(requires { BlockStringEditor{"A"_el, {}}; });
        static_assert(requires { BlockStringEditor{U"A"_el, {}}; });

        const auto inheritedUtf8 = BlockStringEditor{"A"_el, {}};
        const auto inheritedUtf32 = BlockStringEditor{U"A"_el, {}};
        const auto coloredUtf8 = BlockStringEditor{"B"_el, Color{fg::Yellow, bg::Blue}};

        REQUIRE_EQUAL(inheritedUtf8[BlockIndex{0U}].color(), Color{});
        REQUIRE_EQUAL(inheritedUtf32[BlockIndex{0U}].color(), Color{});
        REQUIRE_EQUAL(coloredUtf8[BlockIndex{0U}].color(), Color(fg::Yellow, bg::Blue));
    }

    void testSplitWordsSkipsSpacingAndAppendUsesTheCurrentFormattingWithinOneCall() {
        auto text = BlockStringEditor{};
        auto bold = BlockAttributes{};
        bold.setBold(true);
        auto noBold = BlockAttributes{};
        noBold.setBold(false);
        auto emphasis = BlockStyle{Color{fg::Yellow, bg::Blue}, bold};
        text.append(emphasis, "alpha beta"_el, BlockStringEditor{"!"_el}, noBold, Block{U'?'});

        REQUIRE_EQUAL(text.length(), BlockCount{12U});
        REQUIRE_EQUAL(text[BlockIndex{0U}].color(), Color(fg::Yellow, bg::Blue));
        REQUIRE(text[BlockIndex{0U}].attributes().isBold());
        REQUIRE_EQUAL(text[BlockIndex{5U}].color(), Color(fg::Yellow, bg::Blue));
        REQUIRE(text[BlockIndex{5U}].attributes().isBold());
        REQUIRE_EQUAL(text[BlockIndex{9U}].color(), Color(fg::Yellow, bg::Blue));
        REQUIRE(text[BlockIndex{9U}].attributes().isBold());
        REQUIRE_EQUAL(text[BlockIndex{10U}].color(), Color(fg::Yellow, bg::Blue));
        REQUIRE(text[BlockIndex{10U}].attributes().isBold());
        REQUIRE_EQUAL(text[BlockIndex{10U}], U'!');
        REQUIRE_EQUAL(text[BlockIndex{11U}].color(), Color(fg::Yellow, bg::Blue));
        REQUIRE_FALSE(text[BlockIndex{11U}].attributes().isBold());
        REQUIRE_EQUAL(text[BlockIndex{11U}], U'?');

        auto scopedStyle = BlockStringEditor{};
        scopedStyle.append(fg::Red, "A"_el);
        scopedStyle.append("B"_el);
        scopedStyle.append(Color{fg::Green, bg::Blue}, Block{U'C', fg::Inherited, bg::Inherited});

        REQUIRE_EQUAL(scopedStyle.length(), BlockCount{3U});
        REQUIRE_EQUAL(scopedStyle[BlockIndex{0U}].color(), Color(fg::Red, bg::Inherited));
        REQUIRE_EQUAL(scopedStyle[BlockIndex{1U}].color(), Color{});
        REQUIRE_EQUAL(scopedStyle[BlockIndex{2U}].color(), Color(fg::Green, bg::Blue));

        const auto words = BlockStringEditor{"  alpha\tbeta\n\ngamma  "_el}.splitWords();
        REQUIRE_EQUAL(renderWords(words), std::vector<std::string>({"alpha", "beta", "gamma"}));
    }

private:
    [[nodiscard]] static auto render(const BlockStringEditor &text) -> std::string {
        auto result = std::string{};
        for (const auto &character : text) {
            result += blockToStdString(character);
        }
        return result;
    }

    [[nodiscard]] static auto renderWords(const std::vector<BlockStringEditor> &words) -> std::vector<std::string> {
        auto result = std::vector<std::string>{};
        result.reserve(words.size());
        for (const auto &word : words) {
            result.push_back(render(word));
        }
        return result;
    }
};
