// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UnsafeRingBufferAccess_fwd.hpp"

#include "../ByteSpan.hpp"
#include "../RingBuffer.hpp"

#include <array>
#include <functional>
#include <optional>
#include <span>

namespace erbsland::mem::impl {

/// Exclusive unsafe access lease for low-level APIs operating on a ring buffer.
/// @warning Do not use this class in regular user code.
/// @tested{RingBufferTest}
class UnsafeRingBufferAccess final {
public:
    /// Create an exclusive unsafe access lease.
    explicit UnsafeRingBufferAccess(RingBuffer &buffer) : _buffer{buffer} { buffer.beginUnsafeAccess(); }

    // defaults/deletions
    ~UnsafeRingBufferAccess() { release(); }
    UnsafeRingBufferAccess(const UnsafeRingBufferAccess &) = delete;
    UnsafeRingBufferAccess(UnsafeRingBufferAccess &&other) noexcept : _buffer{other._buffer} { other._buffer.reset(); }
    auto operator=(const UnsafeRingBufferAccess &) -> UnsafeRingBufferAccess & = delete;
    auto operator=(UnsafeRingBufferAccess &&) -> UnsafeRingBufferAccess & = delete;

public:
    /// Get the readable sections around the wrap point.
    [[nodiscard]] auto readableSpans() const noexcept -> std::array<ConstByteSpan, 2> {
        return buffer().readableSpans();
    }
    /// Get the writable sections around the wrap point.
    [[nodiscard]] auto writableSpans() noexcept -> std::array<ByteSpan, 2> { return buffer().writableSpans(); }
    /// Make bytes written through this lease visible to readers.
    void commitWritten(const unit::ByteLength length) { buffer().commitWritten(length.toSizeTOrThrow()); }
    /// Consume bytes read through this lease.
    void consumeRead(const unit::ByteLength length) { buffer().consumeRead(length.toSizeTOrThrow()); }

private:
    /// Access the leased ring buffer.
    [[nodiscard]] auto buffer() const noexcept -> RingBuffer & { return _buffer->get(); }
    /// Release the lease if it is still active.
    void release() noexcept {
        if (_buffer.has_value()) {
            _buffer->get().endUnsafeAccess();
            _buffer.reset();
        }
    }

private:
    std::optional<std::reference_wrapper<RingBuffer>> _buffer;
};

}
