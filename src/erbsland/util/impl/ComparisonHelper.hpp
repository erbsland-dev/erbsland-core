// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <compare>

namespace erbsland::util::impl {

/// Add a static comparison block comparing member variables.
///
/// This macro provides a convenient way to define all comparison operators for a class based on a member variable.
/// @param LEFT_MEMBER The member variable to compare.
/// @param RIGHT_ARG The right-hand argument for the comparison.
/// @param RIGHT_EXPR The expression to compare with the member variable.
/// This macro provides a convenient way to define all comparison operators for a class based on a member variable.
/// The simplest usage is to compare a member variable with another instance of the same class.
/// Example:
/// <code>
/// class Foo {
/// public:
///     ERBSLAND_CORE_COMPARE_MEMBER(_value, const Foo &other, other._value);
/// private:
///     int _value;
/// };
/// </code>
/// @param LEFT_MEMBER The member variable to compare.
/// @param RIGHT_ARG The right-hand argument for the comparison.
/// @param RIGHT_EXPR The expression to compare with the member variable.
#define ERBSLAND_CORE_COMPARE_MEMBER(LEFT_MEMBER, RIGHT_ARG, RIGHT_EXPR)                                               \
    auto operator<=>(RIGHT_ARG) const noexcept -> std::strong_ordering {                                               \
        return (LEFT_MEMBER) <=> (RIGHT_EXPR);                                                                         \
    }                                                                                                                  \
    auto operator==(RIGHT_ARG) const noexcept -> bool {                                                                \
        return (LEFT_MEMBER) == (RIGHT_EXPR);                                                                          \
    }                                                                                                                  \
    auto operator!=(RIGHT_ARG) const noexcept -> bool {                                                                \
        return (LEFT_MEMBER) != (RIGHT_EXPR);                                                                          \
    }                                                                                                                  \
    auto operator<(RIGHT_ARG) const noexcept -> bool {                                                                 \
        return (LEFT_MEMBER) < (RIGHT_EXPR);                                                                           \
    }                                                                                                                  \
    auto operator<=(RIGHT_ARG) const noexcept -> bool {                                                                \
        return (LEFT_MEMBER) <= (RIGHT_EXPR);                                                                          \
    }                                                                                                                  \
    auto operator>(RIGHT_ARG) const noexcept -> bool {                                                                 \
        return (LEFT_MEMBER) > (RIGHT_EXPR);                                                                           \
    }                                                                                                                  \
    auto operator>=(RIGHT_ARG) const noexcept -> bool {                                                                \
        return (LEFT_MEMBER) >= (RIGHT_EXPR);                                                                          \
    }

/// Add a *constexpr* static comparison block comparing member variables.
///
/// Works like `ERBSLAND_CORE_COMPARE_MEMBER` but creates `constexpr` functions.
/// @param LEFT_MEMBER The member variable to compare.
/// @param RIGHT_ARG The right-hand argument for the comparison.
/// @param RIGHT_EXPR The expression to compare with the member variable.
/// Works like `ERBSLAND_CORE_COMPARE_MEMBER` but creates `constexpr` functions.
/// Example:
/// <code>
/// class Foo {
/// public:
///     ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, int other, other);
///     ERBSLAND_CORE_CONSTEXPR_COMPARE_FRIEND(int a, const Foo &b, a, b._value);
/// private:
///     int _value;
/// };
/// </code>
/// Please note that while showing `int` in this example, comparing integer values should be done with caution.
/// @param LEFT_MEMBER The member variable to compare.
/// @param RIGHT_ARG The right-hand argument for the comparison.
/// @param RIGHT_EXPR The expression to compare with the member variable.
#define ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(LEFT_MEMBER, RIGHT_ARG, RIGHT_EXPR)                                     \
    constexpr auto operator<=>(RIGHT_ARG) const noexcept -> std::strong_ordering {                                     \
        return (LEFT_MEMBER) <=> (RIGHT_EXPR);                                                                         \
    }                                                                                                                  \
    constexpr auto operator==(RIGHT_ARG) const noexcept -> bool {                                                      \
        return (LEFT_MEMBER) == (RIGHT_EXPR);                                                                          \
    }                                                                                                                  \
    constexpr auto operator!=(RIGHT_ARG) const noexcept -> bool {                                                      \
        return (LEFT_MEMBER) != (RIGHT_EXPR);                                                                          \
    }                                                                                                                  \
    constexpr auto operator<(RIGHT_ARG) const noexcept -> bool {                                                       \
        return (LEFT_MEMBER) < (RIGHT_EXPR);                                                                           \
    }                                                                                                                  \
    constexpr auto operator<=(RIGHT_ARG) const noexcept -> bool {                                                      \
        return (LEFT_MEMBER) <= (RIGHT_EXPR);                                                                          \
    }                                                                                                                  \
    constexpr auto operator>(RIGHT_ARG) const noexcept -> bool {                                                       \
        return (LEFT_MEMBER) > (RIGHT_EXPR);                                                                           \
    }                                                                                                                  \
    constexpr auto operator>=(RIGHT_ARG) const noexcept -> bool {                                                      \
        return (LEFT_MEMBER) >= (RIGHT_EXPR);                                                                          \
    }

/// Derive regular comparison operators from `operator<=>`.
/// Example:
/// <code>
/// class Foo {
/// public:
///     auto operator<=>(const Foo &other) const noexcept -> std::strong_ordering;
///     ERBSLAND_CORE_COMPARE_FROM_SPACESHIP(const Foo &other, other);
/// };
/// </code>
/// @param RIGHT_ARG The right-hand argument for the comparison.
/// @param RIGHT_EXPR The expression to pass to `operator<=>`.
#define ERBSLAND_CORE_COMPARE_FROM_SPACESHIP(RIGHT_ARG, RIGHT_EXPR)                                                    \
    auto operator==(RIGHT_ARG) const noexcept -> bool {                                                                \
        return operator<=>(RIGHT_EXPR) == std::strong_ordering::equal;                                                 \
    }                                                                                                                  \
    auto operator!=(RIGHT_ARG) const noexcept -> bool {                                                                \
        return operator<=>(RIGHT_EXPR) != std::strong_ordering::equal;                                                 \
    }                                                                                                                  \
    auto operator<(RIGHT_ARG) const noexcept -> bool {                                                                 \
        return operator<=>(RIGHT_EXPR) == std::strong_ordering::less;                                                  \
    }                                                                                                                  \
    auto operator<=(RIGHT_ARG) const noexcept -> bool {                                                                \
        return operator<=>(RIGHT_EXPR) != std::strong_ordering::greater;                                               \
    }                                                                                                                  \
    auto operator>(RIGHT_ARG) const noexcept -> bool {                                                                 \
        return operator<=>(RIGHT_EXPR) == std::strong_ordering::greater;                                               \
    }                                                                                                                  \
    auto operator>=(RIGHT_ARG) const noexcept -> bool {                                                                \
        return operator<=>(RIGHT_EXPR) != std::strong_ordering::less;                                                  \
    }

/// Derive regular comparison operators from a `constexpr operator<=>`.
/// Works like `ERBSLAND_CORE_COMPARE_FROM_SPACESHIP` but creates `constexpr` functions.
/// @param RIGHT_ARG The right-hand argument for the comparison.
/// @param RIGHT_EXPR The expression to pass to `operator<=>`.
#define ERBSLAND_CORE_CONSTEXPR_COMPARE_FROM_SPACESHIP(RIGHT_ARG, RIGHT_EXPR)                                          \
    constexpr auto operator==(RIGHT_ARG) const noexcept -> bool {                                                      \
        return operator<=>(RIGHT_EXPR) == std::strong_ordering::equal;                                                 \
    }                                                                                                                  \
    constexpr auto operator!=(RIGHT_ARG) const noexcept -> bool {                                                      \
        return operator<=>(RIGHT_EXPR) != std::strong_ordering::equal;                                                 \
    }                                                                                                                  \
    constexpr auto operator<(RIGHT_ARG) const noexcept -> bool {                                                       \
        return operator<=>(RIGHT_EXPR) == std::strong_ordering::less;                                                  \
    }                                                                                                                  \
    constexpr auto operator<=(RIGHT_ARG) const noexcept -> bool {                                                      \
        return operator<=>(RIGHT_EXPR) != std::strong_ordering::greater;                                               \
    }                                                                                                                  \
    constexpr auto operator>(RIGHT_ARG) const noexcept -> bool {                                                       \
        return operator<=>(RIGHT_EXPR) == std::strong_ordering::greater;                                               \
    }                                                                                                                  \
    constexpr auto operator>=(RIGHT_ARG) const noexcept -> bool {                                                      \
        return operator<=>(RIGHT_EXPR) != std::strong_ordering::less;                                                  \
    }

/// Add a *constexpr* static comparison block with all friend comparison functions.
///
/// The most common use is shown the following example.
/// @param LEFT_ARG The parameter declaration for the left parameter.
/// @param RIGHT_ARG The parameter declaration for the right parameter.
/// @param LEFT_EXPR The let expression in the comparison.
/// @param RIGHT_EXPR The right expression in the comparison.
/// The most common use is shown the following example.
/// Example:
/// <code>
/// class Foo {
/// public:
///     ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, int other, other);
///     ERBSLAND_CORE_CONSTEXPR_COMPARE_FRIEND(int a, const Foo &b, a, b._value);
/// private:
///     int _value;
/// };
/// </code>
/// Please note that while showing `int` in this example, comparing integer values should be done with caution.
/// @param LEFT_ARG The parameter declaration for the left parameter.
/// @param RIGHT_ARG The parameter declaration for the right parameter.
/// @param LEFT_EXPR The let expression in the comparison.
/// @param RIGHT_EXPR The right expression in the comparison.
#define ERBSLAND_CORE_CONSTEXPR_COMPARE_FRIEND(LEFT_ARG, RIGHT_ARG, LEFT_EXPR, RIGHT_EXPR)                             \
    friend constexpr auto operator<=>(LEFT_ARG, RIGHT_ARG) noexcept -> std::strong_ordering {                          \
        return (LEFT_EXPR) <=> (RIGHT_EXPR);                                                                           \
    }                                                                                                                  \
    friend constexpr auto operator==(LEFT_ARG, RIGHT_ARG) noexcept -> bool {                                           \
        return (LEFT_EXPR) == (RIGHT_EXPR);                                                                            \
    }                                                                                                                  \
    friend constexpr auto operator!=(LEFT_ARG, RIGHT_ARG) noexcept -> bool {                                           \
        return (LEFT_EXPR) != (RIGHT_EXPR);                                                                            \
    }                                                                                                                  \
    friend constexpr auto operator<(LEFT_ARG, RIGHT_ARG) noexcept -> bool {                                            \
        return (LEFT_EXPR) < (RIGHT_EXPR);                                                                             \
    }                                                                                                                  \
    friend constexpr auto operator<=(LEFT_ARG, RIGHT_ARG) noexcept -> bool {                                           \
        return (LEFT_EXPR) <= (RIGHT_EXPR);                                                                            \
    }                                                                                                                  \
    friend constexpr auto operator>(LEFT_ARG, RIGHT_ARG) noexcept -> bool {                                            \
        return (LEFT_EXPR) > (RIGHT_EXPR);                                                                             \
    }                                                                                                                  \
    friend constexpr auto operator>=(LEFT_ARG, RIGHT_ARG) noexcept -> bool {                                           \
        return (LEFT_EXPR) >= (RIGHT_EXPR);                                                                            \
    }

}
