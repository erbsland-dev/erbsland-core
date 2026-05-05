// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SecureRandom.hpp"

#include <array>
#include <bit>
#include <limits>

namespace erbsland::random {

SecureRandom::SecureRandom() : _entropySource{std::make_unique<impl::SystemEntropySource>()} {
}

auto SecureRandom::getInt32(int32_t minimum, int32_t maximum) -> int32_t {
    math::orderMinimumMaximum(minimum, maximum);
    return static_cast<int32_t>(getInt64(minimum, maximum));
}

auto SecureRandom::getUInt32(uint32_t minimum, uint32_t maximum) -> uint32_t {
    math::orderMinimumMaximum(minimum, maximum);
    return static_cast<uint32_t>(randomBoundedUInt64(minimum, maximum));
}

auto SecureRandom::getInt64(int64_t minimum, int64_t maximum) -> int64_t {
    math::orderMinimumMaximum(minimum, maximum);
    const auto orderedMinimum = toOrderedInt64(minimum);
    const auto orderedMaximum = toOrderedInt64(maximum);
    return fromOrderedInt64(randomBoundedUInt64(orderedMinimum, orderedMaximum));
}

auto SecureRandom::getUInt64(uint64_t minimum, uint64_t maximum) -> uint64_t {
    math::orderMinimumMaximum(minimum, maximum);
    return randomBoundedUInt64(minimum, maximum);
}

auto SecureRandom::getDouble(double minimum, double maximum) -> double {
    math::orderMinimumMaximum(minimum, maximum);
    const auto raw = randomUInt64() >> 11U;
    const auto unit = static_cast<double>(raw) * (1.0 / 9007199254740992.0);
    return minimum + ((maximum - minimum) * unit);
}

auto SecureRandom::getBool() -> bool {
    return (randomUInt64() & 1U) != 0U;
}

void SecureRandom::fillBytes(const std::span<std::byte> destination) {
    _entropySource->fillBytes(destination);
}

auto SecureRandom::randomUInt64() -> uint64_t {
    auto bytes = std::array<std::byte, sizeof(uint64_t)>{};
    fillBytes(bytes);
    return std::bit_cast<uint64_t>(bytes);
}

auto SecureRandom::randomBoundedUInt64(const uint64_t minimum, const uint64_t maximum) -> uint64_t {
    const auto range = maximum - minimum;
    if (range == std::numeric_limits<uint64_t>::max()) {
        return randomUInt64();
    }
    const auto bound = range + 1U;
    const auto threshold = (uint64_t{0U} - bound) % bound;
    while (true) {
        const auto value = randomUInt64();
        if (value >= threshold) {
            return minimum + (value % bound);
        }
    }
}

auto SecureRandom::toOrderedInt64(const int64_t value) noexcept -> uint64_t {
    return std::bit_cast<uint64_t>(value) ^ (uint64_t{1U} << 63U);
}

auto SecureRandom::fromOrderedInt64(const uint64_t value) noexcept -> int64_t {
    return std::bit_cast<int64_t>(value ^ (uint64_t{1U} << 63U));
}

}
