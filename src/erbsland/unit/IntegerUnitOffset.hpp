// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerUnitAmount.hpp"
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
#include <exception>
#include <limits>

namespace erbsland::unit {

/// A signed offset with a unit tag.
///
/// An offset describes movement relative to an index. Positive offsets move forward, negative offsets move backward.
/// Use IntegerUnitAmount for non-negative spans and counts.
///
/// @tparam tIntegerUnit The unit tag for this offset.
template <impl::ValidIntegerUnit tIntegerUnit>
class IntegerUnitOffset {
public:
    /// The raw signed integer type.
    using Value = tIntegerUnit::OffsetType;
    /// The unit type.
    using Unit = tIntegerUnit;
    /// The matching length type.
    using Length = IntegerUnitAmount<tIntegerUnit>;

    /// The smallest raw offset value.
    static constexpr Value cRawMinimum = std::numeric_limits<Value>::min();
    /// The largest raw offset value.
    static constexpr Value cRawMaximum = std::numeric_limits<Value>::max();

public:
    /// Create a zero offset.
    constexpr IntegerUnitOffset() noexcept = default;
    /// Create an offset from a raw value.
    explicit constexpr IntegerUnitOffset(Value value) noexcept : _value{value} {}

    // defaults
    ~IntegerUnitOffset() = default;
    IntegerUnitOffset(const IntegerUnitOffset &) noexcept = default;
    auto operator=(const IntegerUnitOffset &) noexcept -> IntegerUnitOffset & = default;

public: // operators
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const IntegerUnitOffset &other, other._value);

