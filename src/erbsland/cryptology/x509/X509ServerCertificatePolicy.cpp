// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "X509ServerCertificatePolicy.hpp"

#include "../impl/X509ServerCertificateValidator.hpp"

namespace erbsland::cryptology {

auto X509ServerCertificatePolicy::validate(
    const X509CertificateBundle &peerCertificates,
    const network::Host &referenceIdentity,
    const time::DateTime validationTime) const -> X509CertificateValidation {
    return impl::X509ServerCertificateValidator{*this, peerCertificates, referenceIdentity, validationTime}.validate();
}

auto X509ServerCertificatePolicy::validate(
    const X509CertificateBundle &peerCertificates, const network::Host &referenceIdentity) const
    -> X509CertificateValidation {
    return validate(peerCertificates, referenceIdentity, time::DateTime::now());
}

}
