// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PasswordHashAlgorithm.hpp"
#include "PasswordHashPolicy_fwd.hpp"

#include "unsafe/UnsafeCustomPasswordHashParameters.hpp"

#include "../unit/ByteLength.hpp"

#include <cstdint>

namespace erbsland::cryptology {

/// A reviewed password-hashing policy.
/// Safe presets always use a 16-byte salt and a 32-byte raw output.
/// @seedoc{/reference/cryptology/password_hashing}
/// @tested{PasswordHasherTest}
class PasswordHashPolicy final {
public:
    /// Create the recommended Argon2id policy.
    PasswordHashPolicy() noexcept;
    /// Create a checked custom policy through the explicitly unsafe parameter API.
    /// @param parameters The checked algorithm-specific costs.
    explicit PasswordHashPolicy(const unsafe::UnsafeCustomPasswordHashParameters &parameters) noexcept;

public: // tests
    /// Test whether all algorithm and cost parameters match another policy.
    [[nodiscard]] auto isEqualTo(const PasswordHashPolicy &other) const noexcept -> bool;

public: // accessors
    /// Get the password-hashing algorithm.
    [[nodiscard]] auto algorithm() const noexcept -> PasswordHashAlgorithm { return _algorithm; }
    /// Get the salt length.
    [[nodiscard]] auto saltLength() const noexcept -> unit::ByteLength { return unit::ByteLength{16U}; }
    /// Get the derived-output length.
    [[nodiscard]] auto outputLength() const noexcept -> unit::ByteLength { return unit::ByteLength{32U}; }
    /// Get the Argon2id memory cost in kibibytes.
    [[nodiscard]] auto memoryKiB() const noexcept -> uint32_t { return _memoryKiB; }
    /// Get the Argon2id pass count.
    [[nodiscard]] auto passes() const noexcept -> uint32_t { return _passes; }
    /// Get the Argon2id lane count.
    [[nodiscard]] auto lanes() const noexcept -> uint32_t { return _lanes; }
    /// Get the scrypt CPU/memory cost parameter.
    [[nodiscard]] auto scryptCost() const noexcept -> uint64_t { return _scryptCost; }
    /// Get the scrypt block-size parameter.
    [[nodiscard]] auto scryptBlockSize() const noexcept -> uint32_t { return _scryptBlockSize; }
    /// Get the scrypt parallelization parameter.
    [[nodiscard]] auto scryptParallelization() const noexcept -> uint32_t { return _scryptParallelization; }

public: // factories
    /// Argon2id with 64 MiB, three passes, and four lanes.
    [[nodiscard]] static auto recommended() noexcept -> PasswordHashPolicy;
    /// Argon2id with 19 MiB, two passes, and one lane.
    [[nodiscard]] static auto lowMemory() noexcept -> PasswordHashPolicy;
    /// scrypt with N=2^17, r=8, and p=1.
    [[nodiscard]] static auto scrypt() noexcept -> PasswordHashPolicy;

private:
    /// Create a policy from normalized algorithm-specific parameters.
    PasswordHashPolicy(
        PasswordHashAlgorithm algorithm,
        uint32_t memoryKiB,
        uint32_t passes,
        uint32_t lanes,
        uint64_t scryptCost,
        uint32_t scryptBlockSize,
        uint32_t scryptParallelization) noexcept;

private:
    PasswordHashAlgorithm _algorithm;  ///< The selected password-hashing algorithm.
    uint32_t _memoryKiB{};             ///< The Argon2id memory cost in kibibytes.
    uint32_t _passes{};                ///< The Argon2id pass count.
    uint32_t _lanes{};                 ///< The Argon2id lane count.
    uint64_t _scryptCost{};            ///< The scrypt CPU/memory cost parameter.
    uint32_t _scryptBlockSize{};       ///< The scrypt block-size parameter.
    uint32_t _scryptParallelization{}; ///< The scrypt parallelization parameter.
};

}
