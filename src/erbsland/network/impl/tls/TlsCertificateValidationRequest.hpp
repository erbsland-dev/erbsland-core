#pragma once

#include "TlsAlertDescription.hpp"

#include "../../../cryptology/x509/X509CertificateBundle.hpp"
#include "../../../cryptology/x509/X509CertificateValidation.hpp"
#include "../../../cryptology/x509/X509TlsCertificatePolicy.hpp"
#include "../../../time/Timestamp.hpp"
#include "../../Host.hpp"

namespace erbsland::network::impl {

/// An immutable certificate-validation operation emitted by the deterministic protocol.
/// @tested{TlsClientProtocolTest TlsClientConnectionTest TlsMutualAuthenticationTest TlsServerConnectionTest}
struct TlsCertificateValidationRequest final {
    cryptology::X509TlsCertificatePolicy policy;    ///< Captured trust policy.
    cryptology::X509CertificateBundle certificates; ///< Bounded peer certificate list.
    std::optional<Host> host;       ///< Server SAN reference identity; absent for client authentication.
    time::Timestamp validationTime; ///< Captured validation instant.

    /// Execute outside the owning event loop and without connection locks.
    [[nodiscard]] auto execute() const -> cryptology::X509CertificateValidation {
        if (host.has_value()) {
            return policy.validateServer(certificates, *host, validationTime);
        }
        return policy.validateClient(certificates, validationTime);
    }
    /// Map a structured trust failure to a TLS alert.
    /// @param category The rejected certificate condition.
    /// @return The corresponding fatal alert.
    [[nodiscard]] static auto alert(const cryptology::X509CertificateValidationFailureCategory category) noexcept
        -> TlsAlertDescription {
        using Category = cryptology::X509CertificateValidationFailureCategory;
        switch (category) {
        case Category::TrustUnavailable:
        case Category::NativeFailure:
            return TlsAlertDescription::InternalError;
        case Category::CertificateRevoked:
            return TlsAlertDescription::CertificateRevoked;
        case Category::EmptyTrustAnchors:
        case Category::IssuerNotFound:
        case Category::PathLoop:
            return TlsAlertDescription::UnknownCa;
        case Category::CertificateNotYetValid:
        case Category::CertificateExpired:
            return TlsAlertDescription::CertificateExpired;
        case Category::SignatureUnsupported:
            return TlsAlertDescription::UnsupportedCertificate;
        default:
            return TlsAlertDescription::BadCertificate;
        }
    }
};

}
