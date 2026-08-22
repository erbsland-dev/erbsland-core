// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PlatformU8StringAccess_fwd.hpp"
#include "UnsafeU8StringAccess.hpp"

#include "../../mem/UnsafeCharPtr.hpp"

#include <cstddef>

namespace erbsland::text::impl {

/// Provides null-terminated UTF-8 string data for platform API boundaries.
/// A sliced string is materialized so the visible range always ends at the null terminator.
/// @warning Pointers returned by this class remain valid only while this accessor exists and is not modified.
/// @tested{PlatformStringAccessTest}
class PlatformU8StringAccess final {
public:
    /// Create platform access for a string.
    explicit PlatformU8StringAccess(const U8String &string) : _string{normalized(string)} {}

    // defaults
    ~PlatformU8StringAccess() = default;
    PlatformU8StringAccess(const PlatformU8StringAccess &) = default;
    PlatformU8StringAccess(PlatformU8StringAccess &&) noexcept = default;
    auto operator=(const PlatformU8StringAccess &) -> PlatformU8StringAccess & = default;
    auto operator=(PlatformU8StringAccess &&) noexcept -> PlatformU8StringAccess & = default;

public: // accessors
    /// Access the null-terminated character data.
    [[nodiscard]] auto nullTerminatedCharPtr() const noexcept -> mem::UnsafeConstCharPtr {
        const auto data = UnsafeU8StringAccess{_string}.dataSpan();
        return data.empty() ? &cEmptyString : data.data();
    }
    /// Get the data size including the trailing null character.
    [[nodiscard]] auto sizeIncludingNull() const noexcept -> std::size_t { return sizeWithoutNull() + 1U; }
    /// Get the data size without the trailing null character.
    [[nodiscard]] auto sizeWithoutNull() const noexcept -> std::size_t { return _string.length().toSizeT(); }

private:
    /// Share complete strings and materialize sliced strings.
    [[nodiscard]] static auto normalized(const U8String &string) -> U8String {
        return UnsafeU8StringAccess{string}.dataView().isSlice() ? string.copy() : string;
    }

private:
    static constexpr char cEmptyString = '\0';
    U8String _string; ///< The shared original string, or a compact copy if the original is sliced.
};

}
