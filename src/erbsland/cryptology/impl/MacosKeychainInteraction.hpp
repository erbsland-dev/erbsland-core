#pragma once

#include <Security/Security.h>

#include <mutex>

namespace erbsland::cryptology::impl {

/// Serialize library Keychain calls while temporarily selecting the process-wide interaction policy.
/// @seedoc{/reference/cryptology/protected_data}
/// @tested{SecureStorageNativeTest}
class MacosKeychainInteraction final {
public:
    /// Lock the shared native boundary and install the requested policy.
    /// @param allowInteraction Whether this operation explicitly permits authorization UI.
    explicit MacosKeychainInteraction(bool allowInteraction);
    /// Restore the previous policy before releasing the shared lock.
    ~MacosKeychainInteraction();

    // defaults/deletions
    MacosKeychainInteraction(const MacosKeychainInteraction &) = delete;
    auto operator=(const MacosKeychainInteraction &) -> MacosKeychainInteraction & = delete;

public: // accessors
    /// Get policy setup status; do not issue a native call unless this is errSecSuccess.
    [[nodiscard]] auto status() const noexcept -> OSStatus { return _status; }

private:
    static std::mutex _mutex;           ///< Shared boundary for all library legacy-Keychain operations.
    std::unique_lock<std::mutex> _lock; ///< Serialize policy changes and associated native calls.
    Boolean _previous{false};           ///< Prior process-wide native policy.
    OSStatus _status{errSecSuccess};    ///< Native setup failure, if any.
    bool _restore{false};               ///< A policy change must be restored.
};

}
