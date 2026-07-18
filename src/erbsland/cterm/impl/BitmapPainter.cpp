// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BitmapPainter.hpp"

namespace erbsland::cterm::impl {

using namespace bgeo;

void BitmapPainter::drawBitmap(
    const Bitmap &bitmap,
    const BlockPosition pos,
    const BitmapDrawOptions &options,
    const std::size_t animationCycle) noexcept {

    drawBitmap(
        bitmap, BlockRectangle{pos, bitmapRenderSize(bitmap, options)}, Alignment::TopLeft, options, animationCycle);
}

void BitmapPainter::drawBitmap(
    const Bitmap &bitmap,
    const BlockRectangle rect,
    const Alignment alignment,
    const BitmapDrawOptions &options,
    const std::size_t animationCycle) noexcept {

    if (bitmap.size().area() == 0 || rect.width() <= 0 || rect.height() <= 0) {
        return;
    }
    const auto renderedSize = bitmapRenderSize(bitmap, options);
    if (renderedSize.area() == 0) {
        return;
    }
    auto visibleSize = renderedSize.limitedWith(rect.size());
    auto sourceOffset = BlockPosition{};
    auto targetPos = rect.alignmentOffset(renderedSize, alignment);
    const auto alignmentOffset = rect.size().alignmentOffset(renderedSize, alignment);
    if (renderedSize.width() <= rect.width()) {
    } else {
        sourceOffset += BlockPosition{-alignmentOffset.x(), BlockCoordinate{0}};
        targetPos.setX(rect.x1());
    }
    if (renderedSize.height() <= rect.height()) {
    } else {
        sourceOffset += BlockPosition{BlockCoordinate{0}, -alignmentOffset.y()};
        targetPos.setY(rect.y1());
    }
    if (options.block16Style() != nullptr) {
        for (auto y = 0; y < visibleSize.height(); ++y) {
            for (auto x = 0; x < visibleSize.width(); ++x) {
                const auto bitmapPos = sourceOffset + BlockPosition{x, y};
                if (!bitmap.pixel(bitmapPos)) {
                    continue;
                }
                const auto bitMask = bitmapPos.cardinalFourBitmask(
                    [&](const BlockPosition neighborPos) noexcept -> bool { return bitmap.pixel(neighborPos); });
                drawBitmapBlock(
                    targetPos + BlockPosition{x, y},
                    options.block16Style()->block(bitMask),
                    colorForBitmapPosition(options, bitmapPos, animationCycle),
                    options);
            }
        }
        return;
    }
    switch (options.scaleMode()) {
    case BitmapScaleMode::HalfBlock:
        for (auto y = 0; y < visibleSize.height(); ++y) {
            for (auto x = 0; x < visibleSize.width(); ++x) {
                const auto bitmapCellPos = sourceOffset + BlockPosition{x, y};
                const auto bitMask = bitmap.pixelQuad(bitmapCellPos);
                drawBitmapBlock(
                    targetPos + BlockPosition{x, y},
                    options.halfBlocks()[BlockIndex::fromSizeT(bitMask)],
                    colorForBitmapPosition(options, bitmapCellPos, animationCycle),
                    options);
            }
        }
        break;
    case BitmapScaleMode::DoubleBlock:
        for (auto y = 0; y < visibleSize.height(); ++y) {
            for (auto x = 0; x < visibleSize.width(); ++x) {
                const auto renderedBitmapPos = sourceOffset + BlockPosition{x, y};
                const auto bitmapPos = BlockPosition{renderedBitmapPos.x() / 2, renderedBitmapPos.y()};
                if (!bitmap.pixel(bitmapPos)) {
                    continue;
                }
                drawBitmapBlock(
                    targetPos + BlockPosition{x, y},
                    options.doubleBlocks()[BlockIndex::fromSizeT((renderedBitmapPos.x() % 2).toSizeT())],
                    colorForBitmapPosition(options, bitmapPos, animationCycle),
                    options);
            }
        }
        break;
    case BitmapScaleMode::FullBlock:
    default:
        for (auto y = 0; y < visibleSize.height(); ++y) {
            for (auto x = 0; x < visibleSize.width(); ++x) {
                const auto bitmapPos = sourceOffset + BlockPosition{x, y};
                if (!bitmap.pixel(bitmapPos)) {
                    continue;
                }
                drawBitmapBlock(
                    targetPos + BlockPosition{x, y},
                    options.fullBlock(),
                    colorForBitmapPosition(options, bitmapPos, animationCycle),
                    options);
            }
        }
        break;
    }
}

auto BitmapPainter::bitmapRenderSize(const Bitmap &bitmap, const BitmapDrawOptions &options) noexcept -> BlockSize {
    if (options.block16Style() != nullptr) {
        return bitmap.size();
    }
    switch (options.scaleMode()) {
    case BitmapScaleMode::HalfBlock:
        return {(bitmap.size().width() + 1) / 2, (bitmap.size().height() + 1) / 2};
    case BitmapScaleMode::DoubleBlock:
        return {bitmap.size().width() * 2, bitmap.size().height()};
    case BitmapScaleMode::FullBlock:
    default:
        return bitmap.size();
    }
}

auto BitmapPainter::colorForBitmapPosition(
    const BitmapDrawOptions &options,
    const BlockPosition bitmapPosition,
    const std::size_t animationCycle) const noexcept -> Color {

    const auto &colorSequence = options.color();
    if (colorSequence.empty()) {
        return {};
    }
    auto sequenceIndex = static_cast<int64_t>(animationCycle + options.colorAnimationOffset());
    switch (options.colorMode()) {
    case BitmapColorMode::VerticalStripes:
        sequenceIndex += bitmapPosition.x().toRawValue();
        break;
    case BitmapColorMode::HorizontalStripes:
        sequenceIndex += bitmapPosition.y().toRawValue();
        break;
    case BitmapColorMode::ForwardDiagonalStripes:
        sequenceIndex += (bitmapPosition.x() + bitmapPosition.y()).toRawValue();
        break;
    case BitmapColorMode::BackwardDiagonalStripes:
        sequenceIndex += (bitmapPosition.y() - bitmapPosition.x()).toRawValue();
        break;
    case BitmapColorMode::OneColor:
    default:
        break;
    }
    const auto sequenceLength = static_cast<int64_t>(colorSequence.sequenceLength());
    const auto wrappedSequenceIndex = ((sequenceIndex % sequenceLength) + sequenceLength) % sequenceLength;
    return colorSequence.color(static_cast<std::size_t>(wrappedSequenceIndex));
}

void BitmapPainter::drawBitmapBlock(
    const BlockPosition pos, const Block &block, const Color baseColor, const BitmapDrawOptions &options) noexcept {

    if (!rect().contains(pos)) {
        return;
    }
    const auto finalColor = get(pos).color().overlayWith(baseColor).overlayWith(block.color());
    set(pos, block.withColorReplaced(finalColor), options.combinationStyle());
}

}
