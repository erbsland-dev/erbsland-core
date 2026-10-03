#include "WindowsSystemIdentityBackend.hpp"

#include "NativeTlsAlgorithms.hpp"
#include "WindowsTlsSigner.hpp"

#include "../CryptologyError.hpp"

#include "../../text/Literals.hpp"

namespace erbsland::cryptology::impl {

using namespace text::literals;

void WindowsSystemIdentityBackend::open(WindowsIdentityResources &resources) const {
    if (_store != nullptr) {
        resources.store = CertDuplicateStore(_store);
        return;
    }
    resources.store = CertOpenStore(
        CERT_STORE_PROV_SYSTEM_W,
        0,
        0,
        CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_READONLY_FLAG | CERT_STORE_OPEN_EXISTING_FLAG,
        L"MY");
    if (resources.store == nullptr) {
        throw CryptologyError{"The current-user personal certificate store is unavailable."_el};
    }
}

auto WindowsSystemIdentityBackend::certificate(const PCCERT_CONTEXT value) -> X509Certificate {
    return X509Certificate::fromDerOrThrow(
        mem::ByteBlock::fromSpan(std::span<const uint8_t>{value->pbCertEncoded, value->cbCertEncoded}),
        X509CertificateProfileMode::Compatible);
}

auto WindowsSystemIdentityBackend::list() const -> util::List<SystemIdentityInfo> {
    auto resources = WindowsIdentityResources{};
    open(resources);
    auto result = std::vector<SystemIdentityInfo>{};
    auto count = std::size_t{};
    while ((resources.current = CertEnumCertificatesInStore(resources.store, resources.current)) != nullptr) {
        if (++count > 4096U) {
            throw CryptologyError{"The user certificate store exceeds 4096 certificates."_el};
        }
        auto size = DWORD{};
        if (!CertGetCertificateContextProperty(resources.current, CERT_KEY_PROV_INFO_PROP_ID, nullptr, &size)) {
            if (!CertGetCertificateContextProperty(resources.current, CERT_NCRYPT_KEY_HANDLE_PROP_ID, nullptr, &size)) {
                continue;
            }
        }
        auto leaf = certificate(resources.current);
        result.push_back(SystemIdentityInfo{native_tls_algorithms::fingerprint(leaf.toDer()), std::move(leaf)});
    }
    if (GetLastError() != CRYPT_E_NOT_FOUND) {
        throw CryptologyError{"The user certificate store could not be enumerated."_el};
    }
    return util::List<SystemIdentityInfo>{std::move(result)};
}

auto WindowsSystemIdentityBackend::load(const mem::ByteBlock &fingerprint) const -> SystemIdentityMaterial {
    auto resources = WindowsIdentityResources{};
    open(resources);
    auto count = std::size_t{};
    while ((resources.current = CertEnumCertificatesInStore(resources.store, resources.current)) != nullptr) {
        if (++count > 4096U) {
            throw CryptologyError{"The user certificate store exceeds 4096 certificates."_el};
        }
        if (native_tls_algorithms::fingerprint(
                mem::ByteBlock::fromSpan(
                    std::span<const uint8_t>{resources.current->pbCertEncoded, resources.current->cbCertEncoded})) ==
            fingerprint) {
            if (resources.selected != nullptr) {
                throw CryptologyError{"The certificate fingerprint is ambiguous in the user store."_el};
            }
            resources.selected = CertDuplicateCertificateContext(resources.current);
        }
    }
    if (GetLastError() != CRYPT_E_NOT_FOUND) {
        throw CryptologyError{"The user certificate store could not be enumerated."_el};
    }
    if (resources.selected == nullptr) {
        throw CryptologyError{"The requested current-user certificate identity was not found."_el};
    }
    auto key = HCRYPTPROV_OR_NCRYPT_KEY_HANDLE{};
    const auto publicKey = certificate(resources.selected).publicKey();
    auto specification = DWORD{};
    auto freeKey = BOOL{};
    // Cached acquisition also supports non-persistent provider keys. WindowsTlsSigner retains the certificate
    // context for the borrowed handle's full lifetime and verifies provider probes against its public key.
    auto flags = DWORD{CRYPT_ACQUIRE_ONLY_NCRYPT_KEY_FLAG | CRYPT_ACQUIRE_COMPARE_KEY_FLAG | CRYPT_ACQUIRE_CACHE_FLAG};
    if (!_allowInteraction) {
        flags |= CRYPT_ACQUIRE_SILENT_FLAG;
    }
    if (!CryptAcquireCertificatePrivateKey(resources.selected, flags, nullptr, &key, &specification, &freeKey)) {
        throw CryptologyError{
            "A usable CNG private key is unavailable or requires authorization; legacy CAPI keys are unsupported."_el};
    }
    // Ownership is transferred immediately, including exceptions during capability discovery. No export API is used.
    auto signer =
        std::make_shared<WindowsTlsSigner>(key, freeKey != FALSE, _allowInteraction, publicKey, resources.selected);
    auto parameters = CERT_CHAIN_PARA{};
    parameters.cbSize = sizeof(parameters);
    if (!CertGetCertificateChain(
            nullptr,
            resources.selected,
            nullptr,
            resources.store,
            &parameters,
            CERT_CHAIN_CACHE_ONLY_URL_RETRIEVAL,
            nullptr,
            &resources.chain) ||
        resources.chain->cChain == 0) {
        throw CryptologyError{"The local identity certificate chain could not be constructed."_el};
    }
    const auto simple = resources.chain->rgpChain[0];
    if (simple->cElement > 16) {
        throw CryptologyError{"The identity chain exceeds 16 certificates."_el};
    }
    auto chain = std::vector<X509Certificate>{};
    for (auto index = DWORD{}; index < simple->cElement; ++index) {
        auto parsed = certificate(simple->rgpElement[index]->pCertContext);
        if (index > 0 && index + 1 == simple->cElement && native_tls_algorithms::isSelfSigned(parsed)) {
            break;
        }
        chain.push_back(std::move(parsed));
    }
    return {X509CertificateBundle{util::List<X509Certificate>{std::move(chain)}}, std::move(signer)};
}

}
