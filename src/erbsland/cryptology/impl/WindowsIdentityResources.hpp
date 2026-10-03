#pragma once

#include "../../core/impl/WindowsApi.hpp"

#include <wincrypt.h>

namespace erbsland::cryptology::impl {

/// RAII ownership of one current-user certificate-store enumeration and optional chain construction.
/// @tested{SystemIdentityStoreTest WindowsSystemIdentityNativeTest}
class WindowsIdentityResources final {
public:
    /// Create an empty native resource scope.
    WindowsIdentityResources() = default;
    /// Release every retained native resource in dependency order.
    ~WindowsIdentityResources() {
        if (chain != nullptr) {
            CertFreeCertificateChain(chain);
        }
        if (selected != nullptr) {
            CertFreeCertificateContext(selected);
        }
        if (current != nullptr) {
            CertFreeCertificateContext(current);
        }
        if (store != nullptr) {
            CertCloseStore(store, 0);
        }
    }

    // defaults/deletions
    WindowsIdentityResources(const WindowsIdentityResources &) = delete;
    auto operator=(const WindowsIdentityResources &) -> WindowsIdentityResources & = delete;

public:
    HCERTSTORE store{};           ///< Open current-user personal certificate store.
    PCCERT_CONTEXT current{};     ///< Current CertEnumCertificatesInStore result.
    PCCERT_CONTEXT selected{};    ///< Independently retained selected certificate.
    PCCERT_CHAIN_CONTEXT chain{}; ///< Constructed public chain.
};

}
