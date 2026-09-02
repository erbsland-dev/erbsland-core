// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/BufferTestHelper.hpp"

#include <erbsland/cterm/WriteClippedBuffer.hpp>
#include <erbsland/unittest/UnitTest.hpp>

TESTED_TARGETS(WriteClippedBuffer)
class WriteClippedBufferTest final : public UNITTEST_SUBCLASS(BufferTestHelper) {
public:
    void testReferenceWrapperClipsAndOffsetsReadsAndWrites() {
        auto buffer = createBuffer({
            "......",
            "......",
            "......",
            "......",
        });
        auto view = WriteClippedBufferRef{buffer, block::Position{10, 20}, block::Rectangle{2, 1, 3, 2}};

        REQUIRE_EQUAL(view.size(), (block::Size{3, 2}));
        REQUIRE_EQUAL(view.rect(), (block::Rectangle{10, 20, 3, 2}));
        REQUIRE_EQUAL(view.sourceRect(), (block::Rectangle{10, 20, 3, 2}));
        REQUIRE_EQUAL(view.targetRect(), (block::Rectangle{2, 1, 3, 2}));

        view.fill(block::Rectangle{11, 20, 4, 2}, Block{U'+'});
        view.set(block::Position{10, 20}, Block{U'A'});
        view.set(block::Position{12, 21}, Block{U'B'});
        view.set(block::Position{9, 20}, Block{U'X'});
        view.set(block::Position{13, 21}, Block{U'Y'});
        view.set(block::Position{12, 20}, Block{U'界'});
        view.set(block::Position{9, 21}, BlockStringEditor{"ZCD"_el});

        requireRowsEqual(
            buffer,
            {
                "......",
                "..A++.",
                "..CDB.",
                "......",
            });
        REQUIRE_EQUAL(view.get(block::Position{10, 20}), U'A');
        REQUIRE_EQUAL(view.get(block::Position{12, 20}), U'+');
        REQUIRE_EQUAL(view.get(block::Position{13, 20}), U'.');
    }

    void testSharedWrapperHandlesNullContentAndReplacement() {
        auto view = WriteClippedBuffer{block::Size{2, 1}};

        REQUIRE_EQUAL(view.content(), nullptr);
        REQUIRE_EQUAL(view.get(block::Position{0, 0}), U' ');
        view.set(block::Position{0, 0}, Block{U'X'});

        auto firstBuffer = createSharedBuffer({".."});
        view.setContent(firstBuffer);
        view.set(block::Position{0, 0}, Block{U'A'});

        REQUIRE_EQUAL(view.content(), firstBuffer);
        requireRowsEqual(*firstBuffer, {"A."});

        auto secondBuffer = createSharedBuffer({".."});
        view.setContent(secondBuffer);
        view.set(block::Position{1, 0}, Block{U'B'});

        requireRowsEqual(*firstBuffer, {"A."});
        requireRowsEqual(*secondBuffer, {".B"});
    }

    void testResizeOnlyChangesTheWrapperAndCloneIsZeroBased() {
        auto buffer = createBuffer({
            "ABCDE",
            "FGHIJ",
            "KLMNO",
        });
        auto view = WriteClippedBufferRef{buffer, block::Position{5, 7}, block::Rectangle{1, 1, 3, 2}};

        const auto clone = view.clone();
        requireRowsEqual(
            *clone,
            {
                "GHI",
                "LMN",
            });

        view.resize(block::Size{2, 1}, BufferResizeMode::PreserveContent, Block{U'?'});

        REQUIRE_EQUAL(buffer.size(), (block::Size{5, 3}));
        REQUIRE_EQUAL(view.size(), (block::Size{2, 1}));
        REQUIRE_EQUAL(view.rect(), (block::Rectangle{5, 7, 2, 1}));
        REQUIRE_EQUAL(view.targetRect(), (block::Rectangle{1, 1, 2, 1}));
    }

    void testInheritedDrawingPathsStayInsideTheTargetRectangle() {
        auto buffer = createBuffer({
            ".......",
            ".......",
            ".......",
            ".......",
            ".......",
        });
        auto view = WriteClippedBufferRef{buffer, block::Position{100, 200}, block::Rectangle{1, 1, 5, 3}};

        view.drawFrame(block::Rectangle{100, 200, 5, 3}, Block{U'#'});
        auto source = createBuffer({"abcdef"});
        view.drawBuffer(source, BufferDrawOptions{block::Rectangle{98, 201, 6, 1}, block::Rectangle{0, 0, 6, 1}});
        view.drawBlockText(block::Position{99, 201}, BlockStringEditor{"WXY"_el});

        requireRowsEqual(
            buffer,
            {
                ".......",
                ".#####.",
                ".XYef#.",
                ".#####.",
                ".......",
            });
    }

    void testTargetRectanglePartiallyOutsideWrappedBufferIsClipped() {
        auto buffer = createBuffer({
            "....",
            "....",
        });
        auto view = WriteClippedBufferRef{buffer, block::Position{0, 0}, block::Rectangle{2, 0, 4, 2}};

        view.fill(Block{U'*'});

        requireRowsEqual(
            buffer,
            {
                "..**",
                "..**",
            });
        REQUIRE_EQUAL(view.get(block::Position{0, 0}), U'*');
        REQUIRE_EQUAL(view.get(block::Position{1, 0}), U'*');
        REQUIRE_EQUAL(view.get(block::Position{2, 0}), U' ');
    }
};
