// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../text/String.hpp"
#include "../../unit/CpCount.hpp"

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
    [[nodiscard]] auto length() const noexcept -> unit::CpCount { return _length; }

public:
    /// Change the storage policy and discard all retained text.
    void setSensitive(bool sensitive) noexcept;
    /// Discard all retained text.
    void clear() noexcept;
    /// Append decoded text.
    /// @param text The decoded text to retain. Non-empty text must match the retained stream sensitivity policy.
    /// @param textLength The character length determined by the decoder. Must be zero exactly when `text` is empty.
    /// @param lineScanned Whether the decoder already scanned this text for a line ending.
    void append(text::String text, unit::CpCount textLength, bool lineScanned = false);
    /// Test whether a line result can be produced without more input.
    [[nodiscard]] auto lineIsComplete(unit::CpCount maximum) noexcept -> bool;
    /// Consume and return the first retained character.
    [[nodiscard]] auto takeChar() -> text::Char;
    /// Consume and return a prefix.
    [[nodiscard]] auto take(unit::CpCount maximum, bool line) -> text::String;

private:
    /// Stores one retained decoded-text chunk.
    struct Chunk final {
        text::String text;
        unit::CpCount length;
    };

private:
    /// Scan retained text for the next line ending.
    void scanForLineEnd(unit::CpCount maximum) noexcept;
    /// Determine how many retained characters can be consumed.
    [[nodiscard]] auto takeLength(unit::CpCount maximum, bool line) noexcept -> unit::CpCount;
    /// Update retained-text state after consuming characters.
    void didConsume(unit::CpCount length) noexcept;
    /// Discard a character prefix from retained chunks.
    void consumePrefix(unit::CpCount characterLength);
    /// Get the byte length for a retained character prefix.
    [[nodiscard]] auto byteLength(unit::CpCount characterLength) const -> unit::ByteCount;
    /// Copy and discard a retained character prefix.
    void copyAndConsume(unit::CpCount characterLength, std::span<char> destination);

private:
    bool _sensitive{false};
    std::deque<Chunk> _chunks;
    unit::CpCount _length{};
    unit::CpCount _lineScanLength{};
    std::optional<unit::CpCount> _firstLineLength;
};

}
