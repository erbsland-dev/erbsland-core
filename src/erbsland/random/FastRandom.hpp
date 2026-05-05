// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Random.hpp"

#include <random>

namespace erbsland::random {

/// A fast pseudo-random generator for non-security use.
/// @seedoc{/topics/random/overview}
/// @tested{FastRandomTest}
class FastRandom final : public Random {
public:
    /// Create a generator with automatic seed data.
    FastRandom();
    /// Create a generator with an explicit seed for reproducible sequences.
    explicit FastRandom(uint64_t seed);

    // defaults
    ~FastRandom() override = default;
    FastRandom(const FastRandom &) = delete;
    auto operator=(const FastRandom &) -> FastRandom & = delete;
    FastRandom(FastRandom &&) = delete;
    auto operator=(FastRandom &&) -> FastRandom & = delete;

public: // implement Random
    [[nodiscard]] auto getInt32(int32_t minimum, int32_t maximum) -> int32_t override;
    [[nodiscard]] auto getUInt32(uint32_t minimum, uint32_t maximum) -> uint32_t override;
    [[nodiscard]] auto getInt64(int64_t minimum, int64_t maximum) -> int64_t override;
    [[nodiscard]] auto getUInt64(uint64_t minimum, uint64_t maximum) -> uint64_t override;
    [[nodiscard]] auto getDouble(double minimum, double maximum) -> double override;
    [[nodiscard]] auto getBool() -> bool override;
    void fillBytes(std::span<std::byte> destination) override;

private:
    std::mt19937_64 _engine; ///< The pseudo-random engine.
};

}
