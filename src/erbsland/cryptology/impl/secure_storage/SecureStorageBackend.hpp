#pragma once

#include "SecureStorageBackend_fwd.hpp"

#include "../../../text/String.hpp"
#include "../../protected_data/ProtectedByteBlock.hpp"
#include "../../SecureStorageCapabilities.hpp"
#include "../../SecureStorageInteraction.hpp"

#include <memory>
#include <optional>

namespace erbsland::cryptology::impl {

/// Mockable synchronous persistent-secret provider with no public native handles.
/// @tested{SecureStorageTest SecureStorageNativeTest}
class SecureStorageBackend {
public:
    // defaults
    virtual ~SecureStorageBackend() = default;

public: // operations
    /// Get platform capabilities without opening or unlocking storage.
    [[nodiscard]] virtual auto capabilities() const noexcept -> SecureStorageCapabilities = 0;
    /// Read and protect an exact item, or return no value when missing.
    /// @param applicationNamespace The validated application namespace.
    /// @param item The validated item name.
    /// @param interaction The authorization UI policy.
    /// @return The protected value, including an engaged empty value for empty items.
    /// @throws SecureStorageError If native retrieval fails.
    [[nodiscard]] virtual auto read(
        const text::String &applicationNamespace, const text::String &item, SecureStorageInteraction interaction)
        -> std::optional<ProtectedByteBlock> = 0;
    /// Atomically create or replace an item with borrowed plaintext.
    /// @param applicationNamespace The validated application namespace.
    /// @param item The validated item name.
    /// @param data Borrowed bytes valid for this synchronous call only.
    /// @param interaction The authorization UI policy.
    /// @throws SecureStorageError If native persistence fails.
    virtual void write(
        const text::String &applicationNamespace,
        const text::String &item,
        mem::ConstByteSpan data,
        SecureStorageInteraction interaction) = 0;
    /// Remove one item.
    /// @param applicationNamespace The validated application namespace.
    /// @param item The validated item name.
    /// @param interaction The authorization UI policy.
    /// @return Whether an existing item was removed.
    /// @throws SecureStorageError If native removal fails.
    [[nodiscard]] virtual auto remove(
        const text::String &applicationNamespace, const text::String &item, SecureStorageInteraction interaction)
        -> bool = 0;

public: // factories
    /// Create the platform provider without accessing a vault.
    /// @return A provider, including an explicitly unsupported provider on Linux.
    [[nodiscard]] static auto create() -> std::shared_ptr<SecureStorageBackend>;
};

}
