// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UnsafeU8StringLiteralAccess_fwd.hpp"

#include "../u8/U8StringLiteral.hpp"

#include <span>
#include <type_traits>

namespace erbsland::text::impl {

/// Provides unsafe access to the internal read-only string literal data.
/// @warning Do not use this class in user code! Only use if direct and raw data access is required.
/// @tested{UnsafeU8StringLiteralAccessTest}
template <typename tChar>
class UnsafeU8StringLiteralAccess {
public:
    /// Create an accessor.
    /// @param string The literal wrapper, which must outlive this accessor.
    explicit constexpr UnsafeU8StringLiteralAccess(const U8StringLiteral<tChar> &string) : _stringLiteral{string} {}

    // defaults
    ~UnsafeU8StringLiteralAccess() = default;
    UnsafeU8StringLiteralAccess(const UnsafeU8StringLiteralAccess &) = delete;
    UnsafeU8StringLiteralAccess(UnsafeU8StringLiteralAccess &&) = delete;
    auto operator=(const UnsafeU8StringLiteralAccess &) = delete;
    auto operator=(UnsafeU8StringLiteralAccess &&) = delete;

public: // accessors
    /// Constexpr access of the string literals data span.
    /// This is only possible for `U8StringLiteral<char>`, but not for `U8StringLiteral<char8_t>`.
    [[nodiscard]] constexpr auto constexprDataSpan() const noexcept -> std::span<const char>
        requires std::is_same_v<tChar, char>
    {
        return std::span<const char>{_stringLiteral._charPtr, _stringLiteral._size};
    }
    /// Access of the string literals data span.
    [[nodiscard]] auto dataSpan() const noexcept -> std::span<const char> {
        if constexpr (std::is_same_v<tChar, char8_t>) {
            return std::span{reinterpret_cast<const char *>(_stringLiteral._charPtr), _stringLiteral._size};
        } else {
            return constexprDataSpan();
        }
    }

private:
    const U8StringLiteral<tChar> &_stringLiteral; ///< The referenced literal wrapper.
};

}
