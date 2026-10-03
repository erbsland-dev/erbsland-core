#pragma once

#include "SystemIdentityInfo.hpp"
#include "SystemIdentityStore_fwd.hpp"
#include "TlsIdentity.hpp"

#include "../impl/SystemIdentityBackend_fwd.hpp"
#include "../impl/SystemIdentityStoreTestAccess_fwd.hpp"

namespace erbsland::cryptology {

/// Explicit access to current-user OS certificate identities; Linux has no native provider.
/// @seedoc{/reference/cryptology/tls}
/// @tested{SystemIdentityStoreTest}
class SystemIdentityStore final {
    friend class impl::SystemIdentityStoreTestAccess;

public:
    /// Create a store with explicit authorization-UI policy; no hidden worker threads are created.
    /// @param allowInteraction Whether provider authorization UI is permitted.
    explicit SystemIdentityStore(bool allowInteraction = false);

public: // operations
    /// Test whether this platform implements native identities.
    [[nodiscard]] auto isSupported() const noexcept -> bool;
    /// List public certificate metadata from the current-user identity store.
    /// @return Available public identities, without selecting one for a connection.
    /// @throws CryptologyError If native access fails or this platform is unsupported.
    [[nodiscard]] auto list() const -> util::List<SystemIdentityInfo>;
    /// Load exactly one identity by SHA-256 certificate fingerprint, retaining private keys in their provider.
    /// @param fingerprint Exactly 32 fingerprint bytes.
    /// @return The identity and its available chain, ready for explicit TLS configuration.
    /// @throws CryptologyError If no exact usable identity exists, access is denied, or interaction is required.
    [[nodiscard]] auto load(const mem::ByteBlock &fingerprint) const -> TlsIdentity;

private:
    impl::SystemIdentityBackendPtr _backend; ///< Isolated native adapter.
};

}
