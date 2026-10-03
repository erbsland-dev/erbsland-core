#include "MacosSystemTrustBackend.hpp"

#include "MacosTrustResources.hpp"

#include "../MacosOwnedReference.hpp"

#include "../../../err/RuntimeError.hpp"
#include "../../../text/Literals.hpp"

#include <optional>

namespace erbsland::cryptology::impl {

using namespace text::literals;

auto MacosSystemTrustBackend::validate(
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
    if (!validationTime.isValid()) {
        return reject(
            X509CertificateValidationFailureCategory::InvalidValidationTime, "The validation time is invalid."_el);
    }
    if (peerCertificates.certificates().count().toSizeT() > X509TlsCertificatePolicy::cMaximumCandidateCertificates) {
        return reject(
            X509CertificateValidationFailureCategory::CandidateLimitExceeded,
            "The peer certificate limit was exceeded."_el);
    }

    auto native = MacosTrustResources{};
    native.certificates.reset(CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks));
    if (native.certificates.get() == nullptr) {
        return reject(
            X509CertificateValidationFailureCategory::NativeFailure,
            "Unable to allocate the native certificate array."_el);
    }
    for (const auto &certificate : peerCertificates.certificates()) {
        const auto der = certificate.toDer();
        const auto bytes = der.span();
        const auto data = MacosOwnedReference{CFDataCreate(
            kCFAllocatorDefault, reinterpret_cast<const UInt8 *>(bytes.data()), static_cast<CFIndex>(bytes.size()))};
        if (data.get() == nullptr) {
            return reject(
                X509CertificateValidationFailureCategory::NativeFailure,
                "Unable to allocate native certificate data."_el);
        }
        const auto nativeCertificate =
            MacosOwnedReference<SecCertificateRef>{SecCertificateCreateWithData(kCFAllocatorDefault, data.get())};
        if (nativeCertificate.get() == nullptr) {
            return reject(
                X509CertificateValidationFailureCategory::SystemPolicyRejected,
                "Security.framework rejected a certificate encoding."_el);
        }
        CFArrayAppendValue(native.certificates.get(), nativeCertificate.get());
    }

    // RFC 5280 and RFC 8446 section 4.4.2.2: Security.framework owns path, trust settings, and peer-purpose checks.
    // Hostname matching is deliberately deferred to the common RFC 9525 SAN-only checker, never Common Name fallback.
    native.sslPolicy.reset(SecPolicyCreateSSL(referenceIdentity.has_value(), nullptr));
    auto revocationFlags = CFOptionFlags{kSecRevocationUseAnyAvailableMethod};
    if (!policy.allowsNetworkRetrieval()) {
        revocationFlags |= kSecRevocationNetworkAccessDisabled;
    }
    native.revocationPolicy.reset(SecPolicyCreateRevocation(revocationFlags));
    if (native.sslPolicy.get() == nullptr || native.revocationPolicy.get() == nullptr) {
        return reject(
            X509CertificateValidationFailureCategory::NativeFailure, "Unable to create native TLS trust policies."_el);
    }
    const void *policies[]{native.sslPolicy.get(), native.revocationPolicy.get()};
    native.policies.reset(CFArrayCreate(kCFAllocatorDefault, policies, 2, &kCFTypeArrayCallBacks));
    native.date.reset(CFDateCreate(
        kCFAllocatorDefault, static_cast<CFAbsoluteTime>(validationTime.toTimeT()) - kCFAbsoluteTimeIntervalSince1970));
    if (native.policies.get() == nullptr || native.date.get() == nullptr ||
        SecTrustCreateWithCertificates(native.certificates.get(), native.policies.get(), native.trust.put()) !=
            errSecSuccess ||
        SecTrustSetVerifyDate(native.trust.get(), native.date.get()) != errSecSuccess ||
        SecTrustSetNetworkFetchAllowed(native.trust.get(), policy.allowsNetworkRetrieval()) != errSecSuccess) {
        return reject(
            X509CertificateValidationFailureCategory::NativeFailure,
            "Unable to initialize native TLS trust evaluation."_el);
    }

    // This synchronous call may retrieve intermediates or revocation evidence. The connection owns its deadline;
    // logical cancellation never invalidates these RAII-owned native resources while the OS operation is running.
    if (!SecTrustEvaluateWithError(native.trust.get(), native.error.put())) {
        const auto code = native.error.get() == nullptr ? CFIndex{} : CFErrorGetCode(native.error.get());
        auto category = X509CertificateValidationFailureCategory::SystemPolicyRejected;
        if (code == errSecCertificateRevoked) {
            category = X509CertificateValidationFailureCategory::CertificateRevoked;
        } else if (code == errSecCertificateExpired) {
            category = X509CertificateValidationFailureCategory::CertificateExpired;
        } else if (code == errSecCertificateNotValidYet) {
            category = X509CertificateValidationFailureCategory::CertificateNotYetValid;
        } else if (code == errSecNotAvailable || code == errSecInternalComponent) {
            category = X509CertificateValidationFailureCategory::NativeFailure;
        }
        return reject(category, "Security.framework rejected the TLS certificate path."_el);
    }
    native.chain.reset(SecTrustCopyCertificateChain(native.trust.get()));
    if (native.chain.get() == nullptr ||
        CFArrayGetCount(native.chain.get()) > static_cast<CFIndex>(X509TlsCertificatePolicy::cMaximumPathDepth)) {
        return reject(
            X509CertificateValidationFailureCategory::NativeFailure,
            "Security.framework returned an invalid certificate path."_el);
    }
    auto path = util::List<X509Certificate>{};
    try {
        for (auto index = CFIndex{}; index < CFArrayGetCount(native.chain.get()); ++index) {
            const auto certificate =
                static_cast<SecCertificateRef>(const_cast<void *>(CFArrayGetValueAtIndex(native.chain.get(), index)));
            const auto data = MacosOwnedReference{SecCertificateCopyData(certificate)};
            if (data.get() == nullptr) {
                return reject(
                    X509CertificateValidationFailureCategory::NativeFailure,
                    "Unable to copy a native path certificate."_el);
            }
            const auto bytes = mem::toConstByteSpan(
                std::span<const uint8_t>{
                    CFDataGetBytePtr(data.get()), static_cast<std::size_t>(CFDataGetLength(data.get()))});
            path.append(
                X509Certificate::fromDerOrThrow(
                    mem::ByteBlock::fromSpan(bytes), X509CertificateProfileMode::Compatible));
        }
    } catch (const err::RuntimeError &) {
        return reject(
            X509CertificateValidationFailureCategory::SystemPolicyRejected,
            "A native path certificate violates the supported certificate encoding."_el);
    }
    auto revocation = X509RevocationStatus::Unknown;
    native.result.reset(SecTrustCopyResult(native.trust.get()));
    if (native.result.get() != nullptr &&
        CFDictionaryGetValue(native.result.get(), kSecTrustRevocationChecked) == kCFBooleanTrue) {
        revocation = X509RevocationStatus::Checked;
    }
    return acceptNativePath(policy, peerCertificates, referenceIdentity, validationTime, std::move(path), revocation);
}

}
