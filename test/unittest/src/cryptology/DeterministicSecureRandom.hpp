// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/random/Random.hpp>

#include <cstddef>
#include <cstdint>
#include <span>

namespace erbsland::test {

/// A deterministic secure-random test double that emits consecutive byte values.
class DeterministicSecureRandom final : public random::Random {
public:
    [[nodiscard]] auto isSecure() const noexcept -> bool override { return true; }
    [[nodiscard]] auto getInt32(int32_t minimum, int32_t) -> int32_t override { return minimum; }
    [[nodiscard]] auto getUInt32(uint32_t minimum, uint32_t) -> uint32_t override { return minimum; }
    [[nodiscard]] auto getInt64(int64_t minimum, int64_t) -> int64_t override { return minimum; }
    [[nodiscard]] auto getUInt64(uint64_t minimum, uint64_t) -> uint64_t override { return minimum; }
    [[nodiscard]] auto getDouble(double minimum, double) -> double override { return minimum; }
    [[nodiscard]] auto getBool() -> bool override { return false; }
    void fillBytes(std::span<std::byte> destination) override {
        for (auto &byte : destination) {
            byte = static_cast<std::byte>(_next++);
        }
    }

private:
    uint8_t _next{}; ///< Next deterministic byte value.
};

}
