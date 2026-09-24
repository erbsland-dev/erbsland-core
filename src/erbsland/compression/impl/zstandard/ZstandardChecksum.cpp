// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ZstandardChecksum.hpp"

namespace erbsland::compression::impl {

[[nodiscard]] auto ZstandardChecksum::read64(const mem::ConstByteSpan input, const std::size_t position) noexcept
    -> uint64_t {
    auto result = uint64_t{};
    for (auto index = std::size_t{}; index < 8U; ++index) {
        result |= input[position + index].toUInt64() << (index * 8U);
    }
    return result;
}

[[nodiscard]] auto ZstandardChecksum::read32(const mem::ConstByteSpan input, const std::size_t position) noexcept
    -> uint32_t {
    auto result = uint32_t{};
    for (auto index = std::size_t{}; index < 4U; ++index) {
        result |= input[position + index].toUInt32() << (index * 8U);
    }
    return result;
}

[[nodiscard]] auto ZstandardChecksum::round(const uint64_t accumulator, const uint64_t input) noexcept -> uint64_t {
    auto value = accumulator + input * cXxHashPrime2;
    value = std::rotl(value, 31);
    return value * cXxHashPrime1;
}

void ZstandardChecksum::update(mem::ConstByteSpan input) noexcept {
    _length += input.size();
    for (const auto byte : input) {
        _tail.set(unit::ByteIndex::fromSizeT(_used++), byte);
        if (_used == 32U) {
            for (std::size_t i{}; i < 4U; ++i) {
                _state[i] = round(_state[i], read64(_tail.span(), i * 8U));
            }
            _used = 0;
        }
    }
}
auto ZstandardChecksum::value() const noexcept -> uint64_t {
    std::size_t position{};
    uint64_t hash;
    const auto input = _tail.span().first(_used);
    if (_length >= 32U) {
        const auto [v1, v2, v3, v4] = _state;
        hash = std::rotl(v1, 1) + std::rotl(v2, 7) + std::rotl(v3, 12) + std::rotl(v4, 18);
        for (const auto value : {v1, v2, v3, v4}) {
            hash ^= round(0U, value);
            hash = hash * cXxHashPrime1 + cXxHashPrime4;
        }
    } else {
        hash = cXxHashPrime5;
    }
    hash += _length;
    while (position + 8U <= input.size()) {
        const auto value = round(0U, read64(input, position));
        hash ^= value;
        hash = std::rotl(hash, 27) * cXxHashPrime1 + cXxHashPrime4;
        position += 8U;
    }
    if (position + 4U <= input.size()) {
        hash ^= static_cast<uint64_t>(read32(input, position)) * cXxHashPrime1;
        hash = std::rotl(hash, 23) * cXxHashPrime2 + cXxHashPrime3;
        position += 4U;
    }
    while (position < input.size()) {
        hash ^= input[position++].toUInt64() * cXxHashPrime5;
        hash = std::rotl(hash, 11) * cXxHashPrime1;
    }
    hash ^= hash >> 33U;
    hash *= cXxHashPrime2;
    hash ^= hash >> 29U;
    hash *= cXxHashPrime3;
    return hash ^ (hash >> 32U);
}

}
