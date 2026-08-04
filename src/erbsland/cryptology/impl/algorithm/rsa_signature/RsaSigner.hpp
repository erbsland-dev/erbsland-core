// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "RsaSignature.hpp"

#include "../../../../mem/ByteSpan.hpp"

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
/// Validate a private key and derive SubjectPublicKeyInfo using the exact PKCS#8 AlgorithmIdentifier.
[[nodiscard]] auto publicKey(mem::ConstByteSpan privateKey, mem::ConstByteSpan algorithmIdentifier) -> mem::ByteBlock;
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

/// Reduce an arbitrary bounded value modulo an odd or even secret modulus with a fixed bit schedule.
[[nodiscard]] auto reduceSecret(const rsa_signature::Number &value, const rsa_signature::Number &modulus) noexcept
    -> rsa_signature::Number;
/// Multiply reduced values modulo an odd or even secret modulus with a fixed bit schedule.
[[nodiscard]] auto multiplyModuloSecret(
    const rsa_signature::Number &left,
    const rsa_signature::Number &right,
    const rsa_signature::Number &modulus) noexcept -> rsa_signature::Number;
/// Subtract two reduced values modulo a secret modulus with masked correction.
[[nodiscard]] auto subtractModuloSecret(
    const rsa_signature::Number &left,
    const rsa_signature::Number &right,
    const rsa_signature::Number &modulus) noexcept -> rsa_signature::Number;
/// Raise a reduced base to an exponent with a fixed Montgomery square-and-multiply-always schedule.
[[nodiscard]] auto powerModuloSecret(
    const rsa_signature::Number &base,
    const rsa_signature::Number &exponent,
    const rsa_signature::Number &oddModulus) noexcept -> rsa_signature::Number;
/// Multiply two integers without modular reduction into the caller-selected fixed width.
[[nodiscard]] auto multiplyExact(
    const rsa_signature::Number &left, const rsa_signature::Number &right, std::size_t resultWords) noexcept
    -> rsa_signature::Number;

}
