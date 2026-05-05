// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "u16/impl/U16StringLiteralFactory.hpp"
#include "u16/U16String_fwd.hpp"
#include "u16/U16StringLiteral.hpp"
#include "u16/U16StringView_fwd.hpp"
#include "u32/impl/U32StringLiteralFactory.hpp"
#include "u32/U32String_fwd.hpp"
#include "u32/U32StringLiteral.hpp"
#include "u32/U32StringView_fwd.hpp"
#include "u8/impl/U8StringLiteralFactory.hpp"
#include "u8/U8String_fwd.hpp"
#include "u8/U8StringLiteral.hpp"
#include "u8/U8StringView_fwd.hpp"

#include "../mem/UnsafeCharPtr.hpp"

#include <cstddef>

namespace erbsland::text::literals {

// The main `_el` string literals:

/// Create a U8StringLiteral from a string literal.
/// @tested{U8StringLiteralTest}
constexpr auto operator""_el(const char *data, const std::size_t size) noexcept -> U8StringLiteral<char> {
    return impl::createU8StringLiteral(data, size);
}
/// Create a U8StringLiteral from a string literal.
/// @tested{U8StringLiteralTest}
constexpr auto operator""_el(const char8_t *data, const std::size_t size) noexcept -> U8StringLiteral<char8_t> {
    return impl::createU8StringLiteral(data, size);
}
/// Create a U16StringLiteral from a UTF-16 string literal.
/// @tested{U16StringLiteralTest}
constexpr auto operator""_el(const char16_t *data, const std::size_t size) noexcept -> U16StringLiteral {
    return impl::createU16StringLiteral(data, size);
}
/// Create a U32StringLiteral from a UTF-32 string literal.
/// @tested{U32StringLiteralTest}
constexpr auto operator""_el(const char32_t *data, const std::size_t size) noexcept -> U32StringLiteral {
    return impl::createU32StringLiteral(data, size);
}

// Rarely used `_elv` to construct a string view from a copy of the string literal:

/// Create a U8StringView from a string literal.
/// @note The `_elv` suffix is provided for completeness, for most use cases, prefer `_el`.
/// @tested{U8StringLiteralTest}
auto operator""_elv(const char *data, std::size_t size) noexcept -> U8StringView;
/// Create a U8StringView from a string literal.
/// @note The `_elv` suffix is provided for completeness, for most use cases, prefer `_el`.
/// @tested{U8StringLiteralTest}
auto operator""_elv(const char8_t *data, std::size_t size) noexcept -> U8StringView;
/// Create a U16StringView from a UTF-16 string literal.
/// @note The `_elv` suffix is provided for completeness, for most use cases, prefer `_el`.
/// @tested{U16StringLiteralTest}
auto operator""_elv(const char16_t *data, std::size_t size) noexcept -> U16StringView;
/// Create a U32StringView from a UTF-32 string literal.
/// @tested{U32StringLiteralTest}
auto operator""_elv(const char32_t *data, std::size_t size) noexcept -> U32StringView;

// Rarely used `_els` to construct a string from a copy of the string literal:

/// Create a U8String from a string literal.
/// @note The `_els` suffix is provided for completeness, for most use cases, prefer `_el`.
/// @tested{U8StringLiteralTest}
auto operator""_els(const char *data, std::size_t size) -> U8String;
/// Create a U8String from a string literal.
/// @note The `_els` suffix is provided for completeness, for most use cases, prefer `_el`.
/// @tested{U8StringLiteralTest}
auto operator""_els(const char8_t *data, std::size_t size) -> U8String;
/// Create a U16String from a UTF-16 string literal.
/// @note The `_els` suffix is provided for completeness, for most use cases, prefer `_el`.
/// @tested{U16StringLiteralTest}
auto operator""_els(const char16_t *data, std::size_t size) -> U16String;
/// Create a U32String from a UTF-32 string literal.
/// @tested{U32StringLiteralTest}
auto operator""_els(const char32_t *data, std::size_t size) -> U32String;

}
