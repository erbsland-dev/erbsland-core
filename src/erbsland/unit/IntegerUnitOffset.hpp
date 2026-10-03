// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerUnitCount.hpp"
#include "IntegerUnitOffset_fwd.hpp"

#include "impl/Throw.hpp"
#include "impl/TypeTraits.hpp"

#include "../math/ConstexprSaturatingMath.hpp"
#include "../math/IntegerConversion.hpp"
#include "../math/IntegerMath.hpp"
#include "../math/IntegerTraits.hpp"
#include "../math/SaturatingMath.hpp"
#include "../util/impl/ComparisonHelper.hpp"

#include <compare>
#include <concepts>
#include <cstdint>
#include <exception>
#include <limits>

namespace erbsland::unit {

/// A signed offset with a unit tag.
///
/// An offset describes movement relative to an index. Positive offsets move forward, negative offsets move backward.
/// Use IntegerUnitCount for non-negative spans and counts.
///
/// @tparam tIntegerUnit The unit tag for this offset.
/// @tested{IntegerUnitTest}
template <impl::ValidIntegerUnit tIntegerUnit>
class IntegerUnitOffset {
public:
    /// Tag for unchecked construction.
    struct Unchecked {};
    /// The raw signed integer type.
    using Value = int64_t;
    /// The unit type.
    using Unit = tIntegerUnit;
    /// The matching count type.
    using Count = IntegerUnitCount<tIntegerUnit>;

    /// The smallest raw offset value.
    static constexpr Value cRawMinimum = std::numeric_limits<Value>::min();
    /// The largest raw offset value.
    static constexpr Value cRawMaximum = std::numeric_limits<Value>::max();

public:
    /// Create a zero offset.
    constexpr IntegerUnitOffset() noexcept = default;
    /// Construct a finite value from a compile-time integer.
    /// @param value An integer within the finite domain; invalid values fail compilation.
    template <math::NativeInteger T>
    explicit consteval IntegerUnitOffset(T value) : _value{checkedInteger<T>(value)} {}
    /// Construct directly from a raw representation without conversion or range checks.
    /// The caller must ensure the input fits Value before conversion and represents the intended finite or special
    /// state.
    /// @param value The raw signed integer value.
    constexpr IntegerUnitOffset(const Value value, Unchecked) noexcept : _value{value} {}

    // defaults
    ~IntegerUnitOffset() = default;
    IntegerUnitOffset(const IntegerUnitOffset &) noexcept = default;
    constexpr auto operator=(const IntegerUnitOffset &) noexcept -> IntegerUnitOffset & = default;

public: // operators
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const IntegerUnitOffset &other, other._value);

