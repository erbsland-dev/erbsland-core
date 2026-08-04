// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#ifndef _WIN32
#error "This header is only available on Windows."
#endif

#include "../../../core/impl/WindowsApi.hpp"

#include <cstddef>
#include <span>

namespace erbsland::cryptology::impl {

/// Own one buffer allocated by a DPAPI-NG operation.
/// @tested{ProtectedByteBlockTest}
class WindowsLocalBuffer final {
public:
    /// Create an empty buffer guard.
    explicit WindowsLocalBuffer(const bool sensitive) noexcept : _sensitive{sensitive} {}
    /// Erase plaintext when requested and release native storage.
    ~WindowsLocalBuffer();

    // defaults/deletions
    WindowsLocalBuffer(const WindowsLocalBuffer &) = delete;
    WindowsLocalBuffer(WindowsLocalBuffer &&) = delete;
    auto operator=(const WindowsLocalBuffer &) -> WindowsLocalBuffer & = delete;
    auto operator=(WindowsLocalBuffer &&) -> WindowsLocalBuffer & = delete;

public: // native access
    /// Get the native output-buffer address for a DPAPI-NG call.
    [[nodiscard]] auto dataAddress() noexcept -> PBYTE * { return &_data; }
    /// Get the native output-size address for a DPAPI-NG call.
    [[nodiscard]] auto sizeAddress() noexcept -> ULONG * { return &_size; }
    /// View the owned native buffer as bytes.
    [[nodiscard]] auto span() const noexcept -> std::span<const std::byte> {
        return {reinterpret_cast<const std::byte *>(_data), _size};
    }

private:
    PBYTE _data{nullptr};   ///< Native allocation.
    ULONG _size{0U};        ///< Native allocation size.
    bool _sensitive{false}; ///< Whether to erase the allocation before release.
};

}
