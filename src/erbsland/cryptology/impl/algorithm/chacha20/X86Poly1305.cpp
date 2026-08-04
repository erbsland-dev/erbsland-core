// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "X86Poly1305.hpp"

#include "../../../../mem/Endianness.hpp"
#include "../../../../mem/impl/UnsafeByteArrayAccess.hpp"
#include "../../../../mem/SecureErase.hpp"

#include <emmintrin.h>

namespace erbsland::cryptology::impl {

/// Multiply five pairs of 26-bit limbs with SSE2 and return their 64-bit sum.
/// Lanes zero and two hold two independent RFC 8439 limb products; the other lanes are zero.
/// @tested{ChaCha20BackendFullTest}
auto X86Poly1305::multiplyFive(const std::array<uint64_t, 5> &left, const std::array<uint64_t, 5> &right) noexcept
    -> uint64_t {
    // RFC 8439, Section 2.5: form one convolution coefficient from five 26-bit limb products.
    auto sum = uint64_t{};
    auto products = mem::ByteArray<16>{};
    auto productBytes = mem::impl::UnsafeByteArrayAccess{products}.writableData();
    for (auto index = std::size_t{}; index < 5U; index += 2U) {
        const auto second = index + 1U < 5U ? index + 1U : index;
        const auto leftVector =
            _mm_set_epi32(0, static_cast<int32_t>(left[second]), 0, static_cast<int32_t>(left[index]));
        const auto rightVector =
            _mm_set_epi32(0, static_cast<int32_t>(right[second]), 0, static_cast<int32_t>(right[index]));
        const auto productVector = _mm_mul_epu32(leftVector, rightVector);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(productBytes.data()), productVector);
        sum += products.getInteger<uint64_t>(unit::ByteIndex{0U}, mem::Endianness::Little);
        if (index + 1U < 5U) {
            sum += products.getInteger<uint64_t>(unit::ByteIndex{8U}, mem::Endianness::Little);
        }
    }
    products.secureErase();
    return sum;
}

void X86Poly1305::processFullBlocks(const mem::ConstByteSpan blocks, const std::size_t blockCount) noexcept {
    auto index = std::size_t{};
    for (; index + 4U <= blockCount; index += 4U) {
        // RFC 8439, Section 2.5: preserve message order across each four-block backend batch.
        // Inside each recurrence, SSE2 lanes evaluate independent pairs of the five limb products.
        processBlock(blocks.subspan((index + 0U) * 16U, 16U), true);
        processBlock(blocks.subspan((index + 1U) * 16U, 16U), true);
        processBlock(blocks.subspan((index + 2U) * 16U, 16U), true);
        processBlock(blocks.subspan((index + 3U) * 16U, 16U), true);
    }
    if (index < blockCount) {
        PortablePoly1305::processFullBlocks(blocks.subspan(index * 16U), blockCount - index);
    }
}

void X86Poly1305::processBlock(const mem::ConstByteSpan block, const bool complete) noexcept {
    addBlockToAccumulator(block, complete);

    // RFC 8439, Section 2.5: each row is one coefficient of h*r modulo 2^130-5.
    auto factor0 = std::array<uint64_t, 5>{_r[0], _r5[4], _r5[3], _r5[2], _r5[1]};
    auto factor1 = std::array<uint64_t, 5>{_r[1], _r[0], _r5[4], _r5[3], _r5[2]};
    auto factor2 = std::array<uint64_t, 5>{_r[2], _r[1], _r[0], _r5[4], _r5[3]};
    auto factor3 = std::array<uint64_t, 5>{_r[3], _r[2], _r[1], _r[0], _r5[4]};
    auto factor4 = std::array<uint64_t, 5>{_r[4], _r[3], _r[2], _r[1], _r[0]};
    auto product = std::array<uint64_t, 5>{
        multiplyFive(_h, factor0),
        multiplyFive(_h, factor1),
        multiplyFive(_h, factor2),
        multiplyFive(_h, factor3),
        multiplyFive(_h, factor4),
    };
    reduceProduct(product);
    mem::secureErase(std::span{factor0});
    mem::secureErase(std::span{factor1});
    mem::secureErase(std::span{factor2});
    mem::secureErase(std::span{factor3});
    mem::secureErase(std::span{factor4});
}

}
