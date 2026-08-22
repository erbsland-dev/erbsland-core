// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UnsafeU16StringEditorAccess_fwd.hpp"

#include "../u16/U16StringEditor.hpp"

#include <span>

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
    /// Access the bounded span for the string data.
    [[nodiscard]] auto dataSpan() const noexcept -> std::span<const char16_t> { return _string->dataView().dataSpan(); }
    /// Access the internal data view.
    [[nodiscard]] auto dataView() const noexcept -> U16StringDataView { return _string->dataView(); }

private: // using raw-pointers is approved for this class (te)
    const U16StringEditor *_string;
};

}
