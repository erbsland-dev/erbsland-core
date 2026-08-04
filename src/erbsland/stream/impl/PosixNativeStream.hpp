// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "NativeByteStream.hpp"
#include "NativeOutputStream.hpp"
#include "PosixNativeStream_fwd.hpp"

#include "../StreamErrorContext.hpp"

#include "../../system/PosixErrorContext.hpp"

#include <atomic>
#include <mutex>

namespace erbsland::stream::impl {

/// Native byte stream wrapper for POSIX file descriptors.
/// @tested{PosixNativeStreamTest}
class PosixNativeStream final : public NativeOutputStream, public NativeByteStream {
    friend class PosixNativeOperation;

public:
    /// Create a POSIX native stream wrapper.
    /// @param fileDescriptor The file descriptor to wrap.
    /// @param ownership If the wrapper owns the file descriptor.
    /// @param path The path represented by the descriptor, if available.
    explicit PosixNativeStream(int fileDescriptor, NativeStreamOwnership ownership, text::String path = {});

    ~PosixNativeStream() override;

    // defaults/deletions
    PosixNativeStream(const PosixNativeStream &) = delete;
    PosixNativeStream(PosixNativeStream &&) = delete;
    auto operator=(const PosixNativeStream &) -> PosixNativeStream & = delete;
    auto operator=(PosixNativeStream &&) -> PosixNativeStream & = delete;

public: // implement NativeOutputStream
    using StreamErrorSource::throwError;

    void writeBytes(std::span<const char> bytes) override;
    void flush() override;
    [[nodiscard]] auto createErrorContext() const noexcept -> StreamErrorContext override;

public: // implement NativeByteStream
    [[nodiscard]] auto supportsPositioning() const noexcept -> bool override;
    [[nodiscard]] auto position() const -> unit::ByteIndex override;
    auto setPosition(unit::ByteIndex position) -> unit::ByteIndex override;
    auto movePosition(StreamPositionOrigin origin, unit::ByteOffset offset) -> unit::ByteIndex override;
    void close() override;
    void abort() noexcept override;
    [[nodiscard]] auto read(mem::ByteSpan destination) -> unit::ByteLength override;
    void write(mem::ConstByteSpan bytes) override;

public:
    /// Test whether the stream still owns an open descriptor.
    [[nodiscard]] auto isOpen() const noexcept -> bool;

public:
    /// Get the current file size if this stream references a regular file.
    /// @return The file size in bytes, or zero for non-file streams.
    /// @throws stream::StreamError If the stream is closed or size lookup fails.
    [[nodiscard]] auto fileSize() const -> unit::ByteLength;

private:
    /// Throw a stream error with a POSIX error code.
    [[noreturn]] void throwError(
        text::String title, text::String description, system::PosixErrorContext::ErrorCode errorCode) const;
    /// Throw a stream error using the current errno value.
    [[noreturn]] void throwErrorFromErrno(text::String title, text::String description) const;
    /// Complete a native operation.
    void finishOperation() const noexcept;

private:
    std::atomic<int> _fileDescriptor{-1};                              ///< Wrapped POSIX file descriptor.
    NativeStreamOwnership _ownership{NativeStreamOwnership::Borrowed}; ///< Descriptor ownership mode.
    text::String _path;                                                ///< Stream path, when available.
    bool _supportsPositioning{false};                                  ///< Whether this descriptor can be positioned.
    mutable std::mutex _operationMutex;                                ///< Protects operation and deferred close state.
    mutable unsigned int _operationCount{0U};                          ///< Number of native calls using the descriptor.
    mutable int _deferredCloseDescriptor{-1};                          ///< Deferred owned descriptor.
};

}
#include "../../mem/ByteSpan.hpp"
