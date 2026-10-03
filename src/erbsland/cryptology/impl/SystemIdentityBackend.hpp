#pragma once

#include "SystemIdentityBackend_fwd.hpp"
#include "SystemIdentityMaterial.hpp"

#include "../tls/SystemIdentityInfo.hpp"

namespace erbsland::cryptology::impl {

/// Mockable isolated contract for current-user native identity discovery and acquisition.
/// @tested{SystemIdentityStoreTest}
class SystemIdentityBackend {
public:
    // defaults
    virtual ~SystemIdentityBackend() = default;

public: // operations
    /// Test whether the platform has an implemented native provider.
    [[nodiscard]] virtual auto isSupported() const noexcept -> bool = 0;
    /// List public metadata without selecting an identity.
    [[nodiscard]] virtual auto list() const -> util::List<SystemIdentityInfo> = 0;
    /// Load the exact certificate's private signer and public chain.
    [[nodiscard]] virtual auto load(const mem::ByteBlock &fingerprint) const -> SystemIdentityMaterial = 0;

public: // factories
    /// Create the platform backend with an explicit authorization-UI policy.
    [[nodiscard]] static auto create(bool allowInteraction) -> SystemIdentityBackendPtr;
};

}
