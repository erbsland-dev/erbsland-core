// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../u16/U16String.hpp"

namespace erbsland::text::impl {

/// Provides unsafe access to the internal string data.
/// @warning Do not use this class in user code!
/// @tested{UnsafeU16StringAccessTest}
class UnsafeU16StringAccess {
public:
    /// Create an accessor
    explicit UnsafeU16StringAccess(const U16String &string) : _string{string} {}

    // defaults
    ~UnsafeU16StringAccess() = default;
    UnsafeU16StringAccess(const UnsafeU16StringAccess &) = delete;
    UnsafeU16StringAccess(UnsafeU16StringAccess &&) = delete;
    auto operator=(const UnsafeU16StringAccess &) = delete;
    auto operator=(UnsafeU16StringAccess &&) = delete;

public:
    [[nodiscard]] auto data() const noexcept -> const char16_t * { return _string._storage.data(); }

private:
    const U16String &_string;
};

}
