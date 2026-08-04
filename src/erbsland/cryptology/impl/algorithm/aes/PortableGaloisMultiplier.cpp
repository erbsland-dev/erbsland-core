// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PortableGaloisMultiplier.hpp"

namespace erbsland::cryptology::impl {

auto PortableGaloisMultiplier::multiply(const Block &left, const Block &right) const noexcept -> Block {
    auto result = Block{};
    auto value = right;

    // SP 800-38D, Algorithm 1: consume all 128 bits from most significant to least significant.
    for (auto bit = 0U; bit < 128U; ++bit) {
        const auto source = left.get(unit::ByteIndex{bit / 8U}).toUInt8();
        const auto sourceBit = static_cast<uint8_t>((source >> (7U - bit % 8U)) & 1U);
        const auto sourceMask = static_cast<uint8_t>(0U - sourceBit);
        for (auto index = unit::ByteIndex{}; index < Block::endIndex(); ++index) {
            result.xorAt(index, mem::Byte{static_cast<uint8_t>(value.get(index).toUInt8() & sourceMask)});
        }

        // SP 800-38D, Algorithm 1: right shift and conditionally apply R = 11100001 || 0^120.
        const auto leastSignificantBit = static_cast<uint8_t>(value.get(unit::ByteIndex{15U}).toUInt8() & 1U);
        value.shiftRight(1U);
        const auto reductionMask = static_cast<uint8_t>(0U - leastSignificantBit);
        value.xorAt(unit::ByteIndex{}, mem::Byte{static_cast<uint8_t>(0xe1U & reductionMask)});
    }
    return result;
}

}
