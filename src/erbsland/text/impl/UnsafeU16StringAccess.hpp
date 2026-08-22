// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UnsafeU16StringAccess_fwd.hpp"

#include "../u16/U16String.hpp"

#include <span>

namespace erbsland::text::impl {

/// Provides unsafe access to the internal string data.
/// @warning Do not use this class in user code!
/// @tested{UnsafeU16StringEditorAccessTest PlatformStringAccessTest}
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
    /// Access the bounded span for the string data.
    [[nodiscard]] auto dataSpan() const noexcept -> std::span<const char16_t> { return _string.dataView().dataSpan(); }
    /// Access the internal data view.
    [[nodiscard]] auto dataView() const noexcept -> U16StringDataView { return _string.dataView(); }

private: // using raw-pointers is approved for this class (te)
    const U16String &_string;
};

}
