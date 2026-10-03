#pragma once

#include "SystemTrustBackend.hpp"

#include <optional>

namespace erbsland::cryptology::impl {

/// Portable TLS validation using a distribution-owned Linux CA bundle, without network retrieval or revocation.
/// @needtest{Distribution trust success and failed reload integration.}
class LinuxSystemTrustBackend final : public SystemTrustBackend {
public: // implement SystemTrustBackend
    [[nodiscard]] auto validate(
        const X509TlsCertificatePolicy &policy,
        const X509CertificateBundle &peerCertificates,
        const std::optional<network::Host> &referenceIdentity,
        time::Timestamp validationTime) const -> X509CertificateValidation override;
};

}
