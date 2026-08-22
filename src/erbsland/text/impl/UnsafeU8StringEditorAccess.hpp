// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UnsafeU8StringEditorAccess_fwd.hpp"

#include "../u8/U8StringEditor.hpp"

#include <span>

namespace erbsland::text::impl {

/// Provides unsafe access to the internal string data.
/// @warning Do not use this class in user code!
/// @tested{UnsafeU8StringEditorAccessTest}
class UnsafeU8StringEditorAccess {
public:
    /// Create an accessor
    explicit UnsafeU8StringEditorAccess(const U8StringEditor &string) noexcept : _string{&string} {}

    // defaults
    ~UnsafeU8StringEditorAccess() = default;
    UnsafeU8StringEditorAccess(const UnsafeU8StringEditorAccess &) = delete;
    UnsafeU8StringEditorAccess(UnsafeU8StringEditorAccess &&) = delete;
    auto operator=(const UnsafeU8StringEditorAccess &) = delete;
    auto operator=(UnsafeU8StringEditorAccess &&) = delete;

public:
    /// Access the bounded span for the string data.
    [[nodiscard]] auto dataSpan() const noexcept -> std::span<const char> { return _string->dataView().dataSpan(); }
    /// Access the internal data view.
    [[nodiscard]] auto dataView() const noexcept -> U8StringDataView { return _string->dataView(); }

private: // using raw-pointers is approved for this class (te)
    const U8StringEditor *_string;
};

}
