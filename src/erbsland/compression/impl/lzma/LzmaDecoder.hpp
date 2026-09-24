// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "LzmaLengthProbabilities.hpp"
#include "LzmaProbabilities.hpp"
#include "LzmaRangeDecoder.hpp"

#include "../CodecOutput.hpp"

#include "../../../mem/ByteBlock_fwd.hpp"
#include "../../../mem/ByteBlockEditor.hpp"
#include "../../../mem/ByteSpan.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace erbsland::compression::impl {

/// Decodes one complete LZMA payload.
/// @tested{ByteCompressionTest}
class LzmaDecoder final {
private:
    static constexpr auto cMinimumDictionarySize = uint32_t{1U};      ///< Smallest effective dictionary size.
    static constexpr auto cPositionStateCount = std::size_t{16U};     ///< Number of LZMA position states.
    static constexpr auto cMinimumMatchLength = uint32_t{2U};         ///< Match-length symbol base.
    static constexpr auto cEndMarkerDistance = uint32_t{0xffffffffU}; ///< Reserved distance denoting end of stream.
    static constexpr auto cLiteralTreeSize = std::size_t{0x300U};     ///< Probabilities in one literal context.
    static constexpr auto cLiteralSymbolBase = uint32_t{0x100U};      ///< First completed literal-tree symbol.

public:
    /// Create a bounded streaming decoder.
    LzmaDecoder(
        CodecReader &input,
        uint32_t dictionarySize,
        uint8_t lc,
        uint8_t lp,
        uint8_t pb,
        std::optional<std::size_t> expected,
        std::size_t maximum,
        CodecOutput::Write output);

    /// Decode and validate the complete payload.
    void decode();

private:
    /// Append one byte while enforcing the output limit.
    void append(uint8_t value);
    /// Decode one literal byte in the current state.
    void decodeLiteral();
    /// Validate a position property before calculating its probability-table mask.
    static auto positionMask(uint8_t bits) -> uint32_t;
    /// Advance the state machine after decoding a literal.
    void updateLiteralState() noexcept;
    /// Decode a repeated-distance match, returning whether one was present.
    [[nodiscard]] auto decodeRep(std::size_t posState) -> bool;
    /// Decode a match length using the supplied probability model.
    [[nodiscard]] auto decodeLength(LzmaLengthProbabilities &length, std::size_t posState) -> uint32_t;
    /// Decode a match distance for the supplied match length.
    [[nodiscard]] auto decodeDistance(uint32_t length) -> uint32_t;
    /// Decode a reverse probability tree stored without its unused root entry.
    [[nodiscard]] auto decodeCompactReverseTree(std::span<uint16_t> probabilities, unsigned bitCount) -> uint32_t;
    /// Copy a match from the active repeated distance.
    void copyMatch(uint32_t length);

private:
    LzmaRangeDecoder _decoder;            ///< Entropy decoder for the payload.
    LzmaProbabilities _probabilities;     ///< Adaptive probability models.
    uint32_t _dictionarySize;             ///< Maximum permitted match distance.
    uint8_t _lc;                          ///< Literal-context high-byte bit count.
    std::size_t _positionMask;            ///< Position-state mask.
    std::size_t _literalPositionMask;     ///< Literal-position mask.
    std::optional<std::size_t> _expected; ///< Exact decoded length, when known.
    std::size_t _maximum;                 ///< Maximum permitted decoded length.
    CodecOutput _output;                  ///< Decoded output bytes.
    std::size_t _state{};                 ///< LZMA state-machine state.
    uint32_t _rep0{};                     ///< Most recent repeated distance.
    uint32_t _rep1{};                     ///< Second-most-recent repeated distance.
    uint32_t _rep2{};                     ///< Third-most-recent repeated distance.
    uint32_t _rep3{};                     ///< Fourth-most-recent repeated distance.
    bool _sawEndMarker{};                 ///< Whether the reserved end marker was decoded.
};

}
