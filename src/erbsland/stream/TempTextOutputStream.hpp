// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TempTextOutputStream_fwd.hpp"
#include "TextOutputStream.hpp"

#include "../path/Path.hpp"

namespace erbsland::stream {

/// A text output stream for a temporary file.
///
/// A successful close removes the file synchronously. Destruction never closes or waits: it aborts immediately and
/// schedules removal, unless `release()` was called or automatic cleanup was disabled.
/// @tested{PathTemporaryTest}
class TempTextOutputStream final : public TextOutputStream {
    friend class path::PathOperations;

public:
    /// Create an empty temporary text output stream.
    TempTextOutputStream() = default;
    /// Abort without waiting and schedule cleanup of the temporary file.
    ~TempTextOutputStream() override;

    // defaults
    TempTextOutputStream(const TempTextOutputStream &) = delete;
    TempTextOutputStream(TempTextOutputStream &&) = delete;
    auto operator=(const TempTextOutputStream &) -> TempTextOutputStream & = delete;
    auto operator=(TempTextOutputStream &&) -> TempTextOutputStream & = delete;

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

public: // implement OutputStream / TextOutputStream
    using StreamErrorSource::throwError;

    [[nodiscard]] auto outputSettings() const noexcept -> const OutputStreamSettings & override;
    [[nodiscard]] auto state() const noexcept -> StreamState override;
    [[nodiscard]] auto isReady() const noexcept -> bool override;
    [[nodiscard]] auto waitForReady() -> StreamWaitStatus override;
    auto flush() -> StreamWriteStatus override;
    auto close() -> StreamCloseStatus override;
    void abort() noexcept override;
    [[nodiscard]] auto createErrorContext() const noexcept -> StreamErrorContext override;
    [[nodiscard]] auto encoding() const noexcept -> text::StringEncoding override;
    [[nodiscard]] auto effectiveEncoding() const noexcept -> text::StringEncoding override;
    auto write(text::Char character) -> StreamWriteStatus override;
    auto write(const text::String &text) -> StreamWriteStatus override;
    auto writeLine() -> StreamWriteStatus override;
    auto writeLine(const text::String &text) -> StreamWriteStatus override;

private:
    /// Create a temporary text output stream for an existing path and stream.
    TempTextOutputStream(path::Path path, TextOutputStreamPtr stream, bool removeOnClose);

private:
    [[noreturn]] void throwError(
        text::String title, text::String description, system::PlatformErrorContextConstPtr platformContext) const;

private:
    path::Path _path;
    TextOutputStreamPtr _stream;
    OutputStreamSettings _settings;
    bool _removeOnClose{true};
};

}
