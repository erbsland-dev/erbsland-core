// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ArmGaloisMultiplier.hpp"

#include "GaloisOperations.hpp"

#include "../../../../core/Definitions.hpp"
#include "../../../../mem/Endianness.hpp"

#include <arm_neon.h>

#include <array>

namespace erbsland::cryptology::impl {

auto ArmGaloisMultiplier::multiply(const Block &left, const Block &right) const noexcept -> Block {
    auto reflectedLeft = galois::reflect(left);
    auto reflectedRight = galois::reflect(right);
    const auto leftLow = reflectedLeft.getInteger<uint64_t>(unit::ByteIndex{8U}, mem::Endianness::Big);
    const auto leftHigh = reflectedLeft.getInteger<uint64_t>(unit::ByteIndex{}, mem::Endianness::Big);
    const auto rightLow = reflectedRight.getInteger<uint64_t>(unit::ByteIndex{8U}, mem::Endianness::Big);
    const auto rightHigh = reflectedRight.getInteger<uint64_t>(unit::ByteIndex{}, mem::Endianness::Big);

    // PMULL computes the three 64x64 carry-less products; Karatsuba supplies the middle term.
#if defined(ERBSLAND_COMPILER_MSVC)
    // MSVC exposes vmull_p64 through its native-vector wrapper instead of accepting the scalar poly64_t alias.
    const auto lowProduct = vreinterpretq_u64_p128(vmull_p64(
        __poly64x1_t_to_n64(vreinterpret_p64_u64(vcreate_u64(leftLow))),
        __poly64x1_t_to_n64(vreinterpret_p64_u64(vcreate_u64(rightLow)))));
    const auto highProduct = vreinterpretq_u64_p128(vmull_p64(
        __poly64x1_t_to_n64(vreinterpret_p64_u64(vcreate_u64(leftHigh))),
        __poly64x1_t_to_n64(vreinterpret_p64_u64(vcreate_u64(rightHigh)))));
    const auto middleProduct = vreinterpretq_u64_p128(vmull_p64(
        __poly64x1_t_to_n64(vreinterpret_p64_u64(vcreate_u64(leftLow ^ leftHigh))),
        __poly64x1_t_to_n64(vreinterpret_p64_u64(vcreate_u64(rightLow ^ rightHigh)))));
#else
    const auto lowProduct = vreinterpretq_u64_p128(vmull_p64(leftLow, rightLow));
    const auto highProduct = vreinterpretq_u64_p128(vmull_p64(leftHigh, rightHigh));
    const auto middleProduct = vreinterpretq_u64_p128(vmull_p64(leftLow ^ leftHigh, rightLow ^ rightHigh));
#endif
    const auto middleLow =
        vgetq_lane_u64(middleProduct, 0) ^ vgetq_lane_u64(lowProduct, 0) ^ vgetq_lane_u64(highProduct, 0);
    const auto middleHigh =
        vgetq_lane_u64(middleProduct, 1) ^ vgetq_lane_u64(lowProduct, 1) ^ vgetq_lane_u64(highProduct, 1);

    auto result = galois::reduce({
        vgetq_lane_u64(lowProduct, 0),
        vgetq_lane_u64(lowProduct, 1) ^ middleLow,
        vgetq_lane_u64(highProduct, 0) ^ middleHigh,
        vgetq_lane_u64(highProduct, 1),
    });
    result = galois::reflect(result);
    reflectedLeft.secureErase();
    reflectedRight.secureErase();
    return result;
}

}
