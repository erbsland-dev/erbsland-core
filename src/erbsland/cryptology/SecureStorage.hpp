#pragma once

#include "SecureStorageCapabilities.hpp"
#include "SecureStorageInteraction.hpp"

#include "impl/secure_storage/SecureStorageBackend_fwd.hpp"
#include "protected_data/ProtectedByteBlock.hpp"

#include "../text/String.hpp"

#include <memory>
#include <optional>

namespace erbsland::cryptology {

/// Synchronous current-user persistent secret storage in an application namespace.
/// @seedoc{/reference/cryptology/protected_data}
/// @tested{SecureStorageTest SecureStorageNativeTest}
class SecureStorage final {
    friend class impl::SecureStorageTestAccess;

public:
    /// Bind storage to an application namespace without accessing the vault.
    /// @param applicationNamespace A nonempty UTF-8 name of at most 256 bytes without control characters.
    /// @param interaction Whether native authorization UI is allowed.
    /// @throws SecureStorageError If the namespace is invalid.
    explicit SecureStorage(
        text::String applicationNamespace, SecureStorageInteraction interaction = SecureStorageInteraction::Forbid);

public: // accessors
    /// Get the application namespace; it is a collision boundary, not an authorization boundary.
    [[nodiscard]] auto applicationNamespace() const noexcept -> const text::String & { return _namespace; }
    /// Get capabilities without accessing or unlocking the vault.
    [[nodiscard]] auto capabilities() const noexcept -> SecureStorageCapabilities;
    /// Get the common maximum plaintext payload length.
    [[nodiscard]] static constexpr auto maximumPayloadLength() noexcept -> unit::ByteCount {
        return unit::ByteCount{2560U};
    }

public: // operations
    /// Read an exact item; missing and present-but-empty are distinct.
    /// @param item A nonempty UTF-8 name of at most 256 bytes without control characters.
    /// @return Protected plaintext, or no value if the item does not exist.
    /// @throws SecureStorageError If access, authorization, or retrieval fails.
    /// @throws CryptologyError If application-lifetime protection fails.
    [[nodiscard]] auto read(const text::String &item) const -> std::optional<ProtectedByteBlock>;
    /// Create or replace an item from explicitly borrowed plaintext bytes.
    /// @param item The exact item name.
    /// @param data Borrowed plaintext, at most 2,560 bytes; the caller retains erasure responsibility.
    /// @throws SecureStorageError If validation, authorization, or persistence fails.
    void write(const text::String &item, mem::ConstByteSpan data) const;
    /// Create or replace an item from protected plaintext, never persisting its memory envelope.
    /// @param item The exact item name.
    /// @param data Protected bytes to decrypt only for the synchronous native operation.
    /// @throws SecureStorageError If validation, authorization, or persistence fails.
    /// @throws CryptologyError If application-lifetime decryption fails.
    void write(const text::String &item, const ProtectedByteBlock &data) const;
    /// Remove an item.
    /// @param item The exact item name.
    /// @return Whether the item existed.
    /// @throws SecureStorageError If validation, authorization, or removal fails.
    [[nodiscard]] auto remove(const text::String &item) const -> bool;
    /// Validate portable namespace and item syntax without accessing a vault.
    /// @param name The namespace or item name.
    /// @return Whether the UTF-8 name is nonempty, bounded, and free of control characters.
    [[nodiscard]] static auto isValidName(const text::String &name) noexcept -> bool;

private:
    text::String _namespace;                              ///< Exact application namespace.
    SecureStorageInteraction _interaction;                ///< Explicit authorization policy.
    std::shared_ptr<impl::SecureStorageBackend> _backend; ///< Native provider or injected test provider.
};

}
