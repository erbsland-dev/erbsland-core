// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "GaloisOperations.hpp"

#include "../../../../mem/Endianness.hpp"

namespace erbsland::cryptology::impl::galois {

auto reflect(const mem::ByteArray<16> &value) noexcept -> mem::ByteArray<16> {
    auto result = mem::ByteArray<16>{};
    for (auto index = std::size_t{}; index < 16U; ++index) {
        auto byte = value.get(unit::ByteIndex{index});
        // Reverse the bits without lookup tables so the conversion is independent of field values.
        byte = static_cast<mem::Byte>(((byte & 0x55U) << 1U) | ((byte >> 1U) & 0x55U));
        byte = static_cast<mem::Byte>(((byte & 0x33U) << 2U) | ((byte >> 2U) & 0x33U));
        byte = static_cast<mem::Byte>(((byte & 0x0fU) << 4U) | ((byte >> 4U) & 0x0fU));
        result.set(unit::ByteIndex{15U - index}, byte);
    }
    return result;
}

auto reduce(std::array<uint64_t, 4> product) noexcept -> mem::ByteArray<16> {
    // Reduce from the highest coefficient down so newly introduced high coefficients are processed later.
    for (auto bit = 255; bit >= 128; --bit) {
        const auto word = static_cast<std::size_t>(bit / 64);
        const auto wordBit = static_cast<unsigned int>(bit % 64);
        const auto coefficientMask = uint64_t{0U} - ((product[word] >> wordBit) & 1U);
        product[word] ^= (uint64_t{1U} << wordBit) & coefficientMask;
        const auto base = bit - 128;
        for (const auto offset : {0, 1, 2, 7}) {
            const auto target = base + offset;
            product[static_cast<std::size_t>(target / 64)] ^=
                (uint64_t{1U} << static_cast<unsigned int>(target % 64)) & coefficientMask;
        }
    }

    auto result = mem::ByteArray<16>{};
    result.setIntegerOrThrow(unit::ByteIndex{}, product[1], mem::Endianness::Big);
    result.setIntegerOrThrow(unit::ByteIndex{8U}, product[0], mem::Endianness::Big);
    return result;
}

}
