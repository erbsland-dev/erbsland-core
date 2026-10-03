#pragma once

#include <erbsland/cryptology/impl/secure_storage/SecureStorageBackend.hpp>
#include <erbsland/SecureStorage.hpp>

namespace erbsland::cryptology::impl {

/// Substitute a deterministic storage backend without accessing the user's vault.
class SecureStorageTestAccess final {
public:
    /// Bind access to the store under test.
    /// @param store The store, which must outlive this helper.
    explicit SecureStorageTestAccess(SecureStorage &store) noexcept : _store{store} {}

public: // test backend
    /// Replace the backend used by this store.
    /// @param backend The test backend.
    void setBackend(std::shared_ptr<SecureStorageBackend> backend) { _store._backend = std::move(backend); }

private:
    SecureStorage &_store; ///< The store under test.
};

}
