// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Bzip2Crc.hpp"

namespace erbsland::compression::impl {

void Bzip2Crc::update(const uint8_t value) noexcept {
    auto current = _value ^ (static_cast<uint32_t>(value) << 24U);
    for (auto bit = 0U; bit < cByteBitCount; ++bit) {
        current = (current << 1U) ^ ((current & cHighBit) != 0U ? cPolynomial : 0U);
    }
    _value = current;
}

[[nodiscard]] auto Bzip2Crc::value() const noexcept -> uint32_t {
    return ~_value;
}

}
