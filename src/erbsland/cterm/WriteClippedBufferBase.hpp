// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Buffer.hpp"
#include "WritableBuffer.hpp"

#include <array>
#include <cstddef>

namespace erbsland::cterm {

/// The shared base for writable buffer clipping wrappers.
///
/// A write-clipped buffer exposes a source-coordinate rectangle and maps accepted operations into a target
/// rectangle of another writable buffer. Positions outside the source rectangle are ignored for writes, while reads are
/// translated into the wrapped buffer whenever the translated target position exists.
class WriteClippedBufferBase : public WritableBuffer {
public:
    using WritableBuffer::drawBitmap;
    using WritableBuffer::drawBlockText;
    using WritableBuffer::fill;
    using WritableBuffer::get;
    using WritableBuffer::resize;
    using WritableBuffer::set;

public:
    /// Create an empty write-clipped buffer.
    WriteClippedBufferBase() = default;
    /// Create a write-clipped buffer for the given source offset and target rectangle.
    /// @param sourceOffset The top-left source coordinate exposed by this wrapper.
    /// @param targetRect The rectangle where accepted operations land in the wrapped buffer.
    WriteClippedBufferBase(block::Position sourceOffset, block::Rectangle targetRect) noexcept :
        _sourceOffset{sourceOffset}, _targetRect{targetRect} {}

    // defaults
    ~WriteClippedBufferBase() override = default;
    WriteClippedBufferBase(const WriteClippedBufferBase &) = default;
    WriteClippedBufferBase(WriteClippedBufferBase &&) = default;
    auto operator=(const WriteClippedBufferBase &) -> WriteClippedBufferBase & = default;
    auto operator=(WriteClippedBufferBase &&) -> WriteClippedBufferBase & = default;

public: // implement ReadableBuffer
    /// Get the visible source size.
    /// @return The size of the target rectangle.
    [[nodiscard]] auto size() const noexcept -> block::Size final { return _targetRect.size(); }
    /// Get the visible source rectangle.
    /// @return The source rectangle exposed by this wrapper.
    [[nodiscard]] auto rect() const noexcept -> block::Rectangle final { return sourceRect(); }
    /// Create a zero-based writable copy of the visible clipped content.
    /// @return A standalone buffer containing the visible clipped content.
    [[nodiscard]] auto clone() const -> WritableBufferPtr final {
        auto result = std::make_shared<Buffer>(size());
        size().forEach([&](const block::Position pos) -> void { result->set(pos, get(_sourceOffset + pos)); });
        return result;
    }

public: // implement WritableBuffer
    /// Resize the visible source rectangle.
    ///
    /// This only changes the wrapper target size. It never resizes the wrapped buffer.
    /// @param newSize The new visible size.
    void resize(block::Size newSize) final { _targetRect.setSize(newSize); }
    /// Resize the visible source rectangle.
    ///
    /// The resize mode and fill character are ignored because the wrapped buffer is never resized by this wrapper.
    /// @param newSize The new visible size.
    /// @param mode Ignored.
    /// @param fillChar Ignored.
    void resize(block::Size newSize, BufferResizeMode mode, Block fillChar) final {
        (void)mode;
        (void)fillChar;
        resize(newSize);
    }
    /// Write a block at the given source position using a combination style.
    /// @param pos The source coordinates.
    /// @param block The block value to store.
    /// @param combinationStyle The combination style for overwriting existing characters.
    void set(block::Position pos, const Block &block, const BlockCombinationStylePtr &combinationStyle) noexcept final {
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
                const auto surroundPosition =
                    pos + block::Position{block::Coordinate{i} % 3 - 1, block::Coordinate{i} / 3 - 1};
                if (targetBounds().contains(translateToTarget(surroundPosition))) {
                    surroundingBlocks[i] = &get(surroundPosition);
                }
            }
            set(pos, combinationStyle->combine(surroundingBlocks, block));
            return;
        }
        set(pos, combinationStyle->combine(get(pos), block));
    }

