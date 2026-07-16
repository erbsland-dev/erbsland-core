// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UnsafeU32StringViewAccess_fwd.hpp"

#include "../u32/U32StringView.hpp"

namespace erbsland::text::impl {

/// Provides unsafe access to the internal UTF-32 string data.
/// @warning Do not use this class in user code!
/// @tested{RegExUtf16Utf32Test}
class UnsafeU32StringViewAccess {
public:
    /// Create an accessor.
    explicit UnsafeU32StringViewAccess(const U32StringView &string) noexcept : _string{string} {}

    // defaults
    ~UnsafeU32StringViewAccess() = default;
    UnsafeU32StringViewAccess(const UnsafeU32StringViewAccess &) = delete;
    UnsafeU32StringViewAccess(UnsafeU32StringViewAccess &&) = delete;
    auto operator=(const UnsafeU32StringViewAccess &) = delete;
    auto operator=(UnsafeU32StringViewAccess &&) = delete;

public:
    /// Access the internal data view.
    [[nodiscard]] auto dataView() const noexcept -> U32StringDataView { return _string.dataView(); }

private:
    const U32StringView &_string;
};

}
