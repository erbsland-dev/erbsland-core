// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TlsSignatureScheme_fwd.hpp"

#include "../keys/PublicKey_fwd.hpp"
#include "../x509/X509AlgorithmIdentifier_fwd.hpp"

#include "../../text/String_fwd.hpp"
#include "../../util/impl/ComparisonHelper.hpp"

#include <cstdint>
#include <optional>

namespace erbsland::cryptology {

/// A supported TLS 1.3 signature scheme.
///
/// Code points and their permitted uses follow RFC 8446 section 4.2.3. The PKCS#1 v1.5 schemes are retained only for
/// signatures appearing in certificates and are never allowed for TLS 1.3 `CertificateVerify` messages. All values
/// describe public verification inputs; this type creates and retains no secret state requiring erasure.
/// @seedoc{/reference/cryptology/x509_certificates}
/// @tested{TlsSignatureSchemeTest TlsSignatureSchemeFullTest}
class TlsSignatureScheme final {
    friend class PublicKey;

public:
    /// The supported RFC 8446 SignatureScheme values.
    enum Value : uint16_t {
        RsaPkcs1Sha256 = 0x0401U,       ///< Certificate-only RSASSA-PKCS1-v1_5 with SHA-256.
        RsaPkcs1Sha384 = 0x0501U,       ///< Certificate-only RSASSA-PKCS1-v1_5 with SHA-384.
        EcdsaSecp256r1Sha256 = 0x0403U, ///< ECDSA with P-256 and SHA-256.
        EcdsaSecp384r1Sha384 = 0x0503U, ///< ECDSA with P-384 and SHA-384.
        RsaPssRsaeSha256 = 0x0804U,     ///< RSASSA-PSS/SHA-256 with an rsaEncryption public key.
        RsaPssRsaeSha384 = 0x0805U,     ///< RSASSA-PSS/SHA-384 with an rsaEncryption public key.
        Ed25519 = 0x0807U,              ///< Pure Ed25519.
        RsaPssPssSha256 = 0x0809U,      ///< RSASSA-PSS/SHA-256 with an id-RSASSA-PSS public key.
        RsaPssPssSha384 = 0x080aU,      ///< RSASSA-PSS/SHA-384 with an id-RSASSA-PSS public key.
    };

public:
    /// Create the mandatory-to-implement RSA-PSS-RSAE/SHA-256 scheme.
    constexpr TlsSignatureScheme() noexcept = default;
    /// Create a scheme from its supported raw value.
    /// @param value The supported raw scheme value.
    constexpr TlsSignatureScheme(const Value value) noexcept : _value{value} {} // NOLINT(*-explicit-constructor)

    // defaults
    ~TlsSignatureScheme() = default;
    TlsSignatureScheme(const TlsSignatureScheme &) = default;
    TlsSignatureScheme(TlsSignatureScheme &&) = default;
    auto operator=(const TlsSignatureScheme &) -> TlsSignatureScheme & = default;
    auto operator=(TlsSignatureScheme &&) -> TlsSignatureScheme & = default;

public: // operators
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const TlsSignatureScheme &other, other._value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const Value value, value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_FRIEND(const Value value, const TlsSignatureScheme &other, value, other._value);

public: // tests
    /// Test whether this scheme may verify a TLS 1.3 `CertificateVerify` signature.
    [[nodiscard]] auto isAllowedForCertificateVerify() const noexcept -> bool;
    /// Test whether this scheme may describe a signature appearing in an X.509 certificate.
    [[nodiscard]] auto isAllowedForCertificateSignature() const noexcept -> bool;

public: // accessors
    /// Get the exact unsigned 16-bit TLS wire value.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> uint16_t { return static_cast<uint16_t>(_value); }

public: // conversion
    /// Convert the scheme to its stable TLS registry name.
    [[nodiscard]] auto toString() const -> text::String;

public: // factories
    /// Parse one supported unsigned 16-bit TLS wire value.
    /// @param value The raw wire value.
    /// @return The matching scheme, or no value when unsupported.
    [[nodiscard]] static auto fromRawValue(uint16_t value) noexcept -> std::optional<TlsSignatureScheme>;
    /// Parse one supported unsigned 16-bit TLS wire value.
    /// @param value The raw wire value.
    /// @return The matching scheme.
    /// @throws err::ParseError If the value is not supported.
    [[nodiscard]] static auto fromRawValueOrThrow(uint16_t value) -> TlsSignatureScheme;

private:
    /// Map this scheme to the complete signature AlgorithmIdentifier required by the existing verifier.
    [[nodiscard]] auto signatureAlgorithmIdentifier() const -> X509AlgorithmIdentifier;
    /// Test whether this is an RSA-PSS scheme requiring an rsaEncryption public key.
    [[nodiscard]] auto requiresRsaEncryptionKey() const noexcept -> bool;
    /// Test whether this is an RSA-PSS scheme requiring an id-RSASSA-PSS public key.
    [[nodiscard]] auto requiresRsaPssKey() const noexcept -> bool;

private:
    Value _value{RsaPssRsaeSha256}; ///< Supported TLS SignatureScheme code point.
};

}
