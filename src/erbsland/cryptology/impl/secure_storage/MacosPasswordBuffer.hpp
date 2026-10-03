#pragma once

#include "../../../mem/impl/SecureErase.hpp"

#include <Security/Security.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

namespace erbsland::cryptology::impl {

/// Own and erase plaintext returned by the standard user Keychain before native release.
/// @tested{SecureStorageNativeTest}
class MacosPasswordBuffer final {
public:
    /// Adopt one SecKeychainFindGenericPassword plaintext result.
    /// @param data Native password allocation, or null for an empty value.
    /// @param size Exact native allocation payload length.
    MacosPasswordBuffer(void *data, UInt32 size) noexcept : _data{data}, _size{size} {}
    /// Wipe the native plaintext and free it using its matching native allocator.
    ~MacosPasswordBuffer() {
        if (_data != nullptr) {
            mem::impl::secureErase(std::span{static_cast<std::byte *>(_data), static_cast<std::size_t>(_size)});
            SecKeychainItemFreeContent(nullptr, _data);
        }
    }

    // defaults/deletions
    MacosPasswordBuffer(const MacosPasswordBuffer &) = delete;
    auto operator=(const MacosPasswordBuffer &) -> MacosPasswordBuffer & = delete;

private:
    void *_data;  ///< Owned native plaintext.
    UInt32 _size; ///< Plaintext allocation extent.
};

}

#pragma clang diagnostic pop
