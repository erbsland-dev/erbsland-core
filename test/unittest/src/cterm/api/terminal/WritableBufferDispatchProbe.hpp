// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../support/TestHelper.hpp"

#include <memory>
#include <optional>

class WritableBufferDispatchProbe final : public WritableBuffer {
public:
    enum class Call {
        None,
        Resize,
        SetFrom,
        FillBlock,
        FillTile9,
        FrameBlock,
        FrameChar16,
        FrameTile9,
        FrameOptions,
        GridLayout,
        TextObject,
        TextRect,
        BitmapPosition,
        BitmapRect,
    };

public:
    using WritableBuffer::resize;

    [[nodiscard]] auto size() const noexcept -> bgeo::BlockSize override { return _buffer.size(); }

    [[nodiscard]] auto rect() const noexcept -> bgeo::BlockRectangle override { return _buffer.rect(); }

    [[nodiscard]] auto get(const bgeo::BlockPosition pos) const noexcept -> const Block & override {
        return _buffer.get(pos);
    }

    [[nodiscard]] auto clone() const -> WritableBufferPtr override { return std::make_shared<Buffer>(_buffer); }

    void resize(const bgeo::BlockSize newSize) override {
        _lastCall = Call::Resize;
        _lastResizeSize = newSize;
        ++_resizeCallCount;
        _buffer.resize(newSize);
    }

    void set(const bgeo::BlockPosition pos, const Block &block) noexcept override { _buffer.set(pos, block); }

    void clearRecording() {
        _lastCall = Call::None;
        _lastRect = {};
        _lastFillChar = {};
        _lastFrameBlock = {};
        _lastOptionalFillBlock.reset();
        _lastBaseStyle = {};
        _lastFrameColor = {};
        _lastAnimationCycle = 0;
        _lastAlignment = bgeo::Alignment::TopLeft;
        _lastGridLayout.reset();
        _lastFrameBorder = {};
        _lastTile9Style.reset();
        _lastBlock16Style.reset();
        _lastText = {};
        _lastBitmapSize = {};
        _lastPosition = {};
        _lastResizeSize = {};
        _resizeCallCount = 0;
    }

public:
    Call _lastCall = Call::None;
    bgeo::BlockRectangle _lastRect{};
    Block _lastFillChar{};
    Block _lastFrameBlock{};
    std::optional<Block> _lastOptionalFillBlock;
    BlockStyle _lastBaseStyle{};
    Color _lastFrameColor{};
    std::size_t _lastAnimationCycle = 0;
    bgeo::Alignment _lastAlignment = bgeo::Alignment::TopLeft;
    std::optional<GridLayout> _lastGridLayout;
    FrameBorder _lastFrameBorder;
    Tile9StylePtr _lastTile9Style;
    Block16StylePtr _lastBlock16Style;
    BlockString _lastText;
    bgeo::BlockSize _lastBitmapSize{};
    bgeo::BlockPosition _lastPosition{};
    bgeo::BlockSize _lastResizeSize{};
    int _resizeCallCount = 0;

protected:
    void setFromImpl(const ReadableBuffer &, const Block fillChar) override {
        _lastCall = Call::SetFrom;
        _lastFillChar = fillChar;
    }

    void fillImpl(
        const bgeo::BlockRectangle rect, const Block &fillBlock, const BlockCombinationStylePtr &) noexcept override {
        _lastCall = Call::FillBlock;
        _lastRect = rect;
        _lastFillChar = fillBlock;
    }

    void fillImpl(
        const bgeo::BlockRectangle rect,
        const Tile9StylePtr &style,
        const BlockStyle baseStyle,
        const BlockCombinationStylePtr &) noexcept override {
        _lastCall = Call::FillTile9;
        _lastRect = rect;
        _lastTile9Style = style;
        _lastBaseStyle = baseStyle;
        _lastFrameColor = baseStyle.color();
    }

    void drawFrameImpl(
        const bgeo::BlockRectangle rect,
        const Block &frameBlock,
        std::optional<Block> fillBlock,
        const BlockCombinationStylePtr &) noexcept override {
        _lastCall = Call::FrameBlock;
        _lastRect = rect;
        _lastFrameBlock = frameBlock;
        _lastOptionalFillBlock = std::move(fillBlock);
    }

    void drawFrameImpl(
        const bgeo::BlockRectangle rect,
        const Block16StylePtr &frameStyle,
        std::optional<Block> fillBlock,
        const BlockCombinationStylePtr &,
        const Color frameColor) noexcept override {
        _lastCall = Call::FrameChar16;
        _lastRect = rect;
        _lastBlock16Style = frameStyle;
        _lastOptionalFillBlock = std::move(fillBlock);
        _lastFrameColor = frameColor;
    }

    void drawFrameImpl(
        const bgeo::BlockRectangle rect,
        const Tile9StylePtr &style,
        std::optional<Block> fillBlock,
        const BlockCombinationStylePtr &,
        const Color frameColor) noexcept override {
        _lastCall = Call::FrameTile9;
        _lastRect = rect;
        _lastTile9Style = style;
        _lastOptionalFillBlock = std::move(fillBlock);
        _lastFrameColor = frameColor;
    }

    void drawFrameImpl(
        const bgeo::BlockRectangle rect, const FrameDrawOptions &, const std::size_t animationCycle) noexcept override {
        _lastCall = Call::FrameOptions;
        _lastRect = rect;
        _lastAnimationCycle = animationCycle;
    }

    void drawGridLayoutImpl(
        const bgeo::BlockPosition pos, const GridLayout &layout, const FrameBorder &border) noexcept override {
        _lastCall = Call::GridLayout;
        _lastPosition = pos;
        _lastGridLayout = layout;
        _lastFrameBorder = border;
    }

    void drawBlockTextImpl(const BlockText &text, const std::size_t animationCycle) override {
        _lastCall = Call::TextObject;
        _lastRect = text.rectangle();
        _lastText = text.blockString();
        _lastAnimationCycle = animationCycle;
    }

    void drawBlockTextImpl(
        const BlockString &text,
        const bgeo::BlockRectangle rect,
        const bgeo::Alignment alignment,
        BlockStyle style,
        const std::size_t animationCycle) override {
        _lastCall = Call::TextRect;
        _lastRect = rect;
        _lastText = BlockStringEditor{text};
        _lastAlignment = alignment;
        _lastAnimationCycle = animationCycle;
    }

    void drawBitmapImpl(
        const Bitmap &bitmap,
        const bgeo::BlockPosition pos,
        const BitmapDrawOptions &,
        const std::size_t animationCycle) noexcept override {
        _lastCall = Call::BitmapPosition;
        _lastBitmapSize = bitmap.size();
        _lastPosition = pos;
        _lastAnimationCycle = animationCycle;
    }

    void drawBitmapImpl(
        const Bitmap &bitmap,
        const bgeo::BlockRectangle rect,
        const bgeo::Alignment alignment,
        const BitmapDrawOptions &,
        const std::size_t animationCycle) noexcept override {
        _lastCall = Call::BitmapRect;
        _lastBitmapSize = bitmap.size();
        _lastRect = rect;
        _lastAlignment = alignment;
        _lastAnimationCycle = animationCycle;
    }

private:
    Buffer _buffer{bgeo::BlockSize{4, 4}};
};
