// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PlatformU16StringAccess_fwd.hpp"
#include "UnsafeU16StringAccess.hpp"

#include "../../core/Definitions.hpp"

#include <cstddef>

namespace erbsland::text::impl {

/// Provides null-terminated UTF-16 string data for platform API boundaries.
/// A sliced string is materialized so the visible range always ends at the null terminator.
/// @warning Pointers returned by this class remain valid only while this accessor exists and is not modified.
/// @tested{PlatformStringAccessTest}
class PlatformU16StringAccess final {
public:
    /// Create platform access for a string.
    explicit PlatformU16StringAccess(const U16String &string) : _string{normalized(string)} {}

    // defaults
    ~PlatformU16StringAccess() = default;
    PlatformU16StringAccess(const PlatformU16StringAccess &) = default;
    PlatformU16StringAccess(PlatformU16StringAccess &&) noexcept = default;
    auto operator=(const PlatformU16StringAccess &) -> PlatformU16StringAccess & = default;
    auto operator=(PlatformU16StringAccess &&) noexcept -> PlatformU16StringAccess & = default;

public: // accessors
    /// Access the null-terminated UTF-16 character data.
    [[nodiscard]] auto nullTerminatedCharPtr() const noexcept -> const char16_t * {
        const auto data = UnsafeU16StringAccess{_string}.dataSpan();
        return data.empty() ? &cEmptyString : data.data();
    }
#ifdef ERBSLAND_WCHAR_16BIT
    /// Access the null-terminated UTF-16 data as wide characters.
    [[nodiscard]] auto nullTerminatedWideCharPtr() const noexcept -> const wchar_t * {
        return reinterpret_cast<const wchar_t *>(nullTerminatedCharPtr());
    }
#endif
    /// Get the data size including the trailing null character.
    [[nodiscard]] auto sizeIncludingNull() const noexcept -> std::size_t { return sizeWithoutNull() + 1U; }
    /// Get the data size without the trailing null character.
    [[nodiscard]] auto sizeWithoutNull() const noexcept -> std::size_t { return _string.length().toSizeT(); }

private:
    /// Share complete strings and materialize sliced strings.
    [[nodiscard]] static auto normalized(const U16String &string) -> U16String {
        return UnsafeU16StringAccess{string}.dataView().isSlice() ? string.copy() : string;
    }

private:
    static constexpr char16_t cEmptyString = u'\0';
    U16String _string; ///< The shared original string, or a compact copy if the original is sliced.
};

}
