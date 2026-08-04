// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

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

public:
    /// Create checked custom Argon2id parameters.
    /// @param memoryKiB The memory cost in kibibytes.
    /// @param passes The pass count.
    /// @param lanes The parallel lane count.
    /// @return The validated custom parameters.
    /// @throws err::ParameterError If a value is outside the implementation's safety limits.
    [[nodiscard]] static auto argon2id(uint32_t memoryKiB, uint32_t passes, uint32_t lanes)
        -> UnsafeCustomPasswordHashParameters;
    /// Create checked custom scrypt parameters.
    /// @param cost The CPU/memory cost parameter, which must be a power of two.
    /// @param blockSize The block-size parameter.
    /// @param parallelization The parallelization parameter.
    /// @return The validated custom parameters.
    /// @throws err::ParameterError If a value is outside the implementation's safety limits.
    [[nodiscard]] static auto scrypt(uint64_t cost, uint32_t blockSize, uint32_t parallelization)
        -> UnsafeCustomPasswordHashParameters;

private:
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
