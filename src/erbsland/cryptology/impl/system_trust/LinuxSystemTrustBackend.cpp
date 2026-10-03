#include "LinuxSystemTrustBackend.hpp"

#include "LinuxTrustBundleCache.hpp"

#include "../../../err/RuntimeError.hpp"

#include <optional>

namespace erbsland::cryptology::impl {

auto LinuxSystemTrustBackend::validate(
    const X509TlsCertificatePolicy &policy,
    const X509CertificateBundle &peerCertificates,
    const std::optional<network::Host> &referenceIdentity,
    const time::Timestamp validationTime) const -> X509CertificateValidation {
    try {
        const auto anchors = LinuxTrustBundleCache::load(policy.systemBundlePath());
        const auto explicitPolicy = X509TlsCertificatePolicy{*anchors, policy.intermediates()};
        if (referenceIdentity.has_value()) {
            return explicitPolicy.validateServer(peerCertificates, *referenceIdentity, validationTime);
        }
        return explicitPolicy.validateClient(peerCertificates, validationTime);
    } catch (const err::RuntimeError &error) {
        return reject(X509CertificateValidationFailureCategory::TrustUnavailable, error.reason());
    }
}

}
