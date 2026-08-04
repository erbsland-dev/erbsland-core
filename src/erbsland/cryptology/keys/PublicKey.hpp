// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../asn1/Asn1Node.hpp"
#include "../impl/X509Parser_fwd.hpp"
#include "../tls/TlsSignatureScheme_fwd.hpp"
#include "../x509/X509AlgorithmIdentifier.hpp"

#include "../../mem/ByteBlock.hpp"
#include "../../mem/ByteSpan.hpp"

#include <cstdint>

namespace erbsland::cryptology {

/// An immutable public-key facade for X.509 SubjectPublicKeyInfo.
/// RSA verification follows RFC 8017 sections 8 and 9 and RFC 4055 section 3. ECDSA verification follows FIPS 186-5
/// section 6.4.2, RFC 5480 sections 2.1.1 and 2.2, and RFC 5758 section 3.2. Ed25519 verification follows RFC 8032
/// section 5.1.7 and RFC 8410 sections 3, 4, and 6.
/// @seedoc{/reference/cryptology/x509_certificates}
/// @tested{EcdsaSignatureTest Ed25519SignatureTest RsaSignatureTest TlsSignatureSchemeFullTest X509CertificateTest}
class PublicKey final {
public:
    /// Create an empty public key.
    PublicKey() = default;

    // defaults
    ~PublicKey() = default;
    PublicKey(const PublicKey &) = default;
    PublicKey(PublicKey &&) noexcept = default;
    auto operator=(const PublicKey &) -> PublicKey & = default;
    auto operator=(PublicKey &&) noexcept -> PublicKey & = default;

public: // tests
    /// Test if this facade contains SubjectPublicKeyInfo.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return _der.isEmpty(); }

public: // accessors
    /// Get the public-key algorithm identifier.
    [[nodiscard]] auto algorithm() const noexcept -> const X509AlgorithmIdentifier & { return _algorithm; }
    /// Get the subjectPublicKey BIT STRING data without its unused-bit-count octet.
    [[nodiscard]] auto keyData() const noexcept -> const mem::ByteBlock & { return _keyData; }
    /// Get the number of unused low bits in the final key-data octet.
    [[nodiscard]] auto unusedBitCount() const noexcept -> uint8_t { return _unusedBitCount; }
    /// Get the exact SubjectPublicKeyInfo ASN.1 node.
    [[nodiscard]] auto asn1() const noexcept -> const Asn1Node & { return _node; }

public: // verification
    /// Verify a supported X.509 signature over an exact message.
    ///
    /// RSA-PSS and RSASSA-PKCS1-v1_5 with SHA-256/SHA-384 follow RFC 8017 and RFC 4055. ECDSA P-256/SHA-256 and
    /// P-384/SHA-384 follow FIPS 186-5 section 6.4.2, RFC 5480, and RFC 5758. ECDSA signatures use canonical DER and
    /// accept compressed or uncompressed validated public points. Pure Ed25519 follows RFC 8032 section 5.1.7 and RFC
    /// 8410. Ed25519ctx, Ed25519ph, and Ed448 are not accepted.
    /// @param signatureAlgorithm The complete X.509 signature AlgorithmIdentifier.
    /// @param message The exact message bytes covered by the signature.
    /// @param signature The signature octets without an ASN.1 BIT STRING unused-bit-count octet.
    /// @return `true` only if the signature is valid.
    /// @throws err::LogicError If this public key is empty.
    /// @throws err::ParseError If the key or algorithm encoding is malformed, unsupported, or outside fixed bounds.
    [[nodiscard]] auto verifySignature(
        const X509AlgorithmIdentifier &signatureAlgorithm,
        mem::ConstByteSpan message,
        mem::ConstByteSpan signature) const -> bool;
    /// Verify a TLS 1.3 `CertificateVerify` signature over exact caller-supplied content.
    ///
    /// This operation follows RFC 8446 sections 4.2.3 and 4.4.3. It rejects the certificate-only PKCS#1 v1.5
    /// schemes, enforces the RSAE versus RSASSA-PSS public-key OID distinction, and uses digest-sized PSS salts.
    /// `message` must already contain the RFC 8446 section 4.4.3 pad, context string, separator, and transcript hash.
    /// All inputs and intermediates are public verification data; no secret state requiring erasure is created.
    /// @param scheme The supported TLS SignatureScheme received in `CertificateVerify`.
    /// @param message The exact content covered by the TLS signature.
    /// @param signature The signature octets from the TLS message.
    /// @return `true` only if the signature is valid.
    /// @throws err::LogicError If this public key is empty.
    /// @throws err::ParseError If the scheme is forbidden, the key is incompatible, or an encoding is malformed.
    [[nodiscard]] auto verifyTlsCertificateVerifySignature(
        TlsSignatureScheme scheme, mem::ConstByteSpan message, mem::ConstByteSpan signature) const -> bool;

public: // conversion
    /// Get the exact canonical DER SubjectPublicKeyInfo.
    [[nodiscard]] auto toDer() const noexcept -> const mem::ByteBlock & { return _der; }

public: // factories
    /// Parse one canonical DER SubjectPublicKeyInfo, returning an empty key on error.
    /// @param der The complete DER SubjectPublicKeyInfo.
    /// @return The parsed public key, or an empty key on error.
    [[nodiscard]] static auto fromDer(const mem::ByteBlock &der) noexcept -> PublicKey;
    /// Parse one canonical DER SubjectPublicKeyInfo.
    /// @param der The complete DER SubjectPublicKeyInfo.
    /// @return The parsed public key.
    /// @throws err::ParseError If the DER or SubjectPublicKeyInfo structure is malformed.
    /// @throws err::OutOfRangeError If a fixed parser resource limit is exceeded.
    [[nodiscard]] static auto fromDerOrThrow(const mem::ByteBlock &der) -> PublicKey;

private:
    friend class impl::X509Parser;
    /// Create a public key from parsed SubjectPublicKeyInfo components.
    PublicKey(
        X509AlgorithmIdentifier algorithm,
        mem::ByteBlock keyData,
        uint8_t unusedBitCount,
        Asn1Node node,
        mem::ByteBlock der) noexcept :
        _algorithm{std::move(algorithm)},
        _keyData{std::move(keyData)},
        _unusedBitCount{unusedBitCount},
        _node{std::move(node)},
        _der{std::move(der)} {}

private:
    X509AlgorithmIdentifier _algorithm; ///< SubjectPublicKeyInfo algorithm.
    mem::ByteBlock _keyData;            ///< BIT STRING contents without the unused-bit count.
    uint8_t _unusedBitCount{};          ///< Final-octet unused bits.
    Asn1Node _node;                     ///< Exact SubjectPublicKeyInfo node.
    mem::ByteBlock _der;                ///< Exact SubjectPublicKeyInfo DER.
};

}
