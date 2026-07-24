// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UnsafeU8StringAccess_fwd.hpp"

#include "../u8/U8String.hpp"

namespace erbsland::text::impl {

/// Provides unsafe access to the internal read-only string data.
/// @warning Do not use this class in user code!
/// @tested{OptionsParserTest}
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
    /// Access the null-terminated string data.
    [[nodiscard]] auto data() const noexcept -> mem::UnsafeConstCharPtr { return _string.dataView().data().data(); }
    /// Access the internal data view.
    [[nodiscard]] auto dataView() const noexcept -> U8StringDataView { return _string.dataView(); }

private:
    const U8String &_string;
};

}
