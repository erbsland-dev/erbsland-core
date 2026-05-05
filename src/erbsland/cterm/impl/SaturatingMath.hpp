// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <concepts>
#include <cstdint>
#include <exception>
#include <limits>
#include <type_traits>

#ifdef _MSC_VER
#include <safeint.h>
#endif

// Note: These methods are copied from the Erbsland Core framework.
// They are well-tested and proven to work correctly on all supported platforms.

namespace erbsland::cterm::impl {

/// Check if A and B are int values
/// @tparam A The first type to test.
/// @tparam B The second type to test.
template <typename A, typename B>
struct AreBothIntegral {
    static constexpr bool value = std::is_integral_v<A> && std::is_integral_v<B>;
};
/// @see AreBothIntegral
template <typename A, typename B>
constexpr bool AreBothIntegralV = AreBothIntegral<A, B>::value;

/// Check if both types are integer and are both either signed or unsigned.
/// - signed, signed => true
/// - unsigned, unsigned => true
/// - signed, unsigned => false
/// - unsigned, signed => false
/// - ???, ??? => false
/// @tparam A The first type to test.
/// @tparam B The second type to test.
template <typename A, typename B>
struct AreSignCompatibleInt {
    static constexpr bool value = (AreBothIntegralV<A, B> && (std::is_signed_v<A> == std::is_signed_v<B>));
};

/// @see AreSignCompatibleInt
template <typename A, typename B>
constexpr bool AreSignCompatibleIntV = AreSignCompatibleInt<A, B>::value;

/// Get the larger integer of both.
/// @tparam A The first integer type.
/// @tparam B The second integer type.
template <std::integral A, std::integral B>
struct GetLargerInt {
    using type = std::conditional_t<sizeof(A) >= sizeof(B), A, B>;
};
/// @see GetLargerInt
template <std::integral A, std::integral B>
using GetLargerIntT = typename GetLargerInt<A, B>::type;

/// Check if the second type can hold larger positive values.
/// @tparam A The first type.
/// @tparam B The second type.
template <std::integral A, std::integral B>
struct HasLargerAbsoluteInt {
    static constexpr bool value =
        (sizeof(A) == sizeof(B) && std::is_signed_v<A> && std::is_unsigned_v<B>) || (sizeof(B) > sizeof(A));
};
/// @see HasLargerAbsoluteInt
template <std::integral A, std::integral B>
constexpr bool HasLargerAbsoluteIntV = HasLargerAbsoluteInt<A, B>::value;

/// Get a common integer for an operation between type A and B.
/// This template provides a convenient way to find a safe common integer type for an integer operation.
/// Compared with `std::common_type` it does not allow mixing of signed and unsigned integers.
/// Compared with `std::common_type` it handles `uint16_t` as 16bit values if the implementation allows it.
/// The result is `type`, which is only a valid integer if both type A and B are integer and are compatible
/// for a safe operation. See `IntegerMath.hpp` for mixed operations and comparisons.
/// @tparam A The first integer type.
/// @tparam B The second integer type.
template <std::integral A, std::integral B>
struct CommonInt {
    using type = std::conditional_t<AreSignCompatibleIntV<A, B>, std::remove_const_t<GetLargerIntT<A, B>>, void>;
};

/// @see CommonInt
template <std::integral A, std::integral B>
using CommonIntT = typename CommonInt<A, B>::type;

/// Test if an integer value is negative.
/// This helper is safe to use with unsigned integers, where it always returns `false`.
/// @tparam T Any signed or unsigned integer type.
/// @param value The value to test.
/// @return `true` if the value is signed and below zero.
template <std::integral T>
constexpr auto isNegative(T value) noexcept -> bool {
    if constexpr (std::is_signed_v<T>) {
        return value < T{0};
    } else {
        return false;
    }
}

/// Get the absolute value of an integer as an unsigned integer.
/// This helper avoids undefined behavior for the minimum value of signed integer types.
/// @tparam T Any signed or unsigned integer type.
/// @param value The value to convert.
/// @return The absolute value represented as the matching unsigned integer type.
template <std::integral T>
constexpr auto unsignedIntAbs(T value) noexcept -> std::make_unsigned_t<T> {
    using UnsignedType = std::make_unsigned_t<T>;
    if constexpr (std::is_signed_v<T>) {
        return (value < T{0}) ? (UnsignedType{0} - static_cast<UnsignedType>(value)) : static_cast<UnsignedType>(value);
    } else {
        return value;
    }
}

/// Get the saturated extreme value for a multiplication overflow.
/// The returned value is the minimum of `A` for a negative signed product, and the maximum of `A` otherwise.
/// @tparam A The target integer type.
/// @tparam B The second operand integer type.
/// @param a The first multiplication operand.
/// @param b The second multiplication operand.
/// @return The saturated extreme value for the product sign.
template <std::integral A, std::integral B>
constexpr auto extremeIntFromMultiplication(A a, B b) noexcept -> A {
    if constexpr (std::is_signed_v<A>) {
        return (isNegative(a) != isNegative(b)) ? std::numeric_limits<A>::min() : std::numeric_limits<A>::max();
    } else {
        return std::numeric_limits<A>::max();
    }
}

/// Add two integers but limit the result to the maximum possible values.
/// @tparam T Any signed or unsigned integer type
/// @param a The first value.
/// @param b The second value.
/// @return The addition result, limited to the used type.
template <std::integral T>
auto saturatingAdd(T a, T b) noexcept -> T {
    // Use reliable built-in functions or stop compilation.
#if __GNUC__ || __clang__
#if __has_builtin(__builtin_add_overflow)
    T result;
    if (__builtin_add_overflow(a, b, &result)) {
        if constexpr (std::is_signed_v<T>) {
            return (a < 0) ? std::numeric_limits<T>::min() : std::numeric_limits<T>::max();
        } else {
            return std::numeric_limits<T>::max();
        }
    }
    return result;
#else
#error "Please use a current version of the GCC or clang compiler."
#endif
#elif _MSC_VER
    T result;
    if (!msl::utilities::SafeAdd(a, b, result)) {
        if constexpr (std::is_signed_v<T>) {
            return (a < 0) ? std::numeric_limits<T>::min() : std::numeric_limits<T>::max();
        } else {
            return std::numeric_limits<T>::max();
        }
    }
    return result;
#else
#error "Missing saturating add implementation for the compiler you use."
#endif
}

/// Subtract two integers but limit the result to the maximum possible values.
/// @tparam T Any signed or unsigned integer type
/// @param a The first value.
/// @param b The second value.
/// @return The subtraction result, limited to the used type.
template <std::integral T>
auto saturatingSubtract(T a, T b) noexcept -> T {
    // Use reliable built-in functions or stop compilation.
#if __GNUC__ || __clang__
#if __has_builtin(__builtin_sub_overflow)
    T result;
    if (__builtin_sub_overflow(a, b, &result)) {
        return (b < 0) ? std::numeric_limits<T>::max() : std::numeric_limits<T>::min();
    }
    return result;
#else
#error "Please use a current version of the GCC or clang compiler."
#endif
#elif _MSC_VER
    T result;
    if (!msl::utilities::SafeSubtract(a, b, result)) {
        return (b < 0) ? std::numeric_limits<T>::max() : std::numeric_limits<T>::min();
    }
    return result;
#else
#error "Missing saturating subtract implementation for the compiler you use."
#endif
}

/// Multiply two integers but limit the result to the maximum possible values.
/// @tparam T Any signed or unsigned integer type.
/// @param a The first multiplier.
/// @param b The second multiplier.
/// @return The product, limited to the used type.
template <std::integral T>
auto saturatingMultiply(T a, T b) noexcept -> T {
    // Use reliable built-in functions or stop compilation.
#if __GNUC__ || __clang__
#if __has_builtin(__builtin_mul_overflow)
    T result;
    if (__builtin_mul_overflow(a, b, &result)) {
        if constexpr (std::is_signed_v<T>) {
            return ((a < 0) ^ (b < 0)) ? std::numeric_limits<T>::min() : std::numeric_limits<T>::max();
        } else {
            return std::numeric_limits<T>::max();
        }
    }
    return result;
#else
#error "Please use a current version of the GCC or clang compiler."
#endif
#elif _MSC_VER
    T result;
    if (!msl::utilities::SafeMultiply(a, b, result)) {
        if constexpr (std::is_signed_v<T>) {
            return ((a < 0) ^ (b < 0)) ? std::numeric_limits<T>::min() : std::numeric_limits<T>::max();
        } else {
            return std::numeric_limits<T>::max();
        }
    }
    return result;
#else
#error "Missing saturating multiply implementation for the compiler you use."
#endif
}

/// Divide integers but limit the result to the maximum possible values.
/// @tparam T Any signed or unsigned integer type.
/// @param a dividend.
/// @param b divisor.
/// @return The result of the division, limited to the used type.
template <std::integral T>
auto saturatingDivide(T a, T b) noexcept -> T {
    if (b == 0) {
        std::terminate(); // division by zero.
    }
    // There is one rance condition:
    // - (-0x80 / -0x01) => -0x80 WRONG! should be 0x7f
    // There are no reliable built-in functions to handle these saturation issues
    if constexpr (std::is_signed_v<T>) {
        return (a == std::numeric_limits<T>::min() && b == T{-1}) ? std::numeric_limits<T>::max() : a / b;
    } else {
        return a / b;
    }
}

/// Get the remainder of a division between integers but limit the result to the maximum possible values.
/// @tparam T Any signed or unsigned integer type.
/// @param a dividend.
/// @param b divisor.
/// @return The result of the division, limited to the used type.
template <std::integral T>
auto saturatingModulo(T a, T b) noexcept -> T {
    if (b == 0) {
        std::terminate(); // division by zero.
    }
    if constexpr (std::is_signed_v<T>) {
        if (a == std::numeric_limits<T>::min() && b == T{-1}) {
            return T{0};
        }
    }
    // Modulo with two equal types is always safe.
    return a % b;
}

/// Test if an addition will overflow.
/// @tparam T The type for the operands.
/// @param a The first value of the operation to test.
/// @param b The second value of the operation to test.
/// @return `true` if an overflow will occur, `false` otherwise.
template <std::integral T>
auto willAddOverflow(T a, T b) noexcept -> bool {
    // Use reliable built-in functions or stop compilation.
#if __GNUC__ || __clang__
#if __has_builtin(__builtin_add_overflow)
    T result;
    return __builtin_add_overflow(a, b, &result);
#else
#error "Please use a current version of the GCC or clang compiler."
#endif
#elif _MSC_VER
    T result;
    return !msl::utilities::SafeAdd(a, b, result);
#else
#error "Missing saturating add implementation for the compiler you use."
#endif
}

/// Test if a subtraction will overflow.
/// @tparam T The type for the operands.
/// @param a The first value of the operation to test.
/// @param b The second value of the operation to test.
/// @return `true` if an overflow will occur, `false` otherwise.
template <std::integral T>
auto willSubtractOverflow(T a, T b) noexcept -> bool {
    // Use reliable built-in functions or stop compilation.
#if __GNUC__ || __clang__
#if __has_builtin(__builtin_sub_overflow)
    T result;
    return __builtin_sub_overflow(a, b, &result);
#else
#error "Please use a current version of the GCC or clang compiler."
#endif
#elif _MSC_VER
    T result;
    return !msl::utilities::SafeSubtract(a, b, result);
#else
#error "Missing saturating subtract implementation for the compiler you use."
#endif
}

/// Test if a multiplication will overflow.
/// @tparam T The type for the operands.
/// @param a The first value of the operation to test.
/// @param b The second value of the operation to test.
/// @return `true` if an overflow will occur, `false` otherwise.
template <std::integral T>
auto willMultiplyOverflow(T a, T b) noexcept -> bool {
    // Use reliable built-in functions or stop compilation.
#if __GNUC__ || __clang__
#if __has_builtin(__builtin_mul_overflow)
    T result;
    return __builtin_mul_overflow(a, b, &result);
#else
#error "Please use a current version of the GCC or clang compiler."
#endif
#elif _MSC_VER
    T result;
    return !msl::utilities::SafeMultiply(a, b, result);
#else
#error "Missing saturating multiply implementation for the compiler you use."
#endif
}

/// Test if a division will overflow.
/// @tparam T The type for the operands.
/// @param a The first value of the operation to test.
/// @param b The second value of the operation to test.
/// @return `true` if an overflow will occur, `false` otherwise.
template <std::integral T>
auto willDivideOverflow(T a, T b) noexcept -> bool {
    if constexpr (std::is_signed_v<T>) {
        return a == std::numeric_limits<T>::min() && b == T{-1};
    }
    return false;
}

/// Test if a modulo will overflow.
/// @tparam T The type for the operands.
/// @param a The first value of the operation to test.
/// @param b The second value of the operation to test.
/// @return `true` if an overflow will occur, `false` otherwise.
template <std::integral T>
auto willModuloOverflow(T a, T b) noexcept -> bool {
    if constexpr (std::is_signed_v<T>) {
        return a == std::numeric_limits<T>::min() && b == T{-1};
    }
    return false;
}

/// Convert an integer type into another one, but make sure the result will not overflow.
/// If e.g. the unsigned 16bit value 0x2000 is casted to a unsigned 8bit value, the result is 0xff, which is the
/// maximum possible for the target type. If a signed 8bit value -10 is cases to a unsigned 8bit value, the result
/// is zero, because this is the smallest possible value for the type.
/// @tparam TargetType The target type for the cast.
/// @tparam SourceType The source type to cast.
/// @param value The source value to cast.
/// @return A value as target type.
template <std::integral TargetType, std::integral SourceType>
constexpr auto saturatingCast(SourceType value) noexcept -> TargetType {
    // This code is sadly not very readable. Yet making it constexpr is huge and measurable gain.
    // The surrounding `if constexpr` structure narrows the conversions down to the actual required checks
    // for a given source and target type. In the end, the code for each case consists of a single `return` statement.
    if constexpr (std::is_same_v<TargetType, SourceType>) {
        // no cast required.
        return value;
    } else if constexpr (std::is_signed_v<SourceType> != std::is_signed_v<TargetType>) {
        if constexpr (std::is_unsigned_v<TargetType>) {
            // source is signed, target unsigned.
            if constexpr (sizeof(TargetType) < sizeof(SourceType)) {
                return (value < 0) ? 0
                                   : ((value > static_cast<SourceType>(std::numeric_limits<TargetType>::max()))
                                             ? std::numeric_limits<TargetType>::max()
                                             : static_cast<TargetType>(value));
            } else {
                return (value < 0) ? 0 : static_cast<TargetType>(value);
            }
        } else {
            // source is unsigned, target signed.
            if constexpr (sizeof(TargetType) <= sizeof(SourceType)) {
                return (value > static_cast<SourceType>(std::numeric_limits<TargetType>::max()))
                    ? std::numeric_limits<TargetType>::max()
                    : static_cast<TargetType>(value);
            } else {
                return static_cast<TargetType>(value);
            }
        }
    } else if constexpr (sizeof(TargetType) < sizeof(SourceType)) {
        // target is smaller than source.
        if constexpr (std::is_signed_v<TargetType>) {
            return (value < static_cast<SourceType>(std::numeric_limits<TargetType>::min()))
                ? std::numeric_limits<TargetType>::min()
                : ((value > static_cast<SourceType>(std::numeric_limits<TargetType>::max()))
                          ? std::numeric_limits<TargetType>::max()
                          : static_cast<TargetType>(value));
        } else {
            return (value > static_cast<SourceType>(std::numeric_limits<TargetType>::max()))
                ? std::numeric_limits<TargetType>::max()
                : static_cast<TargetType>(value);
        }
    } else {
        // target is larger than source.
        return static_cast<TargetType>(value);
    }
}

/// Check if a saturating cast will overflow.
/// @tparam TargetType The target type for the cast.
/// @tparam SourceType The source type to cast.
/// @param value The source value to check for an overflow.
/// @return `true` if the case would overflow and change `value`.
template <std::integral TargetType, std::integral SourceType>
constexpr auto willSaturatingCastOverflow(SourceType value) noexcept -> bool {
    if constexpr (std::is_signed_v<SourceType> != std::is_signed_v<TargetType>) {
        if constexpr (std::is_unsigned_v<TargetType>) { // source is signed, target unsigned.
            if constexpr (sizeof(SourceType) > sizeof(TargetType)) {
                return value < 0 || value > static_cast<SourceType>(std::numeric_limits<TargetType>::max());
            } else {
                return value < 0;
            }
        } else { // source is unsigned, target signed.
            if constexpr (sizeof(SourceType) >= sizeof(TargetType)) {
                return value > static_cast<SourceType>(std::numeric_limits<TargetType>::max());
            } else {
                return false; // if target is larger than source, it will never overflow.
            }
        }
    } else {
        if constexpr (sizeof(SourceType) > sizeof(TargetType)) {
            if constexpr (std::is_signed_v<TargetType>) {
                return (value < static_cast<SourceType>(std::numeric_limits<TargetType>::min())) ||
                    (value > static_cast<SourceType>(std::numeric_limits<TargetType>::max()));
            } else {
                return (value > static_cast<SourceType>(std::numeric_limits<TargetType>::max()));
            }
        } else {
            return false; // if target is larger or equal than source, it will never overflow.
        }
    }
}

/// Increment a value but never overflow.
/// @tparam T The type of the value.
/// @param value The value to increment.
template <std::integral T>
void saturatingIncrement(T &value) noexcept {
    if (value < std::numeric_limits<T>::max()) {
        ++value;
    }
}

/// Increment a value but never overflow.
/// @tparam T The type of the value.
/// @param value The value to decrement.
template <std::integral T>
void saturatingDecrement(T &value) noexcept {
    if (value > std::numeric_limits<T>::min()) {
        --value;
    }
}

/// Saturated add with any integer type.
/// This will allow to use any integer type as second operand for a saturated addition.
/// The result type will be the type of the first operand.
/// @tparam A The target type for the operation.
/// @tparam B The type of the second summand.
/// @param a The value to change.
/// @param b The value to add.
/// @return The result.
template <std::integral A, std::integral B>
auto saturatingAdd(A a, B b) noexcept -> A {
    if constexpr (std::is_signed_v<A> == std::is_signed_v<B>) {
        using C = CommonIntT<A, B>;
        return saturatingCast<A>(saturatingAdd<C>(static_cast<C>(a), static_cast<C>(b)));
    } else if constexpr (std::is_signed_v<A>) { // signed target, unsigned operation
        return (willSaturatingCastOverflow<A>(b))
            ? saturatingAdd<A>(
                  saturatingAdd<A>(saturatingAdd<A>(a, std::numeric_limits<A>::max()), A{1}),
                  saturatingCast<A>(b - (B{1} << (sizeof(A) * 8 - 1))))
            : saturatingAdd<A>(a, saturatingCast<A>(b));
    } else { // unsigned target, signed operation.
        return (b < 0) ? saturatingSubtract<A>(a, saturatingCast<A>(unsignedIntAbs(b)))
                       : saturatingAdd<A>(a, saturatingCast<A>(b));
    }
}

/// Saturated subtract with any integer type.
/// This will allow to use any integer type as second operand for a saturated subtraction.
/// The result type will be the type of the first operand.
/// @tparam A The target type for the operation.
/// @tparam B The type of the subtrahend.
/// @param a The value to change.
/// @param b The value to subtract.
/// @return The result.
template <std::integral A, std::integral B>
auto saturatingSubtract(A a, B b) noexcept -> A {
    if constexpr (std::is_signed_v<A> == std::is_signed_v<B>) {
        using C = CommonIntT<A, B>;
        return saturatingCast<A>(saturatingSubtract<C>(static_cast<C>(a), static_cast<C>(b)));
    } else if constexpr (std::is_signed_v<A>) { // signed target, unsigned operation
        return (willSaturatingCastOverflow<A>(b))
            ? saturatingSubtract<A>(
                  saturatingSubtract<A>(saturatingSubtract<A>(a, std::numeric_limits<A>::max()), A{1}),
                  saturatingCast<A>(b - (B{1} << (sizeof(A) * 8u - 1u))))
            : saturatingSubtract<A>(a, saturatingCast<A>(b));
    } else { // unsigned target, signed operation.
        return (b < 0) ? saturatingAdd<A>(a, saturatingCast<A>(unsignedIntAbs(b)))
                       : saturatingSubtract<A>(a, saturatingCast<A>(b));
    }
}

/// Saturated multiply with any compatible integer type.
/// This will allow to use (almost) any integer type as second operand.
/// The result type will be the type of the first operand.
/// @tparam A The target type for the operation.
/// @tparam B The type of the factor.
/// @param a The value to change.
/// @param b The factor.
/// @return The result.
template <std::integral A, std::integral B>
auto saturatingMultiply(A a, B b) noexcept -> A {
    if constexpr (std::is_unsigned_v<A> && std::is_signed_v<B>) {
        // unsigned/signed needs an additional check.
        return (b < 0) ? 0
                       : (willSaturatingCastOverflow<A>(b) ? extremeIntFromMultiplication(a, b)
                                                           : saturatingMultiply<A>(a, saturatingCast<A>(b)));
    } else {
        return willSaturatingCastOverflow<A>(b) ? extremeIntFromMultiplication(a, b)
                                                : saturatingMultiply<A>(a, saturatingCast<A>(b));
    }
}

/// Saturated division with any compatible integer type.
///
/// This will allow to use any integer type as second operand.
/// The result type will be the type of the first operand.
///
/// @tparam A The target type for the operation.
/// @tparam B The type of the divisor.
/// @param a The value to change.
/// @param b The divisor.
/// @return The result.
///
template <std::integral A, std::integral B>
auto saturatingDivide(A a, B b) noexcept -> A {
    // race conditions to handle (8bit signed/8bit unsigned example):
    // 1. -0x80/0x80 = 1 / After overflow cast -0x80/0x7f = 1 OK
    // 2. -0x80/0x81 = 0 / After overflow cast -0x80/0x7f = 1 WRONG!
    if constexpr (std::is_unsigned_v<A> && std::is_signed_v<B>) {
        // unsigned/signed needs an additional check.
        if constexpr (HasLargerAbsoluteIntV<A, B>) {
            return (b < 0 || a < unsignedIntAbs(b)) ? 0 : saturatingDivide<A>(a, saturatingCast<A>(b));
        } else {
            return (b < 0) ? 0 : saturatingDivide<A>(a, saturatingCast<A>(b));
        }
    } else {
        if constexpr (HasLargerAbsoluteIntV<A, B>) {
            return (unsignedIntAbs(a) < unsignedIntAbs(b)) ? 0 : saturatingDivide<A>(a, saturatingCast<A>(b));
        } else {
            return saturatingDivide<A>(a, saturatingCast<A>(b));
        }
    }
}

/// Saturated modulo with any compatible integer type.
/// This will allow to use any integer type as second operand.
/// The result type will be the type of the first operand.
/// @tparam A The target type for the operation.
/// @tparam B The type of the divisor.
/// @param a The value to change.
/// @param b The divisor.
/// @return The result.
///
template <std::integral A, std::integral B>
auto saturatingModulo(A a, B b) noexcept -> A {
    if (b == B{0}) {
        std::terminate(); // division by zero.
    }
    // Race conditions to handle (8bit signed/8bit unsigned example):
    // 1. -0x80%0x80 = 0 / After overflow cast -0x80/0x7f = -1 WRONG!
    //
    // Explanation for this implementation:
    // In c++, modulo is defined as remainder of the division, so ((a/b)*b+m)==a.
    // Therefore, the sign of the modulo operator has no influence to the result. The result always has the same
    // sign as `a`.
    //
    // By converting `b` to an absolute unsigned integer, the race condition of [int min]!=[int max] is
    // avoided. This is working, except if `a` is [int min] and `b` is a larger type, abs([int min]) == b.
    // E.g. with `a` = signed 8-bit, -0x80 and `b` = unsigned 0x80. The cast would overflow, `b` limited to 0x7f.
    // `a` would be taken as result and lead to the wrong result.
    //
    // Therefore, this condition is checked, by comparing abs(a)==abs(b) first and return zero.
    //
    using AbsoluteB = std::make_unsigned_t<B>;
    const auto absoluteB = unsignedIntAbs(b);
    if constexpr (std::is_signed_v<B> || HasLargerAbsoluteIntV<A, B>) {
        if constexpr (HasLargerAbsoluteIntV<A, AbsoluteB>) {
            return (unsignedIntAbs(a) == absoluteB)
                ? 0
                : (willSaturatingCastOverflow<A>(absoluteB) ? a : saturatingModulo<A>(a, saturatingCast<A>(absoluteB)));
        } else {
            return saturatingModulo<A>(a, saturatingCast<A>(absoluteB));
        }
    } else {
        return (unsignedIntAbs(a) == absoluteB)
            ? 0
            : (willSaturatingCastOverflow<A>(absoluteB) ? a : saturatingModulo<A>(a, saturatingCast<A>(absoluteB)));
    }
}

/// Test if an addition will overflow.
/// @tparam A The target type of the operation to test for the overflow.
/// @tparam B Operator type with no influence to the result type.
/// @param a The first value of the operation to test.
/// @param b The second value of the operation to test.
/// @return `true` if an overflow will occur, `false` otherwise.
template <std::integral A, std::integral B>
auto willAddOverflow(A a, B b) noexcept -> bool {
    if constexpr (std::is_signed_v<A> == std::is_signed_v<B>) {
        using C = CommonIntT<A, B>;
        if (willAddOverflow<C>(static_cast<C>(a), static_cast<C>(b))) {
            return true;
        }
        const C c = saturatingAdd(static_cast<C>(a), static_cast<C>(b));
        return willSaturatingCastOverflow<A>(c);
    } else if constexpr (std::is_signed_v<A>) { // signed target, unsigned operation
        if (willSaturatingCastOverflow<A>(b)) {
            if (willAddOverflow<A>(a, std::numeric_limits<A>::max())) {
                return true;
            }
            a = saturatingAdd<A>(a, std::numeric_limits<A>::max());
            if (willAddOverflow<A>(a, A{1})) {
                return true;
            }
            a = saturatingAdd<A>(a, A{1});
            if (willSaturatingCastOverflow<A>(b - (static_cast<B>(1) << (sizeof(A) * 8 - 1)))) {
                return true;
            }
            return willAddOverflow<A>(a, saturatingCast<A>(b - (static_cast<B>(1) << (sizeof(A) * 8 - 1))));
        } else {
            return willAddOverflow<A>(a, saturatingCast<A>(b));
        }
    } else { // unsigned target, signed operation.
        if (b < 0) {
            if (willSaturatingCastOverflow<A>(unsignedIntAbs(b))) {
                return true;
            }
            return willSubtractOverflow<A>(a, saturatingCast<A>(unsignedIntAbs(b)));
        } else {
            if (willSaturatingCastOverflow<A>(b)) {
                return true;
            }
            return willAddOverflow<A>(a, saturatingCast<A>(b));
        }
    }
}

/// Test if a subtraction will overflow.
/// @tparam A The target type of the operation to test for the overflow.
/// @tparam B Operator type with no influence to the result type.
/// @param a The first value of the operation to test.
/// @param b The second value of the operation to test.
/// @return `true` if an overflow will occur, `false` otherwise.
template <std::integral A, std::integral B>
auto willSubtractOverflow(A a, B b) noexcept -> bool {
    if constexpr (std::is_signed_v<A> == std::is_signed_v<B>) {
        using C = CommonIntT<A, B>;
        if (willSubtractOverflow<C>(static_cast<C>(a), static_cast<C>(b))) {
            return true;
        }
        const C c = saturatingSubtract(static_cast<C>(a), static_cast<C>(b));
        return willSaturatingCastOverflow<A>(c);
    } else if constexpr (std::is_signed_v<A>) { // signed target, unsigned operation
        if (willSaturatingCastOverflow<A>(b)) {
            if (willSubtractOverflow<A>(a, std::numeric_limits<A>::max())) {
                return true;
            }
            a = saturatingSubtract<A>(a, std::numeric_limits<A>::max());
            if (willSubtractOverflow<A>(a, 1)) {
                return true;
            }
            a = saturatingSubtract<A>(a, 1);
            if (willSaturatingCastOverflow<A>(b - (static_cast<B>(1) << (sizeof(A) * 8 - 1)))) {
                return true;
            }
            return willSubtractOverflow<A>(a, saturatingCast<A>(b - (static_cast<B>(1) << (sizeof(A) * 8 - 1))));
        } else {
            return willSubtractOverflow<A>(a, saturatingCast<A>(b));
        }
    } else { // unsigned target, signed operation.
        if (b < 0) {
            if (willSaturatingCastOverflow<A>(unsignedIntAbs(b))) {
                return true;
            }
            return willAddOverflow<A>(a, saturatingCast<A>(unsignedIntAbs(b)));
        } else {
            if (willSaturatingCastOverflow<A>(b)) {
                return true;
            }
            return willSubtractOverflow<A>(a, saturatingCast<A>(b));
        }
    }
}

/// Test if a multiplication will overflow.
/// @tparam A The target type of the operation to test for the overflow.
/// @tparam B Operator type with no influence to the result type.
/// @param a The first value of the operation to test.
/// @param b The second value of the operation to test.
/// @return `true` if an overflow will occur, `false` otherwise.
template <std::integral A, std::integral B>
auto willMultiplyOverflow(A a, B b) noexcept -> bool {
    if (a == 0 || b == 0 || b == 1) {
        return false;
    }
    if constexpr (std::is_unsigned_v<A> && std::is_signed_v<B>) {
        // unsigned/signed needs an additional check.
        if (b < 0) {
            return true;
        }
    }
    if constexpr (std::is_signed_v<A> && HasLargerAbsoluteIntV<A, B>) {
        // edge condition, when a == -1 and b == a[min]
        if (a == -1 && b == static_cast<B>(unsignedIntAbs(std::numeric_limits<A>::min()))) {
            return false;
        }
    }
    if (willSaturatingCastOverflow<A>(b) && a != 0) {
        return true;
    }
    return willMultiplyOverflow<A>(a, saturatingCast<A>(b));
}

/// Test if a division will overflow.
/// @tparam A The target type of the operation to test for the overflow.
/// @tparam B Operator type with no influence to the result type.
/// @param a The first value of the operation to test.
/// @param b The second value of the operation to test.
/// @return `true` if an overflow will occur, `false` otherwise.
template <std::integral A, std::integral B>
auto willDivideOverflow(A a, B b) noexcept -> bool {
    if constexpr (std::is_unsigned_v<A> && std::is_signed_v<B>) {
        // unsigned/signed needs an additional check.
        if constexpr (HasLargerAbsoluteIntV<A, B>) {
            if (a < unsignedIntAbs(b)) {
                return false;
            }
        }
        return (a > 0) && (b < 0);
    } else if constexpr (std::is_signed_v<A> && std::is_signed_v<B>) {
        return (a == std::numeric_limits<A>::min()) && b == B{-1};
    } else {
        return false;
    }
}

/// Test if a modulo operation will overflow.
/// @tparam A The target type of the operation to test for the overflow.
/// @tparam B Operator type with no influence to the result type.
/// @param a The first value of the operation to test.
/// @param b The second value of the operation to test.
/// @return `true` if an overflow will occur, `false` otherwise.
template <std::integral A, std::integral B>
auto willModuloOverflow(A a, B b) noexcept -> bool {
    if constexpr (std::is_signed_v<A> && std::is_signed_v<B>) {
        return a == std::numeric_limits<A>::min() && b == B{-1};
    }
    return false;
}

}
