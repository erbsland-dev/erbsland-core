// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "BitmapTestSupport.hpp"

#include <erbsland/unittest/UnitTest.hpp>

TESTED_TARGETS(Bitmap)
class BitmapTest final : public UNITTEST_SUBCLASS(BitmapTestSupport) {
public:
    void testPixelsCanBeSetAndRead() {
        auto bitmap = Bitmap{bgeo::BlockSize{4, 3}};
        bitmap.setPixel(bgeo::BlockPosition{1, 2}, true);

        REQUIRE(bitmap.pixel(bgeo::BlockPosition{1, 2}));
        REQUIRE_FALSE(bitmap.pixel(bgeo::BlockPosition{0, 0}));
        REQUIRE_FALSE(bitmap.pixel(bgeo::BlockPosition{99, 99}));
    }

    void testSetPixelIgnoresOutsideCoordinatesWithoutChangingTheBitmap() {
        auto bitmap = Bitmap::fromPattern({
            "#."_el,
            ".#"_el,
        });

        bitmap.setPixel(bgeo::BlockPosition{-1, 0}, true);
        bitmap.setPixel(bgeo::BlockPosition{2, 1}, true);
        bitmap.setPixel(bgeo::BlockPosition{0, 2}, false);

        requireRowsEqual(
            bitmap,
            {
                "#."_el,
                ".#"_el,
            });
    }

    void testRectMatchesBitmapBounds() {
        const auto bitmap = Bitmap{bgeo::BlockSize{4, 3}};

        REQUIRE_EQUAL(bitmap.rect().topLeft(), bgeo::BlockPosition(0, 0));
        REQUIRE_EQUAL(bitmap.rect().size(), bgeo::BlockSize(4, 3));
    }

    void testDataAccessorsExposeTheUnderlyingPixelStorage() {
        auto bitmap = Bitmap{bgeo::BlockSize{2, 2}};

        bitmap.data()[1] = true;
        bitmap.data()[2] = true;

        const auto &constData = static_cast<const Bitmap &>(bitmap).data();
        REQUIRE_EQUAL(constData.size(), static_cast<std::size_t>(4));
        REQUIRE(constData[1]);
        REQUIRE(constData[2]);
        REQUIRE(bitmap.pixel(bgeo::BlockPosition{1, 0}));
        REQUIRE(bitmap.pixel(bgeo::BlockPosition{0, 1}));
    }

    void testProtectedPixelRefAllowsMutationAndThrowsForOutOfBoundsAccess() {
        auto bitmap = BitmapAccessor{bgeo::BlockSize{2, 1}};

        bitmap.writePixelRef(bgeo::BlockPosition{1, 0}, true);

        REQUIRE(bitmap.pixel(bgeo::BlockPosition{1, 0}));
        REQUIRE(bitmap.readPixelRef(bgeo::BlockPosition{1, 0}));
        REQUIRE_THROWS_AS(std::out_of_range, bitmap.readPixelRef(bgeo::BlockPosition{2, 0}));
    }

    void testFlipHorizontalMirrorsContent() {
        auto bitmap = Bitmap{bgeo::BlockSize{4, 1}};
        bitmap.setPixel(bgeo::BlockPosition{0, 0}, true);
        bitmap.setPixel(bgeo::BlockPosition{1, 0}, true);

        bitmap.flipHorizontal();

        REQUIRE_FALSE(bitmap.pixel(bgeo::BlockPosition{0, 0}));
        REQUIRE_FALSE(bitmap.pixel(bgeo::BlockPosition{1, 0}));
        REQUIRE(bitmap.pixel(bgeo::BlockPosition{2, 0}));
        REQUIRE(bitmap.pixel(bgeo::BlockPosition{3, 0}));
    }

