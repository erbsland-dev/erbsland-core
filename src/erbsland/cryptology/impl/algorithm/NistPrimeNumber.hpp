// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../mem/SecureErase.hpp"

#include <array>
#include <cstdint>
#include <span>

namespace erbsland::cryptology::impl {

/// Owned, automatically erased fixed-width arithmetic scratch.
/// @tested{P256KeyAgreementTest SigningPrivateKeyTest NistPrimeNumberTest}
class NistPrimeNumber final {
public:
    /// Copy public curve parameters into independently erased arithmetic scratch.
    /// @param parameters The little-endian base-2^32 limbs to copy.
    NistPrimeNumber(const std::array<uint32_t, 12U> &parameters) noexcept : _words{parameters} {}
    /// Erase every scratch value, including unnamed intermediate results.
    ~NistPrimeNumber() { secureErase(); }

    // defaults
    NistPrimeNumber() = default;
    NistPrimeNumber(const NistPrimeNumber &) = default;

public:
    /// Install a completed replacement after erasing the previous limbs.
    /// @param other The independently owned value to copy.
    /// @return This number.
    auto operator=(const NistPrimeNumber &other) noexcept -> NistPrimeNumber & {
        if (this != &other) {
            secureErase();
            _words = other._words;
        }
        return *this;
    }

public: // accessors
    /// Access the fixed-size limb storage for arithmetic operations.
    [[nodiscard]] auto words() noexcept -> std::span<uint32_t, 12U> { return _words; }
    /// Read the fixed-size limb storage.
    [[nodiscard]] auto words() const noexcept -> std::span<const uint32_t, 12U> { return _words; }

public: // erasure
    /// Erase every fixed-width limb.
    void secureErase() noexcept { mem::secureErase(std::span{_words}); }

private:
    std::array<uint32_t, 12U> _words{}; ///< Little-endian base-2^32 limbs.
};

}
