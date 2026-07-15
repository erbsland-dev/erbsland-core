// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/TestHelper.hpp"

#include <erbsland/cterm/BlockCombinationStyle.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <stdexcept>

TESTED_TARGETS(BlockCombinationStyle)
class BlockCombinationStyleTest final : public el::UnitTest {
public:
    void testMatrixCombinationStyleUsesIndexedLookup() {
        static constexpr auto cResultMatrix = std::array<uint8_t, 9>{{
            0x00U,
            0x01U,
            0x02U,
            0x01U,
            0x01U,
            0x02U,
            0x02U,
            0x01U,
            0x02U,
        }};
        const auto style = MatrixBlockCombinationStyle{
            U" xy"_el,
            cResultMatrix,
        };

        REQUIRE_EQUAL(style.combine(Block{U' '}, Block{U'x'}), U'x');
        REQUIRE_EQUAL(style.combine(Block{U'x'}, Block{U'y'}), U'y');
        REQUIRE_EQUAL(style.combine(Block{U'?'}, Block{U'y'}), U'y');
    }

    void testOverwriteAndColorOverlayStylesUseExpectedColorRules() {
        const auto current = Block{U'A', fg::Green, bg::Blue};
        const auto overlay = Block{U'B', fg::Inherited, bg::Yellow};

        const auto overwritten = BlockCombinationStyle::overwrite()->combine(current, overlay);
        REQUIRE_EQUAL(overwritten, U'B');
        REQUIRE_EQUAL(overwritten.color(), Color(fg::Inherited, bg::Yellow));

        const auto colorOverlaid = BlockCombinationStyle::colorOverlay()->combine(current, overlay);
        REQUIRE_EQUAL(colorOverlaid, U'B');
        REQUIRE_EQUAL(colorOverlaid.color(), Color(fg::Green, bg::Yellow));
    }

    void testSimpleCombinationStyleUsesConfiguredMapAndColorOverlay() {
        auto style = SimpleBlockCombinationStyle{};
        style.add("a"_el, "b"_el, "c"_el);

        auto attributes = BlockAttributes{};
        attributes.setUnderline(true);
        const auto result = style.combine(
            Block{U'a', fg::Green, bg::Blue}, Block{U'b', Color{fg::BrightWhite, bg::Inherited}, attributes});

        REQUIRE_EQUAL(result, U'c');
        REQUIRE_EQUAL(result.color(), Color(fg::BrightWhite, bg::Blue));
        REQUIRE(result.attributes().isUnderline());
    }

    void testMatrixCombinationStyleRejectsInvalidDefinitions() {
        static constexpr auto cTooSmallResultMatrix = std::array<uint8_t, 2>{{0x00U, 0x01U}};
        REQUIRE_THROWS_AS(erbsland::err::ParameterError, MatrixBlockCombinationStyle(U"ab"_el, cTooSmallResultMatrix));
        REQUIRE_THROWS_AS(
            erbsland::err::ParameterError,
            MatrixBlockCombinationStyle(
                erbsland::text::U32String::fromCharacter(U'a', erbsland::unit::CpLength{256U}), {}));
    }

    void testCommonBoxFrameCombinesExactMatches() {
        const auto style = BlockCombinationStyle::commonBoxFrame();

        REQUIRE_EQUAL(style->combine(Block{U'┌'}, Block{U'┛'}), U'╃');
        REQUIRE_EQUAL(style->combine(Block{U'┛'}, Block{U'┌'}), U'╃');
        REQUIRE_EQUAL(style->combine(Block{U'╴'}, Block{U'╶'}), U'─');
        REQUIRE_EQUAL(style->combine(Block{U'│'}, Block{U'═'}), U'╪');
        REQUIRE_EQUAL(style->combine(Block{U'║'}, Block{U'─'}), U'╫');
        REQUIRE_EQUAL(style->combine(Block{U'╱'}, Block{U'╲'}), U'╳');
    }

    void testCommonBoxFrameFallsBackToOverlayForUnsupportedBlend() {
        const auto style = BlockCombinationStyle::commonBoxFrame();

        REQUIRE_EQUAL(style->combine(Block{U'╱'}, Block{U'┌'}), U'┌');
    }

    void testCommonBoxFrameOverwritesCenterOnlyCharactersWithLines() {
        const auto style = BlockCombinationStyle::commonBoxFrame();

        REQUIRE_EQUAL(style->combine(Block{U'▫'}, Block{U'┌'}), U'┌');
        REQUIRE_EQUAL(style->combine(Block{U'┌'}, Block{U'▫'}), U'┌');
        REQUIRE_EQUAL(style->combine(Block{U'▪'}, Block{U'╱'}), U'╱');
    }

    void testCommonBoxFrameKeepsSpecialBoxDrawingRules() {
        const auto style = BlockCombinationStyle::commonBoxFrame();

        REQUIRE_EQUAL(style->combine(Block{U'╭'}, Block{U' '}), U'╭');
        REQUIRE_EQUAL(style->combine(Block{U'╭'}, Block{U'─'}), U'┬');
        REQUIRE_EQUAL(style->combine(Block{U'╭'}, Block{U'│'}), U'├');
        REQUIRE_EQUAL(style->combine(Block{U'┄'}, Block{U'│'}), U'┼');
        REQUIRE_EQUAL(style->combine(Block{U'┉'}, Block{U'┃'}), U'╂');
        REQUIRE_EQUAL(style->combine(Block{U'╱'}, Block{U' '}), U'╱');
        REQUIRE_EQUAL(style->combine(Block{U' '}, Block{U'╲'}), U'╲');
        REQUIRE_EQUAL(style->combine(Block{U'∙'}, Block{U'▫'}), U'▫');
        REQUIRE_EQUAL(style->combine(Block{U'▫'}, Block{U'▪'}), U'▪');
    }

    void testCommonBoxFrameOverlaysColors() {
        const auto style = BlockCombinationStyle::commonBoxFrame();

        const auto current = Block{U'│', fg::Green, bg::Blue};
        const auto overlay = Block{U'═', fg::BrightWhite, bg::Inherited};
        const auto result = style->combine(current, overlay);

        REQUIRE_EQUAL(result, U'╪');
        REQUIRE_EQUAL(result.color(), Color(fg::BrightWhite, bg::Blue));
    }

    void testCombinationStylesResolveCharacterAttributesLikeColors() {
        const auto style = SimpleBlockCombinationStyle{};
        auto currentAttributes = BlockAttributes{};
        currentAttributes.setBold(true);
        auto overlayAttributes = BlockAttributes{};
        overlayAttributes.setBold(false);
        overlayAttributes.setItalic(true);

        const auto result = style.combine(
            Block{U'a', Color{fg::Green, bg::Blue}, currentAttributes},
            Block{U'b', Color{fg::BrightWhite, bg::Inherited}, overlayAttributes});

        REQUIRE_FALSE(result.attributes().isBold());
        REQUIRE(result.attributes().isItalic());
    }
};
