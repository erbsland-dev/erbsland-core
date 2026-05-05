// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "FormatAsDefaults.hpp"

#include "../FormatArgument.hpp"

#include <array>
#include <concepts>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace erbsland::text::impl {

template <typename>
inline constexpr auto cAlwaysFalse = false;

template <template <typename> typename tFormatAs, typename T>
concept HasFormatAs = requires(const T &value) {
    typename tFormatAs<T>::Argument;
    { tFormatAs<T>{}.format(value) } -> std::convertible_to<typename tFormatAs<T>::Argument>;
};

template <typename T>
[[nodiscard]] consteval auto formatAsMatchCount() -> std::size_t {
    auto result = std::size_t{0};
    if constexpr (HasFormatAs<FormatAsInt64, T>) {
        result += 1U;
    }
    if constexpr (HasFormatAs<FormatAsUInt64, T>) {
        result += 1U;
    }
    if constexpr (HasFormatAs<FormatAsDouble, T>) {
        result += 1U;
    }
    if constexpr (HasFormatAs<FormatAsBool, T>) {
        result += 1U;
    }
    if constexpr (HasFormatAs<FormatAsChar, T>) {
        result += 1U;
    }
    if constexpr (HasFormatAs<FormatAsText, T>) {
        result += 1U;
    }
    if constexpr (HasFormatAs<FormatAsU8Text, T>) {
        result += 1U;
    }
    if constexpr (HasFormatAs<FormatAsU16Text, T>) {
        result += 1U;
    }
    if constexpr (HasFormatAs<FormatAsU32Text, T>) {
        result += 1U;
    }
    return result;
}

template <typename T>
[[nodiscard]] auto makeFormatArgument(const T &value) -> FormatArgument {
    using Value = std::remove_cvref_t<T>;
    constexpr auto matchCount = formatAsMatchCount<Value>();
    if constexpr (matchCount == 0U) {
        static_assert(
            cAlwaysFalse<Value>, "Unsupported format argument type. Add exactly one FormatAs* specialization.");
    } else if constexpr (matchCount > 1U) {
        static_assert(
            cAlwaysFalse<Value>, "Ambiguous format argument type. Only one FormatAs* specialization may match.");
    } else if constexpr (HasFormatAs<FormatAsInt64, Value>) {
        return FormatArgument{FormatAsInt64<Value>{}.format(value)};
    } else if constexpr (HasFormatAs<FormatAsUInt64, Value>) {
        return FormatArgument{FormatAsUInt64<Value>{}.format(value)};
    } else if constexpr (HasFormatAs<FormatAsDouble, Value>) {
        return FormatArgument{FormatAsDouble<Value>{}.format(value)};
    } else if constexpr (HasFormatAs<FormatAsBool, Value>) {
        return FormatArgument{FormatAsBool<Value>{}.format(value)};
    } else if constexpr (HasFormatAs<FormatAsChar, Value>) {
        return FormatArgument{FormatAsChar<Value>{}.format(value)};
    } else if constexpr (HasFormatAs<FormatAsText, Value>) {
        return FormatArgument{FormatAsText<Value>{}.format(value)};
    } else if constexpr (HasFormatAs<FormatAsU8Text, Value>) {
        return FormatArgument{FormatAsU8Text<Value>{}.format(value)};
    } else if constexpr (HasFormatAs<FormatAsU16Text, Value>) {
        return FormatArgument{FormatAsU16Text<Value>{}.format(value)};
    } else if constexpr (HasFormatAs<FormatAsU32Text, Value>) {
        return FormatArgument{FormatAsU32Text<Value>{}.format(value)};
    }
}

/// Convert all user arguments into runtime format arguments.
/// @tested{U8FormatTest}
template <typename... Args>
[[nodiscard]] auto makeFormatArguments(Args &&...args) -> std::array<FormatArgument, sizeof...(Args)> {
    return {makeFormatArgument(std::forward<Args>(args))...};
}

}
