// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "RsaSignature.hpp"

#include "../../../../mem/ByteSpan.hpp"
#include "../../DerEncoder.hpp"

namespace erbsland::cryptology::impl::rsa_signer {

/// Parsed two-prime PKCS#1 private components.
///
/// The representation follows RFC 8017 appendix A.1.2. Every integer uses bounded little-endian 32-bit limbs and is
/// erased as one unit on every exit from validation or signing.
/// @tested{SigningPrivateKeyTest}
struct PrivateKeyData final {
    rsa_signature::Number modulus;         ///< Public modulus n.
    mem::ByteBlock publicExponent;         ///< Public exponent e, minimal big-endian.
    rsa_signature::Number privateExponent; ///< Private exponent d.
    rsa_signature::Number prime1;          ///< First prime p.
    rsa_signature::Number prime2;          ///< Second prime q.
    rsa_signature::Number exponent1;       ///< d mod (p - 1).
    rsa_signature::Number exponent2;       ///< d mod (q - 1).
    rsa_signature::Number coefficient;     ///< q^-1 mod p.
    std::size_t modulusBits{};             ///< Exact public modulus bit length.
    std::size_t encodedLength{};           ///< RFC 8017 modulus length k.

    /// Erase all private and derived arithmetic components.
    void secureErase() noexcept;
};

/// Parse and validate one RFC 8017 two-prime RSAPrivateKey.
[[nodiscard]] auto decodePrivateKey(mem::ConstByteSpan privateKey) -> PrivateKeyData;
/// Validate a private key and append SubjectPublicKeyInfo using the exact PKCS#8 AlgorithmIdentifier.
/// @param encoder Destination DER encoder.
/// @param privateKey PKCS#1 private-key data.
/// @param algorithmIdentifier Exact validated PKCS#8 AlgorithmIdentifier.
/// @tested{SigningPrivateKeyTest}
void appendPublicKey(DerEncoder &encoder, mem::ConstByteSpan privateKey, mem::ConstByteSpan algorithmIdentifier);
/// Produce an RSA-PSS signature using application secure randomness for salt and message blinding.
[[nodiscard]] auto sign(
    mem::ConstByteSpan privateKey, TlsSignatureScheme scheme, mem::ConstByteSpan message, const PublicKey &publicKey)
    -> mem::ByteBlock;
/// Produce an RSA-PSS signature with explicit deterministic test randomness.
[[nodiscard]] auto signWithRandom(
    mem::ConstByteSpan privateKey,
    TlsSignatureScheme scheme,
    mem::ConstByteSpan message,
    const PublicKey &publicKey,
    mem::ConstByteSpan salt,
    mem::ConstByteSpan blindingFactor) -> mem::ByteBlock;

}
