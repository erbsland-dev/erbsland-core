// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ThreadSafeFastRandom.hpp"

namespace erbsland::random {

ThreadSafeFastRandom::ThreadSafeFastRandom() = default;

ThreadSafeFastRandom::ThreadSafeFastRandom(const uint64_t seed) : _random{seed} {
}

auto ThreadSafeFastRandom::getInt32(const int32_t minimum, const int32_t maximum) -> int32_t {
    auto lock = std::scoped_lock{_mutex};
    return _random.getInt32(minimum, maximum);
}

auto ThreadSafeFastRandom::getUInt32(const uint32_t minimum, const uint32_t maximum) -> uint32_t {
    auto lock = std::scoped_lock{_mutex};
    return _random.getUInt32(minimum, maximum);
}

auto ThreadSafeFastRandom::getInt64(const int64_t minimum, const int64_t maximum) -> int64_t {
    auto lock = std::scoped_lock{_mutex};
    return _random.getInt64(minimum, maximum);
}

auto ThreadSafeFastRandom::getUInt64(const uint64_t minimum, const uint64_t maximum) -> uint64_t {
    auto lock = std::scoped_lock{_mutex};
    return _random.getUInt64(minimum, maximum);
}

auto ThreadSafeFastRandom::getDouble(const double minimum, const double maximum) -> double {
    auto lock = std::scoped_lock{_mutex};
    return _random.getDouble(minimum, maximum);
}

auto ThreadSafeFastRandom::getBool() -> bool {
    auto lock = std::scoped_lock{_mutex};
    return _random.getBool();
}

void ThreadSafeFastRandom::fillBytes(const std::span<std::byte> destination) {
    auto lock = std::scoped_lock{_mutex};
    _random.fillBytes(destination);
}

}