    /// Add another offset and return the saturated result.
    [[nodiscard]] constexpr auto operator+(const IntegerUnitOffset other) const noexcept -> IntegerUnitOffset {
        return added(other);
    }
    /// Add another offset to this offset with saturation.
    constexpr auto operator+=(const IntegerUnitOffset other) noexcept -> IntegerUnitOffset & { return add(other); }
    /// Subtract another offset and return the saturated result.
    [[nodiscard]] constexpr auto operator-(const IntegerUnitOffset other) const noexcept -> IntegerUnitOffset {
        return subtracted(other);
    }
    /// Subtract another offset from this offset with saturation.
    constexpr auto operator-=(const IntegerUnitOffset other) noexcept -> IntegerUnitOffset & { return subtract(other); }
    /// Multiply this offset by a signed scalar and return the saturated result.
    template <math::AnyIntegerType T>
        requires std::signed_integral<math::NativeIntegerOfT<T>>
    [[nodiscard]] constexpr auto operator*(T scalar) const noexcept -> IntegerUnitOffset {
        return multiplied(scalar);
    }
    /// Multiply this offset by a signed scalar with saturation.
    template <math::AnyIntegerType T>
        requires std::signed_integral<math::NativeIntegerOfT<T>>
    constexpr auto operator*=(T scalar) noexcept -> IntegerUnitOffset & {
        return multiply(scalar);
    }
    /// Divide this offset by a signed scalar.
    template <math::AnyIntegerType T>
        requires std::signed_integral<math::NativeIntegerOfT<T>>
    [[nodiscard]] constexpr auto operator/(T scalar) const noexcept -> IntegerUnitOffset {
        return divided(scalar);
    }
    /// Divide this offset by a signed scalar.
    template <math::AnyIntegerType T>
        requires std::signed_integral<math::NativeIntegerOfT<T>>
    constexpr auto operator/=(T scalar) noexcept -> IntegerUnitOffset & {
        return divide(scalar);
    }
    /// Calculate the modulo of this offset and a signed scalar.
    template <math::AnyIntegerType T>
        requires std::signed_integral<math::NativeIntegerOfT<T>>
    [[nodiscard]] constexpr auto operator%(T scalar) const noexcept -> IntegerUnitOffset {
        return modulo(scalar);
    }
    /// Apply modulo with a signed scalar.
    template <math::AnyIntegerType T>
        requires std::signed_integral<math::NativeIntegerOfT<T>>
    constexpr auto operator%=(T scalar) noexcept -> IntegerUnitOffset & {
        return applyModulo(scalar);
    }
    /// Return this offset with the sign inverted, saturating minimum() to maximum().
    [[nodiscard]] constexpr auto operator-() const noexcept -> IntegerUnitOffset { return negated(); }
    /// Increment this offset by one with saturation.
    constexpr auto operator++() noexcept -> IntegerUnitOffset & {
        if (_value != cRawMaximum) {
            ++_value;
        }
        return *this;
    }
    /// Increment this offset by one with saturation and return the previous value.
    constexpr auto operator++(int) noexcept -> IntegerUnitOffset {
        auto result = *this;
        ++*this;
        return result;
    }
    /// Decrement this offset by one with saturation.
    constexpr auto operator--() noexcept -> IntegerUnitOffset & {
        if (_value != cRawMinimum) {
            --_value;
        }
        return *this;
    }
    /// Decrement this offset by one with saturation and return the previous value.
    constexpr auto operator--(int) noexcept -> IntegerUnitOffset {
        auto result = *this;
        --*this;
        return result;
    }

public: // tests
    /// Test if this offset is zero.
    [[nodiscard]] constexpr auto isZero() const noexcept -> bool { return _value == 0; }
    /// Test if this offset is one.
    [[nodiscard]] constexpr auto isOne() const noexcept -> bool { return _value == 1; }
    /// Test if this offset is negative one.
    [[nodiscard]] constexpr auto isMinusOne() const noexcept -> bool { return _value == -1; }
    /// Test if this offset is less than zero.
    [[nodiscard]] constexpr auto isNegative() const noexcept -> bool { return _value < 0; }
    /// Test if this offset is greater than zero.
    [[nodiscard]] constexpr auto isPositive() const noexcept -> bool { return _value > 0; }
    /// Test if this offset has the minimum value.
    [[nodiscard]] constexpr auto isMinimum() const noexcept -> bool { return _value == cRawMinimum; }
    /// Test if this offset has the maximum value.
    [[nodiscard]] constexpr auto isMaximum() const noexcept -> bool { return _value == cRawMaximum; }

public: // accessors and modifiers
    /// Access the raw value of this offset.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> Value { return _value; }
    /// Convert this offset into the matching non-negative length.
    [[nodiscard]] constexpr auto absoluteLength() const noexcept -> Count {
        return Count{math::toUnsignedAbsolute(_value), typename Count::Unchecked{}};
    }

public: // math
    /// Add another offset, saturating at minimum() or maximum().
    constexpr auto add(const IntegerUnitOffset other) noexcept -> IntegerUnitOffset & {
        *this = added(other);
        return *this;
    }
    /// Create a new offset by adding another offset to it.
    [[nodiscard]] constexpr auto added(const IntegerUnitOffset other) const noexcept -> IntegerUnitOffset {
        if (other._value > 0 && _value > cRawMaximum - other._value) {
            return maximum();
        }
        if (other._value < 0 && _value < cRawMinimum - other._value) {
            return minimum();
        }
        return {_value + other._value, Unchecked{}};
    }
    /// Add another offset.
    /// @throws OverflowError if the result would exceed minimum() or maximum().
    constexpr auto addOrThrow(const IntegerUnitOffset other) -> IntegerUnitOffset & {
        *this = addedOrThrow(other);
        return *this;
    }
    /// Create a new offset by adding another offset to it.
    /// @throws OverflowError if the result would exceed minimum() or maximum().
    [[nodiscard]] constexpr auto addedOrThrow(const IntegerUnitOffset other) const -> IntegerUnitOffset {
        if ((other._value > 0 && _value > cRawMaximum - other._value) ||
            (other._value < 0 && _value < cRawMinimum - other._value)) {
            impl::throwOverflow("Offset addition would exceed offset bounds");
        }
        return {static_cast<Value>(_value + other._value), Unchecked{}};
    }
    /// Subtract another offset, saturating at minimum() or maximum().
    constexpr auto subtract(const IntegerUnitOffset other) noexcept -> IntegerUnitOffset & {
        *this = subtracted(other);
        return *this;
    }
    /// Create a new offset by subtracting another offset from it.
    [[nodiscard]] constexpr auto subtracted(const IntegerUnitOffset other) const noexcept -> IntegerUnitOffset {
        if (other._value > 0 && _value < cRawMinimum + other._value) {
            return minimum();
        }
        if (other._value < 0 && _value > cRawMaximum + other._value) {
            return maximum();
        }
        return {_value - other._value, Unchecked{}};
    }
    /// Subtract another offset.
    /// @throws OverflowError if the result would exceed minimum() or maximum().
    constexpr auto subtractOrThrow(const IntegerUnitOffset other) -> IntegerUnitOffset & {
        *this = subtractedOrThrow(other);
        return *this;
    }
    /// Create a new offset by subtracting another offset from it.
    /// @throws OverflowError if the result would exceed minimum() or maximum().
    [[nodiscard]] constexpr auto subtractedOrThrow(const IntegerUnitOffset other) const -> IntegerUnitOffset {
        if ((other._value > 0 && _value < cRawMinimum + other._value) ||
            (other._value < 0 && _value > cRawMaximum + other._value)) {
            impl::throwOverflow("Offset subtraction would exceed offset bounds");
        }
        return {static_cast<Value>(_value - other._value), Unchecked{}};
    }
    /// Multiply this offset by a signed scalar with saturation.
    template <math::AnyIntegerType T>
        requires std::signed_integral<math::NativeIntegerOfT<T>>
    constexpr auto multiply(T scalar) noexcept -> IntegerUnitOffset & {
        *this = multiplied(scalar);
        return *this;
    }
    /// Create a new offset by multiplying this offset with a signed scalar.
    template <math::AnyIntegerType T>
        requires std::signed_integral<math::NativeIntegerOfT<T>>
    [[nodiscard]] constexpr auto multiplied(T scalar) const noexcept -> IntegerUnitOffset {
        const auto scalarValue = scalarToRawValue(scalar);
        return {math::saturatingMultiplyBounded(_value, scalarValue, cRawMinimum, cRawMaximum), Unchecked{}};
    }
    /// Divide this offset by a signed scalar.
    template <math::AnyIntegerType T>
        requires std::signed_integral<math::NativeIntegerOfT<T>>
    constexpr auto divide(T scalar) noexcept -> IntegerUnitOffset & {
        *this = divided(scalar);
        return *this;
    }
    /// Create a new offset by dividing this offset by a signed scalar.
    template <math::AnyIntegerType T>
        requires std::signed_integral<math::NativeIntegerOfT<T>>
    [[nodiscard]] constexpr auto divided(T scalar) const noexcept -> IntegerUnitOffset {
        const auto scalarValue = scalarToRawValue(scalar);
        if (scalarValue == decltype(scalarValue){0}) {
            std::terminate();
        }
        if (isMinimum() && scalarValue == -1) {
            return maximum();
        }
        return {_value / scalarValue, Unchecked{}};
    }
    /// Apply modulo with a signed scalar.
    template <math::AnyIntegerType T>
        requires std::signed_integral<math::NativeIntegerOfT<T>>
    constexpr auto applyModulo(T scalar) noexcept -> IntegerUnitOffset & {
        *this = modulo(scalar);
        return *this;
    }
    /// Calculate the modulo of this offset and a signed scalar.
    template <math::AnyIntegerType T>
        requires std::signed_integral<math::NativeIntegerOfT<T>>
    [[nodiscard]] constexpr auto modulo(T scalar) const noexcept -> IntegerUnitOffset {
        const auto scalarValue = scalarToRawValue(scalar);
        if (scalarValue == decltype(scalarValue){0}) {
            std::terminate();
        }
        if (scalarValue == -1) {
            return zero();
        }
        return {_value % scalarValue, Unchecked{}};
    }
    /// Create a new offset with the sign inverted, saturating the minimum value to maximum().
    [[nodiscard]] constexpr auto negated() const noexcept -> IntegerUnitOffset {
        if (isMinimum()) {
            return maximum();
        }
        return {-_value, Unchecked{}};
    }

public: // factory methods
    /// Return the zero offset.
    [[nodiscard]] constexpr static auto zero() noexcept -> IntegerUnitOffset { return {0, Unchecked{}}; }
    /// Return the offset one.
    [[nodiscard]] constexpr static auto one() noexcept -> IntegerUnitOffset { return {1, Unchecked{}}; }
    /// Return the offset negative one.
    [[nodiscard]] constexpr static auto minusOne() noexcept -> IntegerUnitOffset { return {-1, Unchecked{}}; }
    /// Return the smallest offset.
    [[nodiscard]] constexpr static auto minimum() noexcept -> IntegerUnitOffset { return {cRawMinimum, Unchecked{}}; }
    /// Return the largest offset.
    [[nodiscard]] constexpr static auto maximum() noexcept -> IntegerUnitOffset { return {cRawMaximum, Unchecked{}}; }
    /// Convert an integer, saturating into the finite domain.
    /// @param value The integer to convert.
    /// @return The bounded finite value.
    template <math::NativeInteger T>
    [[nodiscard]] constexpr static auto fromInteger(T value) noexcept -> IntegerUnitOffset {
        const auto raw = math::saturatingCast<Value>(value);
        return {static_cast<Value>(raw), Unchecked{}};
    }
    /// Convert an integer exactly into the finite domain.
    /// @param value The integer to convert.
    /// @return The finite value.
    /// @throws err::OverflowError if value is outside the finite domain.
    template <math::NativeInteger T>
    [[nodiscard]] constexpr static auto fromIntegerOrThrow(T value) -> IntegerUnitOffset {
        return {checkedInteger<T>(value), Unchecked{}};
    }

private:
    /// Check if an integer is in the range of this unit.
    /// @param value The integer to check.
    /// @return The checked value.
    /// @throws err::OverflowError if value is outside the finite domain.
    template <math::NativeInteger T>
    [[nodiscard]] constexpr static auto checkedInteger(T value) -> Value {
        if (math::willCastOverflow<Value>(value)) {
            impl::throwOverflow("Integer is outside the unit's finite range");
        }
        return static_cast<Value>(value);
    }
    /// Convert a scalar to the unit's raw representation.
    template <math::AnyIntegerType T>
        requires std::signed_integral<math::NativeIntegerOfT<T>>
    [[nodiscard]] constexpr static auto scalarToRawValue(T scalar) noexcept -> Value {
        return static_cast<Value>(math::toNativeInteger(scalar));
    }

    Value _value{0}; ///< The raw value of this offset.
};

/// Multiply a unit offset by a scalar.
template <math::AnyIntegerType T, impl::ValidIntegerUnit tIntegerUnit>
    requires std::signed_integral<math::NativeIntegerOfT<T>>
[[nodiscard]] constexpr auto operator*(T scalar, IntegerUnitOffset<tIntegerUnit> offset) noexcept
    -> IntegerUnitOffset<tIntegerUnit> {
    return offset * scalar;
}

}

template <erbsland::unit::impl::ValidIntegerUnit tIntegerUnit>
struct std::hash<erbsland::unit::IntegerUnitOffset<tIntegerUnit>> {
    auto operator()(const erbsland::unit::IntegerUnitOffset<tIntegerUnit> &value) const noexcept -> std::size_t {
        return std::hash<typename erbsland::unit::IntegerUnitOffset<tIntegerUnit>::Value>{}(value.toRawValue());
    }
};