    void testInvertTogglesAllPixelsInPlace() {
        auto bitmap = Bitmap::fromPattern({
            "#."_el,
            " ."_el,
        });

        bitmap.invert();

        REQUIRE_FALSE(bitmap.pixel(bgeo::BlockPosition{0, 0}));
        REQUIRE(bitmap.pixel(bgeo::BlockPosition{1, 0}));
        REQUIRE(bitmap.pixel(bgeo::BlockPosition{0, 1}));
        REQUIRE(bitmap.pixel(bgeo::BlockPosition{1, 1}));

        bitmap.invert();

        REQUIRE(bitmap.pixel(bgeo::BlockPosition{0, 0}));
        REQUIRE_FALSE(bitmap.pixel(bgeo::BlockPosition{1, 0}));
        REQUIRE_FALSE(bitmap.pixel(bgeo::BlockPosition{0, 1}));
        REQUIRE_FALSE(bitmap.pixel(bgeo::BlockPosition{1, 1}));
    }

    void testPixelCountCountsSetAndClearedPixels() {
        const auto bitmap = Bitmap::fromPattern({
            "#.#"_el,
            ".#."_el,
        });

        REQUIRE_EQUAL(bitmap.pixelCount(), static_cast<std::size_t>(3));
        REQUIRE_EQUAL(bitmap.pixelCount(false), static_cast<std::size_t>(3));
    }

    void testInvertedReturnsAnInvertedCopyWithoutChangingTheOriginal() {
        const auto bitmap = Bitmap::fromPattern({
            "#.."_el,
            ".##"_el,
        });

        const auto inverted = bitmap.inverted();

        requireRowsEqual(
            bitmap,
            {
                "#.."_el,
                ".##"_el,
            });
        requireRowsEqual(
            inverted,
            {
                ".##"_el,
                "#.."_el,
            });
    }

    void testPixelQuadBuildsExpectedMask() {
        auto bitmap = Bitmap{bgeo::BlockSize{2, 2}};
        bitmap.setPixel(bgeo::BlockPosition{0, 0}, true);
        bitmap.setPixel(bgeo::BlockPosition{1, 1}, true);

        REQUIRE_EQUAL(bitmap.pixelQuad(bgeo::BlockPosition{0, 0}), static_cast<std::uint8_t>(0b1001U));
    }

    void testPixelCardinalBuildsExpectedMaskInClockwiseOrder() {
        const auto bitmap = Bitmap::fromPattern({
            ".#."_el,
            "..#"_el,
            ".#."_el,
        });

        REQUIRE_EQUAL(bitmap.pixelCardinal(bgeo::BlockPosition{1, 1}), static_cast<std::uint8_t>(0b1011U));
    }

    void testPixelCardinalIgnoresTheCenterPixelAndOutOfBoundsNeighbors() {
        auto bitmap = Bitmap{bgeo::BlockSize{2, 1}};
        bitmap.setPixel(bgeo::BlockPosition{0, 0}, true);
        bitmap.setPixel(bgeo::BlockPosition{1, 0}, true);

        REQUIRE_EQUAL(bitmap.pixelCardinal(bgeo::BlockPosition{0, 0}), static_cast<std::uint8_t>(0b0001U));
        REQUIRE_EQUAL(bitmap.pixelCardinal(bgeo::BlockPosition{-1, 0}), static_cast<std::uint8_t>(0b0001U));
    }

    void testPixelRingBuildsExpectedMaskInClockwiseOrder() {
        const auto bitmap = Bitmap::fromPattern({
            "###"_el,
            "#.#"_el,
            ".##"_el,
        });

        REQUIRE_EQUAL(bitmap.pixelRing(bgeo::BlockPosition{1, 1}), static_cast<std::uint8_t>(0b11110111U));
    }

    void testPixelRingIgnoresTheCenterPixelAndOutOfBoundsNeighbors() {
        auto bitmap = Bitmap{bgeo::BlockSize{2, 2}};
        bitmap.setPixel(bgeo::BlockPosition{0, 0}, true);
        bitmap.setPixel(bgeo::BlockPosition{1, 1}, true);

        REQUIRE_EQUAL(bitmap.pixelRing(bgeo::BlockPosition{0, 0}), static_cast<std::uint8_t>(0b00000010U));
        REQUIRE_EQUAL(bitmap.pixelRing(bgeo::BlockPosition{-1, -1}), static_cast<std::uint8_t>(0b00000010U));
    }

