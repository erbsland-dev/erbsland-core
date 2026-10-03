// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "X509CertificateBuilder.hpp"

#include "../CryptologyError.hpp"
#include "../impl/CryptologyOids.hpp"
#include "../impl/X509ArtifactWriter.hpp"

#include "../../err/LogicFailure.hpp"
#include "../../text/Literals.hpp"

namespace erbsland::cryptology {

using namespace text;
using namespace text::literals;

X509CertificateBuilder::X509CertificateBuilder(const X509CertificateProfile profile, String commonName) noexcept :
    _profile{profile}, _commonName{std::move(commonName)} {
}

auto X509CertificateBuilder::certificateAuthority(String commonName) -> X509CertificateBuilder {
    return X509CertificateBuilder{X509CertificateProfile::CertificateAuthority, std::move(commonName)};
}

auto X509CertificateBuilder::tlsServer(String commonName) -> X509CertificateBuilder {
    return X509CertificateBuilder{X509CertificateProfile::TlsServer, std::move(commonName)};
}

auto X509CertificateBuilder::tlsClient(String commonName) -> X509CertificateBuilder {
    return X509CertificateBuilder{X509CertificateProfile::TlsClient, std::move(commonName)};
}

auto X509CertificateBuilder::tlsServerAndClient(String commonName) -> X509CertificateBuilder {
    return X509CertificateBuilder{X509CertificateProfile::TlsServerAndClient, std::move(commonName)};
}

auto X509CertificateBuilder::fromCertificate(const X509Certificate &certificate, const X509CertificateProfile profile)
    -> X509CertificateBuilder {
    if (certificate.isEmpty()) {
        throw err::LogicFailure{"A source certificate is required for reissuance."_el};
    }
    auto result = X509CertificateBuilder{profile, certificate.subject().commonNames().first()};
    result._sourceSubject = certificate.subject();
    result._dnsNames = certificate.dnsNames();
    result._ipAddresses = certificate.ipAddresses();
    for (const auto &extension : certificate.extensions()) {
        const auto id = extension.oid().toString();
        const auto supported = id == impl::cryptology_oids::subjectKeyIdentifier ||
            id == impl::cryptology_oids::keyUsage || id == impl::cryptology_oids::subjectAlternativeName ||
            id == impl::cryptology_oids::basicConstraints || id == impl::cryptology_oids::authorityKeyIdentifier ||
            id == impl::cryptology_oids::extendedKeyUsage;
        if (!supported) {
            if (extension.isCritical()) {
                throw CryptologyError{"Cannot reissue a certificate containing an unknown critical extension."_el};
            }
            result._preservedExtensions.append(extension);
        }
    }
    return result;
}

auto X509CertificateBuilder::setCommonName(String value) -> X509CertificateBuilder & {
    _commonName = std::move(value);
    return *this;
}

auto X509CertificateBuilder::setCountry(String value) -> X509CertificateBuilder & {
    _country = std::move(value);
    return *this;
}

auto X509CertificateBuilder::setState(String value) -> X509CertificateBuilder & {
    _state = std::move(value);
    return *this;
}

auto X509CertificateBuilder::setLocality(String value) -> X509CertificateBuilder & {
    _locality = std::move(value);
    return *this;
}

auto X509CertificateBuilder::setOrganization(String value) -> X509CertificateBuilder & {
    _organization = std::move(value);
    return *this;
}

auto X509CertificateBuilder::setOrganizationalUnit(String value) -> X509CertificateBuilder & {
    _organizationalUnit = std::move(value);
    return *this;
}

auto X509CertificateBuilder::addDnsName(String value) -> X509CertificateBuilder & {
    _dnsNames.append(std::move(value));
    return *this;
}

auto X509CertificateBuilder::addIpAddress(network::IpAddress value) -> X509CertificateBuilder & {
    _ipAddresses.append(std::move(value));
    return *this;
}

auto X509CertificateBuilder::setValidFrom(const time::Timestamp value) noexcept -> X509CertificateBuilder & {
    _validFrom = value;
    return *this;
}

auto X509CertificateBuilder::setValidTo(const time::Timestamp value) noexcept -> X509CertificateBuilder & {
    _validTo = value;
    _lifetime.reset();
    return *this;
}

auto X509CertificateBuilder::setValidity(const time::Timestamp from, const time::Timestamp to) noexcept
    -> X509CertificateBuilder & {
    _validFrom = from;
    _validTo = to;
    _lifetime.reset();
    return *this;
}

auto X509CertificateBuilder::setLifetime(const time::CalendarDelta value) noexcept -> X509CertificateBuilder & {
    _validTo.reset();
    _lifetime = value;
    return *this;
}

auto X509CertificateBuilder::setCaPathLength(const uint32_t value) noexcept -> X509CertificateBuilder & {
    _caPathLength = value;
    return *this;
}

auto X509CertificateBuilder::createSelfSignedCertificate(const SigningPrivateKey &key) const -> X509Certificate {
    return impl::X509ArtifactWriter{*this}.createSelfSignedCertificate(key);
}

auto X509CertificateBuilder::createCertificate(
    const SigningPrivateKey &subjectKey,
    const X509Certificate &issuerCertificate,
    const SigningPrivateKey &issuerKey) const -> X509Certificate {
    return impl::X509ArtifactWriter{*this}.createCertificate(subjectKey, issuerCertificate, issuerKey);
}

auto X509CertificateBuilder::createSigningRequest(const SigningPrivateKey &subjectKey) const
    -> X509CertificateSigningRequest {
    return impl::X509ArtifactWriter{*this}.createSigningRequest(subjectKey);
}

}
