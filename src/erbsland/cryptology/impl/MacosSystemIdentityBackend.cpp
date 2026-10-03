#include "MacosSystemIdentityBackend.hpp"

#include "MacosKeychainInteraction.hpp"
#include "MacosTlsSigner.hpp"
#include "NativeTlsAlgorithms.hpp"

#include "../CryptologyError.hpp"

#include "../../text/Literals.hpp"

namespace erbsland::cryptology::impl {

using namespace text::literals;

auto MacosSystemIdentityBackend::identities() const -> MacosOwnedReference<CFArrayRef> {
    const auto query = MacosOwnedReference<CFMutableDictionaryRef>{CFDictionaryCreateMutable(
        kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks)};
    CFDictionarySetValue(query.get(), kSecClass, kSecClassIdentity);
    CFDictionarySetValue(query.get(), kSecMatchLimit, kSecMatchLimitAll);
    CFDictionarySetValue(query.get(), kSecReturnRef, kCFBooleanTrue);
    if (_keychain.get() != nullptr) {
        auto reference = static_cast<const void *>(_keychain.get());
        const auto search =
            MacosOwnedReference<CFArrayRef>{CFArrayCreate(kCFAllocatorDefault, &reference, 1, &kCFTypeArrayCallBacks)};
        CFDictionarySetValue(query.get(), kSecMatchSearchList, search.get());
    }
    auto result = MacosOwnedReference<CFTypeRef>{};
    auto status = OSStatus{};
    {
        const auto interaction = MacosKeychainInteraction{_allowInteraction};
        status = interaction.status();
        if (status == errSecSuccess) {
            status = SecItemCopyMatching(query.get(), result.put());
        }
    }
    if (status == errSecItemNotFound) {
        return MacosOwnedReference<CFArrayRef>{};
    }
    if (status != errSecSuccess || result.get() == nullptr || CFGetTypeID(result.get()) != CFArrayGetTypeID()) {
        throw CryptologyError{"The current-user Keychain identity list is unavailable or requires authorization."_el};
    }
    return MacosOwnedReference<CFArrayRef>{static_cast<CFArrayRef>(result.release())};
}

auto MacosSystemIdentityBackend::certificate(const SecIdentityRef identity) -> X509Certificate {
    auto certificate = MacosOwnedReference<SecCertificateRef>{};
    if (SecIdentityCopyCertificate(identity, certificate.put()) != errSecSuccess) {
        throw CryptologyError{"The Keychain identity certificate is unavailable."_el};
    }
    const auto data = MacosOwnedReference<CFDataRef>{SecCertificateCopyData(certificate.get())};
    return X509Certificate::fromDerOrThrow(
        mem::ByteBlock::fromSpan(
            std::span<const uint8_t>{
                CFDataGetBytePtr(data.get()), static_cast<std::size_t>(CFDataGetLength(data.get()))}),
        X509CertificateProfileMode::Compatible);
}

auto MacosSystemIdentityBackend::list() const -> util::List<SystemIdentityInfo> {
    const auto values = identities();
    auto result = std::vector<SystemIdentityInfo>{};
    if (values.get() == nullptr) {
        return {};
    }
    const auto count = CFArrayGetCount(values.get());
    if (count > 4096) {
        throw CryptologyError{"The user identity store exceeds the 4096-identity resource bound."_el};
    }
    for (auto index = CFIndex{}; index < count; ++index) {
        const auto value = static_cast<SecIdentityRef>(const_cast<void *>(CFArrayGetValueAtIndex(values.get(), index)));
        auto leaf = certificate(value);
        result.push_back(SystemIdentityInfo{native_tls_algorithms::fingerprint(leaf.toDer()), std::move(leaf)});
    }
    return util::List<SystemIdentityInfo>{std::move(result)};
}

auto MacosSystemIdentityBackend::chain(const SecCertificateRef certificate) -> X509CertificateBundle {
    const auto policy = MacosOwnedReference<SecPolicyRef>{SecPolicyCreateBasicX509()};
    auto trust = MacosOwnedReference<SecTrustRef>{};
    if (SecTrustCreateWithCertificates(certificate, policy.get(), trust.put()) != errSecSuccess ||
        SecTrustSetNetworkFetchAllowed(trust.get(), false) != errSecSuccess) {
        throw CryptologyError{"The Keychain certificate chain could not be constructed."_el};
    }
    auto error = MacosOwnedReference<CFErrorRef>{};
    // This constructs available public issuer material, not an authentication decision. Expired or untrusted local
    // identities remain selectable; the peer authenticates their path. No background network retrieval is performed.
    [[maybe_unused]] const auto trusted = SecTrustEvaluateWithError(trust.get(), error.put());
    const auto certificates = MacosOwnedReference<CFArrayRef>{SecTrustCopyCertificateChain(trust.get())};
    if (certificates.get() == nullptr) {
        throw CryptologyError{"The Keychain certificate chain is unavailable."_el};
    }
    auto chain = std::vector<X509Certificate>{};
    const auto count = CFArrayGetCount(certificates.get());
    if (count > 16) {
        throw CryptologyError{"The identity chain exceeds 16 certificates."_el};
    }
    for (auto index = CFIndex{}; index < count; ++index) {
        const auto value =
            static_cast<SecCertificateRef>(const_cast<void *>(CFArrayGetValueAtIndex(certificates.get(), index)));
        const auto data = MacosOwnedReference<CFDataRef>{SecCertificateCopyData(value)};
        auto parsed = X509Certificate::fromDerOrThrow(
            mem::ByteBlock::fromSpan(
                std::span<const uint8_t>{
                    CFDataGetBytePtr(data.get()), static_cast<std::size_t>(CFDataGetLength(data.get()))}),
            X509CertificateProfileMode::Compatible);
        if (index > 0 && index + 1 == count && native_tls_algorithms::isSelfSigned(parsed)) {
            break;
        }
        chain.push_back(std::move(parsed));
    }
    return X509CertificateBundle{util::List<X509Certificate>{std::move(chain)}};
}

auto MacosSystemIdentityBackend::load(const mem::ByteBlock &fingerprint) const -> SystemIdentityMaterial {
    const auto values = identities();
    if (values.get() == nullptr) {
        throw CryptologyError{"The requested certificate identity was not found."_el};
    }
    auto selected = static_cast<SecIdentityRef>(nullptr);
    if (CFArrayGetCount(values.get()) > 4096) {
        throw CryptologyError{"The user identity store exceeds 4096 identities."_el};
    }
    for (auto index = CFIndex{}; index < CFArrayGetCount(values.get()); ++index) {
        const auto value = static_cast<SecIdentityRef>(const_cast<void *>(CFArrayGetValueAtIndex(values.get(), index)));
        if (native_tls_algorithms::fingerprint(certificate(value).toDer()) == fingerprint) {
            if (selected != nullptr) {
                throw CryptologyError{"The certificate fingerprint identifies multiple native identities."_el};
            }
            selected = value;
        }
    }
    if (selected == nullptr) {
        throw CryptologyError{"The requested certificate identity was not found."_el};
    }
    auto key = MacosOwnedReference<SecKeyRef>{};
    auto leaf = MacosOwnedReference<SecCertificateRef>{};
    auto status = OSStatus{};
    {
        const auto interaction = MacosKeychainInteraction{_allowInteraction};
        status = interaction.status();
        if (status == errSecSuccess) {
            status = SecIdentityCopyPrivateKey(selected, key.put());
        }
    }
    if (status != errSecSuccess || SecIdentityCopyCertificate(selected, leaf.put()) != errSecSuccess) {
        throw CryptologyError{"The Keychain identity requires unavailable private-key authorization."_el};
    }
    auto certificates = chain(leaf.get());
    auto signer = std::make_shared<MacosTlsSigner>(
        std::move(key), certificates.certificates().first().publicKey(), _allowInteraction);
    return {std::move(certificates), std::move(signer)};
}

}