    void testBoundingRectReturnsTheCoveredAreaForSetPixels() {
        const auto bitmap = Bitmap::fromPattern({
            "......"_el,
            ".#..#."_el,
            "..##.."_el,
            "......"_el,
        });

        requireRectangleEqual(bitmap.boundingRect(), bgeo::BlockRectangle(1, 1, 4, 2));
    }

    void testBoundingRectSupportsSinglePixelAndSingleRowSpans() {
        auto singlePixel = Bitmap{bgeo::BlockSize{5, 4}};
        singlePixel.setPixel(bgeo::BlockPosition{3, 2}, true);
        requireRectangleEqual(singlePixel.boundingRect(), bgeo::BlockRectangle(3, 2, 1, 1));

        auto singleRow = Bitmap{bgeo::BlockSize{6, 4}};
        singleRow.fillRect(bgeo::BlockRectangle{1, 3, 4, 1}, true);
        requireRectangleEqual(singleRow.boundingRect(), bgeo::BlockRectangle(1, 3, 4, 1));
    }

    void testBoundingRectReturnsEmptyRectangleWhenTheRequestedValueDoesNotExist() {
        requireRectangleEqual(Bitmap{bgeo::BlockSize{3, 2}}.boundingRect(), bgeo::BlockRectangle{});

        auto filled = Bitmap{bgeo::BlockSize{3, 2}};
        filled.fillRect(filled.rect(), true);
        requireRectangleEqual(filled.boundingRect(false), bgeo::BlockRectangle{});
    }

    void testBoundingRectCanLocateClearedPixelsInsideAFilledBitmap() {
        auto bitmap = Bitmap{bgeo::BlockSize{5, 4}};
        bitmap.fillRect(bitmap.rect(), true);
        bitmap.fillRect(bgeo::BlockRectangle{1, 1, 3, 2}, false);

        requireRectangleEqual(bitmap.boundingRect(false), bgeo::BlockRectangle(1, 1, 3, 2));
    }

    void testFromFunctionCreatesPixelsFromTheGenerator() {
        const auto bitmap = Bitmap::fromFunction(
            bgeo::BlockSize{4, 3}, [](const bgeo::BlockPosition pos) -> bool { return (pos.x() + pos.y()) % 2 == 0; });

        requireRowsEqual(
            bitmap,
            {
                "#.#."_el,
                ".#.#"_el,
                "#.#."_el,
            });
    }

    void testFromFunctionDoesNotInvokeTheGeneratorForAnEmptyBitmap() {
        auto calls = 0;

        const auto bitmap = Bitmap::fromFunction(bgeo::BlockSize{0, 0}, [&](const bgeo::BlockPosition) -> bool {
            ++calls;
            return true;
        });

        REQUIRE_EQUAL(bitmap.size(), bgeo::BlockSize(0, 0));
        REQUIRE_EQUAL(calls, 0);
    }

    void testOutlinedCreatesAnEightConnectedBorderAroundFilledPixels() {
        const auto bitmap = Bitmap::fromPattern({
            "....."_el,
            "....."_el,
            "..#.."_el,
            "....."_el,
            "....."_el,
        });

        const auto outlined = bitmap.outlined();

        requireRowsEqual(
            outlined,
            {
                "....."_el,
                ".###."_el,
                ".#.#."_el,
                ".###."_el,
                "....."_el,
            });
    }

