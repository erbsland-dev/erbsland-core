// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#ifndef _WIN32
#error "This header is only available on Windows."
#endif

#include "ProtectedDataProvider.hpp"
#include "WindowsProtectedDataApi.hpp"

#include "../../../mem/ByteBlock.hpp"

#include <ncrypt.h>
#include <ncryptprotect.h>

#include <cstddef>

namespace erbsland::cryptology::impl {

/// Delegate protected data to Windows DPAPI-NG using a logon-local protection descriptor.
/// The provider follows the `NCryptProtectSecret` and `NCryptUnprotectSecret` API contract and adds an authenticated
/// application context identifier so a later process-local service rejects an older envelope.
/// Source: https://learn.microsoft.com/windows/win32/api/ncryptprotect/nf-ncryptprotect-ncryptprotectsecret
/// @tested{ProtectedByteBlockTest}
class WindowsProtectedDataProvider final : public ProtectedDataProvider {
public:
    /// Create a logon-local protection descriptor.
    WindowsProtectedDataProvider();
    /// Release the protection descriptor.
    ~WindowsProtectedDataProvider() override;

    // defaults/deletions
    WindowsProtectedDataProvider(const WindowsProtectedDataProvider &) = delete;
    WindowsProtectedDataProvider(WindowsProtectedDataProvider &&) = delete;
    auto operator=(const WindowsProtectedDataProvider &) -> WindowsProtectedDataProvider & = delete;
    auto operator=(WindowsProtectedDataProvider &&) -> WindowsProtectedDataProvider & = delete;

public: // implement ProtectedDataProvider
    [[nodiscard]] auto protect(mem::ConstByteSpan plaintext, unit::ByteLength plaintextLength)
        -> mem::ByteBlock override;
    [[nodiscard]] auto unprotect(mem::ConstByteSpan envelope, unit::ByteLength plaintextLength)
        -> mem::ByteBlock override;

private:
    static constexpr std::size_t cContextSize{32U}; ///< Application context identifier size.

    WindowsProtectedDataApi _api;                   ///< Lazily loaded DPAPI-NG system entry points.
    mem::ByteBlock _contextId;                      ///< Sensitive application-lifetime context identifier.
    NCRYPT_DESCRIPTOR_HANDLE _descriptor{nullptr};  ///< Logon-local DPAPI-NG descriptor.
};

}
