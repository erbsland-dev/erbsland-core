// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../IntegerMath.hpp"

#include <compare>

namespace erbsland::math::impl {

/// This helper macro implements all comparison operators.
/// It requires that the type is based around an integer that can be compared with any other integer type.
/// Example:
/// <code>
/// class Foo {
/// public:
///     ERBSLAND_CORE_CONSTEXPR_MIXED_INTEGER_COMPARE(_value);
///     ERBSLAND_CORE_CONSTEXPR_MIXED_INTEGER_COMPARE_FRIEND(_value, const Foo &second, second._value);
/// private:
///     int _value;
/// };
/// </code>
/// @param MEMBER_VAR_NAME The name of the member variable that holds the integer value.
#define ERBSLAND_CORE_CONSTEXPR_MIXED_INTEGER_COMPARE(MEMBER_VAR_NAME)                                                 \
    template <::erbsland::math::NativeInteger T>                                                                       \
    constexpr auto operator<=>(const T other) const noexcept -> std::strong_ordering {                                 \
        return mixedIntegerCompare(MEMBER_VAR_NAME, other);                                                            \
    }                                                                                                                  \
    template <::erbsland::math::NativeInteger T>                                                                       \
    constexpr auto operator==(const T other) const noexcept -> bool {                                                  \
        return operator<=>(other) == std::strong_ordering::equal;                                                      \
    }                                                                                                                  \
    template <::erbsland::math::NativeInteger T>                                                                       \
    constexpr auto operator!=(const T other) const noexcept -> bool {                                                  \
        return operator<=>(other) != std::strong_ordering::equal;                                                      \
    }                                                                                                                  \
    template <::erbsland::math::NativeInteger T>                                                                       \
    constexpr auto operator<(const T other) const noexcept -> bool {                                                   \
        return operator<=>(other) == std::strong_ordering::less;                                                       \
    }                                                                                                                  \
    template <::erbsland::math::NativeInteger T>                                                                       \
    constexpr auto operator<=(const T other) const noexcept -> bool {                                                  \
        return operator<=>(other) != std::strong_ordering::greater;                                                    \
    }                                                                                                                  \
    template <::erbsland::math::NativeInteger T>                                                                       \
    constexpr auto operator>(const T other) const noexcept -> bool {                                                   \
        return operator<=>(other) == std::strong_ordering::greater;                                                    \
    }                                                                                                                  \
    template <::erbsland::math::NativeInteger T>                                                                       \
    constexpr auto operator>=(const T other) const noexcept -> bool {                                                  \
        return operator<=>(other) != std::strong_ordering::less;                                                       \
    }

/// This helper macro implements comparison operators for mixed types with a member variable holding an integer.
/// It requires that the type is based around an integer that can be compared with any other integer type.
/// Example:
/// <code>
/// class Foo {
/// public:
///     ERBSLAND_CORE_CONSTEXPR_MIXED_INTEGER_COMPARE(_value);
///     ERBSLAND_CORE_CONSTEXPR_MIXED_INTEGER_COMPARE_FRIEND(_value, const Foo &second, second._value);
/// private:
///     int _value;
/// };
/// </code>
/// @param MEMBER_VAR_NAME The name of the member variable that holds the integer value.
/// @param RIGHT_ARG The right-hand side argument for comparison. Do not use `first`,
///     it is used for the integer argument.
/// @param RIGHT_EXPR The expression to use for comparison with the right-hand side argument.
#define ERBSLAND_CORE_CONSTEXPR_MIXED_INTEGER_COMPARE_FRIEND(MEMBER_VAR_NAME, RIGHT_ARG, RIGHT_EXPR)                   \
    template <::erbsland::math::NativeInteger T>                                                                       \
    friend constexpr auto operator<=>(const T first, RIGHT_ARG) noexcept -> std::strong_ordering {                     \
        return mixedIntegerCompare(first, RIGHT_EXPR);                                                                 \
    }                                                                                                                  \
    template <::erbsland::math::NativeInteger T>                                                                       \
    friend constexpr auto operator==(const T first, RIGHT_ARG) noexcept -> bool {                                      \
        return mixedIntegerCompare(first, RIGHT_EXPR) == std::strong_ordering::equal;                                  \
    }                                                                                                                  \
    template <::erbsland::math::NativeInteger T>                                                                       \
    friend constexpr auto operator!=(const T first, RIGHT_ARG) noexcept -> bool {                                      \
        return mixedIntegerCompare(first, RIGHT_EXPR) != std::strong_ordering::equal;                                  \
    }                                                                                                                  \
    template <::erbsland::math::NativeInteger T>                                                                       \
    friend constexpr auto operator<(const T first, RIGHT_ARG) noexcept -> bool {                                       \
        return mixedIntegerCompare(first, RIGHT_EXPR) == std::strong_ordering::less;                                   \
    }                                                                                                                  \
    template <::erbsland::math::NativeInteger T>                                                                       \
    friend constexpr auto operator<=(const T first, RIGHT_ARG) noexcept -> bool {                                      \
        return mixedIntegerCompare(first, RIGHT_EXPR) != std::strong_ordering::greater;                                \
    }                                                                                                                  \
    template <::erbsland::math::NativeInteger T>                                                                       \
    friend constexpr auto operator>(const T first, RIGHT_ARG) noexcept -> bool {                                       \
        return mixedIntegerCompare(first, RIGHT_EXPR) == std::strong_ordering::greater;                                \
    }                                                                                                                  \
    template <::erbsland::math::NativeInteger T>                                                                       \
    friend constexpr auto operator>=(const T first, RIGHT_ARG) noexcept -> bool {                                      \
        return mixedIntegerCompare(first, RIGHT_EXPR) != std::strong_ordering::less;                                   \
    }

}
