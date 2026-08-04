// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "X86ChaCha20Backend.hpp"

#include "../../../../mem/ByteIntegerAccess.hpp"
#include "../../../../mem/Endianness.hpp"
#include "../../../../mem/impl/UnsafeByteArrayAccess.hpp"
#include "../../../../mem/SecureErase.hpp"

namespace erbsland::cryptology::impl {

/// Rotate all four 32-bit lanes left, as required by RFC 8439 Section 2.1.
/// @tested{ChaCha20BackendFullTest}
auto X86ChaCha20Backend::rotateLeft(const __m128i value, const int amount) noexcept -> __m128i {
    return _mm_or_si128(_mm_slli_epi32(value, amount), _mm_srli_epi32(value, 32 - amount));
}

/// Apply the RFC 8439 Section 2.1 quarter round independently to four blocks.
/// @tested{ChaCha20BackendFullTest}
void X86ChaCha20Backend::quarterRound(__m128i &a, __m128i &b, __m128i &c, __m128i &d) noexcept {
    // RFC 8439, Section 2.1: SSE2 performs the scalar add/XOR/rotate sequence independently in every lane.
    a = _mm_add_epi32(a, b);
    d = rotateLeft(_mm_xor_si128(d, a), 16);
    c = _mm_add_epi32(c, d);
    b = rotateLeft(_mm_xor_si128(b, c), 12);
    a = _mm_add_epi32(a, b);
    d = rotateLeft(_mm_xor_si128(d, a), 8);
    c = _mm_add_epi32(c, d);
    b = rotateLeft(_mm_xor_si128(b, c), 7);
}

auto X86ChaCha20Backend::generateBlocks(const uint32_t firstCounter, const std::size_t blockCount) noexcept -> Batch {
    if (blockCount != 4U) {
        return PortableChaCha20Backend::generateBlocks(firstCounter, blockCount);
    }
    auto result = Batch{};
    constexpr auto stateWordCount = std::size_t{16};

    // RFC 8439, Section 2.3: each SSE2 lane represents the same state word in four consecutive blocks.
    // Keep SIMD vectors outside standard-library templates: GCC attributes __m128i and rejects it as a template
    // argument with -Werror=ignored-attributes.
    __m128i initial[stateWordCount]{};
    initial[0] = _mm_set1_epi32(0x61707865U);
    initial[1] = _mm_set1_epi32(0x3320646eU);
    initial[2] = _mm_set1_epi32(0x79622d32U);
    initial[3] = _mm_set1_epi32(0x6b206574U);
    for (auto index = std::size_t{}; index < 8U; ++index) {
        initial[index + 4U] = _mm_set1_epi32(
            static_cast<int32_t>(_key.getInteger<uint32_t>(unit::ByteIndex{index * 4U}, mem::Endianness::Little)));
    }
    initial[12] = _mm_set_epi32(
        static_cast<int32_t>(firstCounter + 3U),
        static_cast<int32_t>(firstCounter + 2U),
        static_cast<int32_t>(firstCounter + 1U),
        static_cast<int32_t>(firstCounter));
    for (auto index = std::size_t{}; index < 3U; ++index) {
        initial[index + 13U] = _mm_set1_epi32(
            static_cast<int32_t>(_nonce.getInteger<uint32_t>(unit::ByteIndex{index * 4U}, mem::Endianness::Little)));
    }
    __m128i working[stateWordCount]{};
    for (auto index = std::size_t{}; index < stateWordCount; ++index) {
        working[index] = initial[index];
    }

    // RFC 8439, Section 2.3.1: lane-wise column and diagonal rounds evaluate four independent blocks.
    for (auto round = std::size_t{}; round < 10U; ++round) {
        quarterRound(working[0], working[4], working[8], working[12]);
        quarterRound(working[1], working[5], working[9], working[13]);
        quarterRound(working[2], working[6], working[10], working[14]);
        quarterRound(working[3], working[7], working[11], working[15]);
        quarterRound(working[0], working[5], working[10], working[15]);
        quarterRound(working[1], working[6], working[11], working[12]);
        quarterRound(working[2], working[7], working[8], working[13]);
        quarterRound(working[3], working[4], working[9], working[14]);
    }

    // Transpose the lane-wise words into four RFC little-endian 64-byte blocks through the byte abstraction.
    auto lanes = mem::ByteArray<16>{};
    auto laneBytes = mem::impl::UnsafeByteArrayAccess{lanes}.writableData();
    for (auto wordIndex = std::size_t{}; wordIndex < stateWordCount; ++wordIndex) {
        working[wordIndex] = _mm_add_epi32(working[wordIndex], initial[wordIndex]);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(laneBytes.data()), working[wordIndex]);
        for (auto lane = std::size_t{}; lane < 4U; ++lane) {
            const auto value = lanes.getInteger<uint32_t>(unit::ByteIndex{lane * 4U}, mem::Endianness::Little);
            result.setIntegerOrThrow(unit::ByteIndex{lane * 64U + wordIndex * 4U}, value, mem::Endianness::Little);
        }
    }
    lanes.secureErase();
    mem::secureErase(std::span{working});
    mem::secureErase(std::span{initial});
    return result;
}

}
