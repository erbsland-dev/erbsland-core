// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "FastRandom.hpp"

#include <mutex>

namespace erbsland::random {

/// A thread-safe fast pseudo-random generator for shared non-security use.
/// @seedoc{/topics/random/overview}
/// @tested{ThreadSafeFastRandomTest}
class ThreadSafeFastRandom final : public Random {
public:
    /// Create a generator with automatic seed data.
    ThreadSafeFastRandom();
    /// Create a generator with an explicit seed for reproducible sequences.
    explicit ThreadSafeFastRandom(uint64_t seed);

    // defaults
    ~ThreadSafeFastRandom() override = default;
    ThreadSafeFastRandom(const ThreadSafeFastRandom &) = delete;
    auto operator=(const ThreadSafeFastRandom &) -> ThreadSafeFastRandom & = delete;
    ThreadSafeFastRandom(ThreadSafeFastRandom &&) = delete;
    auto operator=(ThreadSafeFastRandom &&) -> ThreadSafeFastRandom & = delete;

public: // implement Random
    [[nodiscard]] auto getInt32(int32_t minimum, int32_t maximum) -> int32_t override;
    [[nodiscard]] auto getUInt32(uint32_t minimum, uint32_t maximum) -> uint32_t override;
    [[nodiscard]] auto getInt64(int64_t minimum, int64_t maximum) -> int64_t override;
    [[nodiscard]] auto getUInt64(uint64_t minimum, uint64_t maximum) -> uint64_t override;
    [[nodiscard]] auto getDouble(double minimum, double maximum) -> double override;
    [[nodiscard]] auto getBool() -> bool override;
    void fillBytes(std::span<std::byte> destination) override;

private:
    FastRandom _random; ///< The synchronized generator.
    std::mutex _mutex;  ///< The generator mutex.
};

}
