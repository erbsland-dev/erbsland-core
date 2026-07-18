// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UnsafeU16StringEditorAccess_fwd.hpp"

#include "../u16/U16StringEditor.hpp"

namespace erbsland::text::impl {

/// Provides unsafe access to the internal string data.
/// @warning Do not use this class in user code!
/// @tested{UnsafeU16StringEditorAccessTest}
class UnsafeU16StringEditorAccess {
public:
    /// Create an accessor
    explicit UnsafeU16StringEditorAccess(const U16StringEditor &string) noexcept : _string{&string} {}

    // defaults
    ~UnsafeU16StringEditorAccess() = default;
    UnsafeU16StringEditorAccess(const UnsafeU16StringEditorAccess &) = delete;
    UnsafeU16StringEditorAccess(UnsafeU16StringEditorAccess &&) = delete;
    auto operator=(const UnsafeU16StringEditorAccess &) = delete;
    auto operator=(UnsafeU16StringEditorAccess &&) = delete;

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
    const U16StringEditor *_string;
};

}