    /// Add another offset and return the saturated result.
    [[nodiscard]] constexpr auto operator+(const IntegerUnitOffset other) const noexcept -> IntegerUnitOffset {
        return added(other);
    }
    /// Add another offset to this offset with saturation.
    auto operator+=(const IntegerUnitOffset other) noexcept -> IntegerUnitOffset & { return add(other); }
    /// Subtract another offset and return the saturated result.
    [[nodiscard]] constexpr auto operator-(const IntegerUnitOffset other) const noexcept -> IntegerUnitOffset {
        return subtracted(other);
    }
    /// Subtract another offset from this offset with saturation.
    auto operator-=(const IntegerUnitOffset other) noexcept -> IntegerUnitOffset & { return subtract(other); }
    /// Multiply this offset by a signed scalar and return the saturated result.
    template <math::AnyIntegerType T>
    [[nodiscard]] constexpr auto operator*(T scalar) const noexcept -> IntegerUnitOffset {
        return multiplied(scalar);
    }
    /// Multiply this offset by a signed scalar with saturation.
    template <math::AnyIntegerType T>
    auto operator*=(T scalar) noexcept -> IntegerUnitOffset & {
        return multiply(scalar);
    }
    /// Divide this offset by a signed scalar.
    template <math::AnyIntegerType T>
    [[nodiscard]] constexpr auto operator/(T scalar) const noexcept -> IntegerUnitOffset {
        return divided(scalar);
    }
    /// Divide this offset by a signed scalar.
    template <math::AnyIntegerType T>
    auto operator/=(T scalar) noexcept -> IntegerUnitOffset & {
        return divide(scalar);
    }
    /// Calculate the modulo of this offset and a signed scalar.
    template <math::AnyIntegerType T>
    [[nodiscard]] constexpr auto operator%(T scalar) const noexcept -> IntegerUnitOffset {
        return modulo(scalar);
    }
    /// Apply modulo with a signed scalar.
    template <math::AnyIntegerType T>
    auto operator%=(T scalar) noexcept -> IntegerUnitOffset & {
        return applyModulo(scalar);
    }
    /// Return this offset with the sign inverted, saturating minimum() to maximum().
    [[nodiscard]] constexpr auto operator-() const noexcept -> IntegerUnitOffset { return negated(); }
    /// Increment this offset by one with saturation.
    auto operator++() noexcept -> IntegerUnitOffset & { return add(one()); }
    /// Increment this offset by one with saturation and return the previous value.
    auto operator++(int) noexcept -> IntegerUnitOffset {
        auto result = *this;
        add(one());
        return result;
    }
    /// Decrement this offset by one with saturation.
    auto operator--() noexcept -> IntegerUnitOffset & { return subtract(one()); }
    /// Decrement this offset by one with saturation and return the previous value.
    auto operator--(int) noexcept -> IntegerUnitOffset {
        auto result = *this;
        subtract(one());
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
    [[nodiscard]] constexpr auto absoluteLength() const noexcept -> Length {
        return Length{math::toUnsignedAbsolute(_value)};
    }

public: // math
    /// Test if adding another offset would saturate this offset.
    [[nodiscard]] constexpr auto wouldAddSaturate(const IntegerUnitOffset other) const noexcept -> bool {
        return math::willAddBoundedSaturate(_value, other._value, cRawMinimum, cRawMaximum);
    }
    /// Add another offset, saturating at minimum() or maximum().
    auto add(const IntegerUnitOffset other) noexcept -> IntegerUnitOffset & {
        *this = added(other);
        return *this;
    }
    /// Create a new offset by adding another offset to it.
    [[nodiscard]] constexpr auto added(const IntegerUnitOffset other) const noexcept -> IntegerUnitOffset {
        return IntegerUnitOffset{math::saturatingAddBounded(_value, other._value, cRawMinimum, cRawMaximum)};
    }
    /// Add another offset.
    /// @throws OverflowError if the result would exceed minimum() or maximum().
    auto addOrThrow(const IntegerUnitOffset other) -> IntegerUnitOffset & {
        *this = addedOrThrow(other);
        return *this;
    }
    /// Create a new offset by adding another offset to it.
    /// @throws OverflowError if the result would exceed minimum() or maximum().
    [[nodiscard]] constexpr auto addedOrThrow(const IntegerUnitOffset other) const -> IntegerUnitOffset {
        if (wouldAddSaturate(other)) {
            impl::throwOverflow("Offset addition would exceed offset bounds");
        }
        return IntegerUnitOffset{static_cast<Value>(_value + other._value)};
    }
    /// Test if subtracting another offset would saturate this offset.
    [[nodiscard]] constexpr auto wouldSubtractSaturate(const IntegerUnitOffset other) const noexcept -> bool {
        return math::willSubtractBoundedSaturate(_value, other._value, cRawMinimum, cRawMaximum);
    }
    /// Subtract another offset, saturating at minimum() or maximum().
    auto subtract(const IntegerUnitOffset other) noexcept -> IntegerUnitOffset & {
        *this = subtracted(other);
        return *this;
    }
    /// Create a new offset by subtracting another offset from it.
    [[nodiscard]] constexpr auto subtracted(const IntegerUnitOffset other) const noexcept -> IntegerUnitOffset {
        return IntegerUnitOffset{math::saturatingSubtractBounded(_value, other._value, cRawMinimum, cRawMaximum)};
    }
    /// Subtract another offset.
    /// @throws OverflowError if the result would exceed minimum() or maximum().
    auto subtractOrThrow(const IntegerUnitOffset other) -> IntegerUnitOffset & {
        *this = subtractedOrThrow(other);
        return *this;
    }
    /// Create a new offset by subtracting another offset from it.
    /// @throws OverflowError if the result would exceed minimum() or maximum().
    [[nodiscard]] constexpr auto subtractedOrThrow(const IntegerUnitOffset other) const -> IntegerUnitOffset {
        if (wouldSubtractSaturate(other)) {
            impl::throwOverflow("Offset subtraction would exceed offset bounds");
        }
        return IntegerUnitOffset{static_cast<Value>(_value - other._value)};
    }
    /// Multiply this offset by a signed scalar with saturation.
    template <math::AnyIntegerType T>
    auto multiply(T scalar) noexcept -> IntegerUnitOffset & {
        *this = multiplied(scalar);
        return *this;
    }
    /// Create a new offset by multiplying this offset with a signed scalar.
    template <math::AnyIntegerType T>
    [[nodiscard]] constexpr auto multiplied(T scalar) const noexcept -> IntegerUnitOffset {
        const auto scalarValue = scalarToRawValue(scalar);
        return IntegerUnitOffset{math::saturatingMultiplyBounded(_value, scalarValue, cRawMinimum, cRawMaximum)};
    }
    /// Divide this offset by a signed scalar.
    template <math::AnyIntegerType T>
    auto divide(T scalar) noexcept -> IntegerUnitOffset & {
        *this = divided(scalar);
        return *this;
    }
    /// Create a new offset by dividing this offset by a signed scalar.
    template <math::AnyIntegerType T>
    [[nodiscard]] constexpr auto divided(T scalar) const noexcept -> IntegerUnitOffset {
        const auto scalarValue = scalarToRawValue(scalar);
        if (scalarValue == decltype(scalarValue){0}) {
            std::terminate();
        }
        return IntegerUnitOffset{math::saturatingDivideBounded(_value, scalarValue, cRawMinimum, cRawMaximum)};
    }
    /// Apply modulo with a signed scalar.
    template <math::AnyIntegerType T>
    auto applyModulo(T scalar) noexcept -> IntegerUnitOffset & {
        *this = modulo(scalar);
        return *this;
    }
    /// Calculate the modulo of this offset and a signed scalar.
    template <math::AnyIntegerType T>
    [[nodiscard]] constexpr auto modulo(T scalar) const noexcept -> IntegerUnitOffset {
        const auto scalarValue = scalarToRawValue(scalar);
        if (scalarValue == decltype(scalarValue){0}) {
            std::terminate();
        }
        return IntegerUnitOffset{math::saturatingModuloBounded(_value, scalarValue, cRawMinimum, cRawMaximum)};
    }
    /// Create a new offset with the sign inverted, saturating the minimum value to maximum().
    [[nodiscard]] constexpr auto negated() const noexcept -> IntegerUnitOffset {
        return IntegerUnitOffset{math::saturatingNegateBounded(_value, cRawMinimum, cRawMaximum)};
    }

public: // factory methods
    /// Return the zero offset.
    [[nodiscard]] constexpr static auto zero() noexcept -> IntegerUnitOffset { return IntegerUnitOffset{0}; }
    /// Return the offset one.
    [[nodiscard]] constexpr static auto one() noexcept -> IntegerUnitOffset { return IntegerUnitOffset{1}; }
    /// Return the offset negative one.
    [[nodiscard]] constexpr static auto minusOne() noexcept -> IntegerUnitOffset { return IntegerUnitOffset{-1}; }
    /// Return the smallest offset.
    [[nodiscard]] constexpr static auto minimum() noexcept -> IntegerUnitOffset {
        return IntegerUnitOffset{cRawMinimum};
    }
    /// Return the largest offset.
    [[nodiscard]] constexpr static auto maximum() noexcept -> IntegerUnitOffset {
        return IntegerUnitOffset{cRawMaximum};
    }

private:
    /// Convert a scalar to the unit's raw representation.
    template <math::AnyIntegerType T>
    [[nodiscard]] constexpr static auto scalarToRawValue(T scalar) noexcept -> Value {
        using Scalar = math::NativeIntegerOfT<T>;
        if constexpr (std::signed_integral<Scalar>) {
            return math::saturatingCast<Value>(math::toNativeInteger(scalar));
        } else {
            static_assert(
                std::signed_integral<Scalar>,
                "IntegerUnitOffset scalar arithmetic requires a signed integer scalar. Use 3, int64_t{3}, or a signed "
                "SaturatingInteger.");
            return Value{0};
        }
    }

    Value _value{0}; ///< The raw value of this offset.
};

/// Multiply a unit offset by a scalar.
template <math::AnyIntegerType T, impl::ValidIntegerUnit tIntegerUnit>
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
