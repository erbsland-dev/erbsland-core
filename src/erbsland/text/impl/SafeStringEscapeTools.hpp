// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EscapeFormatter.hpp"

#include "../AnyStringBuilder_fwd.hpp"
#include "../Char.hpp"
#include "../SafeStringFlag.hpp"

#include "../../unit/CpLength.hpp"

#include <cstddef>
#include <vector>

namespace erbsland::text::impl {

/// Shared limit-aware escaping state for `toSafeString()`.
/// @tested{StringTransformTest}
class SafeStringEscapeTools final {
    /// Stores an escaped source-text fragment and its output metadata.
    struct Chunk {
        std::vector<Char> text;
        std::size_t sourceStart{};
        bool quoteNeeded{};
    };

public:
    /// Create a scanner for the requested output width and options.
    explicit SafeStringEscapeTools(unit::CpLength maximumWidth, SafeStringFlags flags);

public:
    /// Add one decoded source character.
    ///
    /// @param character The decoded character, already passed through tolerant replacement if needed.
    /// @param sourceStart The source data-unit offset where this character starts.
    /// @return `true` if scanning can continue, or `false` if the output width was reached.
    auto add(Char character, std::size_t sourceStart) -> bool;
    /// Finish scanning after the input was consumed.
    void finish(std::size_t sourceEnd) noexcept;
    /// Append the final bounded escaped representation to the given builder.
    void appendTo(AnyStringBuilder &builder, std::size_t sourceLength) const;

private:
    /// Create an escaped fragment for one character.
    [[nodiscard]] auto escapedChunk(Char character) const -> Chunk;
    /// Get the count of fragments fitting an output budget.
    [[nodiscard]] auto selectedChunkCount(std::size_t budget) const noexcept -> std::size_t;
    /// Test if selected fragments require enclosing quotes.
    [[nodiscard]] auto needsQuotes(std::size_t chunkCount) const noexcept -> bool;
    /// Get the escaped-body length of selected fragments.
    [[nodiscard]] auto bodyLength(std::size_t chunkCount) const noexcept -> std::size_t;

    /// Get the number of decimal digits in a value.
    [[nodiscard]] static auto decimalLength(std::size_t value) noexcept -> std::size_t;
    /// Get the output length of a truncation suffix.
    [[nodiscard]] static auto suffixLength(std::size_t remainingUnits) noexcept -> std::size_t;
    /// Create the truncation suffix for remaining input units.
    [[nodiscard]] static auto suffix(std::size_t remainingUnits) -> std::vector<Char>;

private:
    unit::CpLength _maximumWidth;
    SafeStringFlags _flags;
    EscapeAmount _amount;
    EscapeFormatterPtr _formatter;
    std::vector<Chunk> _chunks;
    bool _truncated{};
    std::size_t _outputLength{};
    std::size_t _remainingStart{};
};

}
