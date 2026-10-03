// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringTraits.hpp"

#include "../u16/U16String.hpp"
#include "../u32/U32String.hpp"
#include "../u8/U8String.hpp"

#include <concepts>
#include <type_traits>
#include <utility>

namespace erbsland::text::impl {

template <typename T>
concept StringJoinPart =
    AnyStringOrStringEditorType<T> || std::same_as<std::remove_cvref_t<T>, U8StringLiteral<char>> ||
    std::same_as<std::remove_cvref_t<T>, U8StringLiteral<char8_t>> ||
    std::same_as<std::remove_cvref_t<T>, U16StringLiteral> || std::same_as<std::remove_cvref_t<T>, U32StringLiteral>;

/// Select the read-only result encoding of a supported join input.
/// @tparam T A concrete Core string, editor, or literal.
template <StringJoinPart T>
using JoinedStringFor = std::conditional_t<
    std::convertible_to<const T &, U8String>,
    U8String,
    std::conditional_t<std::convertible_to<const T &, U16String>, U16String, U32String>>;

/// Join a fixed array using the existing checked concatenation implementation.
/// @tparam T The read-only string type.
/// @tparam tSize The positive number of parts.
/// @tparam tIndex The array indices.
/// @param parts The parts in output order.
/// @return The joined string with the sensitivity of all inputs.
/// @tested{StringJoinsTest}
template <AnyStringType T, std::size_t tSize, std::size_t... tIndex>
[[nodiscard]] auto joinStringArray(const T (&parts)[tSize], std::index_sequence<tIndex...>) -> T {
    if constexpr (tSize == 1) {
        return parts[0];
    } else {
        return T::fromJoined({parts[tIndex]...});
    }
}

}
