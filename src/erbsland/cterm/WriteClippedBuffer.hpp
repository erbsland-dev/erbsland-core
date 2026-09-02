// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "WriteClippedBufferBase.hpp"
#include "WriteClippedBufferRef.hpp"

#include <utility>

namespace erbsland::cterm {

/// A write-clipped buffer that owns a shared pointer to the wrapped buffer.
class WriteClippedBuffer final : public WriteClippedBufferBase {
public:
    using WriteClippedBufferBase::drawBitmap;
    using WriteClippedBufferBase::drawBlockText;
    using WriteClippedBufferBase::fill;
    using WriteClippedBufferBase::get;
    using WriteClippedBufferBase::resize;
    using WriteClippedBufferBase::set;

public:
    /// Create an empty write-clipped buffer.
    WriteClippedBuffer() = default;
    /// Create an empty write-clipped buffer with a given visible size.
    /// @param size The visible source size.
    explicit WriteClippedBuffer(block::Size size) noexcept : WriteClippedBufferBase{{}, block::Rectangle{{}, size}} {}
    /// Create a write-clipped buffer with the given content and visible size.
    /// @param content The wrapped writable buffer.
    /// @param size The visible source size.
    WriteClippedBuffer(WritableBufferPtr content, block::Size size) noexcept :
        WriteClippedBufferBase{{}, block::Rectangle{{}, size}}, _content{std::move(content)} {}
    /// Create a write-clipped buffer with the given content and target rectangle.
    /// @param content The wrapped writable buffer.
    /// @param targetRect The target rectangle in the wrapped buffer.
    WriteClippedBuffer(WritableBufferPtr content, block::Rectangle targetRect) noexcept :
        WriteClippedBufferBase{{}, targetRect}, _content{std::move(content)} {}
    /// Create a write-clipped buffer with the given content, source offset, and target rectangle.
    /// @param content The wrapped writable buffer.
    /// @param sourceOffset The top-left source coordinate exposed by this wrapper.
    /// @param targetRect The target rectangle in the wrapped buffer.
    WriteClippedBuffer(WritableBufferPtr content, block::Position sourceOffset, block::Rectangle targetRect) noexcept :
        WriteClippedBufferBase{sourceOffset, targetRect}, _content{std::move(content)} {}

    // defaults
    ~WriteClippedBuffer() override = default;
    WriteClippedBuffer(const WriteClippedBuffer &) = default;
    WriteClippedBuffer(WriteClippedBuffer &&) = default;
    auto operator=(const WriteClippedBuffer &) -> WriteClippedBuffer & = default;
    auto operator=(WriteClippedBuffer &&) -> WriteClippedBuffer & = default;

public: // implement ReadableBuffer
    /// Read a block from the wrapped buffer.
    /// @param pos The source position.
    /// @return The wrapped block, or a space if the translated target position is outside the wrapped buffer.
    [[nodiscard]] auto get(block::Position pos) const noexcept -> const Block & override {
        if (_content == nullptr) {
            return Block::space();
        }
        const auto targetPos = translateToTarget(pos);
        if (!_content->rect().contains(targetPos)) {
            return Block::space();
        }
        return _content->get(targetPos);
    }

public: // implement WritableBuffer
    /// Write a block into the wrapped buffer.
    /// @param pos The source coordinates.
    /// @param block The block to write.
    void set(block::Position pos, const Block &block) noexcept override {
        if (_content == nullptr) {
            return;
        }
        const auto targetPos = translateToTarget(pos);
        if (!canWriteBlock(pos, targetPos, block, _content->rect())) {
            return;
        }
        _content->set(targetPos, block);
    }

public:
    /// Access the wrapped writable buffer.
    /// @return The wrapped writable buffer pointer.
    [[nodiscard]] auto content() const noexcept -> const WritableBufferPtr & { return _content; }
    /// Replace the wrapped writable buffer.
    /// @param content The new wrapped writable buffer pointer.
    void setContent(WritableBufferPtr content) noexcept { _content = std::move(content); }

private: // implement WriteClippedBufferBase
    [[nodiscard]] auto targetBounds() const noexcept -> block::Rectangle override {
        return _content != nullptr ? _content->rect() : block::Rectangle{};
    }

private:
    WritableBufferPtr _content; ///< The wrapped writable buffer.
};

}
