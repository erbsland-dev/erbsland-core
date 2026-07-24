// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../text/String.hpp"
#include "../../unit/CpLength.hpp"

#include <deque>
#include <optional>
#include <span>

namespace erbsland::stream::impl {

/// Retains decoded text using one runtime-selected storage policy.
/// @tested{EncodedTextStreamTest}
class RetainedTextBuffer final {
public:
    /// Create an empty retained-text buffer.
    explicit RetainedTextBuffer(bool sensitive) : _sensitive{sensitive} {}

public: // accessors
    [[nodiscard]] auto isSensitive() const noexcept -> bool { return _sensitive; }
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return _length.isZero(); }
    [[nodiscard]] auto length() const noexcept -> unit::CpLength { return _length; }

public:
    /// Change the storage policy and discard all retained text.
    void setSensitive(bool sensitive) noexcept;
    /// Discard all retained text.
    void clear() noexcept;
    /// Append decoded text.
    /// @param text The decoded text to retain.
    /// @param textLength The character length already determined by the decoder.
    /// @param lineScanned Whether the decoder already scanned this text for a line ending.
    void append(text::String text, unit::CpLength textLength, bool lineScanned = false);
    /// Test whether a line result can be produced without more input.
    [[nodiscard]] auto lineIsComplete(unit::CpLength maximum) noexcept -> bool;
    /// Consume and return the first retained character.
    [[nodiscard]] auto takeChar() -> text::Char;
    /// Consume and return a prefix.
    [[nodiscard]] auto take(unit::CpLength maximum, bool line) -> text::String;

private:
    struct Chunk final {
        text::String text;
        unit::CpLength length;
    };

private:
    void scanForLineEnd(unit::CpLength maximum) noexcept;
    [[nodiscard]] auto takeLength(unit::CpLength maximum, bool line) noexcept -> unit::CpLength;
    void didConsume(unit::CpLength length) noexcept;
    void consumePrefix(unit::CpLength characterLength);
    [[nodiscard]] auto byteLength(unit::CpLength characterLength) const -> unit::ByteLength;
    void copyAndConsume(unit::CpLength characterLength, std::span<char> destination);

private:
    bool _sensitive{false};
    std::deque<Chunk> _chunks;
    unit::CpLength _length{};
    unit::CpLength _lineScanLength{};
    std::optional<unit::CpLength> _firstLineLength;
};

}
