// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../ByteInputStream.hpp"
#include "../StreamErrorSource.hpp"

#include "../../text/StringEditor.hpp"

#include <cstdint>
#include <memory>
#include <span>

namespace erbsland::stream::impl {

class NativeOutputStream;
using NativeOutputStreamPtr = std::shared_ptr<NativeOutputStream>;

/// The standard output stream to wrap.
enum class NativeStandardStream : uint8_t {
    Out, ///< Process standard output.
    Err, ///< Process standard error.
};

/// Defines if a native stream wrapper owns the native handle.
enum class NativeStreamOwnership : uint8_t {
    Borrowed, ///< Do not close the native handle.
    Owned,    ///< Close the native handle when the wrapper is destroyed.
};

/// Base class for native output stream adapters.
/// @tested{StandardTextOutputStreamTest PosixNativeStreamTest WindowsNativeStreamTest}
class NativeOutputStream : public virtual StreamErrorSource {
public:
    virtual ~NativeOutputStream() = default;

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
