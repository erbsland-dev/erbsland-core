// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Random.hpp"

#include "impl/SystemEntropySource.hpp"

#include <memory>

namespace erbsland::random {

/// A cryptographic random generator backed by the operating system entropy source.
/// Every method that draws random data throws `random::RandomError` if the system entropy source cannot provide data.
/// @seedoc{/topics/random/secure_randomness}
/// @tested{SecureRandomTest}
class SecureRandom final : public Random {
public:
    /// Create a secure generator using the system entropy source.
    SecureRandom();

    // defaults
    ~SecureRandom() override = default;
    SecureRandom(const SecureRandom &) = delete;
    auto operator=(const SecureRandom &) -> SecureRandom & = delete;
    SecureRandom(SecureRandom &&) = delete;
    auto operator=(SecureRandom &&) -> SecureRandom & = delete;

public: // implement Random
    [[nodiscard]] auto isSecure() const noexcept -> bool override { return true; }
    [[nodiscard]] auto getInt32(int32_t minimum, int32_t maximum) -> int32_t override;
    [[nodiscard]] auto getUInt32(uint32_t minimum, uint32_t maximum) -> uint32_t override;
    [[nodiscard]] auto getInt64(int64_t minimum, int64_t maximum) -> int64_t override;
    [[nodiscard]] auto getUInt64(uint64_t minimum, uint64_t maximum) -> uint64_t override;
    [[nodiscard]] auto getDouble(double minimum, double maximum) -> double override;
    [[nodiscard]] auto getBool() -> bool override;
    void fillBytes(std::span<std::byte> destination) override;

private:
    /// Obtain an unbiased random 64-bit value from the entropy source.
    [[nodiscard]] auto randomUInt64() -> uint64_t;
    /// Obtain an unbiased value in an inclusive unsigned range.
    [[nodiscard]] auto randomBoundedUInt64(uint64_t minimum, uint64_t maximum) -> uint64_t;
    /// Map a signed 64-bit value to unsigned ordering.
    [[nodiscard]] static auto toOrderedInt64(int64_t value) noexcept -> uint64_t;
    /// Map an ordered unsigned value back to a signed value.
    [[nodiscard]] static auto fromOrderedInt64(uint64_t value) noexcept -> int64_t;

private:
    std::unique_ptr<impl::EntropySource> _entropySource; ///< The OS entropy source.
};

}
