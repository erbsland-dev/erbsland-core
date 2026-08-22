// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BitReader.hpp"

#include <algorithm>

namespace erbsland::mem {

BitReader::BitReader(const ConstByteSpan data, const std::size_t bitPosition) noexcept : _data{data} {
    setBitPosition(bitPosition);
}

void BitReader::setBitPosition(const std::size_t bitPosition) noexcept {
    _bitPosition = std::min(bitPosition, bitCount());
}

void BitReader::advance(const std::size_t count) noexcept {
    _bitPosition += std::min(count, remainingBitCount());
}

auto BitReader::readBool() noexcept -> bool {
    if (isAtEnd()) {
        return false;
    }
    const auto byte = _data[_bitPosition / 8U].toUInt8();
    const auto shift = 7U - (_bitPosition % 8U);
    ++_bitPosition;
    return ((byte >> shift) & 1U) != 0U;
}

}
