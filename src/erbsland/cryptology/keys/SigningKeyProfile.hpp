// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::cryptology {

/// A modern signing-key generation profile with fixed safe parameters.
enum class SigningKeyProfile : uint8_t {
    EcdsaP256, ///< ECDSA over NIST P-256.
    EcdsaP384, ///< ECDSA over NIST P-384.
    Rsa2048,   ///< Two-prime RSA with a 2048-bit modulus and e=65537.
    Rsa3072,   ///< Two-prime RSA with a 3072-bit modulus and e=65537.
    Rsa4096,   ///< Two-prime RSA with a 4096-bit modulus and e=65537.
};

}
