// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "X86GaloisMultiplier.hpp"

#include "GaloisOperations.hpp"

#include "../../../../mem/Endianness.hpp"

#include <wmmintrin.h>

#include <array>

namespace erbsland::cryptology::impl {

auto X86GaloisMultiplier::multiply(const Block &left, const Block &right) const noexcept -> Block {
    auto reflectedLeft = galois::reflect(left);
    auto reflectedRight = galois::reflect(right);
    const auto leftLow = reflectedLeft.getInteger<uint64_t>(unit::ByteIndex{8U}, mem::Endianness::Big);
    const auto leftHigh = reflectedLeft.getInteger<uint64_t>(unit::ByteIndex{}, mem::Endianness::Big);
    const auto rightLow = reflectedRight.getInteger<uint64_t>(unit::ByteIndex{8U}, mem::Endianness::Big);
    const auto rightHigh = reflectedRight.getInteger<uint64_t>(unit::ByteIndex{}, mem::Endianness::Big);
    const auto leftNative = _mm_set_epi64x(static_cast<int64_t>(leftHigh), static_cast<int64_t>(leftLow));
    const auto rightNative = _mm_set_epi64x(static_cast<int64_t>(rightHigh), static_cast<int64_t>(rightLow));

    // PCLMULQDQ computes three carry-less products; Karatsuba supplies the middle term.
    const auto lowProduct = _mm_clmulepi64_si128(leftNative, rightNative, 0x00);
    const auto highProduct = _mm_clmulepi64_si128(leftNative, rightNative, 0x11);
    const auto leftCross = _mm_xor_si128(leftNative, _mm_shuffle_epi32(leftNative, 0x4e));
    const auto rightCross = _mm_xor_si128(rightNative, _mm_shuffle_epi32(rightNative, 0x4e));
    auto middleProduct = _mm_clmulepi64_si128(leftCross, rightCross, 0x00);
    middleProduct = _mm_xor_si128(middleProduct, _mm_xor_si128(lowProduct, highProduct));

    auto lowWords = std::array<uint64_t, 2>{};
    auto highWords = std::array<uint64_t, 2>{};
    auto middleWords = std::array<uint64_t, 2>{};
    _mm_storeu_si128(reinterpret_cast<__m128i *>(lowWords.data()), lowProduct);
    _mm_storeu_si128(reinterpret_cast<__m128i *>(highWords.data()), highProduct);
    _mm_storeu_si128(reinterpret_cast<__m128i *>(middleWords.data()), middleProduct);
    auto result =
        galois::reduce({lowWords[0], lowWords[1] ^ middleWords[0], highWords[0] ^ middleWords[1], highWords[1]});
    result = galois::reflect(result);
    reflectedLeft.secureErase();
    reflectedRight.secureErase();
    return result;
}

}
