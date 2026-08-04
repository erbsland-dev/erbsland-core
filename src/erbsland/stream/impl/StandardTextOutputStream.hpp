// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "NativeOutputStream.hpp"
#include "StandardTextOutputStreamData_fwd.hpp"

#include "../TextOutputStream.hpp"

#include <memory>

namespace erbsland::stream::impl {

/// Text output stream implementation for the process standard streams.
/// @tested{StandardTextOutputStreamTest StandardStreamsTest}
class StandardTextOutputStream final : public TextOutputStream {
public:
    /// Create a standard text output stream.
    /// @param nativeOutputStream The native stream to write to.
    /// @throws stream::StreamError If `nativeOutputStream` is empty.
    explicit StandardTextOutputStream(NativeOutputStreamPtr nativeOutputStream);

    ~StandardTextOutputStream() override { abort(); }

    // defaults/deletions
    StandardTextOutputStream(const StandardTextOutputStream &) = delete;
    StandardTextOutputStream(StandardTextOutputStream &&) = delete;
    auto operator=(const StandardTextOutputStream &) -> StandardTextOutputStream & = delete;
    auto operator=(StandardTextOutputStream &&) -> StandardTextOutputStream & = delete;

public: // implement TextOutputStream
    [[nodiscard]] auto encoding() const noexcept -> text::StringEncoding override;
    [[nodiscard]] auto effectiveEncoding() const noexcept -> text::StringEncoding override;
    [[nodiscard]] auto outputSettings() const noexcept -> const OutputStreamSettings & override;
    [[nodiscard]] auto state() const noexcept -> StreamState override;
    [[nodiscard]] auto isReady() const noexcept -> bool override;
    [[nodiscard]] auto waitForReady() -> StreamWaitStatus override;
    auto flush() -> StreamWriteStatus override;
    auto close() -> StreamCloseStatus override;
    void abort() noexcept override;
    [[nodiscard]] auto createErrorContext() const noexcept -> StreamErrorContext override;
    auto write(text::Char character) -> StreamWriteStatus override;
    auto write(const text::String &text) -> StreamWriteStatus override;
    auto writeLine() -> StreamWriteStatus override;
    auto writeLine(const text::String &text) -> StreamWriteStatus override;

public:
    using TextOutputStream::write;
    using TextOutputStream::writeLine;

private:
    std::shared_ptr<StandardTextOutputStreamData> _data;
};

}
