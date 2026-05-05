// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "FastRandom.hpp"

#include <array>

namespace erbsland::random {

FastRandom::FastRandom() {
    auto seedData = std::array<std::random_device::result_type, 16>{};
    auto device = std::random_device{};
    std::ranges::generate(seedData, [&device]() -> std::random_device::result_type { return device(); });
    auto seedSequence = std::seed_seq{seedData.begin(), seedData.end()};
    _engine.seed(seedSequence);
}

FastRandom::FastRandom(const uint64_t seed) : _engine{seed} {
}

auto FastRandom::getInt32(int32_t minimum, int32_t maximum) -> int32_t {
    math::orderMinimumMaximum(minimum, maximum);
    auto distribution = std::uniform_int_distribution<int32_t>{minimum, maximum};
    return distribution(_engine);
}

auto FastRandom::getUInt32(uint32_t minimum, uint32_t maximum) -> uint32_t {
    math::orderMinimumMaximum(minimum, maximum);
    auto distribution = std::uniform_int_distribution<uint32_t>{minimum, maximum};
    return distribution(_engine);
}

auto FastRandom::getInt64(int64_t minimum, int64_t maximum) -> int64_t {
    math::orderMinimumMaximum(minimum, maximum);
    auto distribution = std::uniform_int_distribution<int64_t>{minimum, maximum};
    return distribution(_engine);
}

auto FastRandom::getUInt64(uint64_t minimum, uint64_t maximum) -> uint64_t {
    math::orderMinimumMaximum(minimum, maximum);
    auto distribution = std::uniform_int_distribution<uint64_t>{minimum, maximum};
    return distribution(_engine);
}

auto FastRandom::getDouble(double minimum, double maximum) -> double {
    math::orderMinimumMaximum(minimum, maximum);
    auto distribution = std::uniform_real_distribution<double>{minimum, maximum};
    return distribution(_engine);
}

auto FastRandom::getBool() -> bool {
    auto distribution = std::bernoulli_distribution{};
    return distribution(_engine);
}

void FastRandom::fillBytes(const std::span<std::byte> destination) {
    for (auto &byte : destination) {
        byte = static_cast<std::byte>(getUInt32(0U, 0xffU));
    }
}

}
