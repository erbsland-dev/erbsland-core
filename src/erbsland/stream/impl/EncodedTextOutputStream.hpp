// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../ByteOutputStream.hpp"
#include "../TextOutputStream.hpp"

#include "../../text/StringBomMode.hpp"

namespace erbsland::stream::impl {

/// Text output stream that encodes text into a byte output stream.
/// @tested{EncodedTextStreamTest}
class EncodedTextOutputStream final : public TextOutputStream {
public:
    /// Create an encoded text output stream.
    /// @param byteOutputStream The byte stream to write to.
    /// @param encoding The configured text encoding.
    /// @param bomMode How byte order marks are written.
    /// @throws err::StreamError If `byteOutputStream` is empty.
    explicit EncodedTextOutputStream(
        ByteOutputStreamPtr byteOutputStream,
        text::StringEncoding encoding,
        text::StringBomMode bomMode = text::StringBomMode::Automatic);

    // defaults
    ~EncodedTextOutputStream() override = default;
    EncodedTextOutputStream(const EncodedTextOutputStream &) = delete;
    EncodedTextOutputStream(EncodedTextOutputStream &&) = delete;
    auto operator=(const EncodedTextOutputStream &) -> EncodedTextOutputStream & = delete;
    auto operator=(EncodedTextOutputStream &&) -> EncodedTextOutputStream & = delete;

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
    [[nodiscard]] auto bomModeForNextWrite() noexcept -> text::StringBomMode;
    [[nodiscard]] static auto effectiveEncodingFor(text::StringEncoding encoding) noexcept -> text::StringEncoding;

private:
    ByteOutputStreamPtr _byteOutputStream;
    text::StringEncoding _encoding{text::StringEncoding::Utf8};
    text::StringEncoding _effectiveEncoding{text::StringEncoding::Utf8};
    text::StringBomMode _bomMode{text::StringBomMode::Automatic};
    bool _bomWritten{false};
};

}
