// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../support/TestHelper.hpp"

#include <optional>
#include <vector>

class CursorWriterProbe final : public CursorWriter {
public:
    [[nodiscard]] auto color() const noexcept -> Color override { return _color; }
    [[nodiscard]] auto blockAttributes() const noexcept -> BlockAttributes override { return _attributes; }
    void setColor(const Color color) noexcept override { _color = color; }
    void setBlockAttributes(const BlockAttributes attributes) noexcept override { _attributes = attributes; }
    void setForeground(const Foreground color) noexcept override { _color.setFg(color); }
    void setBackground(const Background color) noexcept override { _color.setBg(color); }
    [[nodiscard]] auto supportedBlockAttributes() const noexcept -> BlockAttributes override {
        return BlockAttributes::all();
    }
    void moveCursor(const bgeo::BlockPosition posOrDelta, const MoveMode mode) noexcept override {
        _lastMove = posOrDelta;
        _lastMoveMode = mode;
    }
    void setAutoWrap(const bool enabled) noexcept override { _autoWrap = enabled; }
    [[nodiscard]] auto size() const noexcept -> bgeo::BlockSize override { return _size; }
    void clearScreen() noexcept override { _clearScreenCallCount += 1; }
    void write(const Block &character) noexcept override { _writtenChars.push_back(character); }
    void write(const BlockString &str) noexcept override { _writtenStrings.push_back(BlockStringEditor{str}); }
    void writeResolved(const Block &character) noexcept override { _writtenResolvedChars.push_back(character); }
    void writeResolved(const BlockString &str) noexcept override {
        _writtenResolvedStrings.push_back(BlockStringEditor{str});
    }
    void write(const ReadableBuffer &) noexcept override { _writeBufferCallCount += 1; }
    void writeLineBreak() noexcept override { _lineBreakCount += 1; }

protected:
    auto printParagraphImpl(const BlockString &paragraph, const ParagraphOptions &options) noexcept -> int override {
        _lastParagraph = BlockStringEditor{paragraph};
        _lastParagraphAlignment = options.alignment();
        _lastParagraphTabStops = options.tabStops();
        _lastParagraphTabOverflowBehavior = options.tabOverflowBehavior();
        return 7;
    }

public:
    bgeo::BlockSize _size{80, 25};
    Color _color{};
    BlockAttributes _attributes{};
    bgeo::BlockPosition _lastMove{};
    MoveMode _lastMoveMode{MoveMode::Absolute};
    bool _autoWrap{false};
    int _clearScreenCallCount{0};
    std::vector<Block> _writtenChars;
    std::vector<BlockStringEditor> _writtenStrings;
    std::vector<Block> _writtenResolvedChars;
    std::vector<BlockStringEditor> _writtenResolvedStrings;
    int _writeBufferCallCount{0};
    int _lineBreakCount{0};
    BlockStringEditor _lastParagraph{};
    bgeo::Alignment _lastParagraphAlignment{bgeo::Alignment::TopLeft};
    std::vector<int> _lastParagraphTabStops;
    TabOverflowBehavior _lastParagraphTabOverflowBehavior{TabOverflowBehavior::AddSpace};
};
