// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../WritableBuffer.hpp"

namespace erbsland::cterm::impl {

/// Draw bitmaps into a writable terminal buffer.
/// @tested{BufferTest}
class BitmapPainter final {
public:
    /// Create a bitmap painter for the given buffer.
    explicit BitmapPainter(WritableBuffer &buffer) : _buffer{buffer} {}

    // defaults/deletions
    ~BitmapPainter() = default;
    BitmapPainter(const BitmapPainter &) = delete;
    BitmapPainter(BitmapPainter &&) = delete;
    auto operator=(const BitmapPainter &) -> BitmapPainter & = delete;
    auto operator=(BitmapPainter &&) -> BitmapPainter & = delete;

public:
    /// Draw a bitmap at the given buffer position.
    void drawBitmap(
        const Bitmap &bitmap,
        block::Position pos,
        const BitmapDrawOptions &options = BitmapDrawOptions::defaultOptions(),
        std::size_t animationCycle = 0) noexcept;
    /// Draw a bitmap aligned within the given buffer rectangle.
    void drawBitmap(
        const Bitmap &bitmap,
        block::Rectangle rect,
        geometry::Alignment alignment = geometry::Alignment::TopLeft,
        const BitmapDrawOptions &options = BitmapDrawOptions::defaultOptions(),
        std::size_t animationCycle = 0) noexcept;

private: // wrapper (to keep code simple)
    /// Get the target buffer rectangle.
    [[nodiscard]] auto rect() const noexcept -> block::Rectangle { return _buffer.rect(); }
    /// Get one block from the target buffer.
    [[nodiscard]] auto get(block::Position pos) const noexcept -> const Block & { return _buffer.get(pos); }
    /// Set one block in the target buffer.
    void set(block::Position pos, const Block &block, const BlockCombinationStylePtr &combinationStyle) noexcept {
        _buffer.set(pos, block, combinationStyle);
    }
    /// Fill a rectangle in the target buffer.
    void fill(
        block::Rectangle rect, const Block &fillBlock, const BlockCombinationStylePtr &combinationStyle = {}) noexcept {
        _buffer.fill(rect, fillBlock, combinationStyle);
    }

private: // helper
    /// Calculate the buffer size needed to render a bitmap.
    [[nodiscard]] static auto bitmapRenderSize(const Bitmap &bitmap, const BitmapDrawOptions &options) noexcept
        -> block::Size;
    /// Get the bitmap color at a position and animation cycle.
    [[nodiscard]] auto colorForBitmapPosition(
        const BitmapDrawOptions &options, block::Position bitmapPosition, std::size_t animationCycle) const noexcept
        -> Color;
    /// Draw one bitmap block into the target buffer.
    void drawBitmapBlock(
        block::Position pos, const Block &block, Color baseColor, const BitmapDrawOptions &options) noexcept;

private:
    WritableBuffer &_buffer;
};

}
