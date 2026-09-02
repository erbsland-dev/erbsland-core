// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "DerEncoder.hpp"
#include "X509ArtifactWriter_fwd.hpp"

#include "../asn1/Asn1ObjectIdentifier.hpp"
#include "../x509/X509CertificateBuilder.hpp"

#include "../../mem/ByteBlock_fwd.hpp"

namespace erbsland::cryptology::impl {

/// Shared profile, identity, extension, and signature writer for X.509 certificates and PKCS#10 requests.
///
/// Certificate construction follows RFC 5280 sections 4.1 and 4.2. PKCS#10 construction follows RFC 2986 section 4,
/// with the extensionRequest attribute from RFC 2985 section 5.4.2.
/// @tested{X509CertificateBuilderTest}
class X509ArtifactWriter final {
public:
    /// Bind the writer to one immutable builder snapshot.
    explicit X509ArtifactWriter(const X509CertificateBuilder &builder) noexcept : _builder{builder} {}

public:
    /// Create a self-signed certificate using this writer's CA profile.
    [[nodiscard]] auto createSelfSignedCertificate(const SigningPrivateKey &key) const -> X509Certificate;
    /// Create an issuer-signed certificate using this writer's profile.
    [[nodiscard]] auto createCertificate(
        const SigningPrivateKey &subjectKey,
        const X509Certificate &issuerCertificate,
        const SigningPrivateKey &issuerKey) const -> X509Certificate;
    /// Create a PKCS#10 request using this writer's profile.
    [[nodiscard]] auto createSigningRequest(const SigningPrivateKey &subjectKey) const -> X509CertificateSigningRequest;

private:
    /// Create either a self-signed or issuer-signed certificate after common validation.
    [[nodiscard]] auto createCertificateImpl(
        const SigningPrivateKey &subjectKey,
        const mem::ByteBlock &issuerName,
        const mem::ByteBlock &authorityKeyIdentifier,
        const SigningPrivateKey &issuerKey,
        const X509Certificate *issuerCertificate) const -> X509Certificate;
    /// Validate the profile's required identity values.
    void validateIdentity() const;
    /// Append the canonical subject Name.
    void appendSubjectName(DerEncoder &encoder) const;
    /// Append extensions shared by certificate and request output.
    void appendExtensions(
        DerEncoder &encoder,
        const PublicKey &subjectKey,
        const mem::ByteBlock &authorityKeyIdentifier,
        bool request) const;
    /// Select the fixed signature scheme for a key.
    [[nodiscard]] auto signatureScheme(const SigningPrivateKey &key) const -> TlsSignatureScheme;
    /// Derive the RFC 5280 key identifier for a public key.
    [[nodiscard]] static auto keyIdentifier(const PublicKey &key) -> mem::ByteBlock;
    /// Generate a fresh positive nonzero serial number.
    [[nodiscard]] static auto serialNumber() -> mem::ByteBlock;
    /// Append one X.509 extension directly to the final extension sequence.
    /// @tparam AppendValue Callable that appends the DER value wrapped by extnValue.
    /// @param encoder Destination DER encoder.
    /// @param oidValue Extension object identifier.
    /// @param critical Whether to append the critical flag.
    /// @param appendValue Callable receiving the destination encoder.
    template <typename AppendValue>
    static void appendExtension(
        DerEncoder &encoder, const text::String &oidValue, bool critical, AppendValue appendValue) {
        const auto extension = encoder.beginSequence();
        encoder.appendObjectIdentifier(Asn1ObjectIdentifier::fromStringOrThrow(oidValue));
        if (critical) {
            encoder.appendBoolean(true);
        }
        const auto value = encoder.beginOctetString();
        appendValue(encoder);
        encoder.end(value);
        encoder.end(extension);
    }

private:
    const X509CertificateBuilder &_builder;
};

}
