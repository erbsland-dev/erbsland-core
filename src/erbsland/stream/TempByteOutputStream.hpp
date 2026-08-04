// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ByteOutputStream.hpp"
#include "TempByteOutputStream_fwd.hpp"

#include "../path/Path.hpp"

namespace erbsland::stream {

/// A byte output stream for a temporary file.
///
/// A successful close removes the file synchronously. Destruction never closes or waits: it aborts immediately and
/// schedules removal, unless `release()` was called or automatic cleanup was disabled.
/// @tested{PathTemporaryTest}
class TempByteOutputStream final : public ByteOutputStream {
    friend class path::PathOperations;

public:
    /// Create an empty temporary byte output stream.
    TempByteOutputStream() = default;
    /// Abort without waiting and schedule cleanup of the temporary file.
    ~TempByteOutputStream() override;

    // defaults
    TempByteOutputStream(const TempByteOutputStream &) = delete;
    TempByteOutputStream(TempByteOutputStream &&) = delete;
    auto operator=(const TempByteOutputStream &) -> TempByteOutputStream & = delete;
    auto operator=(TempByteOutputStream &&) -> TempByteOutputStream & = delete;

public: // accessors
    /// Test if this stream has no temporary file path.
    [[nodiscard]] auto isEmpty() const noexcept -> bool;
    /// Access the temporary file path.
    [[nodiscard]] auto path() const noexcept -> const path::Path &;
    /// Test if the file is removed when the stream is closed or destroyed.
    [[nodiscard]] auto removeOnClose() const noexcept -> bool;
    /// Set whether to remove the file when the stream is closed or destroyed.
    void setRemoveOnClose(bool value) noexcept;
    /// Disable automatic cleanup and return the file path.
    [[nodiscard]] auto release() noexcept -> path::Path;

public: // implement OutputStream / ByteOutputStream
    using StreamErrorSource::throwError;

    [[nodiscard]] auto outputSettings() const noexcept -> const OutputStreamSettings & override;
    [[nodiscard]] auto state() const noexcept -> StreamState override;
    [[nodiscard]] auto isReady() const noexcept -> bool override;
    [[nodiscard]] auto waitForReady() -> StreamWaitStatus override;
    auto flush() -> StreamWriteStatus override;
    auto close() -> StreamCloseStatus override;
    void abort() noexcept override;
    [[nodiscard]] auto createErrorContext() const noexcept -> StreamErrorContext override;
    [[nodiscard]] auto endianness() const noexcept -> mem::Endianness override;
    void setEndianness(mem::Endianness endianness) noexcept override;
    auto write(mem::ConstByteSpan bytes) -> StreamWriteStatus override;

private:
    /// Create a temporary byte output stream for an existing path and stream.
    TempByteOutputStream(path::Path path, ByteOutputStreamPtr stream, bool removeOnClose);

private:
    /// Throw a stream error with the captured platform context.
    [[noreturn]] void throwError(
        text::String title, text::String description, system::PlatformErrorContextConstPtr platformContext) const;

private:
    path::Path _path;
    ByteOutputStreamPtr _stream;
    OutputStreamSettings _settings;
    bool _removeOnClose{true};
};

}
#include "../mem/ByteSpan.hpp"
