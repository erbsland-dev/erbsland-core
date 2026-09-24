// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../CodecOutput.hpp"

#include "../../../mem/ByteBlock_fwd.hpp"
#include "../../../mem/ByteBlockEditor.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace erbsland::compression::impl {

/// Range encoder for the entropy-coded portion of an LZMA payload.
/// @tested{ByteCompressionTest CompressionCodecTest}
class LzmaRangeEncoder final {
private:
    static constexpr auto cInitialRange = uint32_t{0xffffffffU}; ///< Initial arithmetic-coder range.
    static constexpr auto cTopValue = uint32_t{1U << 24U};       ///< Range threshold requiring normalization.
    static constexpr auto cProbabilityBits = 11U;                ///< Fixed-point probability precision.
    static constexpr auto cProbabilityTotal = uint16_t{1U << cProbabilityBits}; ///< Full probability range.
    static constexpr auto cProbabilityMoveBits = 5U;               ///< Adaptation shift applied after each bit.
    static constexpr auto cFinalByteCount = std::size_t{5U};       ///< Bytes flushed when finalizing the stream.
    static constexpr auto cStableLowLimit = uint32_t{0xff000000U}; ///< Low values whose high byte is stable.
    static constexpr auto cPendingByteValue = uint8_t{0xffU};      ///< Byte emitted while propagating a carry.

public:
    /// Route encoded bytes to a bounded sink.
    void setOutput(CodecOutput::Write output) {
        _output = CodecOutput{std::move(output), 0U, unit::ByteLength::infinite()};
    }

    /// Encode one adaptive probability bit.
    void encodeBit(uint16_t &probability, const uint32_t bit);

    /// Encode a most-significant-bit-first probability tree.
    /// @throws err::LogicError If the probability span is too small for `bitCount`.
    void encodeTree(std::span<uint16_t> probabilities, unsigned bitCount, uint32_t value);

    /// Encode a least-significant-bit-first probability tree.
    /// @throws err::LogicError If the probability span is too small for `bitCount`.
    void encodeReverseTree(std::span<uint16_t> probabilities, unsigned bitCount, uint32_t value);

    /// Encode bits without an adaptive probability model.
    void encodeDirect(const uint32_t value, const unsigned bitCount);

    /// Flush the range state and return the encoded bytes.
    auto finalize() -> mem::ByteBlock;

private:
    /// Validate that a probability span contains the complete requested tree.
    /// @throws err::LogicError If the probability span is too small for `bitCount`.
    static void requireTreeSize(std::span<uint16_t> probabilities, unsigned bitCount);
    /// Restore range precision after encoding a symbol.
    void normalize();
    /// Flush stable high bytes from the low accumulator.
    void shiftLow();

private:
    CodecOutput _output;            ///< Encoded output bytes.
    uint64_t _low{};                ///< Arithmetic-coder low accumulator.
    uint32_t _range{cInitialRange}; ///< Current arithmetic-coder range.
    uint8_t _cache{};               ///< Delayed high byte.
    std::size_t _cacheSize{1U};     ///< Number of delayed bytes.
};

}
