// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BigInteger_fwd.hpp"
#include "BigUnsignedInteger.hpp"
#include "IntegerMath.hpp"
#include "IntegerTypes.hpp"

#include "../text/String.hpp"
#include "../util/impl/ComparisonHelper.hpp"

#include <compare>
#include <optional>

namespace erbsland::math {

/// An arbitrary-precision signed integer.
///
/// Arithmetic is exact and division truncates toward zero. A remainder has the dividend's sign. Division and modulo by
/// zero terminate the process. Operators intentionally accept only another `BigInteger`; use an explicit constructor
/// when starting from a native integer.
/// @seedoc{/reference/math/mathematics}
/// @tested{BigIntegerTest}
class BigInteger final {
public:
    /// Create a zero value.
    BigInteger() = default;
    /// Create a value from a native integer.
    /// @tparam T The native integer type.
    /// @param value The initial value.
    template <NativeInteger T>
    explicit BigInteger(T value);
    /// Create a positive value from an unsigned magnitude.
    /// @param magnitude The initial magnitude.
    explicit BigInteger(BigUnsignedInteger magnitude) noexcept;

    // defaults
    ~BigInteger() = default;
    BigInteger(const BigInteger &) = default;
    BigInteger(BigInteger &&) = default;
    auto operator=(const BigInteger &) -> BigInteger & = default;
    auto operator=(BigInteger &&) -> BigInteger & = default;

public: // operators: comparison
    /// Compare this value mathematically with another value.
    /// @param other The other value.
    /// @return The ordering of this value compared with `other`.
    [[nodiscard]] auto operator<=>(const BigInteger &other) const noexcept -> std::strong_ordering;
    ERBSLAND_CORE_COMPARE_FROM_SPACESHIP(const BigInteger &other, other);

public: // operators: arithmetic
    /// Add another value.
    /// @param other The value to add.
    /// @return The exact sum.
    [[nodiscard]] auto operator+(const BigInteger &other) const -> BigInteger;
    /// Add another value in place.
    /// @param other The value to add.
    /// @return This value after the addition.
    auto operator+=(const BigInteger &other) -> BigInteger &;
    /// Subtract another value.
    /// @param other The value to subtract.
    /// @return The exact difference.
    [[nodiscard]] auto operator-(const BigInteger &other) const -> BigInteger;
    /// Subtract another value in place.
    /// @param other The value to subtract.
    /// @return This value after the subtraction.
    auto operator-=(const BigInteger &other) -> BigInteger &;
    /// Negate this value.
    /// @return The exact negated value.
    [[nodiscard]] auto operator-() const -> BigInteger;
    /// Multiply by another value.
    /// @param other The factor.
    /// @return The exact product.
    [[nodiscard]] auto operator*(const BigInteger &other) const -> BigInteger;
    /// Multiply by another value in place.
    /// @param other The factor.
    /// @return This value after the multiplication.
    auto operator*=(const BigInteger &other) -> BigInteger &;
    /// Divide by another value, truncating toward zero.
    /// @param other The divisor, which must not be zero.
    /// @return The quotient.
    [[nodiscard]] auto operator/(const BigInteger &other) const -> BigInteger;
    /// Divide by another value in place, truncating toward zero.
    /// @param other The divisor, which must not be zero.
    /// @return This value after the division.
    auto operator/=(const BigInteger &other) -> BigInteger &;
    /// Calculate the remainder of a division.
    /// @param other The divisor, which must not be zero.
    /// @return The remainder, with the same sign as this value.
    [[nodiscard]] auto operator%(const BigInteger &other) const -> BigInteger;
    /// Store the remainder of a division in place.
    /// @param other The divisor, which must not be zero.
    /// @return This value after the modulo operation.
    auto operator%=(const BigInteger &other) -> BigInteger &;

public: // tests and accessors
    /// Test whether this value is zero.
    [[nodiscard]] auto isZero() const noexcept -> bool;
    /// Test whether this value is one.
    [[nodiscard]] auto isOne() const noexcept -> bool;
    /// Test whether this value is negative.
    [[nodiscard]] auto isNegative() const noexcept -> bool;
    /// Get the absolute magnitude.
    [[nodiscard]] auto magnitude() const noexcept -> const BigUnsignedInteger &;

public: // conversion
    /// Cast this value to a native integer, clamping it to the target range.
    /// @tparam T The target native integer type.
    /// @return The converted value.
    template <NativeInteger T>
    [[nodiscard]] auto cast() const -> T;
    /// Cast this value to a native integer without clamping.
    /// @tparam T The target native integer type.
    /// @return The converted value.
    /// @throws err::OverflowError If this value cannot be represented by `T`.
    template <NativeInteger T>
    [[nodiscard]] auto castOrThrow() const -> T;
    /// Convert this value to decimal text.
    [[nodiscard]] auto toString() const -> text::String;
    /// Parse a complete decimal value.
    /// @param text The text to parse.
    /// @return The parsed value, or an empty optional for invalid text.
    [[nodiscard]] static auto fromString(const text::String &text) -> std::optional<BigInteger>;
    /// Parse a complete decimal value.
    /// @param text The text to parse.
    /// @return The parsed value.
    /// @throws err::ParseError If `text` is not a valid signed decimal integer.
    [[nodiscard]] static auto fromStringOrThrow(const text::String &text) -> BigInteger;
    /// Create a value from a sign and an unsigned magnitude.
    /// @param negative Whether the value shall be negative.
    /// @param magnitude The absolute magnitude.
    /// @return The normalized value. A zero magnitude is always positive.
    [[nodiscard]] static auto fromSignAndMagnitude(bool negative, BigUnsignedInteger magnitude) noexcept -> BigInteger;

public: // combined and named arithmetic
    /// Return this value with its sign changed.
    [[nodiscard]] auto negated() const -> BigInteger;
    /// Divide this value in place and return the remainder.
    /// @param divisor The divisor, which must not be zero.
    /// @return The remainder, with the dividend's original sign.
    [[nodiscard]] auto divideGetRemainder(const BigInteger &divisor) -> BigInteger;

private:
    /// Create a value from an already separated sign and magnitude.
    BigInteger(bool negative, BigUnsignedInteger magnitude) noexcept;
    /// Divide this value in place without producing a remainder.
    void divideAssign(const BigInteger &divisor);
    /// Normalize zero to a positive value.
    void normalizeSign() noexcept;
    /// Throw for an inexact native cast.
    [[noreturn]] static void throwCastOverflow();

    BigUnsignedInteger _magnitude; ///< The absolute magnitude.
    bool _negative{false};         ///< Whether the non-zero value is negative.
};

}

#include "BigInteger.tpp"
