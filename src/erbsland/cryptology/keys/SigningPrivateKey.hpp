// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PublicKey.hpp"
#include "SigningKeyAlgorithm.hpp"
#include "SigningKeyProfile.hpp"
#include "SigningPrivateKey_fwd.hpp"

#include "../impl/PrivateKeyParser_fwd.hpp"
#include "../impl/SigningKeyGenerator_fwd.hpp"
#include "../PemDerFormat.hpp"
#include "../protected_data/ProtectedByteBlock.hpp"
#include "../tls/TlsSignatureScheme.hpp"

#include "../../mem/ByteBlock.hpp"
#include "../../mem/ByteSpan.hpp"
#include "../../path/Path_fwd.hpp"
#include "../../text/String.hpp"

namespace erbsland::cryptology {

/// A move-only private signing key retained in application-protected storage.
///
/// Keys use canonical PKCS#8. Ed25519 follows RFC 8410 section 7 and RFC 8032 sections 5.1.5-5.1.6; ECDSA P-256/P-384
/// follows RFC 5915 and FIPS 186-5 section 6.4.1; RSA-PSS follows RFC 5208 and RFC 8017 sections 8.1 and 9.1.
/// Serialization exposes private material only through explicitly sensitive return values or user-only files.
/// @seedoc{/reference/cryptology/key_management}
/// @tested{SigningPrivateKeyTest X509CertificateBuilderTest}
class SigningPrivateKey final {
    friend class impl::PrivateKeyParser;
    friend class impl::SigningKeyGenerator;

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

public: // conversion
    /// Encode this key as canonical unencrypted PKCS#8 DER in sensitive memory.
    [[nodiscard]] auto toDer() const -> mem::ByteBlock;
    /// Encode this key as one `PRIVATE KEY` PEM block in sensitive memory.
    [[nodiscard]] auto toPem() const -> text::String;
    /// Write this key without replacing an existing file and with user-only access.
    void writeToFile(const path::Path &path, PemDerFormat format = PemDerFormat::Automatic) const;
    /// Encrypt this key as PBES2 EncryptedPrivateKeyInfo DER.
    [[nodiscard]] auto toEncryptedDer(const text::String &password) const -> mem::ByteBlock;
    /// Encrypt this key as one `ENCRYPTED PRIVATE KEY` PEM block.
    [[nodiscard]] auto toEncryptedPem(const text::String &password) const -> text::String;
    /// Write an encrypted key without replacing an existing file and with user-only access.
    void writeEncryptedToFile(
        const path::Path &path, const text::String &password, PemDerFormat format = PemDerFormat::Automatic) const;

public: // factories
    /// Generate a signing key from application secure randomness.
    [[nodiscard]] static auto generate(SigningKeyProfile profile = SigningKeyProfile::EcdsaP256) -> SigningPrivateKey;
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
    /// Read one unencrypted PKCS#8 key from a file, returning an empty key on error.
    [[nodiscard]] static auto fromFile(const path::Path &path, PemDerFormat format = PemDerFormat::Automatic) noexcept
        -> SigningPrivateKey;
    /// Read one unencrypted PKCS#8 key from a file.
    [[nodiscard]] static auto fromFileOrThrow(const path::Path &path, PemDerFormat format = PemDerFormat::Automatic)
        -> SigningPrivateKey;
    /// Decrypt EncryptedPrivateKeyInfo DER, returning an empty key on failure.
    [[nodiscard]] static auto fromEncryptedDer(const mem::ByteBlock &der, const text::String &password) noexcept
        -> SigningPrivateKey;
    /// Decrypt EncryptedPrivateKeyInfo DER.
    [[nodiscard]] static auto fromEncryptedDerOrThrow(const mem::ByteBlock &der, const text::String &password)
        -> SigningPrivateKey;
    /// Decrypt one `ENCRYPTED PRIVATE KEY` PEM block, returning an empty key on failure.
    [[nodiscard]] static auto fromEncryptedPem(const text::String &pem, const text::String &password) noexcept
        -> SigningPrivateKey;
    /// Decrypt one `ENCRYPTED PRIVATE KEY` PEM block.
    [[nodiscard]] static auto fromEncryptedPemOrThrow(const text::String &pem, const text::String &password)
        -> SigningPrivateKey;
    /// Read and decrypt one encrypted PKCS#8 key, returning an empty key on failure.
    [[nodiscard]] static auto fromEncryptedFile(
        const path::Path &path, const text::String &password, PemDerFormat format = PemDerFormat::Automatic) noexcept
        -> SigningPrivateKey;
    /// Read and decrypt one encrypted PKCS#8 key.
    [[nodiscard]] static auto fromEncryptedFileOrThrow(
        const path::Path &path, const text::String &password, PemDerFormat format = PemDerFormat::Automatic)
        -> SigningPrivateKey;

private:
    /// Create a validated key from normalized secret and cached public material.
    SigningPrivateKey(SigningKeyAlgorithm algorithm, mem::ConstByteSpan privateData, PublicKey publicKey);

private:
    SigningKeyAlgorithm _algorithm{SigningKeyAlgorithm::Unknown}; ///< Supported private-key algorithm.
    ProtectedByteBlock _privateData;                              ///< Protected normalized private components.
    PublicKey _publicKey;                                         ///< Cached ordinary public key.
};

}
