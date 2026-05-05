// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../ByteInputStream.hpp"
#include "../TextInputStream.hpp"

#include "../../text/EncodingErrorMode.hpp"
#include "../../text/StringBomMode.hpp"
#include "../../text/StringBuilder.hpp"
#include "../../text/StringCharReader.hpp"

namespace erbsland::stream::impl {

/// Text input stream that decodes text from a byte input stream.
/// @tested{EncodedTextStreamTest}
class EncodedTextInputStream final : public TextInputStream {
public:
    /// Create an encoded text input stream.
    /// @param byteInputStream The byte stream to read from.
    /// @param encoding The configured text encoding.
    /// @param bomMode How byte order marks are handled.
    /// @param errorMode How decoding errors are handled.
    /// @throws err::StreamError If `byteInputStream` is empty.
    explicit EncodedTextInputStream(
        ByteInputStreamPtr byteInputStream,
        text::StringEncoding encoding,
        text::StringBomMode bomMode = text::StringBomMode::Automatic,
        text::EncodingErrorMode errorMode = text::EncodingErrorMode::Replace);

    // defaults
    ~EncodedTextInputStream() override = default;
    EncodedTextInputStream(const EncodedTextInputStream &) = delete;
    EncodedTextInputStream(EncodedTextInputStream &&) = delete;
    auto operator=(const EncodedTextInputStream &) -> EncodedTextInputStream & = delete;
    auto operator=(EncodedTextInputStream &&) -> EncodedTextInputStream & = delete;

public: // implement TextInputStream
    [[nodiscard]] auto encoding() const noexcept -> text::StringEncoding override;
    [[nodiscard]] auto effectiveEncoding() const noexcept -> text::StringEncoding override;
    [[nodiscard]] auto isOpen() const noexcept -> bool override;
    void close() override;
    [[nodiscard]] auto readChar() -> std::optional<text::Char> override;
    [[nodiscard]] auto read(unit::CpLength maximum) -> std::optional<text::String> override;
    [[nodiscard]] auto readLine(unit::CpLength maximum) -> std::optional<text::String> override;
    [[nodiscard]] auto readAll(unit::CpLength maximum) -> text::String override;

public:
    using TextInputStream::read;
    using TextInputStream::readAll;
    using TextInputStream::readLine;

private:
    void load();
    [[nodiscard]] auto readIntoBuilder(unit::CpLength maximum) -> std::optional<text::String>;
    [[nodiscard]] static auto resolveEffectiveEncoding(
        const mem::ByteBlock &data, text::StringEncoding encoding) noexcept -> text::StringEncoding;

private:
    ByteInputStreamPtr _byteInputStream;
    text::StringEncoding _encoding{text::StringEncoding::Utf8};
    text::StringEncoding _effectiveEncoding{text::StringEncoding::Utf8};
    text::StringBomMode _bomMode{text::StringBomMode::Automatic};
    text::EncodingErrorMode _errorMode{text::EncodingErrorMode::Replace};
    bool _loaded{false};
    text::String _text;
    text::StringCharReader _reader;
};

}
