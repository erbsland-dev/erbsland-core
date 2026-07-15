// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../u8/U8StringView.hpp"

namespace erbsland::text::impl {

/// Provides unsafe access to the internal string view data.
/// @warning Do not use this class in user code!
/// @notest{implicitly tested via U8String}
class UnsafeU8StringViewAccess {
public:
    /// Create an accessor
    explicit UnsafeU8StringViewAccess(const U8StringView &string) : _string{string} {}

    // defaults
    ~UnsafeU8StringViewAccess() = default;
    UnsafeU8StringViewAccess(const UnsafeU8StringAccess &) = delete;
    UnsafeU8StringViewAccess(UnsafeU8StringViewAccess &&) = delete;
    auto operator=(const UnsafeU8StringViewAccess &) = delete;
    auto operator=(UnsafeU8StringViewAccess &&) = delete;

public:
    /// Access the null-terminated string data.
    [[nodiscard]] auto data() const noexcept -> mem::UnsafeConstCharPtr { return _string.dataView().data().data(); }
    /// Access the internal data view.
    [[nodiscard]] auto dataView() const noexcept -> U8StringDataView { return _string.dataView(); }

private:
    const U8StringView &_string;
};

}