    void testOutlinedReturnsEmptyForAnEmptyBitmapAndMarksInteriorHoles() {
        requireRowsEqual(
            Bitmap{bgeo::BlockSize{3, 3}}.outlined(),
            {
                "..."_el,
                "..."_el,
                "..."_el,
            });

        const auto bitmapWithHole = Bitmap::fromPattern({
            "###"_el,
            "#.#"_el,
            "###"_el,
        });

        requireRowsEqual(
            bitmapWithHole.outlined(),
            {
                "..."_el,
                ".#."_el,
                "..."_el,
            });
    }

    void testOutlinedMarksAllEmptyNeighborsOfAComplexShape() {
        const auto bitmap = Bitmap::fromPattern({
            "....."_el,
            ".##.."_el,
            "..#.."_el,
            "....."_el,
        });

        requireRowsEqual(
            bitmap.outlined(),
            {
                "####."_el,
                "#..#."_el,
                "##.#."_el,
                ".###."_el,
            });
    }

    void testExpandedAddsClearedMarginsAroundTheBitmap() {
        const auto bitmap = Bitmap::fromPattern({
            "#."_el,
            ".#"_el,
        });

        const auto expanded = bitmap.expanded(bgeo::BlockMargins{1, 2, 1, 3}, false);

        requireRowsEqual(
            expanded,
            {
                "......."_el,
                "...#..."_el,
                "....#.."_el,
                "......."_el,
            });
        requireRowsEqual(
            bitmap,
            {
                "#."_el,
                ".#"_el,
            });
    }

    void testExpandedCanFillTheAddedMarginArea() {
        const auto bitmap = Bitmap::fromPattern({
            "#."_el,
            "##"_el,
        });

        const auto expanded = bitmap.expanded(bgeo::BlockMargins{1, 1, 0, 2}, true);

        requireRowsEqual(
            expanded,
            {
                "#####"_el,
                "###.#"_el,
                "#####"_el,
            });
    }

    void testExpandedWithNegativeMarginsCropsTheBitmap() {
        const auto bitmap = Bitmap::fromPattern({
            "....."_el,
            ".##.."_el,
            "..#.."_el,
            ".##.."_el,
        });

        const auto expanded = bitmap.expanded(bgeo::BlockMargins{-1, -2, 0, -1}, false);

        requireRowsEqual(
            expanded,
            {
                "##"_el,
                ".#"_el,
                "##"_el,
            });
    }

    void testExpandedReturnsAnEmptyBitmapIfMarginsRemoveAWholeDimension() {
        const auto bitmap = Bitmap::fromPattern({
            "##"_el,
            "##"_el,
        });

        const auto emptyWidth = bitmap.expanded(bgeo::BlockMargins{0, -2, 0, 0}, false);
        const auto emptyHeight = bitmap.expanded(bgeo::BlockMargins{-2, 0, 0, 0}, false);

        REQUIRE_EQUAL(emptyWidth.size(), bgeo::BlockSize(0, 0));
        REQUIRE_EQUAL(emptyHeight.size(), bgeo::BlockSize(0, 0));
    }

    void testDrawCopiesOtherBitmap() {
        auto source = Bitmap{bgeo::BlockSize{2, 2}};
        source.setPixel(bgeo::BlockPosition{0, 1}, true);
        auto destination = Bitmap{bgeo::BlockSize{4, 4}};

        destination.draw(bgeo::BlockPosition{1, 1}, source);

        REQUIRE(destination.pixel(bgeo::BlockPosition{1, 2}));
        REQUIRE_FALSE(destination.pixel(bgeo::BlockPosition{1, 1}));
    }

    void testDrawFromUnsignedBitRowsUsesLeastSignificantBitsFromLeftToRight() {
        auto bitmap = Bitmap{bgeo::BlockSize{4, 2}};

        bitmap.draw(bgeo::BlockPosition{0, 0}, std::vector<std::uint8_t>{0b0101U, 0b1010U});

        requireRowsEqual(
            bitmap,
            {
                "#.#."_el,
                ".#.#"_el,
            });
    }

