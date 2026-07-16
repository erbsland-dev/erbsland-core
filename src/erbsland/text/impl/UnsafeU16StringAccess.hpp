// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UnsafeU16StringAccess_fwd.hpp"

#include "../u16/U16String.hpp"

namespace erbsland::text::impl {

/// Provides unsafe access to the internal string data.
/// @warning Do not use this class in user code!
/// @tested{UnsafeU16StringAccessTest}
class UnsafeU16StringAccess {
public:
    /// Create an accessor
    explicit UnsafeU16StringAccess(const U16String &string) noexcept : _string{&string} {}

    // defaults
    ~UnsafeU16StringAccess() = default;
    UnsafeU16StringAccess(const UnsafeU16StringAccess &) = delete;
    UnsafeU16StringAccess(UnsafeU16StringAccess &&) = delete;
    auto operator=(const UnsafeU16StringAccess &) = delete;
    auto operator=(UnsafeU16StringAccess &&) = delete;

public:
    /// Access the null-terminated string data.
    [[nodiscard]] auto data() const noexcept -> const char16_t * { return _string->_storage.data(); }
#ifdef ERBSLAND_WCHAR_16BIT
    /// Access the null-terminated string data as wchar_t.
    [[nodiscard]] auto dataWide() const noexcept -> const wchar_t * {
        return reinterpret_cast<const wchar_t *>(_string->_storage.data());
    }
#endif

private: // using raw-pointers is approved for this class (te)
    const U16String *_string;
};

}
