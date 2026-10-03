#pragma once

#include "../../../core/impl/WindowsApi.hpp"

#include <wincrypt.h>

namespace erbsland::cryptology::impl {

/// Own native resources for one Windows certificate-chain evaluation.
/// @needtest{Windows trust failure cleanup.}
struct WindowsTrustResources final {
    HCERTSTORE store{};           ///< Owned memory store containing untrusted peer certificates.
    PCCERT_CONTEXT target{};      ///< Owned target certificate context.
    PCCERT_CHAIN_CONTEXT chain{}; ///< Owned native chain context.

    /// Release the selected path and target before closing their peer store.
    ~WindowsTrustResources() {
        if (chain != nullptr) {
            CertFreeCertificateChain(chain);
        }
        if (target != nullptr) {
            CertFreeCertificateContext(target);
        }
        if (store != nullptr) {
            CertCloseStore(store, 0);
        }
    }
    // defaults/deletions
    WindowsTrustResources() = default;
    WindowsTrustResources(const WindowsTrustResources &) = delete;
    auto operator=(const WindowsTrustResources &) -> WindowsTrustResources & = delete;
};

}
