// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UnsafeDecodeBufferAccess_fwd.hpp"

#include "../StringDecodeBuffer.hpp"

#include <span>
#include <utility>

namespace erbsland::text::impl {

/// Unsafe write access for `StringDecodeBuffer`.
/// @warning Do not use this class in user code!
/// @tested{StringDecodeBufferTest}
class UnsafeDecodeBufferAccess final {
public:
    /// Create unsafe access for a decode buffer.
    explicit UnsafeDecodeBufferAccess(StringDecodeBuffer &buffer) noexcept : _buffer{buffer} {}

public:
    /// Access the first contiguous writable span.
    [[nodiscard]] auto writableSpan() noexcept -> mem::ByteSpan { return _buffer.writableSpan(); }
    /// Commit bytes written through `writableSpan()`.
    void commitWritten(unit::ByteLength length) { _buffer.commitWritten(length); }
    /// Get bytes consumed since the last reset.
    [[nodiscard]] auto consumedByteLength() const noexcept -> unit::ByteLength { return _buffer.consumedByteLength(); }
    /// Test if initial BOM handling has completed.
    [[nodiscard]] auto isBomResolved() const noexcept -> bool { return _buffer.isBomResolved(); }
    /// Reset decoding at a nonzero continuation position.
    void resetForContinuation(StringEncoding effectiveEncoding) noexcept {
        _buffer.resetForContinuation(effectiveEncoding);
    }
    /// Decode and consume text while retaining the already determined character count.
    /// @param maximum The maximum number of characters to decode.
    /// @param stopAtLineEnd Whether decoding stops after a line ending.
    /// @return The decoded text and its character count.
    [[nodiscard]] auto takeStringWithLength(unit::CpLength maximum, bool stopAtLineEnd)
        -> std::pair<String, unit::CpLength> {
        return _buffer.takeStringWithLength(maximum, stopAtLineEnd);
    }

private:
    StringDecodeBuffer &_buffer; ///< The accessed buffer.
};

}
#include "../../mem/ByteSpan.hpp"
