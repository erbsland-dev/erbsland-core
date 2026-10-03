// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../impl/PasswordHashData_fwd.hpp"
#include "../PasswordHashAlgorithm.hpp"
#include "../PasswordHashPolicy_fwd.hpp"

#include <cstdint>

namespace erbsland::cryptology::unsafe {

/// Explicit access to custom password-hashing costs.
/// Prefer the reviewed `PasswordHashPolicy` presets. Custom values are checked against hard safety limits, but can
/// still be too cheap for a real deployment.
/// @seedoc{/reference/cryptology/password_hashing}
/// @tested{PasswordHasherTest}
class UnsafeCustomPasswordHashParameters final {
    friend class cryptology::PasswordHashPolicy;
    friend class cryptology::impl::PasswordHashData;

public:
    /// Create checked custom Argon2id parameters.
    /// @param memoryKiB The memory cost in kibibytes. Must be from `8 * lanes` through 1,048,576.
    /// @param passes The pass count. Must be between 1 and 10.
    /// @param lanes The parallel lane count. Must be between 1 and 16.
    /// @return The validated custom parameters.
    [[nodiscard]] static auto argon2id(uint32_t memoryKiB, uint32_t passes, uint32_t lanes)
        -> UnsafeCustomPasswordHashParameters;
    /// Create checked custom scrypt parameters.
    /// @param cost The CPU/memory cost. Must be a power of two greater than one, with `cost * blockSize <= 1,048,576`
    /// and `cost * blockSize * parallelization <= 10,485,760`.
    /// @param blockSize The positive block-size parameter. Must be at most `UINT32_MAX / 128`.
    /// @param parallelization The parallelization parameter. Must be between 1 and 16.
    /// @return The validated custom parameters.
    [[nodiscard]] static auto scrypt(uint64_t cost, uint32_t blockSize, uint32_t parallelization)
        -> UnsafeCustomPasswordHashParameters;

private:
    /// Test whether Argon2id costs are within the implementation's safety limits.
    [[nodiscard]] static auto areValidArgon2idCosts(uint32_t memoryKiB, uint32_t passes, uint32_t lanes) noexcept
        -> bool;
    /// Test whether scrypt costs are within the implementation's safety limits.
    [[nodiscard]] static auto areValidScryptCosts(uint64_t cost, uint32_t blockSize, uint32_t parallelization) noexcept
        -> bool;
    /// Create unchecked custom password-hashing parameters.
    UnsafeCustomPasswordHashParameters(
        PasswordHashAlgorithm algorithm, uint32_t first, uint32_t second, uint32_t third, uint64_t large) noexcept;

private:
    PasswordHashAlgorithm _algorithm; ///< The selected password-hashing algorithm.
    uint32_t _first{};                ///< The first algorithm-specific parameter.
    uint32_t _second{};               ///< The second algorithm-specific parameter.
    uint32_t _third{};                ///< The third algorithm-specific parameter.
    uint64_t _large{};                ///< The wide algorithm-specific parameter.
};

}
