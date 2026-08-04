// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PublicKey.hpp"
#include "SigningKeyAlgorithm.hpp"
#include "SigningPrivateKey_fwd.hpp"

#include "../impl/PrivateKeyParser_fwd.hpp"
#include "../protected_data/ProtectedByteBlock.hpp"
#include "../tls/TlsSignatureScheme.hpp"

#include "../../mem/ByteBlock.hpp"
#include "../../mem/ByteSpan.hpp"
#include "../../text/String.hpp"

namespace erbsland::cryptology {

/// A move-only private signing key retained in application-protected storage.
///
/// Keys are imported as strict, unencrypted PKCS#8. Ed25519 follows RFC 8410 section 7 and RFC 8032 sections 5.1.5-
/// 5.1.6; ECDSA P-256 follows RFC 5915 and FIPS 186-5 section 6.4.1; RSA-PSS follows RFC 5208 and RFC 8017 sections
/// 8.1 and 9.1. No operation exposes raw private material.
/// @seedoc{/reference/cryptology/signing_keys}
/// @tested{SigningPrivateKeyTest}
class SigningPrivateKey final {
    friend class impl::PrivateKeyParser;

public:
    /// Create an empty private-key placeholder.
    SigningPrivateKey() noexcept = default;
    /// Securely erase protected private material.
    ~SigningPrivateKey();

    // defaults/deletions
    SigningPrivateKey(const SigningPrivateKey &) = delete;
    SigningPrivateKey(SigningPrivateKey &&) noexcept = default;
    auto operator=(const SigningPrivateKey &) -> SigningPrivateKey & = delete;
    auto operator=(SigningPrivateKey &&) noexcept -> SigningPrivateKey & = default;

public:
    /// Sign exact TLS 1.3 `CertificateVerify` content.
    /// @param scheme A compatible TLS 1.3 signature scheme.
    /// @param message The complete RFC 8446 section 4.4.3 signed content.
    /// @return The signature bytes used in `CertificateVerify`.
    /// @throws err::LogicError If this key is empty.
    /// @throws err::ParameterError If the scheme is incompatible with the key.
    /// @throws CryptologyError If protected storage, randomness, signing, or the RSA fault check fails.
    [[nodiscard]] auto signTlsCertificateVerify(TlsSignatureScheme scheme, mem::ConstByteSpan message) const
        -> mem::ByteBlock;
    /// Securely erase private material and restore the empty state.
    void secureErase() noexcept;

public: // tests
    /// Test whether no private key is stored.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return _privateData.isEmpty(); }
    /// Test whether this key supports a TLS 1.3 `CertificateVerify` scheme.
    [[nodiscard]] auto supports(TlsSignatureScheme scheme) const noexcept -> bool;
    /// Test whether this key semantically matches a public key.
    [[nodiscard]] auto matches(const PublicKey &publicKey) const -> bool;

public: // accessors
    /// Get the signing-key algorithm, or `Unknown` for an empty key.
    [[nodiscard]] auto algorithm() const noexcept -> SigningKeyAlgorithm { return _algorithm; }
    /// Get the cached public key without decrypting private material.
    /// @throws err::LogicError If this key is empty.
    [[nodiscard]] auto publicKey() const -> PublicKey;

public: // factories
    /// Parse one strict unencrypted PKCS#8 DER key, returning an empty key on any error.
    [[nodiscard]] static auto fromDer(const mem::ByteBlock &der) noexcept -> SigningPrivateKey;
    /// Parse one strict unencrypted PKCS#8 DER key.
    /// @throws err::ParseError If DER, PKCS#8, or algorithm-specific key data is invalid.
    /// @throws err::OutOfRangeError If a fixed resource bound is exceeded.
    /// @throws CryptologyError If private material cannot be protected.
    [[nodiscard]] static auto fromDerOrThrow(const mem::ByteBlock &der) -> SigningPrivateKey;
    /// Parse one strict unencrypted `PRIVATE KEY` PEM block, returning an empty key on any error.
    [[nodiscard]] static auto fromPem(const text::String &pem) noexcept -> SigningPrivateKey;
    /// Parse one strict unencrypted `PRIVATE KEY` PEM block.
    /// @throws err::ParseError If PEM, DER, PKCS#8, or algorithm-specific key data is invalid.
    /// @throws err::OutOfRangeError If a fixed resource bound is exceeded.
    /// @throws CryptologyError If private material cannot be protected.
    [[nodiscard]] static auto fromPemOrThrow(const text::String &pem) -> SigningPrivateKey;

private:
    /// Create a validated key from normalized secret and cached public material.
    SigningPrivateKey(SigningKeyAlgorithm algorithm, mem::ConstByteSpan privateData, PublicKey publicKey);

private:
    SigningKeyAlgorithm _algorithm{SigningKeyAlgorithm::Unknown}; ///< Supported private-key algorithm.
    ProtectedByteBlock _privateData;                              ///< Protected normalized private components.
    PublicKey _publicKey;                                         ///< Cached ordinary public key.
};

}
