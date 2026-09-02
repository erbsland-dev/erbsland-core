// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "X509Certificate.hpp"
#include "X509CertificateBuilder_fwd.hpp"
#include "X509CertificateProfile.hpp"
#include "X509CertificateSigningRequest.hpp"

#include "../impl/X509ArtifactWriter_fwd.hpp"
#include "../keys/SigningPrivateKey.hpp"

#include "../../network/IpAddress.hpp"
#include "../../text/String.hpp"
#include "../../text/StringList.hpp"
#include "../../time/CalendarDelta.hpp"
#include "../../time/DateTime.hpp"
#include "../../util/List.hpp"

#include <cstdint>
#include <optional>

namespace erbsland::cryptology {

/// A profile-driven builder for web certificates and PKCS#10 requests.
/// @seedoc{/reference/cryptology/x509_certificates}
/// @tested{X509CertificateBuilderTest}
class X509CertificateBuilder final {
public:
    /// Create a CA profile builder.
    /// @param commonName The required nonempty subject common name.
    /// @return A builder with safe CA defaults.
    [[nodiscard]] static auto certificateAuthority(text::String commonName) -> X509CertificateBuilder;
    /// Create a TLS server profile builder.
    /// @param commonName The required nonempty subject common name.
    /// @return A builder requiring at least one DNS name or IP address before creation.
    [[nodiscard]] static auto tlsServer(text::String commonName) -> X509CertificateBuilder;
    /// Create a TLS client profile builder.
    /// @param commonName The required nonempty subject common name.
    /// @return A builder with the TLS client profile.
    [[nodiscard]] static auto tlsClient(text::String commonName) -> X509CertificateBuilder;
    /// Create a dual-use TLS server/client profile builder.
    /// @param commonName The required nonempty subject common name.
    /// @return A builder requiring at least one DNS name or IP address before creation.
    [[nodiscard]] static auto tlsServerAndClient(text::String commonName) -> X509CertificateBuilder;
    /// Initialize controlled reissuance from a parsed certificate.
    /// @param certificate The source certificate whose supported identity information is copied.
    /// @param profile The safe profile for the new certificate or request.
    /// @return A new independent builder.
    /// @throws err::LogicError If the source is empty or has an unknown critical extension.
    [[nodiscard]] static auto fromCertificate(const X509Certificate &certificate, X509CertificateProfile profile)
        -> X509CertificateBuilder;

public: // subject
    /// Replace the subject common name.
    auto setCommonName(text::String value) -> X509CertificateBuilder &;
    /// Replace the two-character subject country code.
    auto setCountry(text::String value) -> X509CertificateBuilder &;
    /// Replace the subject state or province.
    auto setState(text::String value) -> X509CertificateBuilder &;
    /// Replace the subject locality.
    auto setLocality(text::String value) -> X509CertificateBuilder &;
    /// Replace the subject organization.
    auto setOrganization(text::String value) -> X509CertificateBuilder &;
    /// Replace the subject organizational unit.
    auto setOrganizationalUnit(text::String value) -> X509CertificateBuilder &;
    /// Add a DNS subject alternative name.
    auto addDnsName(text::String value) -> X509CertificateBuilder &;
    /// Add an IP-address subject alternative name.
    auto addIpAddress(network::IpAddress value) -> X509CertificateBuilder &;

public: // validity and constraints
    /// Set an exact not-before time.
    auto setValidFrom(time::DateTime value) noexcept -> X509CertificateBuilder &;
    /// Set an exact not-after time and clear a configured lifetime.
    auto setValidTo(time::DateTime value) noexcept -> X509CertificateBuilder &;
    /// Set an exact validity range and clear a configured lifetime.
    auto setValidity(time::DateTime from, time::DateTime to) noexcept -> X509CertificateBuilder &;
    /// Set the lifetime relative to the configured or default not-before time.
    auto setLifetime(time::CalendarDelta value) noexcept -> X509CertificateBuilder &;
    /// Set the maximum subordinate-CA depth for a CA profile.
    auto setCaPathLength(uint32_t value) noexcept -> X509CertificateBuilder &;

public: // creation
    /// Create a self-signed CA certificate.
    /// @throws err::LogicError If this is not a CA profile or the key is empty.
    /// @throws err::ParameterError If configured identity or validity data is invalid.
    [[nodiscard]] auto createSelfSignedCertificate(const SigningPrivateKey &key) const -> X509Certificate;
    /// Create a certificate signed by an authorized issuer.
    /// @throws err::LogicError If a required value is empty.
    /// @throws err::ParameterError If the profile, validity, issuer, or key relationship is invalid.
    [[nodiscard]] auto createCertificate(
        const SigningPrivateKey &subjectKey,
        const X509Certificate &issuerCertificate,
        const SigningPrivateKey &issuerKey) const -> X509Certificate;
    /// Create a signed PKCS#10 request from the same profile and identity configuration.
    /// @throws err::LogicError If the key is empty.
    /// @throws err::ParameterError If configured profile or identity data is invalid.
    [[nodiscard]] auto createSigningRequest(const SigningPrivateKey &subjectKey) const -> X509CertificateSigningRequest;

private:
    friend class impl::X509ArtifactWriter;
    /// Create a builder for one fixed profile and common name.
    X509CertificateBuilder(X509CertificateProfile profile, text::String commonName) noexcept;

private:
    X509CertificateProfile _profile;
    std::optional<text::String> _commonName;
    std::optional<text::String> _country;
    std::optional<text::String> _state;
    std::optional<text::String> _locality;
    std::optional<text::String> _organization;
    std::optional<text::String> _organizationalUnit;
    text::StringList _dnsNames;
    util::List<network::IpAddress> _ipAddresses;
    std::optional<time::DateTime> _validFrom;
    std::optional<time::DateTime> _validTo;
    std::optional<time::CalendarDelta> _lifetime;
    uint32_t _caPathLength{};
    X509Name _sourceSubject;
    util::List<X509Extension> _preservedExtensions;
};

}
