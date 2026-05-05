// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "InputStream.hpp"

#include "../text/Char.hpp"
#include "../text/String.hpp"
#include "../text/StringEncoding.hpp"
#include "../unit/CpLength.hpp"

#include <optional>

namespace erbsland::stream {

class TextInputStream;
using TextInputStreamPtr = std::shared_ptr<TextInputStream>;

/// A stream that reads decoded Unicode text.
/// Text input streams read `text::Char` values and `text::String` text. Bounded read methods take an explicit maximum;
/// the no-argument convenience methods use `cDefaultTextReadMaximum`.
/// @tested{EncodedTextStreamTest}
class TextInputStream : public InputStream {
public:
    /// The default maximum for no-argument text reads: 10 Mi code points.
    static constexpr auto cDefaultTextReadMaximum = unit::CpLength{10U * 1024U * 1024U};

public:
    ~TextInputStream() override = default;

public: // accessors
    /// Get the encoding configured for the stream.
    [[nodiscard]] virtual auto encoding() const noexcept -> text::StringEncoding = 0;
    /// Get the effective encoding used by the stream.
    /// For BOM-detecting encodings, this returns the resolved concrete byte order after it is known.
    [[nodiscard]] virtual auto effectiveEncoding() const noexcept -> text::StringEncoding = 0;

public: // default interface
    /// Read up to `cDefaultTextReadMaximum` decoded characters.
    /// @return The decoded text, or an empty optional at end-of-stream.
    /// @throws err::StreamError If the stream is closed or the backing source fails.
    /// @throws err::EncodingError If the stream was configured to throw on decoding errors and decoding fails.
    [[nodiscard]] auto read() -> std::optional<text::String>;
    /// Read one line using `cDefaultTextReadMaximum` as the maximum length.
    /// The returned line includes its line ending when one is present.
    /// @return The decoded line, or an empty optional at end-of-stream.
    /// @throws err::StreamError If the stream is closed or the backing source fails.
    /// @throws err::EncodingError If the stream was configured to throw on decoding errors and decoding fails.
    [[nodiscard]] auto readLine() -> std::optional<text::String>;
    /// Read all remaining text using `cDefaultTextReadMaximum` as the maximum length.
    /// @return Decoded text from the current position up to the default maximum or end-of-stream.
    /// @throws err::StreamError If the stream is closed or the backing source fails.
    /// @throws err::EncodingError If the stream was configured to throw on decoding errors and decoding fails.
    [[nodiscard]] auto readAll() -> text::String;

public: // core interface
    /// Read one decoded character.
    /// @return The next character, or an empty optional at end-of-stream.
    /// @throws err::StreamError If the stream is closed or the backing source fails.
    /// @throws err::EncodingError If the stream was configured to throw on decoding errors and decoding fails.
    [[nodiscard]] virtual auto readChar() -> std::optional<text::Char> = 0;
    /// Read up to `maximum` decoded characters.
    /// @param maximum The maximum number of decoded characters to read.
    /// @return The decoded text, or an empty optional at end-of-stream.
    /// @throws err::StreamError If the stream is closed or the backing source fails.
    /// @throws err::EncodingError If the stream was configured to throw on decoding errors and decoding fails.
    [[nodiscard]] virtual auto read(unit::CpLength maximum) -> std::optional<text::String> = 0;
    /// Read one line.
    /// The returned line includes its line ending when one is present.
    /// @param maximum The maximum number of decoded characters to read.
    /// @return The next line, or an empty optional at end-of-stream.
    /// @throws err::StreamError If the stream is closed or the backing source fails.
    /// @throws err::EncodingError If the stream was configured to throw on decoding errors and decoding fails.
    [[nodiscard]] virtual auto readLine(unit::CpLength maximum) -> std::optional<text::String> = 0;
    /// Read all remaining text.
    /// @param maximum The maximum number of decoded characters to read.
    /// @return Decoded text from the current position up to the maximum or end-of-stream.
    /// @throws err::StreamError If the stream is closed or the backing source fails.
    /// @throws err::EncodingError If the stream was configured to throw on decoding errors and decoding fails.
    [[nodiscard]] virtual auto readAll(unit::CpLength maximum) -> text::String = 0;
};

}
