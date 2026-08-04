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
    /// Test if retained text is securely erased on release.
    [[nodiscard]] auto isSensitive() const noexcept -> bool { return _sensitive; }
    /// Test if no decoded text is retained.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return _length.isZero(); }
    /// Get the total retained character length.
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
    /// @throws err::LogicError If the decoder result violates the retained-text invariants.
    void append(text::String text, unit::CpLength textLength, bool lineScanned = false);
    /// Test whether a line result can be produced without more input.
    [[nodiscard]] auto lineIsComplete(unit::CpLength maximum) noexcept -> bool;
    /// Consume and return the first retained character.
    [[nodiscard]] auto takeChar() -> text::Char;
    /// Consume and return a prefix.
    [[nodiscard]] auto take(unit::CpLength maximum, bool line) -> text::String;

private:
    /// Stores one retained decoded-text chunk.
    struct Chunk final {
        text::String text;
        unit::CpLength length;
    };

private:
    /// Scan retained text for the next line ending.
    void scanForLineEnd(unit::CpLength maximum) noexcept;
    /// Determine how many retained characters can be consumed.
    [[nodiscard]] auto takeLength(unit::CpLength maximum, bool line) noexcept -> unit::CpLength;
    /// Update retained-text state after consuming characters.
    void didConsume(unit::CpLength length) noexcept;
    /// Discard a character prefix from retained chunks.
    void consumePrefix(unit::CpLength characterLength);
    /// Get the byte length for a retained character prefix.
    [[nodiscard]] auto byteLength(unit::CpLength characterLength) const -> unit::ByteLength;
    /// Copy and discard a retained character prefix.
    void copyAndConsume(unit::CpLength characterLength, std::span<char> destination);

private:
    bool _sensitive{false};
    std::deque<Chunk> _chunks;
    unit::CpLength _length{};
    unit::CpLength _lineScanLength{};
    std::optional<unit::CpLength> _firstLineLength;
};

}
