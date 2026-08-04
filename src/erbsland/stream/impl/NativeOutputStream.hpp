// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "NativeOutputStream_fwd.hpp"
#include "NativeStandardStream.hpp"
#include "NativeStreamOwnership.hpp"

#include "../ByteInputStream.hpp"
#include "../StreamErrorSource.hpp"

#include "../../text/StringEditor.hpp"

#include <cstdint>
#include <span>

namespace erbsland::stream::impl {

/// Base class for native output stream adapters.
/// @tested{StandardTextOutputStreamTest PosixNativeStreamTest WindowsNativeStreamTest}
class NativeOutputStream : public virtual StreamErrorSource {
public:
    // defaults
    ~NativeOutputStream() override = default;

public:
    /// Write raw bytes to the native stream.
    /// @param bytes The bytes to write.
    /// @throws stream::StreamError If the native stream cannot write all bytes.
    virtual void writeBytes(std::span<const char> bytes) = 0;
    /// Write UTF-8 text to the native stream.
    /// @param text The text to write.
    /// @throws stream::StreamError If the native stream cannot write all text.
    virtual void writeText(const text::String &text);
    /// Flush the native stream.
    /// @throws stream::StreamError If the native stream cannot flush.
    virtual void flush() = 0;
    /// Abort pending native output without waiting.
    virtual void abort() noexcept = 0;
};

/// Create a borrowed native wrapper for one of the process standard output streams.
/// @param stream The standard stream to wrap.
/// @return A native output stream wrapper.
/// @throws stream::StreamError If the process standard stream is not available.
[[nodiscard]] auto createNativeStandardOutputStream(NativeStandardStream stream) -> NativeOutputStreamPtr;
/// Create a borrowed native wrapper for the process standard input stream.
/// @return A native input stream wrapper.
/// @throws stream::StreamError If the process standard input stream is not available.
[[nodiscard]] auto createNativeStandardInputStream() -> ByteInputStreamPtr;

}
