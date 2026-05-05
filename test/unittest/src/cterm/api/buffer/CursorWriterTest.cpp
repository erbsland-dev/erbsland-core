// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CursorWriterTestProbe.hpp"

#include "../../support/BlockStringTestHelper.hpp"

#include <erbsland/unittest/UnitTest.hpp>

#include <optional>

TESTED_TARGETS(CursorWriter)
class CursorWriterTest final : public UNITTEST_SUBCLASS(BlockStringTestHelper) {
public:
    void testStyleAndDefaultColorWrappersUpdateTheTrackedState() {
        auto writer = CursorWriterProbe{};
        auto attributes = BlockAttributes{};
        attributes.setUnderline(true);

        writer.setStyle(BlockStyle{Color{fg::Red, bg::Blue}, attributes});

        REQUIRE_EQUAL(writer.style().color(), Color(fg::Red, bg::Blue));
        REQUIRE(writer.style().attributes().isUnderline());

        writer.setDefaultColor();

        REQUIRE_EQUAL(writer.color(), Color::reset());
    }

    void testAttributeConvenienceWrappersPreserveExistingState() {
        auto writer = CursorWriterProbe{};

        writer.setBold(true);
        writer.setUnderline(true);
        writer.setBold(false);
        writer.setStrikethrough(true);

        REQUIRE(writer.blockAttributes().isBoldSpecified());
        REQUIRE_FALSE(writer.blockAttributes().isBold());
        REQUIRE(writer.blockAttributes().isUnderline());
        REQUIRE(writer.blockAttributes().isStrikethrough());
    }

    void testCursorMovementWrappersTranslateToMoveCursorCalls() {
        auto writer = CursorWriterProbe{};

        writer.moveLeft(blockCoordinate(2));
        REQUIRE_EQUAL(writer._lastMove, (bgeo::BlockPosition{-2, 0}));
        REQUIRE_EQUAL(writer._lastMoveMode, MoveMode::Relative);

        writer.moveRight(blockCoordinate(3));
        REQUIRE_EQUAL(writer._lastMove, (bgeo::BlockPosition{3, 0}));
        REQUIRE_EQUAL(writer._lastMoveMode, MoveMode::Relative);

        writer.moveUp(blockCoordinate(4));
        REQUIRE_EQUAL(writer._lastMove, (bgeo::BlockPosition{0, -4}));
        REQUIRE_EQUAL(writer._lastMoveMode, MoveMode::Relative);

        writer.moveDown(blockCoordinate(5));
        REQUIRE_EQUAL(writer._lastMove, (bgeo::BlockPosition{0, 5}));
        REQUIRE_EQUAL(writer._lastMoveMode, MoveMode::Relative);

        writer.moveTo(bgeo::BlockPosition{6, 7});
        REQUIRE_EQUAL(writer._lastMove, (bgeo::BlockPosition{6, 7}));
        REQUIRE_EQUAL(writer._lastMoveMode, MoveMode::Absolute);

        writer.moveHome();
        REQUIRE_EQUAL(writer._lastMove, (bgeo::BlockPosition{0, 0}));
        REQUIRE_EQUAL(writer._lastMoveMode, MoveMode::Absolute);
    }

    void testPrintAndParagraphWrappersDispatchSupportedArgumentTypes() {
        auto writer = CursorWriterProbe{};
        auto attributes = BlockAttributes{};
        attributes.setItalic(true);

        writer.print(
            Color{fg::Green, bg::Black},
            Foreground{fg::Yellow},
            Background{bg::Blue},
            BlockStyle{Color{fg::Inherited, bg::Magenta}, attributes},
            BlockAttributes{}.withFlag(BlockAttributes::Underline, true),
            Block{U'X'},
            BlockString{"Y"_el},
            BlockStringView{BlockString{"QR"_el}}.slice(BlockRange{BlockIndex{0U}, BlockCount{1U}}),
            "Z"_els,
            U"Ω"_el,
            "!"_el,
            "?"_el);
        const auto paragraphSource = BlockString{"xAA!"_el};
        const auto lineCount = writer.printParagraph(
            BlockStringView{paragraphSource}.slice(BlockRange{BlockIndex{1U}, BlockCount{2U}}),
            ParagraphOptions{bgeo::Alignment::Right});
        writer.printLine("tail"_el);

        REQUIRE_EQUAL(writer.color(), Color(fg::Yellow, bg::Magenta));
        REQUIRE(writer.blockAttributes().isItalic());
        REQUIRE(writer.blockAttributes().isUnderline());
        REQUIRE(writer._writtenChars.empty());
        REQUIRE(writer._writtenStrings.empty());
        REQUIRE(writer._writtenResolvedChars.empty());
        REQUIRE_EQUAL(writer._writtenResolvedStrings.size(), std::size_t{2});
        requireStringEqual(writer._writtenResolvedStrings[0], U"XYQZΩ!?");
        requireStringEqual(writer._writtenResolvedStrings[1], U"tail");
        for (std::size_t index = 0; index < writer._writtenResolvedStrings[0].length().toSizeT(); ++index) {
            const auto character = writer._writtenResolvedStrings[0][BlockIndex::fromSizeT(index)];
            REQUIRE_EQUAL(character.color(), Color(fg::Yellow, bg::Magenta));
            REQUIRE(character.attributes().isItalic());
            REQUIRE(character.attributes().isUnderline());
        }
        REQUIRE_EQUAL(writer._lineBreakCount, 1);
        REQUIRE_EQUAL(lineCount, 7);
        requireStringEqual(writer._lastParagraph, U"AA");
        REQUIRE_EQUAL(writer._lastParagraphAlignment, bgeo::Alignment::Right);
    }

    void testRepeatedWriteWrappersEmitTheRequestedCharacterCount() {
        auto writer = CursorWriterProbe{};

        writer.writeRepeated(Block{U'A'}, 3);
        writer.writeRepeatedResolved(Block{U'B', Color{fg::Cyan, bg::Black}}, 2);
        writer.writeRepeated(Block{U'C'}, 0);
        writer.writeRepeatedResolved(Block{U'D'}, -1);

        REQUIRE_EQUAL(writer._writtenChars.size(), std::size_t{3});
        REQUIRE_EQUAL(writer._writtenChars[0], U'A');
        REQUIRE_EQUAL(writer._writtenChars[1], U'A');
        REQUIRE_EQUAL(writer._writtenChars[2], U'A');
        REQUIRE_EQUAL(writer._writtenResolvedChars.size(), std::size_t{2});
        REQUIRE_EQUAL(writer._writtenResolvedChars[0], U'B');
        REQUIRE_EQUAL(writer._writtenResolvedChars[1], U'B');
    }
};
