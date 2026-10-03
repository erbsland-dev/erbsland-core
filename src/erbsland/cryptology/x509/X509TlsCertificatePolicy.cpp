// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "X509TlsCertificatePolicy.hpp"

#include "../impl/system_trust/SystemTrustBackend.hpp"
#include "../impl/X509TlsCertificateValidator.hpp"

namespace erbsland::cryptology {

auto X509TlsCertificatePolicy::validateServer(
    const X509CertificateBundle &peerCertificates,
    const network::Host &referenceIdentity,
    const time::Timestamp validationTime) const -> X509CertificateValidation {
    if (_systemTrust) {
        return impl::SystemTrustBackend::create()->validate(*this, peerCertificates, referenceIdentity, validationTime);
    }
    return impl::X509TlsCertificateValidator{*this, peerCertificates, referenceIdentity, validationTime}.validate();
}

auto X509TlsCertificatePolicy::systemTrust(const bool allowNetworkRetrieval, path::Path bundlePath)
    -> X509TlsCertificatePolicy {
    auto result = X509TlsCertificatePolicy{X509CertificateBundle{}};
    result._systemTrust = true;
    result._allowNetworkRetrieval = allowNetworkRetrieval;
    result._systemBundlePath = std::move(bundlePath);
    return result;
}

auto X509TlsCertificatePolicy::validateServer(
    const X509CertificateBundle &peerCertificates, const network::Host &referenceIdentity) const
    -> X509CertificateValidation {
    return validateServer(peerCertificates, referenceIdentity, time::Timestamp::now());
}

auto X509TlsCertificatePolicy::validateClient(
    const X509CertificateBundle &peerCertificates, const time::Timestamp validationTime) const
    -> X509CertificateValidation {
    if (_systemTrust) {
        return impl::SystemTrustBackend::create()->validate(*this, peerCertificates, std::nullopt, validationTime);
    }
    return impl::X509TlsCertificateValidator{*this, peerCertificates, std::nullopt, validationTime}.validate();
}

auto X509TlsCertificatePolicy::validateClient(const X509CertificateBundle &peerCertificates) const
    -> X509CertificateValidation {
    return validateClient(peerCertificates, time::Timestamp::now());
}

}
