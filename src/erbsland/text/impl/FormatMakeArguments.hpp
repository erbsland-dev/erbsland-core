// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "FormatDefaultArgument.hpp"

#include <array>
#include <concepts>
#include <type_traits>
#include <utility>

namespace erbsland::text::impl {

template <typename T>
concept HasFormatAs = requires(const T &value) {
    { FormatAs<T>{}.format(value) } -> std::convertible_to<String>;
};

template <typename T>
concept HasFormatText = requires(const T &value) {
    { value.toString() } -> std::same_as<String>;
};

/// Convert one user value into its runtime format argument.
template <typename T>
[[nodiscard]] auto makeFormatArgument(const T &value) -> FormatArgument {
    using Value = std::remove_cvref_t<T>;
    if constexpr (HasFormatAs<Value>) {
        return FormatArgument{FormatAs<Value>{}.format(value)};
    } else if constexpr (HasFormatText<Value>) {
        return FormatArgument{value.toString()};
    } else {
        return makeDefaultFormatArgument(value);
    }
}

/// Convert all user arguments into runtime format arguments.
/// @tested{U8FormatTest}
template <typename... Args>
[[nodiscard]] auto makeFormatArguments(Args &&...args) -> std::array<FormatArgument, sizeof...(Args)> {
    return {makeFormatArgument(std::forward<Args>(args))...};
}

}
