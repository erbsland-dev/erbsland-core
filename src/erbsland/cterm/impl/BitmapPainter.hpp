// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../WritableBuffer.hpp"

namespace erbsland::cterm::impl {

class BitmapPainter final {
public:
    explicit BitmapPainter(WritableBuffer &buffer) : _buffer{buffer} {}

    // delete copy/move
    ~BitmapPainter() = default;
    BitmapPainter(const BitmapPainter &) = delete;
    BitmapPainter(BitmapPainter &&) = delete;
    auto operator=(const BitmapPainter &) -> BitmapPainter & = delete;
    auto operator=(BitmapPainter &&) -> BitmapPainter & = delete;

public:
    void drawBitmap(
        const Bitmap &bitmap,
        bgeo::BlockPosition pos,
        const BitmapDrawOptions &options = BitmapDrawOptions::defaultOptions(),
        std::size_t animationCycle = 0) noexcept;
    void drawBitmap(
        const Bitmap &bitmap,
        bgeo::BlockRectangle rect,
        bgeo::Alignment alignment = bgeo::Alignment::TopLeft,
        const BitmapDrawOptions &options = BitmapDrawOptions::defaultOptions(),
        std::size_t animationCycle = 0) noexcept;

private: // wrapper (to keep code simple)
    [[nodiscard]] auto rect() const noexcept -> bgeo::BlockRectangle { return _buffer.rect(); }
    [[nodiscard]] auto get(bgeo::BlockPosition pos) const noexcept -> const Block & { return _buffer.get(pos); }
    void set(bgeo::BlockPosition pos, const Block &block, const BlockCombinationStylePtr &combinationStyle) noexcept {
        _buffer.set(pos, block, combinationStyle);
    }
    void fill(
        bgeo::BlockRectangle rect,
        const Block &fillBlock,
        const BlockCombinationStylePtr &combinationStyle = {}) noexcept {
        _buffer.fill(rect, fillBlock, combinationStyle);
    }

private: // helper
    [[nodiscard]] static auto bitmapRenderSize(const Bitmap &bitmap, const BitmapDrawOptions &options) noexcept
        -> bgeo::BlockSize;
    [[nodiscard]] auto colorForBitmapPosition(
        const BitmapDrawOptions &options, bgeo::BlockPosition bitmapPosition, std::size_t animationCycle) const noexcept
        -> Color;
    void drawBitmapBlock(
        bgeo::BlockPosition pos, const Block &block, Color baseColor, const BitmapDrawOptions &options) noexcept;

private:
    WritableBuffer &_buffer;
};

}
