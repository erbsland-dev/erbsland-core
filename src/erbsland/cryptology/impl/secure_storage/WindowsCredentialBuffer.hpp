#pragma once

#include "../../../core/impl/WindowsApi.hpp"

#include <wincred.h>

namespace erbsland::cryptology::impl {

/// Own and erase a Credential Manager result before releasing native storage.
/// @tested{SecureStorageNativeTest}
class WindowsCredentialBuffer final {
public:
    /// Adopt a CredReadW allocation.
    explicit WindowsCredentialBuffer(PCREDENTIALW credential) noexcept : _credential{credential} {}
    /// Wipe the complete plaintext payload before freeing the native allocation.
    ~WindowsCredentialBuffer() {
        if (_credential != nullptr) {
            if (_credential->CredentialBlob != nullptr) {
                SecureZeroMemory(_credential->CredentialBlob, _credential->CredentialBlobSize);
            }
            CredFree(_credential);
        }
    }

    // defaults/deletions
    WindowsCredentialBuffer(const WindowsCredentialBuffer &) = delete;
    auto operator=(const WindowsCredentialBuffer &) -> WindowsCredentialBuffer & = delete;

private:
    PCREDENTIALW _credential; ///< Owned credential allocation.
};

}