public: // accessors
    /// Get the top-left source coordinate exposed by this wrapper.
    /// @return The source offset.
    [[nodiscard]] auto sourceOffset() const noexcept -> const block::Position & { return _sourceOffset; }
    /// Set the top-left source coordinate exposed by this wrapper.
    /// @param sourceOffset The new source offset.
    void setSourceOffset(block::Position sourceOffset) noexcept { _sourceOffset = sourceOffset; }
    /// Get the target rectangle in the wrapped buffer.
    /// @return The target rectangle.
    [[nodiscard]] auto targetRect() const noexcept -> const block::Rectangle & { return _targetRect; }
    /// Set the target rectangle in the wrapped buffer.
    /// @param targetRect The new target rectangle.
    void setTargetRect(block::Rectangle targetRect) noexcept { _targetRect = targetRect; }
    /// Get the visible source rectangle.
    /// @return The rectangle in source coordinates that maps to the target rectangle.
    [[nodiscard]] auto sourceRect() const noexcept -> block::Rectangle {
        return block::Rectangle{_sourceOffset, _targetRect.size()};
    }

protected: // implementation
    /// Implement block filling.
    /// @param rect The source rectangle to fill.
    /// @param fillBlock The block to use for filling.
    /// @param combinationStyle The combination style for overwriting existing characters.
    void fillImpl(
        block::Rectangle rect,
        const Block &fillBlock,
        const BlockCombinationStylePtr &combinationStyle) noexcept final {
        (rect & sourceRect()).forEach([&](const block::Position pos) -> void {
            set(pos, fillBlock, combinationStyle);
        });
    }
    /// Implement tile filling.
    /// @param rect The source rectangle to fill.
    /// @param style The tile style to repeat across the rectangle.
    /// @param baseStyle The base style underneath the tile style.
    /// @param combinationStyle The combination style for overwriting existing characters.
    void fillImpl(
        block::Rectangle rect,
        const Tile9StylePtr &style,
        BlockStyle baseStyle,
        const BlockCombinationStylePtr &combinationStyle) noexcept final {
        if (style == nullptr) {
            return;
        }
        (rect & sourceRect()).forEach([&](const block::Position pos) -> void {
            set(pos, style->block(rect, pos).withBase(baseStyle), combinationStyle);
        });
    }
    /// Translate a source position to the wrapped target buffer position.
    /// @param pos The source position.
    /// @return The translated target position.
    [[nodiscard]] auto translateToTarget(block::Position pos) const noexcept -> block::Position {
        return _targetRect.topLeft() + pos - _sourceOffset;
    }
    /// Get the wrapped buffer bounds.
    /// @return The bounds for the wrapped buffer, or an empty rectangle when no wrapped buffer is available.
    [[nodiscard]] virtual auto targetBounds() const noexcept -> block::Rectangle = 0;
    /// Test whether the given block can be written without escaping the source or target rectangle.
    /// @param sourcePos The source position.
    /// @param targetPos The translated target position.
    /// @param block The block to write.
    /// @param targetBounds The wrapped buffer bounds.
    /// @return `true` if the block write is contained by all relevant rectangles.
    [[nodiscard]] auto canWriteBlock(
        block::Position sourcePos,
        block::Position targetPos,
        const Block &block,
        block::Rectangle targetBounds) const noexcept -> bool {
        if (!sourceRect().contains(sourcePos) || !targetBounds.contains(targetPos)) {
            return false;
        }
        const auto displayWidth = block.displayWidth();
        if (displayWidth <= 1) {
            return true;
        }
        return sourcePos.x() + displayWidth <= sourceRect().x2() && targetPos.x() + displayWidth <= _targetRect.x2() &&
            targetPos.x() + displayWidth <= targetBounds.x2();
    }

protected:
    block::Position _sourceOffset;            ///< The top-left source coordinate exposed by this wrapper.
    block::Rectangle _targetRect{0, 0, 1, 1}; ///< The target rectangle in the wrapped buffer.
};

}
