// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#ifndef _WIN32
#error "This header is only available on Windows."
#endif

#include "../../../core/impl/WindowsApi.hpp"

#include <ncrypt.h>
#include <ncryptprotect.h>

namespace erbsland::cryptology::impl {

/// Resolve the Windows DPAPI-NG entry points from the system Ncrypt library on demand.
/// The fixed system32 search scope prevents application-directory DLL substitution. The owning protected-data
/// provider retains this object until it has released every descriptor and native buffer.
/// @tested{ProtectedByteBlockTest}
class WindowsProtectedDataApi final {
private:
    using CreateProtectionDescriptorFn = decltype(&::NCryptCreateProtectionDescriptor);
    using CloseProtectionDescriptorFn = decltype(&::NCryptCloseProtectionDescriptor);
    using ProtectSecretFn = decltype(&::NCryptProtectSecret);
    using UnprotectSecretFn = decltype(&::NCryptUnprotectSecret);

public:
    /// Load Ncrypt.dll from the Windows system directory and resolve all required DPAPI-NG entry points.
    WindowsProtectedDataApi();
    /// Release the native library.
    ~WindowsProtectedDataApi();

    // defaults/deletions
    WindowsProtectedDataApi(const WindowsProtectedDataApi &) = delete;
    WindowsProtectedDataApi(WindowsProtectedDataApi &&) = delete;
    auto operator=(const WindowsProtectedDataApi &) -> WindowsProtectedDataApi & = delete;
    auto operator=(WindowsProtectedDataApi &&) -> WindowsProtectedDataApi & = delete;

public: // entry points
    /// Access NCryptCreateProtectionDescriptor.
    [[nodiscard]] auto createProtectionDescriptorFn() const noexcept { return _createProtectionDescriptorFn; }
    /// Access NCryptCloseProtectionDescriptor.
    [[nodiscard]] auto closeProtectionDescriptorFn() const noexcept { return _closeProtectionDescriptorFn; }
    /// Access NCryptProtectSecret.
    [[nodiscard]] auto protectSecretFn() const noexcept { return _protectSecretFn; }
    /// Access NCryptUnprotectSecret.
    [[nodiscard]] auto unprotectSecretFn() const noexcept { return _unprotectSecretFn; }

private:
    HMODULE _module{nullptr};                                     ///< Loaded system Ncrypt library.
    CreateProtectionDescriptorFn _createProtectionDescriptorFn{}; ///< Descriptor creation entry point.
    CloseProtectionDescriptorFn _closeProtectionDescriptorFn{};   ///< Descriptor release entry point.
    ProtectSecretFn _protectSecretFn{};                           ///< Protection entry point.
    UnprotectSecretFn _unprotectSecretFn{};                       ///< Unprotection entry point.
};

}
