// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PortableX509CertificateData.hpp"

#include "../x509/X509Certificate.hpp"

#include "../../unit/ByteIndex_fwd.hpp"

#include <cstddef>
#include <cstdint>

namespace erbsland::cryptology::impl {

/// RFC 5280 certificate schema decoder over a canonical DER document.
/// @tested{X509CertificateTest}
class X509Parser final {
public:
    /// Parse one canonical DER certificate.
    [[nodiscard]] static auto parse(const mem::ByteBlock &der, X509CertificateProfileMode mode) -> X509Certificate;
    /// Parse one canonical DER SubjectPublicKeyInfo.
    [[nodiscard]] static auto parsePublicKey(const mem::ByteBlock &der) -> PublicKey;
    /// Parse one canonical DER AlgorithmIdentifier.
    [[nodiscard]] static auto parseAlgorithmIdentifier(const mem::ByteBlock &der) -> X509AlgorithmIdentifier;

private:
    /// Create a schema parser over a validated DER document.
    X509Parser(Asn1Node root, X509CertificateProfileMode mode);
    /// Parse the outer Certificate sequence.
    [[nodiscard]] auto parseCertificate() -> X509Certificate;
    /// Parse the ordered TBSCertificate fields.
    void parseTbsCertificate(const Asn1Node &node);
    /// Parse an AlgorithmIdentifier sequence.
    [[nodiscard]] auto parseAlgorithmIdentifierNode(const Asn1Node &node) -> X509AlgorithmIdentifier;
    /// Parse an X.501 Name value.
    [[nodiscard]] auto parseName(const Asn1Node &node) -> X509Name;
    /// Parse an RFC 5280 validity time.
    [[nodiscard]] auto parseTime(const Asn1Node &node) -> time::DateTime;
    /// Parse SubjectPublicKeyInfo without interpreting its algorithm-specific bits.
    [[nodiscard]] auto parseSubjectPublicKeyInfo(const Asn1Node &node) -> PublicKey;
    /// Parse the explicit extensions container.
    void parseExtensions(const Asn1Node &explicitNode);
    /// Dispatch a recognized extension decoder.
    void parseKnownExtension(const X509Extension &extension);
    /// Parse Subject Alternative Name.
    void parseSubjectAlternativeNames(const X509Extension &extension);
    /// Parse one GeneralName alternative.
    [[nodiscard]] auto parseGeneralName(const Asn1Node &node) -> X509GeneralName;
    /// Parse Subject Key Identifier.
    void parseSubjectKeyIdentifier(const X509Extension &extension);
    /// Parse Authority Key Identifier.
    void parseAuthorityKeyIdentifier(const X509Extension &extension);
    /// Parse Basic Constraints.
    void parseBasicConstraints(const X509Extension &extension);
    /// Parse Key Usage.
    void parseKeyUsage(const X509Extension &extension);
    /// Parse Extended Key Usage.
    void parseExtendedKeyUsage(const X509Extension &extension);
    /// Validate cross-field TBSCertificate profile requirements.
    void validateTbsProfile(bool hasIssuerUniqueId, bool hasSubjectUniqueId, bool hasExtensions);
    /// Record a compatible-profile issue or throw in strict mode.
    void addIssue(
        X509CertificateProfileIssueCategory category,
        text::String field,
        Asn1ObjectIdentifier oid,
        text::String diagnostic);
    /// Decode implicitly tagged OBJECT IDENTIFIER content.
    [[nodiscard]] auto decodeImplicitOid(const Asn1Node &node) -> Asn1ObjectIdentifier;
    /// Validate implicitly tagged BIT STRING content.
    void validateImplicitBitString(const Asn1Node &node, const text::String &name) const;
    /// Validate an implicitly tagged RFC 5280 certificate serial number.
    void validateImplicitSerialNumber(const Asn1Node &node, const text::String &name) const;
    /// Decode a bounded nonnegative INTEGER.
    [[nodiscard]] auto unsignedInteger(const Asn1Node &node, uint64_t maximum) const -> uint64_t;
    /// Test a node's exact tag class, number, and encoding form.
    [[nodiscard]] static auto isNode(
        const Asn1Node &node, Asn1TagClass tagClass, uint32_t tagNumber, bool constructed) noexcept -> bool;
    /// Require a node's exact tag class, number, and encoding form.
    void requireNode(
        const Asn1Node &node,
        Asn1TagClass tagClass,
        uint32_t tagNumber,
        bool constructed,
        const text::String &name) const;
    /// Decode primitive ASCII content.
    [[nodiscard]] auto asciiText(const Asn1Node &node) const -> text::String;
    /// Join a field name and diagnostic detail without standard-library strings.
    [[nodiscard]] static auto fieldDiagnostic(const text::String &name, const text::String &detail) -> text::String;
    /// Decode one decimal digit at a known content offset.
    [[nodiscard]] static auto decimalDigit(
        mem::ConstByteSpan bytes, std::size_t index, unit::ByteIndex contentByteIndex) -> uint32_t;
    /// Decode two decimal digits at a known content offset.
    [[nodiscard]] static auto decimalPair(mem::ConstByteSpan bytes, std::size_t index, unit::ByteIndex contentByteIndex)
        -> uint32_t;
    /// Decode four decimal digits at a known content offset.
    [[nodiscard]] static auto decimalQuad(mem::ConstByteSpan bytes, std::size_t index, unit::ByteIndex contentByteIndex)
        -> uint32_t;
    /// Get the first encoded byte index for a node.
    [[nodiscard]] auto nodeByteIndex(const Asn1Node &node) const noexcept -> unit::ByteIndex;
    /// Get the first content byte index for a node.
    [[nodiscard]] auto nodeContentByteIndex(const Asn1Node &node) const noexcept -> unit::ByteIndex;
    /// Throw a parse error associated with a node.
    [[noreturn]] void throwParseError(text::String reason, const Asn1Node &node) const;

private:
    Asn1Node _root; ///< Validated immutable DER tree root.
    /// Selected profile handling mode.
    X509CertificateProfileMode _mode{X509CertificateProfileMode::Strict};
    PortableX509CertificateValues _values; ///< Portable values populated while parsing.
};

}
