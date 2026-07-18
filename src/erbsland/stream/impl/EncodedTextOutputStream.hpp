// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../ByteOutputStream.hpp"
#include "../TextOutputStream.hpp"

#include "../../text/EncodingErrorMode.hpp"
#include "../../text/StringBomMode.hpp"

#include <mutex>

namespace erbsland::stream::impl {

/// Text output stream that encodes text into a byte output stream.
/// @tested{EncodedTextStreamTest}
class EncodedTextOutputStream final : public TextOutputStream {
public:
    /// Create an encoded text output stream.
    /// @param byteOutputStream The byte stream to write to.
    /// @param encoding The configured text encoding.
    /// @param bomMode How byte order marks are written.
    /// @param errorMode How invalid source text is handled.
    /// @throws stream::StreamError If `byteOutputStream` is empty.
    explicit EncodedTextOutputStream(
        ByteOutputStreamPtr byteOutputStream,
        text::StringEncoding encoding,
        text::StringBomMode bomMode = text::StringBomMode::Automatic,
        text::EncodingErrorMode errorMode = text::EncodingErrorMode::Replace,
        bool initialBomAlreadyHandled = false);

    // defaults
    ~EncodedTextOutputStream() override { abort(); }
    EncodedTextOutputStream(const EncodedTextOutputStream &) = delete;
    EncodedTextOutputStream(EncodedTextOutputStream &&) = delete;
    auto operator=(const EncodedTextOutputStream &) -> EncodedTextOutputStream & = delete;
    auto operator=(EncodedTextOutputStream &&) -> EncodedTextOutputStream & = delete;

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

public: // implement StreamPositioning
    [[nodiscard]] auto supportsPositioning() const noexcept -> bool override;
    [[nodiscard]] auto position() const -> unit::ByteIndex override;
    auto setPosition(unit::ByteIndex position) -> StreamPositionStatus override;
    auto movePosition(StreamPositionOrigin origin, unit::ByteOffset offset) -> StreamPositionStatus override;

public:
    using TextOutputStream::write;
    using TextOutputStream::writeLine;

private:
    [[nodiscard]] auto bomModeForNextWrite() const noexcept -> text::StringBomMode;
    auto writeLocked(const text::String &text) -> StreamWriteStatus;

private:
    mutable std::mutex _mutex;
    ByteOutputStreamPtr _byteOutputStream;
    text::StringEncoding _encoding{text::StringEncoding::Utf8};
    text::StringEncoding _effectiveEncoding{text::StringEncoding::Utf8};
    text::StringBomMode _bomMode{text::StringBomMode::Automatic};
    text::EncodingErrorMode _errorMode{text::EncodingErrorMode::Replace};
    bool _bomWritten{false};
};

}
