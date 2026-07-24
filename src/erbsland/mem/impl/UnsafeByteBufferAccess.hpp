// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UnsafeByteBufferAccess_fwd.hpp"

#include "../ByteBuffer.hpp"
#include "../ByteSpan.hpp"

namespace erbsland::mem::impl {

/// Gives low-level implementations explicit writable access to dynamic byte-buffer storage.
/// Returned spans borrow the buffer and become invalid when it is modified, moved, reset, or destroyed.
/// @warning Never retain or expose the returned span beyond the immediate low-level operation.
/// @tested{ByteBufferTest}
class UnsafeByteBufferAccess final {
public:
    /// Create scoped writable access to a buffer.
    explicit UnsafeByteBufferAccess(ByteBuffer &buffer) noexcept : _buffer{buffer} {}

    // defaults/deletions
    UnsafeByteBufferAccess() = delete;
    ~UnsafeByteBufferAccess() = default;
    UnsafeByteBufferAccess(const UnsafeByteBufferAccess &) = default;
    UnsafeByteBufferAccess(UnsafeByteBufferAccess &&) noexcept = default;
    auto operator=(const UnsafeByteBufferAccess &) -> UnsafeByteBufferAccess & = delete;
    auto operator=(UnsafeByteBufferAccess &&) -> UnsafeByteBufferAccess & = delete;

public:
    /// Access the complete writable visible storage.
    [[nodiscard]] auto writableData() noexcept -> ByteSpan { return _buffer.writableSpan(); }

private:
    ByteBuffer &_buffer; ///< The borrowed dynamic byte buffer.
};

}
