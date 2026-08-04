// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerMath.hpp"
#include "IntegerRange.hpp"
#include "IntegerTraits.hpp"
#include "SaturatingInteger_fwd.hpp"
#include "SaturatingMath.hpp"

#include "impl/SaturatingMathHelper.hpp"
#include "impl/Throw.hpp"

#include <compare>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <tuple>
#include <type_traits>
#include <utility>

namespace erbsland::math {

/// Integer wrapper with arithmetic that saturates at the limits of the native value type.
/// Arithmetic operators require matching signedness and return a saturating integer using the larger operand type.
/// The named methods keep this type as result and also accept mixed signedness. Bitwise operations are not supported.
template <NativeInteger tValue>
class SaturatingInteger {
public:
    /// The wrapped native integer type.
    using NativeValue = tValue;
    /// The signed integer type used to report wrap counts.
    using WrapCount = SaturatingInteger<std::int64_t>;

public:
    /// Create a zero value.
    constexpr SaturatingInteger() = default;

    /// Create a saturating integer from another supported integer operand.
    template <AnyIntegerType tOther>
    constexpr explicit SaturatingInteger(tOther value) noexcept;

    // defaults
    /// Destroy this saturating integer.
    ~SaturatingInteger() = default;
    /// Copy a saturating integer.
    SaturatingInteger(const SaturatingInteger &) = default;
    /// Move a saturating integer.
    SaturatingInteger(SaturatingInteger &&) = default;
    /// Copy another saturating integer into this value.
    auto operator=(const SaturatingInteger &) -> SaturatingInteger & = default;
    /// Move another saturating integer into this value.
    auto operator=(SaturatingInteger &&) -> SaturatingInteger & = default;

public: // Operators: Comparison with other integers
    /// Test if this value equals another integer operand.
    /// @tparam T The type of the other value.
    /// @param other The other value to compare.
    /// @return `true` if both values compare equal.
    template <AnyIntegerType T>
    constexpr auto operator==(T other) const noexcept -> bool;
    /// Test if this value differs from another integer operand.
    /// @tparam T The type of the other value.
    /// @param other The other value to compare.
    /// @return `true` if the values do not compare equal.
    template <AnyIntegerType T>
    constexpr auto operator!=(T other) const noexcept -> bool;
    /// Test if this value is less than another integer operand.
    /// @tparam T The type of the other value.
    /// @param other The other value to compare.
    /// @return `true` if this value is less than `other`.
    template <AnyIntegerType T>
    constexpr auto operator<(T other) const noexcept -> bool;
    /// Test if this value is less than or equal to another integer operand.
    /// @tparam T The type of the other value.
    /// @param other The other value to compare.
    /// @return `true` if this value is less than or equal to `other`.
    template <AnyIntegerType T>
    constexpr auto operator<=(T other) const noexcept -> bool;
    /// Test if this value is greater than another integer operand.
    /// @tparam T The type of the other value.
    /// @param other The other value to compare.
    /// @return `true` if this value is greater than `other`.
    template <AnyIntegerType T>
    constexpr auto operator>(T other) const noexcept -> bool;
    /// Test if this value is greater than or equal to another integer operand.
    /// @tparam T The type of the other value.
    /// @param other The other value to compare.
    /// @return `true` if this value is greater than or equal to `other`.
    template <AnyIntegerType T>
    constexpr auto operator>=(T other) const noexcept -> bool;

public: // Operators: Increment and decrement
    /// Increment this value without exceeding the maximum representable value.
    /// @return This value after the increment.
    auto operator++() noexcept -> SaturatingInteger &;
    /// Increment this value without exceeding the maximum representable value.
    /// @return The value before the increment.
    auto operator++(int) noexcept -> SaturatingInteger;
    /// Decrement this value without exceeding the minimum representable value.
    /// @return This value after the decrement.
    auto operator--() noexcept -> SaturatingInteger &;
    /// Decrement this value without exceeding the minimum representable value.
    /// @return The value before the decrement.
    auto operator--(int) noexcept -> SaturatingInteger;

public: // Operators: Arithmetic
    /// Add another integer operand with saturation.
    /// The operands must have compatible signedness; the result uses the larger native type.
    /// @tparam T The type of the other value.
    /// @param other The value to add.
    /// @return The saturated sum.
    template <AnyIntegerType T>
        requires SignCompatibleIntegerOperand<tValue, T>
    auto operator+(T other) const noexcept
        -> SaturatingInteger<CompatibleNativeIntegerT<NativeValue, NativeIntegerOfT<T>>>;
    /// Add another integer operand and store the saturated result in this value.
    /// @tparam T The type of the other value.
    /// @param other The value to add.
    /// @return This value after the addition.
    template <AnyIntegerType T>
    auto operator+=(T other) noexcept -> SaturatingInteger &;
    /// Subtract another integer operand with saturation.
    /// The operands must have compatible signedness; the result uses the larger native type.
    /// @tparam T The type of the other value.
    /// @param other The value to subtract.
    /// @return The saturated difference.
    template <AnyIntegerType T>
        requires SignCompatibleIntegerOperand<tValue, T>
    auto operator-(T other) const noexcept
        -> SaturatingInteger<CompatibleNativeIntegerT<NativeValue, NativeIntegerOfT<T>>>;
    /// Subtract another integer operand and store the saturated result in this value.
    /// @tparam T The type of the other value.
    /// @param other The value to subtract.
    /// @return This value after the subtraction.
    template <AnyIntegerType T>
    auto operator-=(T other) noexcept -> SaturatingInteger &;
    /// Multiply by another integer operand with saturation.
    /// The operands must have compatible signedness; the result uses the larger native type.
    /// @tparam T The type of the other value.
    /// @param other The factor.
    /// @return The saturated product.
    template <AnyIntegerType T>
        requires SignCompatibleIntegerOperand<tValue, T>
    auto operator*(T other) const noexcept
        -> SaturatingInteger<CompatibleNativeIntegerT<NativeValue, NativeIntegerOfT<T>>>;
    /// Multiply by another integer operand and store the saturated result in this value.
    /// @tparam T The type of the other value.
    /// @param other The factor.
    /// @return This value after the multiplication.
    template <AnyIntegerType T>
    auto operator*=(T other) noexcept -> SaturatingInteger &;
    /// Divide by another integer operand with saturation.
    /// The operands must have compatible signedness; the result uses the larger native type.
    /// @tparam T The type of the other value.
    /// @param other The divisor.
    /// @return The saturated quotient.
    template <AnyIntegerType T>
        requires SignCompatibleIntegerOperand<tValue, T>
    auto operator/(T other) const noexcept
        -> SaturatingInteger<CompatibleNativeIntegerT<NativeValue, NativeIntegerOfT<T>>>;
    /// Divide by another integer operand and store the saturated result in this value.
    /// @tparam T The type of the other value.
    /// @param other The divisor.
    /// @return This value after the division.
    template <AnyIntegerType T>
    auto operator/=(T other) noexcept -> SaturatingInteger &;
    /// Apply modulo with another integer operand.
    /// The operands must have compatible signedness; the result uses the larger native type.
    /// @tparam T The type of the other value.
    /// @param other The divisor.
    /// @return The saturated remainder.
    template <AnyIntegerType T>
        requires SignCompatibleIntegerOperand<tValue, T>
    auto operator%(T other) const noexcept
        -> SaturatingInteger<CompatibleNativeIntegerT<NativeValue, NativeIntegerOfT<T>>>;
    /// Apply modulo with another integer operand and store the result in this value.
    /// @tparam T The type of the other value.
    /// @param other The divisor.
    /// @return This value after the modulo operation.
    template <AnyIntegerType T>
    auto operator%=(T other) noexcept -> SaturatingInteger &;

public: // Named arithmetic returning a value
    /// Add another integer operand and keep this type as the result type.
    /// Unlike `operator+`, this method accepts mixed signedness and saturates to this native value type.
    /// @tparam T The type of the other value.
    /// @param other The other value of the operation.
    /// @return The result of the operation.
    template <AnyIntegerType T>
    [[nodiscard]] auto added(T other) const noexcept -> SaturatingInteger;
    /// Subtract another integer operand and keep this type as the result type.
    template <AnyIntegerType T>
    [[nodiscard]] auto subtracted(T other) const noexcept -> SaturatingInteger;
    /// Multiply another integer operand and keep this type as the result type.
    template <AnyIntegerType T>
    [[nodiscard]] auto multiplied(T other) const noexcept -> SaturatingInteger;
    /// Divide another integer operand and keep this type as the result type.
    template <AnyIntegerType T>
    [[nodiscard]] auto divided(T other) const noexcept -> SaturatingInteger;
    /// Calculate the modulo of another integer operand and keep this type as the result type.
    template <AnyIntegerType T>
    [[nodiscard]] auto modulo(T other) const noexcept -> SaturatingInteger;

public: // Named arithmetic modifying this value
    /// Add another integer operand and store the saturated result in this value.
    /// Unlike `operator+=`, this method accepts mixed signedness and cannot be chained accidentally.
    /// @tparam T The type of the other value.
    /// @param other The other value to perform the operation.
    template <AnyIntegerType T>
    void add(T other) noexcept;
    /// Subtract another integer operand and store the saturated result in this value.
    template <AnyIntegerType T>
    void subtract(T other) noexcept;
    /// Multiply another integer operand and store the saturated result in this value.
    template <AnyIntegerType T>
    void multiply(T other) noexcept;
    /// Divide another integer operand and store the saturated result in this value.
    template <AnyIntegerType T>
    void divide(T other) noexcept;
    /// Modulo with another integer operand and store the saturated result in this value.
    template <AnyIntegerType T>
    void applyModulo(T other) noexcept;
    /// Divide this value, store the quotient and return the remainder.
    /// This mirrors `std::div`, but works with every supported integer operand and saturates to this type.
    /// @tparam T The type of the divider.
    /// @param other The divider.
    /// @return The remainder of the division.
    template <AnyIntegerType T>
    [[nodiscard]] auto divideGetRemainder(T other) noexcept -> SaturatingInteger;
    /// Divide this value, store the remainder and return the quotient.
    /// This mirrors `std::div`, but works with every supported integer operand and saturates to this type.
    /// @tparam T The type of the divider.
    /// @param other The divider.
    /// @return The result of the division.
    template <AnyIntegerType T>
    [[nodiscard]] auto divideKeepRemainder(T other) noexcept -> SaturatingInteger;

public: // Static construction from operations
    /// Add two integer operands and saturate the result to this type.
    /// @tparam tFirst The first operand type.
    /// @tparam tSecond The second operand type.
    /// @param first The first value.
    /// @param second The second value.
    /// @return The saturated result using this type.
    template <AnyIntegerType tFirst, AnyIntegerType tSecond>
        requires SignCompatibleIntegerOperandPair<tValue, tFirst, tSecond>
    [[nodiscard]] static auto fromAddition(tFirst first, tSecond second) noexcept -> SaturatingInteger;
    /// Subtract two integer operands and saturate the result to this type.
    template <AnyIntegerType tFirst, AnyIntegerType tSecond>
        requires SignCompatibleIntegerOperandPair<tValue, tFirst, tSecond>
    [[nodiscard]] static auto fromSubtraction(tFirst first, tSecond second) noexcept -> SaturatingInteger;
    /// Multiply two integer operands and saturate the result to this type.
    template <AnyIntegerType tFirst, AnyIntegerType tSecond>
        requires SignCompatibleIntegerOperandPair<tValue, tFirst, tSecond>
    [[nodiscard]] static auto fromMultiplication(tFirst first, tSecond second) noexcept -> SaturatingInteger;
    /// Divide two integer operands and saturate the result to this type.
    template <AnyIntegerType tFirst, AnyIntegerType tSecond>
        requires SignCompatibleIntegerOperandPair<tValue, tFirst, tSecond>
    [[nodiscard]] static auto fromDivision(tFirst first, tSecond second) noexcept -> SaturatingInteger;
    /// Apply modulo to two integer operands and saturate the result to this type.
    template <AnyIntegerType tFirst, AnyIntegerType tSecond>
        requires SignCompatibleIntegerOperandPair<tValue, tFirst, tSecond>
    [[nodiscard]] static auto fromModulo(tFirst first, tSecond second) noexcept -> SaturatingInteger;
    /// Divide two operands and return quotient and remainder.
    /// @tparam tFirst The dividend type and tuple value type.
    /// @tparam tSecond The divisor type.
    /// @param first The value that is divided.
    /// @param second The divider.
    /// @return A tuple with quotient first and remainder second.
    template <AnyIntegerType tFirst, AnyIntegerType tSecond>
        requires SignCompatibleIntegerOperandPair<tValue, tFirst, tSecond>
    [[nodiscard]] static auto fromDivisionWithRemainder(tFirst first, tSecond second) noexcept
        -> std::tuple<SaturatingInteger, SaturatingInteger>;

public:
    /// Raise this value to an integer power.
    /// The implementation uses exponentiation by squaring and saturates once the result exceeds this type.
    /// A zero power always results in `1`. Negative powers always result in `0`.
    /// @tparam T The type of the power value.
    /// @param power The power value.
    /// @return The exponential value.
    template <AnyIntegerType T>
    [[nodiscard]] auto raisedTo(T power) const noexcept -> SaturatingInteger;

public: // saturation checks
    /// Test if adding another integer operand would saturate this value type.
    /// @tparam T The type of the other value.
    /// @param other The value to add.
    /// @return `true` if the addition would saturate.
    template <AnyIntegerType T>
    [[nodiscard]] auto wouldAddSaturate(T other) const noexcept -> bool;
    /// Test if subtracting another integer operand would saturate this value type.
    /// @tparam T The type of the other value.
    /// @param other The value to subtract.
    /// @return `true` if the subtraction would saturate.
    template <AnyIntegerType T>
    [[nodiscard]] auto wouldSubtractSaturate(T other) const noexcept -> bool;
    /// Test if multiplying by another integer operand would saturate this value type.
    /// @tparam T The type of the other value.
    /// @param other The factor.
    /// @return `true` if the multiplication would saturate.
    template <AnyIntegerType T>
    [[nodiscard]] auto wouldMultiplySaturate(T other) const noexcept -> bool;
    /// Test if dividing by another integer operand would saturate this value type.
    /// @tparam T The type of the other value.
    /// @param other The divisor.
    /// @return `true` if the division would saturate.
    template <AnyIntegerType T>
    [[nodiscard]] auto wouldDivideSaturate(T other) const noexcept -> bool;
    /// Test if a modulo operation with another integer operand would saturate this value type.
    /// @tparam T The type of the other value.
    /// @param other The divisor.
    /// @return `true` if the modulo operation would saturate.
    template <AnyIntegerType T>
    [[nodiscard]] auto wouldModuloSaturate(T other) const noexcept -> bool;

public: // accessors and conversion
    /// Cast this value to another supported integer type with saturation.
    /// @tparam T The target type for the cast.
    /// @return The saturated casted value.
    template <AnyIntegerType T>
    [[nodiscard]] constexpr auto cast() const noexcept -> SaturatingInteger<NativeIntegerOfT<T>>;
    /// Cast this value to another supported integer type.
    /// @tparam T The target type for the cast.
    /// @return The casted value.
    /// @throws err::OverflowError if the value cannot be represented by the target type.
    template <AnyIntegerType T>
    [[nodiscard]] constexpr auto castOrThrow() const -> SaturatingInteger<NativeIntegerOfT<T>>;
    /// Convert this saturating integer to a regular integer value.
    /// @return The native integer.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> NativeValue { return _value; }
    /// Convert this value to `std::size_t` with saturation.
    /// @return The saturated integer as `std::size_t`.
    [[nodiscard]] constexpr auto toSizeT() const noexcept -> std::size_t;
    /// Compare this value with another integer operand.
    /// @tparam T The type of the compared value.
    /// @param value The value to compare.
    /// @return The ordering of this value compared to `value`.
    template <AnyIntegerType T>
    [[nodiscard]] constexpr auto compare(T value) const noexcept -> std::strong_ordering;
    /// Clamp this value to a native integer range.
    /// If `minimum` > `maximum`, the behaviour is undefined.
    /// If `minimum` or `maximum` exceed the minimum/maximum value of the integer, they have no effect.
    /// @tparam T The type of the clamping integers.
    /// @param minimum The minimum value to clamp to.
    /// @param maximum The maximum value to clamp to.
    template <AnyIntegerType T>
    constexpr void clamp(T minimum, T maximum) noexcept;
    /// Clamp this value to a native integer range.
    /// @tparam T The native range value type.
    /// @param range The range to clamp to.
    template <NativeInteger T>
    constexpr void clamp(IntegerRange<T> range) noexcept;
    /// Return a new value clamped to a native integer range.
    /// If `minimum` > `maximum`, the behaviour is undefined.
    /// If `minimum` or `maximum` exceed the minimum/maximum value of the integer, they have no effect.
    /// @tparam T The type of the clamping integers.
    /// @param minimum The minimum value to clamp to.
    /// @param maximum The maximum value to clamp to.
    /// @return The new clamped value.
    template <AnyIntegerType T>
    [[nodiscard]] constexpr auto clamped(T minimum, T maximum) const noexcept -> SaturatingInteger;
    /// Return a new value clamped to a native integer range.
    /// @tparam T The native range value type.
    /// @param range The range to clamp to.
    /// @return The new clamped value.
    template <NativeInteger T>
    [[nodiscard]] constexpr auto clamped(IntegerRange<T> range) const noexcept -> SaturatingInteger;
    /// Wrap this value to a native integer range.
    /// Invalid direct bounds or a range outside this native type reset the value to zero.
    /// @tparam T The type of the range bounds.
    /// @param minimum The inclusive minimum value.
    /// @param maximum The inclusive maximum value.
    template <AnyIntegerType T>
    void wrap(T minimum, T maximum) noexcept;
    /// Wrap this value to a native integer range.
    /// A range outside this native type resets the value to zero.
    /// @tparam T The native range value type.
    /// @param range The wrap range.
    template <NativeInteger T>
    void wrap(IntegerRange<T> range) noexcept;
    /// Return this value wrapped to a native integer range.
    /// Invalid direct bounds or a range outside this native type return zero.
    /// @tparam T The type of the range bounds.
    /// @param minimum The inclusive minimum value.
    /// @param maximum The inclusive maximum value.
    /// @return The wrapped value.
    template <AnyIntegerType T>
    [[nodiscard]] auto wrapped(T minimum, T maximum) const noexcept -> SaturatingInteger;
    /// Return this value wrapped to a native integer range.
    /// A range outside this native type returns zero.
    /// @tparam T The native range value type.
    /// @param range The wrap range.
    /// @return The wrapped value.
    template <NativeInteger T>
    [[nodiscard]] auto wrapped(IntegerRange<T> range) const noexcept -> SaturatingInteger;
    /// Wrap this value to a native integer range and return the signed wrap count.
    /// Invalid direct bounds or a range outside this native type reset the value and return zero.
    /// @tparam T The type of the range bounds.
    /// @param minimum The inclusive minimum value.
    /// @param maximum The inclusive maximum value.
    /// @return The signed wrap count.
    template <AnyIntegerType T>
    [[nodiscard]] auto wrapAndCount(T minimum, T maximum) noexcept -> WrapCount;
    /// Wrap this value to a native integer range and return the signed wrap count.
    /// A range outside this native type resets the value and returns zero.
    /// @tparam T The native range value type.
    /// @param range The wrap range.
    /// @return The signed wrap count.
    template <NativeInteger T>
    [[nodiscard]] auto wrapAndCount(IntegerRange<T> range) noexcept -> WrapCount;
    /// Return this value wrapped to a native integer range and the signed wrap count.
    /// Invalid direct bounds or a range outside this native type return zero values.
    /// @tparam T The type of the range bounds.
    /// @param minimum The inclusive minimum value.
    /// @param maximum The inclusive maximum value.
    /// @return The wrapped value and signed wrap count.
    template <AnyIntegerType T>
    [[nodiscard]] auto wrappedAndCount(T minimum, T maximum) const noexcept -> std::tuple<SaturatingInteger, WrapCount>;
    /// Return this value wrapped to a native integer range and the signed wrap count.
    /// A range outside this native type returns zero values.
    /// @tparam T The native range value type.
    /// @param range The wrap range.
    /// @return The wrapped value and signed wrap count.
    template <NativeInteger T>
    [[nodiscard]] auto wrappedAndCount(IntegerRange<T> range) const noexcept
        -> std::tuple<SaturatingInteger, WrapCount>;
    /// Check if this value is zero.
    /// @return `true` if this value is zero.
    [[nodiscard]] constexpr auto isZero() const noexcept -> bool { return _value == 0; }
    /// Check if this value is one.
    /// @return `true` if this value is one.
    [[nodiscard]] constexpr auto isOne() const noexcept -> bool { return _value == 1; }
    /// Check if this value is negative.
    /// @return `true` if this value is negative.
    [[nodiscard]] constexpr auto isNegative() const noexcept -> bool;
    /// Check if this value is equal to the minimum value.
    /// @return `true` if this value is equal to the minimum value.
    [[nodiscard]] constexpr auto isMinimum() const noexcept -> bool { return _value == minimum().toRawValue(); }
    /// Check if this value is equal to the maximum value.
    /// @return `true` if this value is equal to the maximum value.
    [[nodiscard]] constexpr auto isMaximum() const noexcept -> bool { return _value == maximum().toRawValue(); }
    /// Get the absolute value saturated to this type.
    /// @note For signed integers, the minimum value saturates to the maximum positive value.
    /// @return The absolute value.
    [[nodiscard]] constexpr auto toAbsolute() const noexcept -> SaturatingInteger;
    /// Get the absolute value using the matching unsigned integer type.
    /// @return The absolute value.
    [[nodiscard]] constexpr auto toUnsignedAbsolute() const noexcept
        -> SaturatingInteger<std::make_unsigned_t<NativeValue>>;
    /// Get the negated value.
    /// Unsigned values saturate to zero. The minimum signed value saturates to the maximum positive value.
    /// @return The negated value.
    [[nodiscard]] constexpr auto negated() const noexcept -> SaturatingInteger;
    /// Negate this value in place.
    /// Unsigned values saturate to zero. The minimum signed value saturates to the maximum positive value.
    void negate() noexcept;

public: // helpers and compatibility
    /// Efficiently swap two saturating integers.
    friend void swap(SaturatingInteger &first, SaturatingInteger &second) noexcept {
        std::swap(first._value, second._value);
    }

public: // constants
    /// Get the maximum representable value.
    /// @return The maximum value for this saturating integer type.
    [[nodiscard]] static constexpr auto maximum() noexcept -> SaturatingInteger {
        return SaturatingInteger{std::numeric_limits<NativeValue>::max()};
    }
    /// Get the minimum representable value.
    /// @return The minimum value for this saturating integer type.
    [[nodiscard]] static constexpr auto minimum() noexcept -> SaturatingInteger {
        return SaturatingInteger{std::numeric_limits<NativeValue>::min()};
    }
    /// Get the range of all representable values.
    /// @return The native integer range for this type.
    [[nodiscard]] static constexpr auto range() noexcept -> IntegerRange<NativeValue> {
        return IntegerRange<NativeValue>{
            std::numeric_limits<NativeValue>::min(), std::numeric_limits<NativeValue>::max()};
    }
    /// Get the zero value.
    /// @return A zero value for this saturating integer type.
    [[nodiscard]] static constexpr auto zero() noexcept -> SaturatingInteger { return SaturatingInteger{}; }
    /// Get the number of bits of this integer.
    /// @return The number of bits in the native value type.
    [[nodiscard]] static constexpr auto bitCount() noexcept -> std::size_t { return sizeof(NativeValue) * 8; }
    /// Test if this is a signed integer.
    /// @return `true` if the native value type is signed.
    [[nodiscard]] static constexpr auto isSigned() noexcept -> bool { return std::signed_integral<NativeValue>; }

private:
    /// Convert an arbitrary integer into the native integer type without narrowing errors.
    template <AnyIntegerType T>
    [[nodiscard]] static constexpr auto convertToNativeInt(T value) noexcept -> NativeIntegerOfT<T>;

private:
    NativeValue _value{0}; ///< The underlying value of this integer.
};

/// A saturating signed integer with 8bit size.
using SatInt8 = SaturatingInteger<std::int8_t>;
/// A saturating signed integer with 16bit size.
using SatInt16 = SaturatingInteger<std::int16_t>;
/// A saturating signed integer with 32bit size.
using SatInt32 = SaturatingInteger<std::int32_t>;
/// A saturating signed integer with 64bit size.
using SatInt64 = SaturatingInteger<std::int64_t>;

/// A saturating unsigned integer with 8bit size.
using SatUInt8 = SaturatingInteger<std::uint8_t>;
/// A saturating unsigned integer with 16bit size.
using SatUInt16 = SaturatingInteger<std::uint16_t>;
/// A saturating unsigned integer with 32bit size.
using SatUInt32 = SaturatingInteger<std::uint32_t>;
/// A saturating unsigned integer with 64bit size.
using SatUInt64 = SaturatingInteger<std::uint64_t>;

}

#include "SaturatingInteger_arithmetic.tpp"
#include "SaturatingInteger_comparison.tpp"
#include "SaturatingInteger_construction.tpp"
#include "SaturatingInteger_conversion.tpp"
#include "SaturatingInteger_free_operators.tpp"
#include "SaturatingInteger_named.tpp"

template <erbsland::math::NativeInteger tValue>
struct std::hash<erbsland::math::SaturatingInteger<tValue>> {
    auto operator()(const erbsland::math::SaturatingInteger<tValue> &value) const noexcept -> std::size_t {
        return std::hash<typename erbsland::math::SaturatingInteger<tValue>::NativeValue>{}(value.toRawValue());
    }
};
