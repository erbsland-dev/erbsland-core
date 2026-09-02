// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SigningKeyGenerator_fwd.hpp"

#include "../keys/SigningKeyProfile.hpp"
#include "../keys/SigningPrivateKey_fwd.hpp"

namespace erbsland::cryptology::impl {

/// Bounded signing-key generation using application secure randomness.
///
/// EC scalar generation follows FIPS 186-5 section A.2.2 by rejection sampling into the interval `[1, n-1]`.
/// RSA generation delegates to the fixed-width FIPS 186-5 key-generation implementation.
/// @tested{X509CertificateBuilderTest}
class SigningKeyGenerator final {
public:
    /// Create a generator for one fixed key profile.
    explicit SigningKeyGenerator(SigningKeyProfile profile) noexcept : _profile{profile} {}

public:
    /// Generate a key for one fixed profile.
    [[nodiscard]] auto generate() const -> SigningPrivateKey;

private:
    /// Generate a fixed-profile EC key.
    [[nodiscard]] auto generateEc() const -> SigningPrivateKey;
    /// Generate a fixed-profile two-prime RSA key.
    [[nodiscard]] auto generateRsa() const -> SigningPrivateKey;

private:
    SigningKeyProfile _profile;
};

}
