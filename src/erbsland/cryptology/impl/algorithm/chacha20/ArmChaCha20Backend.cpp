// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ArmChaCha20Backend.hpp"

#include "../../../../mem/ByteIntegerAccess.hpp"
#include "../../../../mem/Endianness.hpp"
#include "../../../../mem/impl/UnsafeByteArrayAccess.hpp"
#include "../../../../mem/SecureErase.hpp"

#include <array>

namespace erbsland::cryptology::impl {

/// Rotate all four 32-bit lanes left, as required by RFC 8439 Section 2.1.
/// @tested{ChaCha20BackendFullTest}
template <int tAmount>
auto ArmChaCha20Backend::rotateLeft(const uint32x4_t value) noexcept -> uint32x4_t {
    return vorrq_u32(vshlq_n_u32(value, tAmount), vshrq_n_u32(value, 32 - tAmount));
}

/// Apply the RFC 8439 Section 2.1 quarter round independently to four blocks.
/// @tested{ChaCha20BackendFullTest}
void ArmChaCha20Backend::quarterRound(uint32x4_t &a, uint32x4_t &b, uint32x4_t &c, uint32x4_t &d) noexcept {
    // RFC 8439, Section 2.1: NEON performs the scalar add/XOR/rotate sequence independently in every lane.
    a = vaddq_u32(a, b);
    d = rotateLeft<16>(veorq_u32(d, a));
    c = vaddq_u32(c, d);
    b = rotateLeft<12>(veorq_u32(b, c));
    a = vaddq_u32(a, b);
    d = rotateLeft<8>(veorq_u32(d, a));
    c = vaddq_u32(c, d);
    b = rotateLeft<7>(veorq_u32(b, c));
}

auto ArmChaCha20Backend::generateBlocks(const uint32_t firstCounter, const std::size_t blockCount) noexcept -> Batch {
    if (blockCount != 4U) {
        return PortableChaCha20Backend::generateBlocks(firstCounter, blockCount);
    }
    auto result = Batch{};

    // RFC 8439, Section 2.3: each NEON lane represents the same state word in four consecutive blocks.
    auto initial = std::array<uint32x4_t, 16>{};
    initial[0] = vdupq_n_u32(0x61707865U);
    initial[1] = vdupq_n_u32(0x3320646eU);
    initial[2] = vdupq_n_u32(0x79622d32U);
    initial[3] = vdupq_n_u32(0x6b206574U);
    for (auto index = std::size_t{}; index < 8U; ++index) {
        initial[index + 4U] =
            vdupq_n_u32(_key.getInteger<uint32_t>(unit::ByteIndex{index * 4U}, mem::Endianness::Little));
    }
    const auto counters = std::array<uint32_t, 4>{
        firstCounter,
        firstCounter + 1U,
        firstCounter + 2U,
        firstCounter + 3U,
    };
    initial[12] = vld1q_u32(counters.data());
    for (auto index = std::size_t{}; index < 3U; ++index) {
        initial[index + 13U] =
            vdupq_n_u32(_nonce.getInteger<uint32_t>(unit::ByteIndex{index * 4U}, mem::Endianness::Little));
    }
    auto working = initial;

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
    for (auto wordIndex = std::size_t{}; wordIndex < working.size(); ++wordIndex) {
        working[wordIndex] = vaddq_u32(working[wordIndex], initial[wordIndex]);
        vst1q_u32(reinterpret_cast<uint32_t *>(laneBytes.data()), working[wordIndex]);
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
