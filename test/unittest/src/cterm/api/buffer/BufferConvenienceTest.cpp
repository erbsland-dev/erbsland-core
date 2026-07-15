// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/BufferTestHelper.hpp"

#include <erbsland/unittest/UnitTest.hpp>

TESTED_TARGETS(Buffer)
class BufferConvenienceTest final : public UNITTEST_SUBCLASS(BufferTestHelper) {
public:
    void testFillWritesTheWholeBuffer() {
        auto buffer = Buffer{bgeo::BlockSize{3, 2}};

        buffer.fill(Block{U'X', fg::Green, bg::Black});

        requireRowsEqual(buffer, {"XXX", "XXX"});
        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{1, 1}).color(), Color(fg::Green, bg::Black));
    }

    void testDrawFilledFrameWithTile9StyleUsesTheFillCharacterInside() {
        auto buffer = Buffer{bgeo::BlockSize{5, 3}};

        buffer.drawFilledFrame(bgeo::BlockRectangle{0, 0, 5, 3}, Tile9Style::create("ABCDEFGHI"_el), Block{U'.'});

        requireRowsEqual(buffer, {"ABBBC", "D...F", "GHHHI"});
    }

    void testDrawTextAtPositionUsesTheExistingBufferColorAsBaseColor() {
        auto buffer = Buffer{bgeo::BlockSize{4, 2}};
        buffer.fill(Block{U' ', fg::White, bg::Blue});
        auto text = BlockString{"A\nB"_el};

        buffer.drawBlockText(bgeo::BlockPosition{1, 0}, text);

        requireRowsEqual(buffer, {" A  ", " B  "});
        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{1, 0}).color(), Color(fg::White, bg::Blue));
        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{1, 1}).color(), Color(fg::White, bg::Blue));
    }

    void testDrawBufferAtPositionUsesExistingTargetColorAsBaseColor() {
        auto source = Buffer{bgeo::BlockSize{2, 1}};
        source.set(bgeo::BlockPosition{0, 0}, Block{U'A', fg::Red});
        source.set(bgeo::BlockPosition{1, 0}, Block{U'B', fg::Green});
        auto buffer = Buffer{bgeo::BlockSize{4, 1}};
        buffer.fill(Block{U' ', fg::White, bg::Blue});

        buffer.drawBuffer(source, bgeo::BlockPosition{1, 0});

        requireRowsEqual(buffer, {" AB "});
        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{1, 0}).color(), Color(fg::Red, bg::Blue));
        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{2, 0}).color(), Color(fg::Green, bg::Blue));
    }

    void testDrawBufferRectangleUsesAlignmentWhenCroppingTheSource() {
        auto source = createBuffer({"ABCD"});
        auto buffer = Buffer{bgeo::BlockSize{2, 1}};

        buffer.drawBuffer(source, bgeo::BlockRectangle{0, 0, 2, 1}, bgeo::Alignment::Right);

        requireRowsEqual(buffer, {"CD"});
    }

    void testDrawBufferOptionsCanOverwriteColorsAndCropTheSourceRectangle() {
        auto source = createBuffer({"ABCD"});
        source.set(bgeo::BlockPosition{1, 0}, Block{U'B', fg::Red});
        source.set(bgeo::BlockPosition{2, 0}, Block{U'C', fg::Green});
        auto buffer = Buffer{bgeo::BlockSize{2, 1}};
        buffer.fill(Block{U' ', fg::White, bg::Blue});
        auto options = BufferDrawOptions{};
        options.setSourceRect(bgeo::BlockRectangle{1, 0, 2, 1});
        options.setOverwriteColors(true);

        buffer.drawBuffer(source, options);

        requireRowsEqual(buffer, {"BC"});
        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{0, 0}).color(), Color(fg::Red, bg::Inherited));
        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{1, 0}).color(), Color(fg::Green, bg::Inherited));
    }

    void testDrawBufferRejectsDrawingOntoTheSameBuffer() {
        auto buffer = createBuffer({"ABC "});

        REQUIRE_THROWS_AS(erbsland::err::ParameterError, buffer.drawBuffer(buffer, bgeo::BlockPosition{1, 0}));
    }

    void testDrawTextStringViewReplacesInvalidUtf8() {
        auto buffer = Buffer{bgeo::BlockSize{3, 1}};
        const auto text = bytes({0x41, 0xC3, 0x42});

        buffer.drawBlockText(erbsland::text::String{std::string_view{text}}, bgeo::BlockRectangle{0, 0, 3, 1});

        requireRowsEqual(buffer, {"A�B"});
    }

    void testDrawTextConvenienceOverloadUsesTheSameColorResolutionAsTextOptions() {
        auto text = BlockString{};
        text.append(Block{U'A', fg::Red, bg::Inherited});
        text.append(Block{U'B', fg::Inherited, bg::Blue});
        auto buffer = Buffer{bgeo::BlockSize{2, 1}};

        buffer.drawBlockText(
            text, bgeo::BlockRectangle{0, 0, 2, 1}, bgeo::Alignment::TopLeft, Color{fg::Green, bg::Yellow});

        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{0, 0}).color(), Color(fg::Red, bg::Yellow));
        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{1, 0}).color(), Color(fg::Green, bg::Blue));
    }

    void testDrawTextUtf32ConvenienceOverloadPreservesWideCharacters() {
        auto buffer = Buffer{bgeo::BlockSize{3, 1}};

        buffer.drawBlockText(U"界A"_el, bgeo::BlockRectangle{0, 0, 3, 1});

        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{0, 0}), U'界');
        REQUIRE(buffer.get(bgeo::BlockPosition{1, 0}).isEmpty());
        REQUIRE_EQUAL(buffer.get(bgeo::BlockPosition{2, 0}), U'A');
    }

    void testTextHeightForWidthUsesWrappedTextMeasurement() {
        auto options = BlockTextOptions{};
        options.setMargins(bgeo::BlockMargins{0, 0, 1, 0});

        REQUIRE_EQUAL(
            WritableBuffer::blockTextHeightForWidth(BlockString{"alpha beta gamma"_el}, blockCoordinate(10), options),
            3);
    }
};
