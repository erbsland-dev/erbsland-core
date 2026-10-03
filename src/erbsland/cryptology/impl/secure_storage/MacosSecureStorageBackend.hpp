#pragma once

#include "SecureStorageBackend.hpp"

#include "../MacosOwnedReference.hpp"

#include <Security/Security.h>

namespace erbsland::cryptology::impl {

/// Standard user-Keychain generic-password adapter with scoped native interaction policy.
/// @tested{SecureStorageNativeTest}
class MacosSecureStorageBackend final : public SecureStorageBackend {
public: // implement SecureStorageBackend
    [[nodiscard]] auto capabilities() const noexcept -> SecureStorageCapabilities override { return {true, true}; }
    [[nodiscard]] auto read(
        const text::String &applicationNamespace, const text::String &item, SecureStorageInteraction interaction)
        -> std::optional<ProtectedByteBlock> override;
    void write(
        const text::String &applicationNamespace,
        const text::String &item,
        mem::ConstByteSpan data,
        SecureStorageInteraction interaction) override;
    [[nodiscard]] auto remove(
        const text::String &applicationNamespace, const text::String &item, SecureStorageInteraction interaction)
        -> bool override;

private:
    /// Build the namespace-bound Keychain service name.
    [[nodiscard]] static auto serviceName(const text::String &applicationNamespace) -> text::String;
    /// Open the standard current-user default Keychain.
    [[nodiscard]] static auto defaultKeychain() -> MacosOwnedReference<SecKeychainRef>;
    /// Map a native failure without exposing secret contents.
    [[noreturn]] static void fail(OSStatus status);
};

}
