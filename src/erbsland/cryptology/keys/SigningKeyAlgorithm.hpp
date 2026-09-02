// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::cryptology {

/// A private-key algorithm supported for TLS 1.3 signing.
enum class SigningKeyAlgorithm : uint8_t {
    Unknown,   ///< No signing-key algorithm.
    Ed25519,   ///< Pure Ed25519 using a 32-octet private seed.
    EcdsaP256, ///< ECDSA over NIST P-256 with SHA-256.
    EcdsaP384, ///< ECDSA over NIST P-384 with SHA-384.
    Rsa,       ///< Two-prime RSA for RSASSA-PSS.
};

}
