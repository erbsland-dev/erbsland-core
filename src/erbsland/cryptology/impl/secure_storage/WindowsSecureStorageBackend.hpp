#pragma once

#include "SecureStorageBackend.hpp"

#include "../../../core/impl/WindowsApi.hpp"
#include "../../../text/u16/U16String.hpp"

namespace erbsland::cryptology::impl {

/// Current-user, local-computer generic Credential Manager adapter.
/// @tested{SecureStorageNativeTest}
class WindowsSecureStorageBackend final : public SecureStorageBackend {
public: // implement SecureStorageBackend
    [[nodiscard]] auto capabilities() const noexcept -> SecureStorageCapabilities override { return {true, false}; }
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
    /// Encode exact namespace and item bytes into a collision-free native target name.
    [[nodiscard]] static auto targetName(const text::String &applicationNamespace, const text::String &item)
        -> text::U16String;
    /// Map a native failure without exposing secret contents.
    [[noreturn]] static void fail(DWORD error);
};

}
