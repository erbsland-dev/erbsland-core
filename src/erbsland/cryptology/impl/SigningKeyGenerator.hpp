// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SigningKeyGenerator_fwd.hpp"

#include "../keys/SigningKeyProfile.hpp"
#include "../keys/SigningPrivateKey_fwd.hpp"

namespace erbsland::cryptology::impl {

/// Bounded signing-key generation using application secure randomness.
///
/// EC generation adds the library's extreme-scalar policy to FIPS 186-5 section A.2.2 rejection sampling.
/// Exhausting 128 EC candidates throws the fatal logic failure `random::RandomFailure`.
/// RSA generation delegates to the fixed-width FIPS 186-5 key-generation implementation.
/// @seedoc{/reference/cryptology/key_management}
/// @tested{X509CertificateBuilderTest EcSigningKeyTest}
class SigningKeyGenerator final {
    static constexpr auto cMaximumEcAttempts = 128U; ///< Maximum total EC candidate draws per generated key.

public:
    /// Create a generator for one fixed key profile.
    explicit SigningKeyGenerator(SigningKeyProfile profile) noexcept : _profile{profile} {}

public:
    /// Generate a key for one fixed profile.
    [[nodiscard]] auto generate() const -> SigningPrivateKey;

private:
    /// Generate a pure Ed25519 key.
    [[nodiscard]] auto generateEd25519() const -> SigningPrivateKey;
    /// Generate a fixed-profile EC key.
    [[nodiscard]] auto generateEc() const -> SigningPrivateKey;
    /// Generate a fixed-profile two-prime RSA key.
    [[nodiscard]] auto generateRsa() const -> SigningPrivateKey;

private:
    SigningKeyProfile _profile;
};

}
