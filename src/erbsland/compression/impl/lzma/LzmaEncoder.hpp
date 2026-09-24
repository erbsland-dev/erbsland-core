// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "LzmaLengthProbabilities.hpp"
#include "LzmaProbabilities.hpp"
#include "LzmaRangeEncoder.hpp"

#include "../CodecOutput.hpp"
#include "../CodecReader.hpp"

#include "../../../mem/ByteBlock_fwd.hpp"
#include "../../../mem/ByteSpan.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <vector>

namespace erbsland::compression::impl {

/// Encodes one complete LZMA payload.
/// @tested{ByteCompressionTest}
class LzmaEncoder final {
private:
    static constexpr auto cNone = std::numeric_limits<uint32_t>::max(); ///< Missing match-chain position.
    static constexpr auto cHashTableSize = std::size_t{65'536U};        ///< Entries in the match hash table.
    static constexpr auto cMinimumMatchLength = std::size_t{3U};        ///< Minimum encoded match length.
    static constexpr auto cMaximumMatchLength = std::size_t{273U};      ///< Maximum encoded match length.
    static constexpr auto cPositionStateMask = std::size_t{3U};         ///< Mask for the default `pb = 2` state.
    static constexpr auto cLiteralTreeSize = std::size_t{0x300U};       ///< Probabilities in one literal context.
    static constexpr auto cHashMultiplier = uint32_t{2'654'435'761U};   ///< Multiplicative sequence-hash constant.
    static constexpr auto cLiteralSymbolBase = uint32_t{0x100U};        ///< First completed literal-tree symbol.
    static constexpr auto cEndMarkerSlot = uint32_t{63U};               ///< Distance slot reserved for end marker.
    static constexpr auto cEndMarkerDirectBits = uint32_t{0x03ffffffU}; ///< Direct bits in the end marker.

public:
    /// Encode one continuous stream using bounded input and output.
    void encodeStream(CodecReader &input, const CodecOutput::Write &output);

    /// Create an encoder with its dictionary size and match-search depth.
    LzmaEncoder(uint32_t dictionarySize, std::size_t depth);

private:
    /// Encode currently available input, preserving entropy and match state.
    void encodePart();
    /// Hash the input sequence at the supplied position.
    [[nodiscard]] auto hash(std::size_t position) const noexcept -> std::size_t;
    /// Encode one literal byte in the current state.
    void encodeLiteral(uint8_t value);
    /// Encode a match length with the supplied probability model.
    void encodeLength(LzmaLengthProbabilities &length, std::size_t posState, uint32_t symbol);
    /// Encode a match distance and its slot representation.
    void encodeDistance(uint32_t distance, uint32_t length);
    /// Encode a reverse probability tree stored without its unused root entry.
    void encodeCompactReverseTree(std::span<uint16_t> probabilities, unsigned bitCount, uint32_t value);
    /// Encode a normal-distance match.
    void encodeMatch(uint32_t length, uint32_t distance);
    /// Encode the optional LZMA end marker.
    void encodeEndMarker();

private:
    LzmaRangeEncoder _encoder;            ///< Entropy encoder for the payload.
    LzmaProbabilities _probabilities;     ///< Adaptive probability models.
    mem::ConstByteSpan _input;            ///< Uncompressed input bytes.
    uint32_t _dictionarySize;             ///< Maximum match distance.
    std::size_t _depth;                   ///< Match-chain search depth.
    std::vector<uint32_t> _head;          ///< Head position for each sequence hash.
    std::vector<uint32_t> _previousChain; ///< Previous matching position for each input byte.
    std::size_t _position{};              ///< Current input position.
    std::size_t _state{};                 ///< LZMA state-machine state.
    uint32_t _rep0{};                     ///< Most recent match distance.
    uint8_t _previous{};                  ///< Previous literal byte.
};

}
