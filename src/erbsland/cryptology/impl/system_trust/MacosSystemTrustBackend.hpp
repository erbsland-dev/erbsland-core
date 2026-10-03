#pragma once

#include "SystemTrustBackend.hpp"

#include <optional>

namespace erbsland::cryptology::impl {

/// Security.framework TLS server chain evaluation with explicit network-retrieval policy.
/// @needtest{Native macOS trust, distrust, expiry, and retrieval behavior.}
class MacosSystemTrustBackend final : public SystemTrustBackend {
public: // implement SystemTrustBackend
    [[nodiscard]] auto validate(
        const X509TlsCertificatePolicy &policy,
        const X509CertificateBundle &peerCertificates,
        const std::optional<network::Host> &referenceIdentity,
        time::Timestamp validationTime) const -> X509CertificateValidation override;
};

}
