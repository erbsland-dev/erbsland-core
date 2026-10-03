#pragma once

#include "SystemTrustBackend.hpp"

#include <optional>

namespace erbsland::cryptology::impl {

/// Current-user Windows certificate-chain construction and SSL server-policy validation.
/// @needtest{Windows trust, distrust, expiry, and retrieval behavior.}
class WindowsSystemTrustBackend final : public SystemTrustBackend {
public: // implement SystemTrustBackend
    [[nodiscard]] auto validate(
        const X509TlsCertificatePolicy &policy,
        const X509CertificateBundle &peerCertificates,
        const std::optional<network::Host> &referenceIdentity,
        time::Timestamp validationTime) const -> X509CertificateValidation override;
};

}
