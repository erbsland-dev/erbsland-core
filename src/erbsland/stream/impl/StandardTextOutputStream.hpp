// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "NativeOutputStream.hpp"

#include "../TextOutputStream.hpp"

#include <mutex>

namespace erbsland::stream::impl {

/// Text output stream implementation for the process standard streams.
/// @tested{StandardTextOutputStreamTest StandardStreamsTest}
class StandardTextOutputStream final : public TextOutputStream {
public:
    /// Create a standard text output stream.
    /// @param nativeOutputStream The native stream to write to.
    /// @throws err::StreamError If `nativeOutputStream` is empty.
    explicit StandardTextOutputStream(NativeOutputStreamPtr nativeOutputStream);

    // defaults
    ~StandardTextOutputStream() override = default;
    StandardTextOutputStream(const StandardTextOutputStream &) = delete;
    StandardTextOutputStream(StandardTextOutputStream &&) = delete;
    auto operator=(const StandardTextOutputStream &) -> StandardTextOutputStream & = delete;
    auto operator=(StandardTextOutputStream &&) -> StandardTextOutputStream & = delete;

public: // implement TextOutputStream
    [[nodiscard]] auto encoding() const noexcept -> text::StringEncoding override;
    [[nodiscard]] auto effectiveEncoding() const noexcept -> text::StringEncoding override;
    [[nodiscard]] auto isOpen() const noexcept -> bool override;
    void flush() override;
    void close() override;
    void write(text::Char character) override;
    void write(const text::StringView &text) override;
    void writeLine() override;
    void writeLine(const text::StringView &text) override;

public:
    using TextOutputStream::write;
    using TextOutputStream::writeLine;

private:
    NativeOutputStreamPtr _nativeOutputStream;
    mutable std::mutex _mutex;
};

}
