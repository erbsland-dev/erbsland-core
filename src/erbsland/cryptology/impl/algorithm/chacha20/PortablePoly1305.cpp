// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PortablePoly1305.hpp"

#include "../../../../mem/ByteIntegerAccess.hpp"
#include "../../../../mem/Endianness.hpp"
#include "../../../../mem/SecureErase.hpp"

#include <algorithm>
#include <span>

namespace erbsland::cryptology::impl {

PortablePoly1305::PortablePoly1305(const mem::ConstByteSpan key) noexcept {
    // RFC 8439, Section 2.5: decode the first 128 key bits little-endian before applying the clamp mask.
    auto keyWords = std::array<uint32_t, 4>{
        mem::getInteger<uint32_t>(key, unit::ByteIndex{0U}, mem::Endianness::Little),
        mem::getInteger<uint32_t>(key, unit::ByteIndex{4U}, mem::Endianness::Little),
        mem::getInteger<uint32_t>(key, unit::ByteIndex{8U}, mem::Endianness::Little),
        mem::getInteger<uint32_t>(key, unit::ByteIndex{12U}, mem::Endianness::Little),
    };

    // RFC 8439, Section 2.5: clamp r, represented here as five radix-2^26 limbs.
    _r[0] = keyWords[0] & 0x3ffffffU;
    _r[1] = ((static_cast<uint64_t>(keyWords[0]) >> 26U) | (static_cast<uint64_t>(keyWords[1]) << 6U)) & 0x3ffff03U;
    _r[2] = ((static_cast<uint64_t>(keyWords[1]) >> 20U) | (static_cast<uint64_t>(keyWords[2]) << 12U)) & 0x3ffc0ffU;
    _r[3] = ((static_cast<uint64_t>(keyWords[2]) >> 14U) | (static_cast<uint64_t>(keyWords[3]) << 18U)) & 0x3f03fffU;
    _r[4] = (static_cast<uint64_t>(keyWords[3]) >> 8U) & 0x00fffffU;
    for (auto index = std::size_t{1U}; index < _r.size(); ++index) {
        _r5[index] = _r[index] * 5U;
    }
    _pad.overwrite(key.subspan(16U, 16U));
    mem::secureErase(std::span{keyWords});
}

PortablePoly1305::~PortablePoly1305() noexcept {
    secureErase();
}

void PortablePoly1305::update(mem::ConstByteSpan data) noexcept {
    // RFC 8439, Section 2.5: retain an incomplete 16-byte message block across streaming updates.
    if (_partialLength != 0U) {
        const auto count = std::min(16U - _partialLength, data.size());
        _partial.overwrite(unit::ByteIndex{_partialLength}, data.first(count));
        _partialLength += count;
        data = data.subspan(count);
        if (_partialLength == 16U) {
            processBlock(_partial.span(), true);
            _partial.secureErase();
            _partialLength = 0U;
        }
    }

    // RFC 8439, Section 2.5: every complete block includes the implicit high bit and follows the same recurrence.
    const auto blockCount = data.size() / 16U;
    if (blockCount != 0U) {
        processFullBlocks(data.first(blockCount * 16U), blockCount);
        data = data.subspan(blockCount * 16U);
    }
    if (!data.empty()) {
        _partial.overwrite(data);
        _partialLength = data.size();
    }
}

auto PortablePoly1305::finalize() noexcept -> Tag {
    if (_partialLength != 0U) {
        // RFC 8439, Section 2.5: a partial final block appends one byte and then zero padding.
        _partial.set(unit::ByteIndex{_partialLength}, mem::Byte{1U});
        for (auto index = _partialLength + 1U; index < 16U; ++index) {
            _partial.set(unit::ByteIndex{index}, mem::Byte{});
        }
        processBlock(_partial.span(), false);
    }

    // RFC 8439, Section 2.5: carry the radix-2^26 accumulator into canonical limb widths.
    auto carry = _h[1] >> 26U;
    _h[1] &= 0x3ffffffU;
    _h[2] += carry;
    carry = _h[2] >> 26U;
    _h[2] &= 0x3ffffffU;
    _h[3] += carry;
    carry = _h[3] >> 26U;
    _h[3] &= 0x3ffffffU;
    _h[4] += carry;
    carry = _h[4] >> 26U;
    _h[4] &= 0x3ffffffU;
    _h[0] += carry * 5U;
    carry = _h[0] >> 26U;
    _h[0] &= 0x3ffffffU;
    _h[1] += carry;

    // RFC 8439, Section 2.5: select h or h-(2^130-5) without branching on secret state.
    auto g = std::array<uint64_t, 5>{};
    g[0] = _h[0] + 5U;
    carry = g[0] >> 26U;
    g[0] &= 0x3ffffffU;
    for (auto index = std::size_t{1U}; index < 4U; ++index) {
        g[index] = _h[index] + carry;
        carry = g[index] >> 26U;
        g[index] &= 0x3ffffffU;
    }
    g[4] = _h[4] + carry - (uint64_t{1U} << 26U);
    auto mask = (g[4] >> 63U) - 1U;
    g[4] &= 0x3ffffffU;
    const auto inverseMask = ~mask;
    for (auto index = std::size_t{}; index < _h.size(); ++index) {
        _h[index] = (_h[index] & inverseMask) | (g[index] & mask);
    }

    // RFC 8439, Section 2.5: serialize the low 128 bits and add s modulo 2^128.
    auto outputWords = std::array<uint64_t, 4>{};
    outputWords[0] = ((_h[0] | (_h[1] << 26U)) & 0xffffffffU) + _pad.getInteger<uint32_t>(unit::ByteIndex{0U});
    outputWords[1] = (((_h[1] >> 6U) | (_h[2] << 20U)) & 0xffffffffU) + _pad.getInteger<uint32_t>(unit::ByteIndex{4U}) +
        (outputWords[0] >> 32U);
    outputWords[2] = (((_h[2] >> 12U) | (_h[3] << 14U)) & 0xffffffffU) +
        _pad.getInteger<uint32_t>(unit::ByteIndex{8U}) + (outputWords[1] >> 32U);
    outputWords[3] = (((_h[3] >> 18U) | (_h[4] << 8U)) & 0xffffffffU) +
        _pad.getInteger<uint32_t>(unit::ByteIndex{12U}) + (outputWords[2] >> 32U);
    auto result = Tag{};
    for (auto index = std::size_t{}; index < outputWords.size(); ++index) {
        result.setIntegerOrThrow(
            unit::ByteIndex{index * 4U}, static_cast<uint32_t>(outputWords[index]), mem::Endianness::Little);
    }

    mem::secureErase(std::span{g});
    mem::secureErase(std::span{outputWords});
    return result;
}

void PortablePoly1305::secureErase() noexcept {
    mem::secureErase(std::span{_r});
    mem::secureErase(std::span{_r5});
    mem::secureErase(std::span{_h});
    _pad.secureErase();
    _partial.secureErase();
    _partialLength = 0U;
}

void PortablePoly1305::processFullBlocks(const mem::ConstByteSpan blocks, const std::size_t blockCount) noexcept {
    for (auto index = std::size_t{}; index < blockCount; ++index) {
        processBlock(blocks.subspan(index * 16U, 16U), true);
    }
}

void PortablePoly1305::processBlock(const mem::ConstByteSpan block, const bool complete) noexcept {
    addBlockToAccumulator(block, complete);

    // RFC 8439, Section 2.5: multiply h by r; wrapped limb products use 2^130 == 5 modulo 2^130-5.
    auto product = std::array<uint64_t, 5>{};
    product[0] = _h[0] * _r[0] + _h[1] * _r5[4] + _h[2] * _r5[3] + _h[3] * _r5[2] + _h[4] * _r5[1];
    product[1] = _h[0] * _r[1] + _h[1] * _r[0] + _h[2] * _r5[4] + _h[3] * _r5[3] + _h[4] * _r5[2];
    product[2] = _h[0] * _r[2] + _h[1] * _r[1] + _h[2] * _r[0] + _h[3] * _r5[4] + _h[4] * _r5[3];
    product[3] = _h[0] * _r[3] + _h[1] * _r[2] + _h[2] * _r[1] + _h[3] * _r[0] + _h[4] * _r5[4];
    product[4] = _h[0] * _r[4] + _h[1] * _r[3] + _h[2] * _r[2] + _h[3] * _r[1] + _h[4] * _r[0];
    reduceProduct(product);
}

void PortablePoly1305::addBlockToAccumulator(const mem::ConstByteSpan block, const bool complete) noexcept {
    auto words = std::array<uint32_t, 4>{
        mem::getInteger<uint32_t>(block, unit::ByteIndex{0U}, mem::Endianness::Little),
        mem::getInteger<uint32_t>(block, unit::ByteIndex{4U}, mem::Endianness::Little),
        mem::getInteger<uint32_t>(block, unit::ByteIndex{8U}, mem::Endianness::Little),
        mem::getInteger<uint32_t>(block, unit::ByteIndex{12U}, mem::Endianness::Little),
    };

    // RFC 8439, Section 2.5: add the block with its implicit high one bit to the accumulator.
    _h[0] += words[0] & 0x3ffffffU;
    _h[1] += ((static_cast<uint64_t>(words[0]) >> 26U) | (static_cast<uint64_t>(words[1]) << 6U)) & 0x3ffffffU;
    _h[2] += ((static_cast<uint64_t>(words[1]) >> 20U) | (static_cast<uint64_t>(words[2]) << 12U)) & 0x3ffffffU;
    _h[3] += ((static_cast<uint64_t>(words[2]) >> 14U) | (static_cast<uint64_t>(words[3]) << 18U)) & 0x3ffffffU;
    _h[4] += (static_cast<uint64_t>(words[3]) >> 8U) | (complete ? (uint64_t{1U} << 24U) : 0U);
    mem::secureErase(std::span{words});
}

void PortablePoly1305::reduceProduct(std::array<uint64_t, 5> &product) noexcept {
    // RFC 8439, Section 2.5: propagate radix-2^26 carries and fold the bit-130 carry back by five.
    auto carry = product[0] >> 26U;
    _h[0] = product[0] & 0x3ffffffU;
    auto value = product[1] + carry;
    carry = value >> 26U;
    _h[1] = value & 0x3ffffffU;
    value = product[2] + carry;
    carry = value >> 26U;
    _h[2] = value & 0x3ffffffU;
    value = product[3] + carry;
    carry = value >> 26U;
    _h[3] = value & 0x3ffffffU;
    value = product[4] + carry;
    carry = value >> 26U;
    _h[4] = value & 0x3ffffffU;
    _h[0] += carry * 5U;
    carry = _h[0] >> 26U;
    _h[0] &= 0x3ffffffU;
    _h[1] += carry;
    mem::secureErase(std::span{product});
}

}
