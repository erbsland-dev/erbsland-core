#pragma once

#include "SystemIdentityBackend.hpp"
#include "WindowsIdentityResources.hpp"

namespace erbsland::cryptology::impl {

/// Current-user Windows personal certificate store restricted to non-exported CNG signing keys.
/// @tested{SystemIdentityStoreTest WindowsSystemIdentityNativeTest}
class WindowsSystemIdentityBackend final : public SystemIdentityBackend {
public:
    /// Configure whether provider authorization UI is permitted.
    explicit WindowsSystemIdentityBackend(bool allowInteraction) noexcept : _allowInteraction{allowInteraction} {}
    /// Retain one isolated in-memory store for backend integration tests.
    WindowsSystemIdentityBackend(bool allowInteraction, HCERTSTORE store) noexcept :
        _allowInteraction{allowInteraction}, _store{CertDuplicateStore(store)} {}
    /// Release the retained isolated certificate store, if configured.
    ~WindowsSystemIdentityBackend() override {
        if (_store != nullptr) {
            CertCloseStore(_store, 0);
        }
    }

    // defaults/deletions
    WindowsSystemIdentityBackend(const WindowsSystemIdentityBackend &) = delete;
    auto operator=(const WindowsSystemIdentityBackend &) -> WindowsSystemIdentityBackend & = delete;

public: // implement SystemIdentityBackend
    [[nodiscard]] auto isSupported() const noexcept -> bool override { return true; }
    [[nodiscard]] auto list() const -> util::List<SystemIdentityInfo> override;
    [[nodiscard]] auto load(const mem::ByteBlock &fingerprint) const -> SystemIdentityMaterial override;

private:
    /// Open the current-user store or duplicate the configured isolated test store.
    void open(WindowsIdentityResources &resources) const;
    /// Decode the public DER certificate from a borrowed native context.
    [[nodiscard]] static auto certificate(PCCERT_CONTEXT certificate) -> X509Certificate;

private:
    bool _allowInteraction{}; ///< Explicit authorization-UI permission.
    HCERTSTORE _store{};      ///< Optional retained isolated in-memory store for backend tests.
};

}
