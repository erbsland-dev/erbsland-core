#pragma once

#include <erbsland/cryptology/impl/SystemIdentityBackend.hpp>
#include <erbsland/SystemIdentityStore.hpp>

namespace erbsland::cryptology::impl {

/// Bind identity-store tests to isolated mock or native stores without exposing public native handles.
class SystemIdentityStoreTestAccess final {
public:
    /// Bind access to the store under test.
    /// @param store The store, which must outlive this helper.
    explicit SystemIdentityStoreTestAccess(SystemIdentityStore &store) noexcept : _store{store} {}

public: // test backend
    /// Replace the backend used by this store.
    /// @param backend The test backend.
    void setBackend(SystemIdentityBackendPtr backend) { _store._backend = std::move(backend); }

private:
    SystemIdentityStore &_store; ///< The store under test.
};

}