    void testFillRectFillsTheWholeInteriorAndCanClearPixels() {
        auto bitmap = Bitmap{bgeo::BlockSize{5, 5}};

        bitmap.fillRect(bgeo::BlockRectangle{1, 1, 3, 3}, true);
        requireRowsEqual(
            bitmap,
            {
                "....."_el,
                ".###."_el,
                ".###."_el,
                ".###."_el,
                "....."_el,
            });

        bitmap.fillRect(bgeo::BlockRectangle{2, 2, 1, 1}, false);
        requireRowsEqual(
            bitmap,
            {
                "....."_el,
                ".###."_el,
                ".#.#."_el,
                ".###."_el,
                "....."_el,
            });
    }

    void testFillRectClipsToTheBitmapBounds() {
        auto bitmap = Bitmap{bgeo::BlockSize{4, 3}};

        bitmap.fillRect(bgeo::BlockRectangle{-1, 1, 3, 3}, true);

        requireRowsEqual(
            bitmap,
            {
                "...."_el,
                "##.."_el,
                "##.."_el,
            });
    }

    void testFloodFillOnlyChangesTheConnectedRegionWithTheOriginalValue() {
        auto bitmap = Bitmap::fromPattern({
            "##.."_el,
            "##.."_el,
            "..##"_el,
            "..##"_el,
        });

        bitmap.floodFill(bgeo::BlockPosition{0, 0}, false);

        requireRowsEqual(
            bitmap,
            {
                "...."_el,
                "...."_el,
                "..##"_el,
                "..##"_el,
            });
    }

    void testFloodFillIgnoresOutsideStartPositionsAndMatchingValues() {
        auto bitmap = Bitmap::fromPattern({
            "#."_el,
            ".#"_el,
        });

        bitmap.floodFill(bgeo::BlockPosition{-1, 0}, true);
        bitmap.floodFill(bgeo::BlockPosition{0, 0}, true);

        requireRowsEqual(
            bitmap,
            {
                "#."_el,
                ".#"_el,
            });
    }

    void testFromPatternParsesFilledAndEmptyPixels() {
        const auto bitmap = Bitmap::fromPattern({
            "#.X"_el,
            " o "_el,
        });

        REQUIRE_EQUAL(bitmap.size(), bgeo::BlockSize(3, 2));
        REQUIRE(bitmap.pixel(bgeo::BlockPosition{0, 0}));
        REQUIRE_FALSE(bitmap.pixel(bgeo::BlockPosition{1, 0}));
        REQUIRE(bitmap.pixel(bgeo::BlockPosition{2, 0}));
        REQUIRE_FALSE(bitmap.pixel(bgeo::BlockPosition{0, 1}));
        REQUIRE(bitmap.pixel(bgeo::BlockPosition{1, 1}));
        REQUIRE_FALSE(bitmap.pixel(bgeo::BlockPosition{2, 1}));
    }

    void testFromPatternPadsShortRowsWithClearedPixels() {
        const auto bitmap = Bitmap::fromPattern({
            "##"_el,
            "#"_el,
            ""_el,
        });

        REQUIRE_EQUAL(bitmap.size(), bgeo::BlockSize(2, 3));
        REQUIRE(bitmap.pixel(bgeo::BlockPosition{0, 0}));
        REQUIRE(bitmap.pixel(bgeo::BlockPosition{1, 0}));
        REQUIRE(bitmap.pixel(bgeo::BlockPosition{0, 1}));
        REQUIRE_FALSE(bitmap.pixel(bgeo::BlockPosition{1, 1}));
        REQUIRE_FALSE(bitmap.pixel(bgeo::BlockPosition{0, 2}));
        REQUIRE_FALSE(bitmap.pixel(bgeo::BlockPosition{1, 2}));
    }

    void testToPatternReturnsHashDotRowsWithTrailingNewlines() {
        const auto bitmap = Bitmap::fromPattern({
            "#."_el,
            " #"_el,
            ".."_el,
        });

        REQUIRE_EQUAL(bitmap.toPattern(), "#.\n.#\n..\n"_el);
    }
};
