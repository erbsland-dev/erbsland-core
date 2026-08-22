// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UnsafeU8StringAccess_fwd.hpp"

#include "../u8/U8String.hpp"

#include <span>

namespace erbsland::text::impl {

/// Provides unsafe access to the internal read-only string data.
/// @warning Do not use this class in user code!
/// @tested{UnsafeU8StringEditorAccessTest PlatformStringAccessTest}
class UnsafeU8StringAccess {
public:
    /// Create an accessor
    explicit UnsafeU8StringAccess(const U8String &string) : _string{string} {}

    // defaults
    ~UnsafeU8StringAccess() = default;
    UnsafeU8StringAccess(const UnsafeU8StringEditorAccess &) = delete;
    UnsafeU8StringAccess(UnsafeU8StringAccess &&) = delete;
    auto operator=(const UnsafeU8StringAccess &) = delete;
    auto operator=(UnsafeU8StringAccess &&) = delete;

public:
    /// Access the bounded span for the string data.
    [[nodiscard]] auto dataSpan() const noexcept -> std::span<const char> { return _string.dataView().dataSpan(); }
    /// Access the internal data view.
    [[nodiscard]] auto dataView() const noexcept -> U8StringDataView { return _string.dataView(); }

private:
    const U8String &_string;
};

}
