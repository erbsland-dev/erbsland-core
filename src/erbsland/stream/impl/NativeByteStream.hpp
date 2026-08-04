// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "NativeByteStream_fwd.hpp"

#include "../StreamErrorSource.hpp"
#include "../StreamPositionOrigin.hpp"

#include "../../mem/Byte.hpp"
#include "../../unit/ByteIndex.hpp"
#include "../../unit/ByteLength.hpp"
#include "../../unit/ByteOffset.hpp"

#include <span>

namespace erbsland::stream::impl {

/// Internal synchronous adapter for a native byte stream.
/// Calls may block in the operating system. Only the shared I/O service calls this interface; public stream methods
/// use buffered wrappers.
/// @notest{Platform adapters and buffered wrappers have dedicated behavior tests.}
class NativeByteStream : public virtual StreamErrorSource {
public:
    // defaults
    ~NativeByteStream() override = default;

public:
    /// Test if the native stream supports byte positioning.
    [[nodiscard]] virtual auto supportsPositioning() const noexcept -> bool = 0;
    /// Get the native byte position.
    [[nodiscard]] virtual auto position() const -> unit::ByteIndex = 0;
    /// Set the native byte position and return the resulting position.
    virtual auto setPosition(unit::ByteIndex position) -> unit::ByteIndex = 0;
    /// Move the native byte position and return the resulting position.
    virtual auto movePosition(StreamPositionOrigin origin, unit::ByteOffset offset) -> unit::ByteIndex = 0;
    /// Read bytes synchronously from the native source.
    /// @param destination The writable destination buffer.
    /// @return The number of bytes read, or zero at end-of-stream.
    [[nodiscard]] virtual auto read(mem::ByteSpan destination) -> unit::ByteLength = 0;
    /// Write all bytes synchronously to the native target.
    /// @param bytes The bytes to write completely before returning.
    virtual void write(mem::ConstByteSpan bytes) = 0;
    /// Flush native output synchronously.
    virtual void flush() = 0;
    /// Close the native stream synchronously.
    virtual void close() = 0;
    /// Abandon pending native operations without waiting.
    virtual void abort() noexcept = 0;
};

}
#include "../../mem/ByteSpan.hpp"
