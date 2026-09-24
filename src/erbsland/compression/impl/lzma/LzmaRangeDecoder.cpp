// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LzmaRangeDecoder.hpp"

#include "../../../err/LogicError.hpp"
#include "../../../text/Literals.hpp"
#include "../../CompressionError.hpp"

#include <limits>
#include <span>

namespace erbsland::compression::impl {

using namespace text::literals;

LzmaRangeDecoder::LzmaRangeDecoder(CodecReader &input) : _source{input} {
    if (readByte() != 0U) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Invalid LZMA prefix."_el};
    }
    for (auto i = 0U; i < cCodeByteCount; ++i) {
        _code = (_code << 8U) | readByte();
    }
}

[[nodiscard]] auto LzmaRangeDecoder::decodeBit(uint16_t &probability) -> uint32_t {
    const auto bound = (_range >> cProbabilityBits) * probability;
    uint32_t bit;
    if (_code < bound) {
        _range = bound;
        probability = static_cast<uint16_t>(probability + ((cProbabilityTotal - probability) >> cProbabilityMoveBits));
        bit = 0U;
    } else {
        _range -= bound;
        _code -= bound;
        probability = static_cast<uint16_t>(probability - (probability >> cProbabilityMoveBits));
        bit = 1U;
    }
    normalize();
    return bit;
}

[[nodiscard]] auto LzmaRangeDecoder::decodeTree(const std::span<uint16_t> probabilities, const unsigned bitCount)
    -> uint32_t {
    requireTreeSize(probabilities, bitCount);
    auto symbol = uint32_t{1U};
    for (auto index = 0U; index < bitCount; ++index) {
        symbol = (symbol << 1U) | decodeBit(probabilities[symbol]);
    }
    return symbol - (1U << bitCount);
}

[[nodiscard]] auto LzmaRangeDecoder::decodeReverseTree(const std::span<uint16_t> probabilities, const unsigned bitCount)
    -> uint32_t {
    requireTreeSize(probabilities, bitCount);
    auto symbol = uint32_t{1U};
    auto result = uint32_t{};
    for (auto index = 0U; index < bitCount; ++index) {
        const auto bit = decodeBit(probabilities[symbol]);
        symbol = (symbol << 1U) | bit;
        result |= bit << index;
    }
    return result;
}

[[nodiscard]] auto LzmaRangeDecoder::decodeDirect(const unsigned bitCount) -> uint32_t {
    auto result = uint32_t{};
    for (auto index = 0U; index < bitCount; ++index) {
        _range >>= 1U;
        const auto bit = _code >= _range ? 1U : 0U;
        if (bit != 0U) {
            _code -= _range;
        }
        result = (result << 1U) | bit;
        normalize();
    }
    return result;
}

[[nodiscard]] auto LzmaRangeDecoder::isFinished() const -> bool {
    return _code == 0U;
}

void LzmaRangeDecoder::requireTreeSize(const std::span<uint16_t> probabilities, const unsigned bitCount) {
    if (bitCount >= std::numeric_limits<uint32_t>::digits) {
        throw err::LogicError{"The LZMA probability-tree bit count is too large."_el};
    }
    const auto requiredSize = std::size_t{1U} << bitCount;
    if (probabilities.size() < requiredSize) {
        throw err::LogicError{"The LZMA probability span is too small for the requested tree."_el};
    }
}

[[nodiscard]] auto LzmaRangeDecoder::readByte() -> uint32_t {
    return _source.byte().toUInt32();
}

void LzmaRangeDecoder::normalize() {
    if (_range < cTopValue) {
        _range <<= 8U;
        _code = (_code << 8U) | readByte();
    }
}

}
