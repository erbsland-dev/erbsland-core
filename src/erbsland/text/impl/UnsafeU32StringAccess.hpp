// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UnsafeU32StringAccess_fwd.hpp"

#include "../u32/U32String.hpp"

namespace erbsland::text::impl {

/// Provides unsafe access to the internal UTF-32 string data.
/// @warning Do not use this class in user code!
/// @tested{RegExUtf16Utf32Test}
class UnsafeU32StringAccess {
public:
    /// Create an accessor.
    explicit UnsafeU32StringAccess(const U32String &string) noexcept : _string{string} {}

    // defaults
    ~UnsafeU32StringAccess() = default;
    UnsafeU32StringAccess(const UnsafeU32StringAccess &) = delete;
    UnsafeU32StringAccess(UnsafeU32StringAccess &&) = delete;
    auto operator=(const UnsafeU32StringAccess &) = delete;
    auto operator=(UnsafeU32StringAccess &&) = delete;

public:
    /// Access the internal data view.
    [[nodiscard]] auto dataView() const noexcept -> U32StringDataView { return _string.dataView(); }

private:
    const U32String &_string;
};

}
