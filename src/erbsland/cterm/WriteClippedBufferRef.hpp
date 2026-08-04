// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "WriteClippedBufferBase.hpp"

namespace erbsland::cterm {

/// A write-clipped buffer that stores a reference to the wrapped buffer.
///
/// This wrapper is intended as a lightweight temporary object on the stack.
class WriteClippedBufferRef final : public WriteClippedBufferBase {
public:
    using WriteClippedBufferBase::drawBitmap;
    using WriteClippedBufferBase::drawBlockText;
    using WriteClippedBufferBase::fill;
    using WriteClippedBufferBase::get;
    using WriteClippedBufferBase::resize;
    using WriteClippedBufferBase::set;

public:
    /// Create a write-clipped buffer reference with the given visible size.
    /// @param buffer The wrapped writable buffer.
    /// @param size The visible source size.
    WriteClippedBufferRef(WritableBuffer &buffer, bgeo::BlockSize size) noexcept :
        WriteClippedBufferBase{{}, bgeo::BlockRectangle{{}, size}}, _buffer{buffer} {}
    /// Create a write-clipped buffer reference with the given target rectangle.
    /// @param buffer The wrapped writable buffer.
    /// @param targetRect The target rectangle in the wrapped buffer.
    WriteClippedBufferRef(WritableBuffer &buffer, bgeo::BlockRectangle targetRect) noexcept :
        WriteClippedBufferBase{{}, targetRect}, _buffer{buffer} {}
    /// Create a write-clipped buffer reference with the given source offset and target rectangle.
    /// @param buffer The wrapped writable buffer.
    /// @param sourceOffset The top-left source coordinate exposed by this wrapper.
    /// @param targetRect The target rectangle in the wrapped buffer.
    WriteClippedBufferRef(
        WritableBuffer &buffer, bgeo::BlockPosition sourceOffset, bgeo::BlockRectangle targetRect) noexcept :
        WriteClippedBufferBase{sourceOffset, targetRect}, _buffer{buffer} {}

    // defaults
    ~WriteClippedBufferRef() override = default;

    // delete copy/assign/move
    WriteClippedBufferRef(const WriteClippedBufferRef &) = delete;
    WriteClippedBufferRef(WriteClippedBufferRef &&) = delete;
    auto operator=(const WriteClippedBufferRef &) -> WriteClippedBufferRef & = delete;
    auto operator=(WriteClippedBufferRef &&) -> WriteClippedBufferRef & = delete;

public: // implement ReadableBuffer
    /// Read a block from the wrapped buffer.
    /// @param pos The source position.
    /// @return The wrapped block, or a space if the translated target position is outside the wrapped buffer.
    [[nodiscard]] auto get(bgeo::BlockPosition pos) const noexcept -> const Block & override {
        const auto targetPos = translateToTarget(pos);
        if (!_buffer.rect().contains(targetPos)) {
            return Block::space();
        }
        return _buffer.get(targetPos);
    }

public: // implement WritableBuffer
    /// Write a block into the wrapped buffer.
    /// @param pos The source position.
    /// @param block The block to write.
    void set(bgeo::BlockPosition pos, const Block &block) noexcept override {
        const auto targetPos = translateToTarget(pos);
        if (!canWriteBlock(pos, targetPos, block, _buffer.rect())) {
            return;
        }
        _buffer.set(targetPos, block);
    }

private: // implement WriteClippedBufferBase
    [[nodiscard]] auto targetBounds() const noexcept -> bgeo::BlockRectangle override { return _buffer.rect(); }

private:
    WritableBuffer &_buffer; ///< The wrapped writable buffer.
};

}
