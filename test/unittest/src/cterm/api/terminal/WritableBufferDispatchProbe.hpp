// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../support/TestHelper.hpp"

#include <memory>
#include <optional>

/// Test buffer that records dispatched writable-buffer operations.
/// @notest{Used only by writable-buffer unit tests.}
class WritableBufferDispatchProbe final : public WritableBuffer {
public:
    /// The operation most recently dispatched to the probe.
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

    [[nodiscard]] auto size() const noexcept -> block::Size override { return _buffer.size(); }

    [[nodiscard]] auto rect() const noexcept -> block::Rectangle override { return _buffer.rect(); }

    [[nodiscard]] auto get(const block::Position pos) const noexcept -> const Block & override {
        return _buffer.get(pos);
    }

    [[nodiscard]] auto clone() const -> WritableBufferPtr override { return std::make_shared<Buffer>(_buffer); }

    void resize(const block::Size newSize) override {
        _lastCall = Call::Resize;
        _lastResizeSize = newSize;
        ++_resizeCallCount;
        _buffer.resize(newSize);
    }

    void set(const block::Position pos, const Block &block) noexcept override { _buffer.set(pos, block); }

    /// Clear every recorded operation and argument.
    void clearRecording() {
        _lastCall = Call::None;
        _lastRect = {};
        _lastFillChar = {};
        _lastFrameBlock = {};
        _lastOptionalFillBlock.reset();
        _lastBaseStyle = {};
        _lastFrameColor = {};
        _lastAnimationCycle = 0;
        _lastAlignment = geometry::Alignment::TopLeft;
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
    block::Rectangle _lastRect{};
    Block _lastFillChar{};
    Block _lastFrameBlock{};
    std::optional<Block> _lastOptionalFillBlock;
    BlockStyle _lastBaseStyle{};
    Color _lastFrameColor{};
    std::size_t _lastAnimationCycle = 0;
    geometry::Alignment _lastAlignment = geometry::Alignment::TopLeft;
    std::optional<GridLayout> _lastGridLayout;
    FrameBorder _lastFrameBorder;
    Tile9StylePtr _lastTile9Style;
    Block16StylePtr _lastBlock16Style;
    BlockString _lastText;
    block::Size _lastBitmapSize{};
    block::Position _lastPosition{};
    block::Size _lastResizeSize{};
    int _resizeCallCount = 0;

protected:
    void setFromImpl(const ReadableBuffer &, const Block fillChar) override {
        _lastCall = Call::SetFrom;
        _lastFillChar = fillChar;
    }

    void fillImpl(
        const block::Rectangle rect, const Block &fillBlock, const BlockCombinationStylePtr &) noexcept override {
        _lastCall = Call::FillBlock;
        _lastRect = rect;
        _lastFillChar = fillBlock;
    }

    void fillImpl(
        const block::Rectangle rect,
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
        const block::Rectangle rect,
        const Block &frameBlock,
        std::optional<Block> fillBlock,
        const BlockCombinationStylePtr &) noexcept override {
        _lastCall = Call::FrameBlock;
        _lastRect = rect;
        _lastFrameBlock = frameBlock;
        _lastOptionalFillBlock = std::move(fillBlock);
    }

    void drawFrameImpl(
        const block::Rectangle rect,
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
        const block::Rectangle rect,
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
        const block::Rectangle rect, const FrameDrawOptions &, const std::size_t animationCycle) noexcept override {
        _lastCall = Call::FrameOptions;
        _lastRect = rect;
        _lastAnimationCycle = animationCycle;
    }

    void drawGridLayoutImpl(
        const block::Position pos, const GridLayout &layout, const FrameBorder &border) noexcept override {
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
        const block::Rectangle rect,
        const geometry::Alignment alignment,
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
        const block::Position pos,
        const BitmapDrawOptions &,
        const std::size_t animationCycle) noexcept override {
        _lastCall = Call::BitmapPosition;
        _lastBitmapSize = bitmap.size();
        _lastPosition = pos;
        _lastAnimationCycle = animationCycle;
    }

    void drawBitmapImpl(
        const Bitmap &bitmap,
        const block::Rectangle rect,
        const geometry::Alignment alignment,
        const BitmapDrawOptions &,
        const std::size_t animationCycle) noexcept override {
        _lastCall = Call::BitmapRect;
        _lastBitmapSize = bitmap.size();
        _lastRect = rect;
        _lastAlignment = alignment;
        _lastAnimationCycle = animationCycle;
    }

private:
    Buffer _buffer{block::Size{4, 4}};
};
