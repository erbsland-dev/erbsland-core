#pragma once

#include "MacosOwnedReference.hpp"
#include "SystemIdentityBackend.hpp"

#include <Security/Security.h>

namespace erbsland::cryptology::impl {

/// Standard current-user Keychain identity adapter, available to command-line and desktop applications.
/// @tested{SystemIdentityStoreTest MacosSystemIdentityNativeTest}
class MacosSystemIdentityBackend final : public SystemIdentityBackend {
public:
    /// Configure whether authorization UI is permitted.
    explicit MacosSystemIdentityBackend(bool allowInteraction) noexcept : _allowInteraction{allowInteraction} {}
    /// Retain one isolated keychain for backend integration tests without changing the user search list.
    MacosSystemIdentityBackend(bool allowInteraction, SecKeychainRef keychain) :
        _allowInteraction{allowInteraction},
        _keychain{static_cast<SecKeychainRef>(const_cast<void *>(CFRetain(keychain)))} {}

public: // implement SystemIdentityBackend
    [[nodiscard]] auto isSupported() const noexcept -> bool override { return true; }
    [[nodiscard]] auto list() const -> util::List<SystemIdentityInfo> override;
    [[nodiscard]] auto load(const mem::ByteBlock &fingerprint) const -> SystemIdentityMaterial override;

private:
    /// Enumerate identity references in the configured native search scope.
    [[nodiscard]] auto identities() const -> MacosOwnedReference<CFArrayRef>;
    /// Copy and decode the public certificate of one native identity.
    [[nodiscard]] static auto certificate(SecIdentityRef identity) -> X509Certificate;
    /// Construct the locally available public chain without network retrieval.
    [[nodiscard]] static auto chain(SecCertificateRef certificate) -> X509CertificateBundle;

private:
    bool _allowInteraction{};                      ///< Explicit authorization-UI policy.
    MacosOwnedReference<SecKeychainRef> _keychain; ///< Optional isolated native store selected by backend tests.
};

}
