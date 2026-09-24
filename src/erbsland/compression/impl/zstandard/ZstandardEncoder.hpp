// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ZstandardMatch.hpp"
#include "ZstandardSequence.hpp"
#include "ZstandardSequenceCode.hpp"
#include "ZstandardSequenceTable.hpp"

#include "../../../mem/ByteBuffer.hpp"
#include "../../../mem/ByteSpan.hpp"
#include "../../CompressionLevel.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <vector>

namespace erbsland::compression::impl {

/// Dependency-free LZ77 encoder for Zstandard compressed blocks.
/// @tested{ZstandardInternalTest}
class ZstandardEncoder final {
    static constexpr auto cNoPosition = std::numeric_limits<uint32_t>::max(); ///< Empty chain marker.

public:
    /// Create an encoder over a complete input frame.
    ZstandardEncoder(mem::ConstByteSpan input, CompressionLevel level, std::size_t windowSize);

public:
    /// Rebase retained indexes and attach the next bounded input window.
    void setInput(mem::ConstByteSpan input, std::size_t removed);

public: // encode
    /// Build a compressed-block candidate while retaining this block in match history.
    /// The returned view remains valid until the next encodeBlock() call or encoder destruction.
    [[nodiscard]] auto encodeBlock(std::size_t begin, std::size_t size) -> std::optional<mem::ConstByteSpan>;

private:
    /// Find the best hash-chain match at a position.
    [[nodiscard]] auto findMatch(std::size_t position, std::size_t end) const noexcept -> ZstandardMatch;
    /// Add one position to the hash chain.
    void insertPosition(std::size_t position) noexcept;
    /// Add positions up to an exclusive absolute input offset.
    void insertUntil(std::size_t end) noexcept;
    /// Compute the four-byte match hash.
    [[nodiscard]] auto hashAt(std::size_t position) const noexcept -> std::size_t;
    /// Parse a block into sequences and concatenated literals.
    void parseBlock(
        std::size_t begin, std::size_t end, std::vector<ZstandardSequence> &sequences, mem::ByteBuffer &literals);
    /// Populate symbols and extra values for one sequence.
    static void encodeValues(ZstandardSequence &sequence);
    /// Encode a literal or match length into its format code.
    static void encodeLength(
        uint32_t value,
        uint32_t directMaximum,
        std::span<const uint32_t> bases,
        std::span<const uint8_t> bits,
        uint8_t symbolOffset,
        uint8_t &symbol,
        uint8_t &extraBits,
        uint32_t &extra);
    /// Append a raw or RLE literals section.
    void appendLiterals(mem::ByteBuffer &output, const mem::ByteBuffer &literals) const;
    /// Append a raw or RLE literals section without entropy coding.
    static void appendRawLiterals(mem::ByteBuffer &output, const mem::ByteBuffer &literals, bool repeated);
    /// Append an encoded sequence count.
    static void appendSequenceCount(mem::ByteBuffer &output, std::size_t count);
    /// Select RLE mode when all symbols are equal, otherwise predefined mode.
    [[nodiscard]] static auto selectTable(
        ZstandardSequenceCode kind,
        const std::vector<uint8_t> &symbols,
        ZstandardSequenceTable &table,
        std::optional<mem::Byte> &description) -> uint8_t;
    /// Find an FSE state path that emits the requested symbols.
    [[nodiscard]] static auto buildStatePath(
        const ZstandardSequenceTable &table, const std::vector<uint8_t> &symbols, std::vector<uint32_t> &states)
        -> bool;

private:
    std::vector<ZstandardSequence> _sequences; ///< Reused parsed block sequences.
    mem::ByteBuffer _literals;                 ///< Reused literal bytes.
    mem::ByteBuffer _output;                   ///< Reused compressed candidate.
    std::vector<uint8_t> _literalSymbols;      ///< Reused literal-length symbols.
    std::vector<uint8_t> _offsetSymbols;       ///< Reused offset symbols.
    std::vector<uint8_t> _matchSymbols;        ///< Reused match-length symbols.
    std::vector<uint32_t> _literalStates;      ///< Reused literal-length FSE state path.
    std::vector<uint32_t> _offsetStates;       ///< Reused offset FSE state path.
    std::vector<uint32_t> _matchStates;        ///< Reused match-length FSE state path.
    mem::ConstByteSpan _input;                 ///< Complete source input.
    CompressionLevel _level;                   ///< Requested compression effort.
    std::size_t _windowSize;                   ///< Match distance limit.
    std::size_t _searchDepth;                  ///< Hash-chain search depth.
    std::size_t _lazySteps;                    ///< Match lookahead count.
    std::vector<uint32_t> _heads;              ///< Hash-chain heads.
    std::vector<uint32_t> _previous;           ///< Previous position with the same hash.
    std::size_t _insertedUntil{};              ///< First position not added to history.
};

}
