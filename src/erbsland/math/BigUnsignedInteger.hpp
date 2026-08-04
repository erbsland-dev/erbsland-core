// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BigInteger_fwd.hpp"
#include "IntegerTypes.hpp"

#include "../text/String.hpp"
#include "../util/impl/ComparisonHelper.hpp"

#include <compare>
#include <cstdint>
#include <optional>
#include <vector>

namespace erbsland::math {

/// An arbitrary-precision unsigned integer.
///
/// Arithmetic is exact except for subtraction below zero, which throws an overflow error. Division and modulo by zero
/// terminate the process. Operators intentionally accept only another `BigUnsignedInteger`; use an explicit constructor
/// when starting from a native integer.
/// @seedoc{/reference/math/big_integers}
/// @tested{BigUnsignedIntegerTest}
class BigUnsignedInteger final {
    friend class BigInteger;
    static constexpr std::uint32_t cDigitBase = 1'000'000'000U;

public:
    /// Create a zero value.
    BigUnsignedInteger() = default;
    /// Create a value from a native integer.
    /// @tparam T The native integer type.
    /// @param value The initial value.
    /// @throws err::OverflowError If `value` is negative.
    template <NativeInteger T>
    explicit BigUnsignedInteger(T value);

    // defaults
    ~BigUnsignedInteger() = default;
    BigUnsignedInteger(const BigUnsignedInteger &) = default;
    BigUnsignedInteger(BigUnsignedInteger &&) = default;
    auto operator=(const BigUnsignedInteger &) -> BigUnsignedInteger & = default;
    auto operator=(BigUnsignedInteger &&) -> BigUnsignedInteger & = default;

public: // operators: comparison
    /// Compare this value mathematically with another value.
    /// @param other The other value.
    /// @return The ordering of this value compared with `other`.
    [[nodiscard]] auto operator<=>(const BigUnsignedInteger &other) const noexcept -> std::strong_ordering;
    ERBSLAND_CORE_COMPARE_FROM_SPACESHIP(const BigUnsignedInteger &other, other);

public: // operators: arithmetic
    /// Add another value.
    /// @param other The value to add.
    /// @return The exact sum.
    [[nodiscard]] auto operator+(const BigUnsignedInteger &other) const -> BigUnsignedInteger;
    /// Add another value in place.
    /// @param other The value to add.
    /// @return This value after the addition.
    auto operator+=(const BigUnsignedInteger &other) -> BigUnsignedInteger &;
    /// Subtract another value.
    /// @param other The value to subtract.
    /// @return The exact difference.
    /// @throws err::OverflowError If `other` is larger than this value.
    [[nodiscard]] auto operator-(const BigUnsignedInteger &other) const -> BigUnsignedInteger;
    /// Subtract another value in place.
    /// @param other The value to subtract.
    /// @return This value after the subtraction.
    /// @throws err::OverflowError If `other` is larger than this value.
    auto operator-=(const BigUnsignedInteger &other) -> BigUnsignedInteger &;
    /// Multiply by another value.
    /// @param other The factor.
    /// @return The exact product.
    [[nodiscard]] auto operator*(const BigUnsignedInteger &other) const -> BigUnsignedInteger;
    /// Multiply by another value in place.
    /// @param other The factor.
    /// @return This value after the multiplication.
    auto operator*=(const BigUnsignedInteger &other) -> BigUnsignedInteger &;
    /// Divide by another value.
    /// @param other The divisor, which must not be zero.
    /// @return The quotient.
    [[nodiscard]] auto operator/(const BigUnsignedInteger &other) const -> BigUnsignedInteger;
    /// Divide by another value in place.
    /// @param other The divisor, which must not be zero.
    /// @return This value after the division.
    auto operator/=(const BigUnsignedInteger &other) -> BigUnsignedInteger &;
    /// Calculate the remainder of a division.
    /// @param other The divisor, which must not be zero.
    /// @return The remainder.
    [[nodiscard]] auto operator%(const BigUnsignedInteger &other) const -> BigUnsignedInteger;
    /// Store the remainder of a division in place.
    /// @param other The divisor, which must not be zero.
    /// @return This value after the modulo operation.
    auto operator%=(const BigUnsignedInteger &other) -> BigUnsignedInteger &;

public: // tests
    /// Test whether this value is zero.
    [[nodiscard]] auto isZero() const noexcept -> bool;
    /// Test whether this value is one.
    [[nodiscard]] auto isOne() const noexcept -> bool;

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
    [[nodiscard]] static auto fromString(const text::String &text) -> std::optional<BigUnsignedInteger>;
    /// Parse a complete decimal value.
    /// @param text The text to parse.
    /// @return The parsed value.
    /// @throws err::ParseError If `text` is not a valid unsigned decimal integer.
    [[nodiscard]] static auto fromStringOrThrow(const text::String &text) -> BigUnsignedInteger;

public: // combined arithmetic
    /// Divide this value in place and return the remainder.
    /// @param divisor The divisor, which must not be zero.
    /// @return The remainder of the division.
    [[nodiscard]] auto divideGetRemainder(const BigUnsignedInteger &divisor) -> BigUnsignedInteger;

private:
    /// Assign a native unsigned value.
    void assign(std::uint64_t value);
    /// Add one small value.
    void addSmall(std::uint32_t value);
    /// Multiply by one small value.
    void multiplySmall(std::uint32_t value);
    /// Divide by one small value and return its remainder.
    [[nodiscard]] auto divideSmall(std::uint32_t divisor) noexcept -> std::uint32_t;
    /// Convert a known representable value to 64 bits.
    [[nodiscard]] auto toUInt64Unchecked() const noexcept -> std::uint64_t;
    /// Remove zero groups from the most-significant end.
    void trim() noexcept;
    /// Divide two values and return the remainder.
    /// The return value may be discarded when only the quotient is required.
    static auto divideWithRemainder(
        const BigUnsignedInteger &dividend, const BigUnsignedInteger &divisor, BigUnsignedInteger &quotient)
        -> BigUnsignedInteger;
    /// Throw for a negative construction value.
    [[noreturn]] static void throwNegativeValue();
    /// Throw for unsigned subtraction underflow.
    [[noreturn]] static void throwSubtractionUnderflow();
    /// Throw for an inexact native cast.
    [[noreturn]] static void throwCastOverflow();

private:
    std::vector<std::uint32_t> _digits; ///< Little-endian groups in base 1,000,000,000.
};

}

#include "BigUnsignedInteger.tpp"
