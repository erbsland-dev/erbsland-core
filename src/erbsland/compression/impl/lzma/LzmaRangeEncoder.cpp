// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LzmaRangeEncoder.hpp"

#include "../../../err/LogicError.hpp"
#include "../../../mem/ByteBlock.hpp"
#include "../../../text/Literals.hpp"

#include <limits>
#include <span>

namespace erbsland::compression::impl {

using namespace text::literals;

void LzmaRangeEncoder::encodeBit(uint16_t &probability, const uint32_t bit) {
    const auto bound = (_range >> cProbabilityBits) * probability;
    if (bit == 0U) {
        _range = bound;
        probability = static_cast<uint16_t>(probability + ((cProbabilityTotal - probability) >> cProbabilityMoveBits));
    } else {
        _low += bound;
        _range -= bound;
        probability = static_cast<uint16_t>(probability - (probability >> cProbabilityMoveBits));
    }
    normalize();
}

void LzmaRangeEncoder::encodeTree(
    const std::span<uint16_t> probabilities, const unsigned bitCount, const uint32_t value) {
    requireTreeSize(probabilities, bitCount);
    auto symbol = uint32_t{1U};
    for (auto index = bitCount; index != 0U; --index) {
        const auto bit = (value >> (index - 1U)) & 1U;
        encodeBit(probabilities[symbol], bit);
        symbol = (symbol << 1U) | bit;
    }
}

void LzmaRangeEncoder::encodeReverseTree(
    const std::span<uint16_t> probabilities, const unsigned bitCount, uint32_t value) {
    requireTreeSize(probabilities, bitCount);
    auto symbol = uint32_t{1U};
    for (auto index = 0U; index < bitCount; ++index) {
        const auto bit = value & 1U;
        value >>= 1U;
        encodeBit(probabilities[symbol], bit);
        symbol = (symbol << 1U) | bit;
    }
}

void LzmaRangeEncoder::encodeDirect(const uint32_t value, const unsigned bitCount) {
    for (auto index = bitCount; index != 0U; --index) {
        _range >>= 1U;
        if (((value >> (index - 1U)) & 1U) != 0U) {
            _low += _range;
        }
        normalize();
    }
}

auto LzmaRangeEncoder::finalize() -> mem::ByteBlock {
    for (auto index = std::size_t{}; index < cFinalByteCount; ++index) {
        shiftLow();
    }
    return _output;
}

void LzmaRangeEncoder::requireTreeSize(const std::span<uint16_t> probabilities, const unsigned bitCount) {
    if (bitCount >= std::numeric_limits<uint32_t>::digits) {
        throw err::LogicError{"The LZMA probability-tree bit count is too large."_el};
    }
    const auto requiredSize = std::size_t{1U} << bitCount;
    if (probabilities.size() < requiredSize) {
        throw err::LogicError{"The LZMA probability span is too small for the requested tree."_el};
    }
}

void LzmaRangeEncoder::normalize() {
    if (_range < cTopValue) {
        _range <<= 8U;
        shiftLow();
    }
}

void LzmaRangeEncoder::shiftLow() {
    const auto low32 = static_cast<uint32_t>(_low);
    if (low32 < cStableLowLimit || static_cast<uint32_t>(_low >> 32U) != 0U) {
        auto value = _cache;
        const auto carry = static_cast<uint8_t>(_low >> 32U);
        do {
            _output.append(mem::Byte::fromCroppedUInt16(static_cast<uint16_t>(value) + carry));
            value = cPendingByteValue;
        } while (--_cacheSize != 0U);
        _cache = static_cast<uint8_t>(low32 >> 24U);
    }
    ++_cacheSize;
    _low = low32 << 8U;
}

}
