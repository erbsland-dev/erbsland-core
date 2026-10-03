// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Number.hpp"
#include "RsaSigner.hpp"

#include "../../../../mem/ByteBlock.hpp"
#include "../../../../mem/ByteSpan.hpp"
#include "../../../HashAlgorithm.hpp"
#include "../../../keys/PublicKey_fwd.hpp"
#include "../../../tls/TlsSignatureScheme.hpp"

#include <cstddef>

namespace erbsland::cryptology::impl::rsa_signer::signing {

/// Select the digest for a TLS 1.3 RSA-PSS signature scheme.
/// @param scheme An RSA-PSS signature scheme.
/// @return The SHA-2 digest required by the scheme.
/// @tested{SigningPrivateKeyTest}
[[nodiscard]] auto hashForScheme(TlsSignatureScheme scheme) -> HashAlgorithm;
/// Encode a message using EMSA-PSS from RFC 8017 section 9.1.1.
/// @param hash The message and mask-generation digest.
/// @param message The message bytes.
/// @param salt Salt with exactly the digest length.
/// @param emBits The modulus bit length minus one; must accommodate the digest and salt.
/// @return The sensitive encoded message representative.
/// @tested{SigningPrivateKeyTest}
[[nodiscard]] auto encodePss(
    HashAlgorithm hash, mem::ConstByteSpan message, mem::ConstByteSpan salt, std::size_t emBits) -> mem::ByteBlock;
/// Apply the private RSA primitive using two-prime CRT arithmetic.
/// @param key The validated private-key components.
/// @param value The blinded message representative below the modulus.
/// @return The private exponentiation result at the modulus width.
/// @tested{SigningPrivateKeyTest}
[[nodiscard]] auto crtExponentiation(const PrivateKeyData &key, const rsa_signature::Number &value) noexcept
    -> rsa_signature::Number;
/// Sign with validated key components and verify the result before releasing it.
/// @param key The validated private-key components.
/// @param scheme An RSA-PSS signature scheme.
/// @param message The message bytes.
/// @param publicKeyValue The matching public key used for fault detection.
/// @param salt Salt with exactly the scheme's digest length.
/// @param blindingFactor A modulus-sized big-endian value in the interval 1..n-1.
/// @return The verified RSA-PSS signature.
/// @throws CryptologyError If blinding randomness is out of range or public verification fails.
/// @tested{SigningPrivateKeyTest}
[[nodiscard]] auto signDecoded(
    const PrivateKeyData &key,
    TlsSignatureScheme scheme,
    mem::ConstByteSpan message,
    const PublicKey &publicKeyValue,
    mem::ConstByteSpan salt,
    mem::ConstByteSpan blindingFactor) -> mem::ByteBlock;

}
