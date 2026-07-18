// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UnsafeU16StringAccess_fwd.hpp"

#include "../u16/U16String.hpp"

#include "../../core/Definitions.hpp"

namespace erbsland::text::impl {

/// Provides unsafe access to the internal string data.
/// @warning Do not use this class in user code!
/// @tested{UnsafeU16StringEditorAccessTest}
class UnsafeU16StringAccess {
public:
    /// Create an accessor
    explicit UnsafeU16StringAccess(const U16String &string) noexcept : _string{string} {}

    // defaults
    ~UnsafeU16StringAccess() = default;
    UnsafeU16StringAccess(const UnsafeU16StringAccess &) = delete;
    UnsafeU16StringAccess(UnsafeU16StringAccess &&) = delete;
    auto operator=(const UnsafeU16StringAccess &) = delete;
    auto operator=(UnsafeU16StringAccess &&) = delete;

public:
    /// Access the null-terminated string data.
    [[nodiscard]] auto data() const noexcept -> const char16_t * { return _string.dataView().data().data(); }
#ifdef ERBSLAND_WCHAR_16BIT
    /// Access the null-terminated string data as wide string.
    [[nodiscard]] auto dataAsWide() const noexcept -> const wchar_t * {
        return reinterpret_cast<const wchar_t *>(data());
    }
#endif
    /// Access the internal data view.
    [[nodiscard]] auto dataView() const noexcept -> U16StringDataView { return _string.dataView(); }

private: // using raw-pointers is approved for this class (te)
    const U16String &_string;
};

}
