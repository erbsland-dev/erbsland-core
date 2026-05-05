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
        auto view = WriteClippedBufferRef{buffer, bgeo::BlockPosition{10, 20}, bgeo::BlockRectangle{2, 1, 3, 2}};

        REQUIRE_EQUAL(view.size(), (bgeo::BlockSize{3, 2}));
        REQUIRE_EQUAL(view.rect(), (bgeo::BlockRectangle{10, 20, 3, 2}));
        REQUIRE_EQUAL(view.sourceRect(), (bgeo::BlockRectangle{10, 20, 3, 2}));
        REQUIRE_EQUAL(view.targetRect(), (bgeo::BlockRectangle{2, 1, 3, 2}));

        view.fill(bgeo::BlockRectangle{11, 20, 4, 2}, Block{U'+'});
        view.set(bgeo::BlockPosition{10, 20}, Block{U'A'});
        view.set(bgeo::BlockPosition{12, 21}, Block{U'B'});
        view.set(bgeo::BlockPosition{9, 20}, Block{U'X'});
        view.set(bgeo::BlockPosition{13, 21}, Block{U'Y'});
        view.set(bgeo::BlockPosition{12, 20}, Block{U'界'});
        view.set(bgeo::BlockPosition{9, 21}, BlockString{"ZCD"_el});

        requireRowsEqual(
            buffer,
            {
                "......",
                "..A++.",
                "..CDB.",
                "......",
            });
        REQUIRE_EQUAL(view.get(bgeo::BlockPosition{10, 20}), U'A');
        REQUIRE_EQUAL(view.get(bgeo::BlockPosition{12, 20}), U'+');
        REQUIRE_EQUAL(view.get(bgeo::BlockPosition{13, 20}), U'.');
    }

    void testSharedWrapperHandlesNullContentAndReplacement() {
        auto view = WriteClippedBuffer{bgeo::BlockSize{2, 1}};

        REQUIRE(view.content() == nullptr);
        REQUIRE_EQUAL(view.get(bgeo::BlockPosition{0, 0}), U' ');
        view.set(bgeo::BlockPosition{0, 0}, Block{U'X'});

        auto firstBuffer = createSharedBuffer({".."});
        view.setContent(firstBuffer);
        view.set(bgeo::BlockPosition{0, 0}, Block{U'A'});

        REQUIRE(view.content() == firstBuffer);
        requireRowsEqual(*firstBuffer, {"A."});

        auto secondBuffer = createSharedBuffer({".."});
        view.setContent(secondBuffer);
        view.set(bgeo::BlockPosition{1, 0}, Block{U'B'});

        requireRowsEqual(*firstBuffer, {"A."});
        requireRowsEqual(*secondBuffer, {".B"});
    }

    void testResizeOnlyChangesTheWrapperAndCloneIsZeroBased() {
        auto buffer = createBuffer({
            "ABCDE",
            "FGHIJ",
            "KLMNO",
        });
        auto view = WriteClippedBufferRef{buffer, bgeo::BlockPosition{5, 7}, bgeo::BlockRectangle{1, 1, 3, 2}};

        const auto clone = view.clone();
        requireRowsEqual(
            *clone,
            {
                "GHI",
                "LMN",
            });

        view.resize(bgeo::BlockSize{2, 1}, BufferResizeMode::PreserveContent, Block{U'?'});

        REQUIRE_EQUAL(buffer.size(), (bgeo::BlockSize{5, 3}));
        REQUIRE_EQUAL(view.size(), (bgeo::BlockSize{2, 1}));
        REQUIRE_EQUAL(view.rect(), (bgeo::BlockRectangle{5, 7, 2, 1}));
        REQUIRE_EQUAL(view.targetRect(), (bgeo::BlockRectangle{1, 1, 2, 1}));
    }

    void testInheritedDrawingPathsStayInsideTheTargetRectangle() {
        auto buffer = createBuffer({
            ".......",
            ".......",
            ".......",
            ".......",
            ".......",
        });
        auto view = WriteClippedBufferRef{buffer, bgeo::BlockPosition{100, 200}, bgeo::BlockRectangle{1, 1, 5, 3}};

        view.drawFrame(bgeo::BlockRectangle{100, 200, 5, 3}, Block{U'#'});
        auto source = createBuffer({"abcdef"});
        view.drawBuffer(
            source, BufferDrawOptions{bgeo::BlockRectangle{98, 201, 6, 1}, bgeo::BlockRectangle{0, 0, 6, 1}});
        view.drawBlockText(bgeo::BlockPosition{99, 201}, BlockString{"WXY"_el});

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
        auto view = WriteClippedBufferRef{buffer, bgeo::BlockPosition{0, 0}, bgeo::BlockRectangle{2, 0, 4, 2}};

        view.fill(Block{U'*'});

        requireRowsEqual(
            buffer,
            {
                "..**",
                "..**",
            });
        REQUIRE_EQUAL(view.get(bgeo::BlockPosition{0, 0}), U'*');
        REQUIRE_EQUAL(view.get(bgeo::BlockPosition{1, 0}), U'*');
        REQUIRE_EQUAL(view.get(bgeo::BlockPosition{2, 0}), U' ');
    }
};
