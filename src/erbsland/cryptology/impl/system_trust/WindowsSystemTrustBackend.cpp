#include "WindowsSystemTrustBackend.hpp"

#include "WindowsTrustResources.hpp"

#include "../../../err/RuntimeError.hpp"
#include "../../../text/Literals.hpp"

#include <wininet.h>

#include <limits>
#include <optional>

namespace erbsland::cryptology::impl {

using namespace text::literals;

auto WindowsSystemTrustBackend::validate(
    const X509TlsCertificatePolicy &policy,
    const X509CertificateBundle &peerCertificates,
    const std::optional<network::Host> &referenceIdentity,
    const time::Timestamp validationTime) const -> X509CertificateValidation {
    if (!policy.systemBundlePath().isEmpty()) {
        return reject(
            X509CertificateValidationFailureCategory::TrustUnavailable,
            "A bundle-path override is only supported by Linux system trust."_el);
    }
    if (peerCertificates.isEmpty()) {
        return reject(
            X509CertificateValidationFailureCategory::EmptyPeerCertificates, "The peer supplied no certificate."_el);
    }
    const auto unixSeconds = static_cast<int64_t>(validationTime.toTimeT());
    if (!validationTime.isValid() || unixSeconds < -11'644'473'600LL ||
        unixSeconds > static_cast<int64_t>(std::numeric_limits<uint64_t>::max() / 10'000'000ULL) - 11'644'473'600LL) {
        return reject(
            X509CertificateValidationFailureCategory::InvalidValidationTime,
            "The validation time is outside the native range."_el);
    }
    if (peerCertificates.certificates().count().toSizeT() > X509TlsCertificatePolicy::cMaximumCandidateCertificates) {
        return reject(
            X509CertificateValidationFailureCategory::CandidateLimitExceeded,
            "The peer certificate limit was exceeded."_el);
    }
    const auto ticks = static_cast<uint64_t>(unixSeconds + 11'644'473'600LL) * 10'000'000ULL;
    auto nativeTime = FILETIME{static_cast<DWORD>(ticks), static_cast<DWORD>(ticks >> 32U)};
    auto native = WindowsTrustResources{};
    native.store = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0, CERT_STORE_CREATE_NEW_FLAG, nullptr);
    if (native.store == nullptr) {
        return reject(
            X509CertificateValidationFailureCategory::NativeFailure,
            "Unable to create the native peer certificate store."_el);
    }
    for (const auto &certificate : peerCertificates.certificates()) {
        const auto der = certificate.toDer();
        const auto bytes = der.span();
        auto targetOutput = native.target == nullptr ? &native.target : nullptr;
        if (!CertAddEncodedCertificateToStore(
                native.store,
                X509_ASN_ENCODING,
                reinterpret_cast<const BYTE *>(bytes.data()),
                static_cast<DWORD>(bytes.size()),
                CERT_STORE_ADD_ALWAYS,
                targetOutput)) {
            return reject(
                X509CertificateValidationFailureCategory::SystemPolicyRejected,
                "Windows rejected a peer certificate encoding."_el);
        }
    }

    // RFC 5280 and RFC 8446 section 4.4.2.2: require the peer purpose while the native engine owns trust and
    // constraints.
    auto parameters = CERT_CHAIN_PARA{};
    parameters.cbSize = sizeof(parameters);
    auto requestedUsage = const_cast<LPSTR>(szOID_PKIX_KP_CLIENT_AUTH);
    if (referenceIdentity.has_value()) {
        requestedUsage = const_cast<LPSTR>(szOID_PKIX_KP_SERVER_AUTH);
    }
    parameters.RequestedUsage.dwType = USAGE_MATCH_TYPE_AND;
    parameters.RequestedUsage.Usage.cUsageIdentifier = 1;
    parameters.RequestedUsage.Usage.rgpszUsageIdentifier = &requestedUsage;
    auto flags = DWORD{CERT_CHAIN_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT | CERT_CHAIN_REVOCATION_ACCUMULATIVE_TIMEOUT};
    if (!policy.allowsNetworkRetrieval()) {
        flags |= CERT_CHAIN_CACHE_ONLY_URL_RETRIEVAL | CERT_CHAIN_REVOCATION_CHECK_CACHE_ONLY;
    }
    // Native retrieval remains synchronous; connection deadlines cancel logical ownership, not the OS operation.
    if (!CertGetCertificateChain(
            HCCE_CURRENT_USER, native.target, &nativeTime, native.store, &parameters, flags, nullptr, &native.chain)) {
        return reject(
            X509CertificateValidationFailureCategory::NativeFailure,
            "Windows certificate-chain construction failed."_el);
    }
    const auto trustErrors = native.chain->TrustStatus.dwErrorStatus;
    if ((trustErrors & (CERT_TRUST_IS_REVOKED | CERT_TRUST_IS_EXPLICIT_DISTRUST)) != 0U) {
        return reject(
            X509CertificateValidationFailureCategory::CertificateRevoked,
            "Windows reports a revoked or explicitly distrusted certificate."_el);
    }
    auto sslParameters = SSL_EXTRA_CERT_CHAIN_POLICY_PARA{};
    sslParameters.cbSize = sizeof(sslParameters);
    sslParameters.dwAuthType = AUTHTYPE_CLIENT;
    if (referenceIdentity.has_value()) {
        sslParameters.dwAuthType = AUTHTYPE_SERVER;
    }
    // RFC 9525 SAN-only matching is applied after native TLS policy; no platform Common Name fallback is accepted.
    sslParameters.fdwChecks = SECURITY_FLAG_IGNORE_CERT_CN_INVALID;
    auto policyParameters = CERT_CHAIN_POLICY_PARA{};
    policyParameters.cbSize = sizeof(policyParameters);
    policyParameters.dwFlags = CERT_CHAIN_POLICY_IGNORE_ALL_REV_UNKNOWN_FLAGS;
    policyParameters.pvExtraPolicyPara = &sslParameters;
    auto policyStatus = CERT_CHAIN_POLICY_STATUS{};
    policyStatus.cbSize = sizeof(policyStatus);
    if (!CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_SSL, native.chain, &policyParameters, &policyStatus)) {
        return reject(
            X509CertificateValidationFailureCategory::NativeFailure,
            "Windows SSL certificate-policy evaluation failed."_el);
    }
    if (policyStatus.dwError != 0U) {
        auto category = X509CertificateValidationFailureCategory::SystemPolicyRejected;
        if (policyStatus.dwError == CERT_E_EXPIRED) {
            category = X509CertificateValidationFailureCategory::CertificateExpired;
        }
        return reject(category, "Windows SSL policy rejected the certificate path."_el);
    }
    if (native.chain->cChain == 0U || native.chain->rgpChain[0]->cElement == 0U ||
        native.chain->rgpChain[0]->cElement > X509TlsCertificatePolicy::cMaximumPathDepth) {
        return reject(
            X509CertificateValidationFailureCategory::NativeFailure,
            "Windows returned an invalid certificate path."_el);
    }
    auto path = util::List<X509Certificate>{};
    try {
        const auto chain = native.chain->rgpChain[0];
        for (auto index = DWORD{}; index < chain->cElement; ++index) {
            const auto certificate = chain->rgpElement[index]->pCertContext;
            const auto bytes = mem::toConstByteSpan(
                std::span<const uint8_t>{
                    certificate->pbCertEncoded, static_cast<std::size_t>(certificate->cbCertEncoded)});
            path.append(
                X509Certificate::fromDerOrThrow(
                    mem::ByteBlock::fromSpan(bytes), X509CertificateProfileMode::Compatible));
        }
    } catch (const err::RuntimeError &) {
        return reject(
            X509CertificateValidationFailureCategory::SystemPolicyRejected,
            "A native path certificate violates the supported certificate encoding."_el);
    }
    auto revocation = X509RevocationStatus::Checked;
    if ((trustErrors & (CERT_TRUST_REVOCATION_STATUS_UNKNOWN | CERT_TRUST_IS_OFFLINE_REVOCATION)) != 0U) {
        revocation = X509RevocationStatus::Unknown;
    }
    return acceptNativePath(policy, peerCertificates, referenceIdentity, validationTime, std::move(path), revocation);
}

}
