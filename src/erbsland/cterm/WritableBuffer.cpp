// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WritableBuffer.hpp"

#include "impl/BitmapPainter.hpp"
#include "impl/BlockTextPainter.hpp"
#include "impl/FramePainter.hpp"

#include "../bgeo/BlockAlignedSource.hpp"
#include "../err/ParameterError.hpp"
#include "../text/EncodingErrorMode.hpp"

namespace erbsland::cterm {

using namespace bgeo;
using impl::BitmapPainter;
using impl::BlockTextPainter;
using impl::FramePainter;

void WritableBuffer::resize(const BlockSize newSize, const BufferResizeMode mode, const Block fillChar) {
    if (size() == newSize) {
        return;
    }
    if (mode == BufferResizeMode::Fast) {
        resize(newSize);
        return;
    }
    const auto previous = clone();
    resize(newSize);
    setFrom(*previous, fillChar);
}

void WritableBuffer::set(
    const BlockPosition pos, const Block &block, const BlockCombinationStylePtr &combinationStyle) noexcept {
    if (!rect().contains(pos)) {
        return;
    }
    if (combinationStyle == nullptr) {
        set(pos, block);
        return;
    }
    if (combinationStyle->isSurroundingAware()) {
        std::array<const Block *, 9> surroundingBlocks{};
        for (std::size_t i = 0; i < 9; ++i) {
            const auto surroundPosition = pos + BlockPosition{BlockCoordinate{i} % 3 - 1, BlockCoordinate{i} / 3 - 1};
            if (rect().contains(surroundPosition)) {
                surroundingBlocks[i] = &get(surroundPosition);
            }
        }
        set(pos, combinationStyle->combine(surroundingBlocks, block));
    } else {
        set(pos, combinationStyle->combine(get(pos), block));
    }
}

void WritableBuffer::set(BlockPosition pos, const BlockString &str) noexcept {
    if (str.isEmpty()) {
        return;
    }
    const auto x = pos.x();
    for (const auto character : str) {
        if (character == U'\n') {
            pos.setX(x);
            pos += BlockPosition{0, 1};
            continue;
        }
        if (character.displayWidth() == 0) {
            continue;
        }
        set(pos, character);
        pos += BlockPosition{character.displayWidth(), 0};
    }
}

void WritableBuffer::setFrom(const ReadableBuffer &other, const Block fillChar) {
    setFromImpl(other, fillChar);
}

void WritableBuffer::setFromImpl(const ReadableBuffer &other, const Block fillChar) {
    rect().forEach([&](const BlockPosition pos) -> void {
        if (other.rect().contains(pos)) {
            set(pos, other.get(pos));
        } else {
            set(pos, fillChar);
        }
    });
}

void WritableBuffer::setAndResizeFrom(const ReadableBuffer &other) {
    resize(other.size());
    setFrom(other);
}

void WritableBuffer::fill(const Block &fillBlock) noexcept {
    rect().forEach([&, this](const BlockPosition pos) -> void { set(pos, fillBlock); });
}

void WritableBuffer::fill(
    const BlockRectangle rect, const Block &fillBlock, const BlockCombinationStylePtr &combinationStyle) noexcept {
    fillImpl(rect, fillBlock, combinationStyle);
}

void WritableBuffer::fillImpl(
    const BlockRectangle rect, const Block &fillBlock, const BlockCombinationStylePtr &combinationStyle) noexcept {
    rect.forEach([&, this](const BlockPosition pos) -> void { set(pos, fillBlock, combinationStyle); });
}

void WritableBuffer::fill(
    const BlockRectangle rect,
    const Tile9StylePtr &style,
    const Color baseColor,
    const BlockCombinationStylePtr &combinationStyle) noexcept {
    fillImpl(rect, style, BlockStyle{baseColor}, combinationStyle);
}

void WritableBuffer::fill(
    const BlockRectangle rect,
    const Tile9StylePtr &style,
    const BlockStyle baseStyle,
    const BlockCombinationStylePtr &combinationStyle) noexcept {
    fillImpl(rect, style, baseStyle, combinationStyle);
}

void WritableBuffer::fillImpl(
    const BlockRectangle rect,
    const Tile9StylePtr &style,
    const BlockStyle baseStyle,
    const BlockCombinationStylePtr &combinationStyle) noexcept {
    if (style == nullptr) {
        return;
    }
    rect.forEach([&, this](const BlockPosition pos) -> void {
        set(pos, style->block(rect, pos).withBase(baseStyle), combinationStyle);
    });
}

void WritableBuffer::drawFrame(
    const BlockRectangle rect, const Block &frameBlock, const BlockCombinationStylePtr &combinationStyle) noexcept {
    drawFrameImpl(rect, frameBlock, std::nullopt, combinationStyle);
}

void WritableBuffer::drawFrameImpl(
    const BlockRectangle rect,
    const Block &frameBlock,
    const std::optional<Block> fillBlock,
    const BlockCombinationStylePtr &combinationStyle) noexcept {
    if (fillBlock.has_value()) {
        FramePainter{*this}.drawFilledFrame(rect, frameBlock, *fillBlock, combinationStyle);
        return;
    }
    FramePainter{*this}.drawFrame(rect, frameBlock, combinationStyle);
}

void WritableBuffer::drawFrame(
    const BlockRectangle rect,
    const Block16StylePtr &frameStyle,
    const BlockCombinationStylePtr &combinationStyle,
    const Color frameColor) noexcept {
    drawFrameImpl(rect, frameStyle, std::nullopt, combinationStyle, frameColor);
}

void WritableBuffer::drawFrameImpl(
    const BlockRectangle rect,
    const Block16StylePtr &frameStyle,
    const std::optional<Block> fillBlock,
    const BlockCombinationStylePtr &combinationStyle,
    const Color frameColor) noexcept {
    if (fillBlock.has_value()) {
        FramePainter{*this}.drawFilledFrame(rect, frameStyle, *fillBlock, combinationStyle, frameColor);
        return;
    }
    FramePainter{*this}.drawFrame(rect, frameStyle, combinationStyle, frameColor);
}

void WritableBuffer::drawFrame(
    const BlockRectangle rect,
    const Tile9StylePtr &style,
    const Color frameColor,
    const BlockCombinationStylePtr &combinationStyle) noexcept {
    drawFrameImpl(rect, style, std::nullopt, combinationStyle, frameColor);
}

void WritableBuffer::drawFrameImpl(
    const BlockRectangle rect,
    const Tile9StylePtr &style,
    const std::optional<Block> fillBlock,
    const BlockCombinationStylePtr &combinationStyle,
    const Color frameColor) noexcept {
    if (fillBlock.has_value()) {
        FramePainter{*this}.drawFilledFrame(rect, style, *fillBlock, combinationStyle, frameColor);
        return;
    }
    FramePainter{*this}.drawFrame(rect, style, frameColor, combinationStyle);
}

void WritableBuffer::drawFrame(
    const BlockRectangle rect, const FrameStyle frameStyle, const Color frameColor) noexcept {
    if (const auto tile9Style = Tile9Style::forStyle(frameStyle); tile9Style != nullptr) {
        drawFrameImpl(rect, tile9Style, std::nullopt, BlockCombinationStyle::commonBoxFrame(), frameColor);
        return;
    }
    if (const auto block16Style = Block16Style::forStyle(frameStyle); block16Style != nullptr) {
        drawFrameImpl(rect, block16Style, std::nullopt, BlockCombinationStyle::commonBoxFrame(), frameColor);
    }
}

void WritableBuffer::drawFrame(
    BlockRectangle rect, const FrameDrawOptions &options, std::size_t animationCycle) noexcept {
    drawFrameImpl(rect, options, animationCycle);
}

void WritableBuffer::drawFrameImpl(
    const BlockRectangle rect, const FrameDrawOptions &options, const std::size_t animationCycle) noexcept {
    FramePainter{*this}.drawFrame(rect, options, animationCycle);
}

void WritableBuffer::drawGridLayout(
    const BlockPosition pos, const GridLayout &layout, const FrameBorder &border) noexcept {
    drawGridLayoutImpl(pos, layout, border);
}

void WritableBuffer::drawGridLayoutImpl(
    const BlockPosition pos, const GridLayout &layout, const FrameBorder &border) noexcept {
    FramePainter{*this}.drawGridLayout(pos, layout, border);
}

void WritableBuffer::drawFilledFrame(
    const BlockRectangle rect,
    const Block &frameBlock,
    const Block &fillBlock,
    const BlockCombinationStylePtr &combinationStyle) noexcept {
    drawFrameImpl(rect, frameBlock, fillBlock, combinationStyle);
}

void WritableBuffer::drawFilledFrame(
    const BlockRectangle rect,
    const Block16StylePtr &frameStyle,
    const Block &fillBlock,
    const BlockCombinationStylePtr &combinationStyle,
    const Color frameColor) noexcept {
    drawFrameImpl(rect, frameStyle, fillBlock, combinationStyle, frameColor);
}

void WritableBuffer::drawFilledFrame(
    const BlockRectangle rect,
    const Tile9StylePtr &style,
    const Block &fillBlock,
    const BlockCombinationStylePtr &combinationStyle,
    const Color frameColor) noexcept {
    drawFrameImpl(rect, style, fillBlock, combinationStyle, frameColor);
}

void WritableBuffer::drawFilledFrame(
    const BlockRectangle rect, const FrameStyle frameStyle, const Block &fillBlock, const Color frameColor) noexcept {
    if (const auto tile9Style = Tile9Style::forStyle(frameStyle); tile9Style != nullptr) {
        drawFrameImpl(rect, tile9Style, fillBlock, BlockCombinationStyle::commonBoxFrame(), frameColor);
        return;
    }
    if (const auto block16Style = Block16Style::forStyle(frameStyle); block16Style != nullptr) {
        drawFrameImpl(rect, block16Style, fillBlock, BlockCombinationStyle::commonBoxFrame(), frameColor);
    }
}

void WritableBuffer::drawBlockText(BlockPosition pos, const BlockString &str) {
    BlockTextPainter{*this}.drawBlockText(pos, str);
}

void WritableBuffer::drawBlockText(const BlockText &text, std::size_t animationCycle) {
    drawBlockTextImpl(text, animationCycle);
}

void WritableBuffer::drawBlockTextImpl(const BlockText &text, const std::size_t animationCycle) {
    BlockTextPainter{*this}.drawBlockText(text, animationCycle);
}

void WritableBuffer::drawBlockText(
    const text::String &text,
    const BlockRectangle rect,
    const Alignment alignment,
    const BlockStyle style,
    const std::size_t animationCycle) {
    drawBlockText(BlockStringEditor{text, text::EncodingErrorMode::Replace}, rect, alignment, style, animationCycle);
}

void WritableBuffer::drawBlockText(
    const text::U32String &text,
    const BlockRectangle rect,
    const Alignment alignment,
    const BlockStyle style,
    const std::size_t animationCycle) {
    drawBlockText(BlockStringEditor{text}, rect, alignment, style, animationCycle);
}

void WritableBuffer::drawBlockText(
    const BlockString &text,
    const BlockRectangle rect,
    const Alignment alignment,
    const BlockStyle style,
    const std::size_t animationCycle) {
    drawBlockTextImpl(text, rect, alignment, style, animationCycle);
}

void WritableBuffer::drawBlockText(
    const BlockString &text,
    const BlockRectangle rect,
    const BlockTextOptions &options,
    const std::size_t animationCycle) {
    drawBlockTextImpl(text, rect, options, animationCycle);
}

auto WritableBuffer::blockTextHeightForWidth(
    const BlockString &text, const BlockCoordinate width, const BlockTextOptions &options) noexcept -> BlockCoordinate {
    return text.wrappedBlockTextHeight(width, options);
}

void WritableBuffer::drawBlockTextImpl(
    const BlockString &text,
    const BlockRectangle rect,
    const Alignment alignment,
    const BlockStyle style,
    const std::size_t animationCycle) {
    BlockTextPainter{*this}.drawBlockText(text, rect, alignment, style, animationCycle);
}

void WritableBuffer::drawBlockTextImpl(
    const BlockString &text,
    const BlockRectangle rect,
    const BlockTextOptions &options,
    const std::size_t animationCycle) {
    BlockTextPainter{*this}.drawBlockText(text, rect, options, animationCycle);
}

void WritableBuffer::drawBitmap(
    const Bitmap &bitmap,
    const BlockPosition pos,
    const BitmapDrawOptions &options,
    const std::size_t animationCycle) noexcept {
    drawBitmapImpl(bitmap, pos, options, animationCycle);
}

void WritableBuffer::drawBitmapImpl(
    const Bitmap &bitmap,
    const BlockPosition pos,
    const BitmapDrawOptions &options,
    const std::size_t animationCycle) noexcept {
    BitmapPainter{*this}.drawBitmap(bitmap, pos, options, animationCycle);
}

void WritableBuffer::drawBitmap(
    const Bitmap &bitmap,
    const BlockRectangle rect,
    const Alignment alignment,
    const BitmapDrawOptions &options,
    const std::size_t animationCycle) noexcept {
    drawBitmapImpl(bitmap, rect, alignment, options, animationCycle);
}

void WritableBuffer::drawBuffer(const ReadableBuffer &buffer, const BlockPosition targetPos) {
    drawBuffer(buffer, BufferDrawOptions{targetPos});
}

void WritableBuffer::drawBuffer(
    const ReadableBuffer &buffer, const BlockRectangle targetRect, const Alignment alignment) {
    if (buffer.size().area() == 0 || targetRect.width() <= 0 || targetRect.height() <= 0) {
        return;
    }
    const auto alignedSource = targetRect.alignedSource(buffer.rect(), alignment);
    drawBuffer(buffer, BufferDrawOptions{alignedSource.targetRect, alignedSource.sourceRect});
}

void WritableBuffer::drawBuffer(const ReadableBuffer &buffer, const BufferDrawOptions &options) {
    if (static_cast<const ReadableBuffer *>(this) == &buffer) {
        throw err::ParameterError{
            "WritableBuffer::drawBuffer() does not support drawing a buffer onto itself.", "buffer"};
    }
    const auto *source = &buffer;
    auto sourceRect = options.useFullSource() ? source->rect() : options.sourceRect();
    sourceRect &= source->rect();
    if (sourceRect.width() <= 0 || sourceRect.height() <= 0) {
        return;
    }
    auto targetRect = options.isTargetPosition() ? BlockRectangle{options.targetRect().topLeft(), sourceRect.size()}
                                                 : options.targetRect();
    targetRect.setSize(targetRect.size().limitedWith(sourceRect.size()));
    if (targetRect.width() <= 0 || targetRect.height() <= 0) {
        return;
    }
    const auto visibleTargetRect = targetRect & rect();
    if (visibleTargetRect.width() <= 0 || visibleTargetRect.height() <= 0) {
        return;
    }
    const auto clippedOffset = visibleTargetRect.topLeft() - targetRect.topLeft();
    const auto sourceStartPos = sourceRect.topLeft() + clippedOffset;
    const auto &combinationStyle = options.combinationStyle();
    visibleTargetRect.forEach([&](const BlockPosition targetPos) -> void {
        auto sourceBlock = source->get(sourceStartPos + targetPos - visibleTargetRect.topLeft());
        if (combinationStyle != nullptr) {
            set(targetPos, sourceBlock, combinationStyle);
            return;
        }
        if (!options.overwriteColors()) {
            sourceBlock = sourceBlock.withBase(get(targetPos).color());
        }
        set(targetPos, sourceBlock);
    });
}

void WritableBuffer::drawBitmapImpl(
    const Bitmap &bitmap,
    const BlockRectangle rect,
    const Alignment alignment,
    const BitmapDrawOptions &options,
    const std::size_t animationCycle) noexcept {
    BitmapPainter{*this}.drawBitmap(bitmap, rect, alignment, options, animationCycle);
}

}
