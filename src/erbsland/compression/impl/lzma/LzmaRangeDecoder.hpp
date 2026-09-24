// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../CodecReader.hpp"

#include <cstdint>
#include <span>

namespace erbsland::compression::impl {

/// Range decoder for the entropy-coded portion of an LZMA payload.
/// @tested{ByteCompressionTest CompressionCodecTest}
class LzmaRangeDecoder final {
private:
    static constexpr auto cInitialRange = uint32_t{0xffffffffU}; ///< Initial arithmetic-coder range.
    static constexpr auto cTopValue = uint32_t{1U << 24U};       ///< Range threshold requiring normalization.
    static constexpr auto cProbabilityBits = 11U;                ///< Fixed-point probability precision.
    static constexpr auto cProbabilityTotal = uint16_t{1U << cProbabilityBits}; ///< Full probability range.
    static constexpr auto cProbabilityMoveBits = 5U; ///< Adaptation shift applied after each bit.
    static constexpr auto cCodeByteCount = 4U;       ///< Bytes loaded into the initial code value.

public:
    /// Create a streaming range decoder.
    explicit LzmaRangeDecoder(CodecReader &input);

public:
    /// Decode one adaptive probability bit.
    [[nodiscard]] auto decodeBit(uint16_t &probability) -> uint32_t;
    /// Decode a most-significant-bit-first probability tree.
    /// @throws err::LogicError If the probability span is too small for `bitCount`.
    [[nodiscard]] auto decodeTree(std::span<uint16_t> probabilities, unsigned bitCount) -> uint32_t;
    /// Decode a least-significant-bit-first probability tree.
    /// @throws err::LogicError If the probability span is too small for `bitCount`.
    [[nodiscard]] auto decodeReverseTree(std::span<uint16_t> probabilities, unsigned bitCount) -> uint32_t;
    /// Decode bits without an adaptive probability model.
    [[nodiscard]] auto decodeDirect(unsigned bitCount) -> uint32_t;
    /// Test whether the arithmetic code is in its canonical finished state.
    [[nodiscard]] auto isFinished() const -> bool;

private:
    /// Validate that a probability span contains the complete requested tree.
    /// @throws err::LogicError If the probability span is too small for `bitCount`.
    static void requireTreeSize(std::span<uint16_t> probabilities, unsigned bitCount);
    /// Read one encoded byte or reject truncated input.
    [[nodiscard]] auto readByte() -> uint32_t;
    /// Refill the range state when its precision becomes too small.
    void normalize();

private:
    CodecReader &_source;           ///< Borrowed streaming source.
    uint32_t _range{cInitialRange}; ///< Current arithmetic-coder range.
    uint32_t _code{};               ///< Current arithmetic-coder code value.
};

}
