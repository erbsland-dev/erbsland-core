#pragma once

#include "SecureStorageBackend.hpp"

#include "../../SecureStorageError.hpp"

namespace erbsland::cryptology::impl {

/// Explicit unsupported-platform provider; never falls back to plaintext files.
/// @tested{SecureStorageTest}
class UnsupportedSecureStorageBackend final : public SecureStorageBackend {
public: // implement SecureStorageBackend
    [[nodiscard]] auto capabilities() const noexcept -> SecureStorageCapabilities override { return {}; }
    [[nodiscard]] auto read(const text::String &, const text::String &, SecureStorageInteraction)
        -> std::optional<ProtectedByteBlock> override {
        fail();
    }
    void write(const text::String &, const text::String &, mem::ConstByteSpan, SecureStorageInteraction) override {
        fail();
    }
    [[nodiscard]] auto remove(const text::String &, const text::String &, SecureStorageInteraction) -> bool override {
        fail();
    }

private:
    /// Reject unsupported persistent storage without attempting an insecure fallback.
    [[noreturn]] static void fail() {
        throw SecureStorageError{
            SecureStorageErrorCategory::Unsupported,
            text::String{"Persistent secure storage is supported only on macOS and Windows."}};
    }
};

}
