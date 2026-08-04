// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerUnitAmount_fwd.hpp"

#include "impl/Throw.hpp"
#include "impl/TypeTraits.hpp"

#include "../math/ConstexprSaturatingMath.hpp"
#include "../math/IntegerConversion.hpp"
#include "../math/IntegerTraits.hpp"
#include "../math/SaturatingMath.hpp"
#include "../util/impl/ComparisonHelper.hpp"

#include <compare>
#include <concepts>
#include <cstddef>
#include <limits>

namespace erbsland::unit {

/// A non-negative length with a integer unit.
/// @seedoc{/reference/unit/integer_unit_types}
/// @tparam tIntegerUnit The integer unit for this length.
template <impl::ValidIntegerUnit tIntegerUnit>
class IntegerUnitAmount {
public:
    /// The raw unsigned integer type.
    using Value = tIntegerUnit::AmountType;
    /// The unit type.
    using Unit = tIntegerUnit;

    /// The raw value used for the special infinite length.
    static constexpr Value cRawInfinite = std::numeric_limits<Value>::max();
    /// The largest finite raw value.
    static constexpr Value cRawMaximum = cRawInfinite - 1U;

public:
    /// Create a zero length.
    constexpr IntegerUnitAmount() noexcept = default;

    /// Create a length from a raw value.
    /// @param value The raw value. Passing cInfinite creates the special infinite length.
    explicit constexpr IntegerUnitAmount(Value value) noexcept : _value{value} {}

    // defaults
    ~IntegerUnitAmount() = default;
    IntegerUnitAmount(const IntegerUnitAmount &) noexcept = default;
    auto operator=(const IntegerUnitAmount &) noexcept -> IntegerUnitAmount & = default;

public: // operators
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const IntegerUnitAmount &other, other._value);

    /// Add another length and return the saturated result.
    [[nodiscard]] constexpr auto operator+(const IntegerUnitAmount other) const noexcept -> IntegerUnitAmount {
        return added(other);
    }
    /// Add another length to this length with saturation.
    auto operator+=(const IntegerUnitAmount other) noexcept -> IntegerUnitAmount & { return add(other); }
    /// Subtract another length and return the saturated result.
    [[nodiscard]] constexpr auto operator-(const IntegerUnitAmount other) const noexcept -> IntegerUnitAmount {
        return subtracted(other);
    }
    /// Subtract another length from this length with saturation.
    auto operator-=(const IntegerUnitAmount other) noexcept -> IntegerUnitAmount & { return subtract(other); }
    /// Multiply this length by an unsigned scalar and return the saturated result.
    template <math::AnyIntegerType T>
    [[nodiscard]] constexpr auto operator*(T scalar) const noexcept -> IntegerUnitAmount {
        return multiplied(scalar);
    }
    /// Multiply this length by an unsigned scalar with saturation.
    template <math::AnyIntegerType T>
    auto operator*=(T scalar) noexcept -> IntegerUnitAmount & {
        return multiply(scalar);
    }
    /// Divide this length by an unsigned scalar.
    template <math::AnyIntegerType T>
    [[nodiscard]] constexpr auto operator/(T scalar) const noexcept -> IntegerUnitAmount {
        return divided(scalar);
    }
    /// Divide this length by an unsigned scalar.
    template <math::AnyIntegerType T>
    auto operator/=(T scalar) noexcept -> IntegerUnitAmount & {
        return divide(scalar);
    }
    /// Calculate the modulo of this length and an unsigned scalar.
    template <math::AnyIntegerType T>
    [[nodiscard]] constexpr auto operator%(T scalar) const noexcept -> IntegerUnitAmount {
        return modulo(scalar);
    }
    /// Apply modulo with an unsigned scalar.
    template <math::AnyIntegerType T>
    auto operator%=(T scalar) noexcept -> IntegerUnitAmount & {
        return applyModulo(scalar);
    }
    /// Increment this length by one with saturation.
    auto operator++() noexcept -> IntegerUnitAmount & { return add(one()); }
    /// Increment this length by one with saturation and return the previous value.
    auto operator++(int) noexcept -> IntegerUnitAmount {
        auto result = *this;
        add(one());
        return result;
    }
    /// Decrement this length by one with saturation.
    auto operator--() noexcept -> IntegerUnitAmount & { return subtract(one()); }
    /// Decrement this length by one with saturation and return the previous value.
    auto operator--(int) noexcept -> IntegerUnitAmount {
        auto result = *this;
        subtract(one());
        return result;
    }

public: // tests
    /// Test if this length is zero.
    [[nodiscard]] constexpr auto isZero() const noexcept -> bool { return _value == 0U; }
    /// Test if this length is one.
    [[nodiscard]] constexpr auto isOne() const noexcept -> bool { return _value == 1U; }
    /// Test if this length has the minimum value.
    [[nodiscard]] constexpr auto isMinimum() const noexcept -> bool { return _value == 0U; }
    /// Test if this length has the largest finite value.
    [[nodiscard]] constexpr auto isMaximum() const noexcept -> bool { return _value == cRawMaximum; }
    /// Test if this length has the infinite value.
    [[nodiscard]] constexpr auto isInfinite() const noexcept -> bool { return _value == cRawInfinite; }
    /// Test if this length has a finite value.
    [[nodiscard]] constexpr auto isFinite() const noexcept -> bool { return !isInfinite(); }

public: // accessors and modifiers
    /// Access the raw value of this length.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> Value { return _value; }

public: // math
    /// Test if adding another length would saturate this length.
    [[nodiscard]] constexpr auto wouldAddSaturate(const IntegerUnitAmount other) const noexcept -> bool {
        return isInfinite() || other.isInfinite() ||
            math::willAddBoundedSaturate(_value, other._value, Value{0U}, cRawMaximum);
    }
    /// Add another length, saturating at the largest finite length or infinite length.
    /// If either operand is infinite, the result is infinite.
    auto add(const IntegerUnitAmount other) noexcept -> IntegerUnitAmount & {
        *this = added(other);
        return *this;
    }
    /// Create a new length by adding another length to it.
    /// If either operand is infinite, the result is infinite.
    [[nodiscard]] constexpr auto added(const IntegerUnitAmount other) const noexcept -> IntegerUnitAmount {
        if (isInfinite() || other.isInfinite()) {
            return infinite();
        }
        return IntegerUnitAmount{math::saturatingAddBounded(_value, other._value, Value{0U}, cRawMaximum)};
    }
    /// Add another length.
    /// @throws OverflowError if either operand is infinite or the finite result exceeds maximum().
    auto addOrThrow(const IntegerUnitAmount other) -> IntegerUnitAmount & {
        *this = addedOrThrow(other);
        return *this;
    }
    /// Create a new length by adding another length to it.
    /// @throws OverflowError if either operand is infinite or the finite result exceeds maximum().
    [[nodiscard]] constexpr auto addedOrThrow(const IntegerUnitAmount other) const -> IntegerUnitAmount {
        if (wouldAddSaturate(other)) {
            impl::throwOverflow("Length addition would exceed finite length bounds");
        }
        return IntegerUnitAmount{static_cast<Value>(_value + other._value)};
    }
    /// Test if subtracting another length would saturate this length.
    [[nodiscard]] constexpr auto wouldSubtractSaturate(const IntegerUnitAmount other) const noexcept -> bool {
        return isInfinite() || other.isInfinite() ||
            math::willSubtractBoundedSaturate(_value, other._value, Value{0U}, cRawMaximum);
    }
    /// Subtract another length, saturating at zero.
    /// If this length is infinite, it remains infinite. Subtracting infinite from a finite length produces zero.
    auto subtract(const IntegerUnitAmount other) noexcept -> IntegerUnitAmount & {
        *this = subtracted(other);
        return *this;
    }
    /// Create a new length by subtracting another length from it.
    /// If this length is infinite, the result is infinite. Subtracting infinite from a finite length produces zero.
    [[nodiscard]] constexpr auto subtracted(const IntegerUnitAmount other) const noexcept -> IntegerUnitAmount {
        if (isInfinite()) {
            return infinite();
        }
        if (other.isInfinite()) {
            return zero();
        }
        return IntegerUnitAmount{math::saturatingSubtractBounded(_value, other._value, Value{0U}, cRawMaximum)};
    }
    /// Subtract another length.
    /// @throws OverflowError if either operand is infinite or the finite result would be below zero.
    auto subtractOrThrow(const IntegerUnitAmount other) -> IntegerUnitAmount & {
        *this = subtractedOrThrow(other);
        return *this;
    }
    /// Create a new length by subtracting another length from it.
    /// @throws OverflowError if either operand is infinite or the finite result would be below zero.
    [[nodiscard]] constexpr auto subtractedOrThrow(const IntegerUnitAmount other) const -> IntegerUnitAmount {
        if (wouldSubtractSaturate(other)) {
            impl::throwOverflow("Length subtraction would exceed finite length bounds");
        }
        return IntegerUnitAmount{static_cast<Value>(_value - other._value)};
    }
    /// Test if a multiplication would saturate.
    template <math::AnyIntegerType T>
    [[nodiscard]] auto wouldMultiplySaturate(T scalar) const noexcept -> bool {
        return math::willMultiplyOverflow(_value, scalarToRawValue(scalar));
    }
    /// Multiply this length by an unsigned scalar with saturation.
    template <math::AnyIntegerType T>
    auto multiply(T scalar) noexcept -> IntegerUnitAmount & {
        *this = multiplied(scalar);
        return *this;
    }
    /// Create a new length by multiplying this length with an unsigned scalar.
    /// Infinite lengths stay infinite.
    template <math::AnyIntegerType T>
    [[nodiscard]] constexpr auto multiplied(T scalar) const noexcept -> IntegerUnitAmount {
        const auto scalarValue = scalarToRawValue(scalar);
        if (isInfinite()) {
            return infinite();
        }
        if (_value == Value{0U} || scalarValue == decltype(scalarValue){0U}) {
            return zero();
        }
        return IntegerUnitAmount{math::saturatingMultiplyBounded(_value, scalarValue, Value{0U}, cRawMaximum)};
    }
    /// Divide this length by an unsigned scalar.
    /// Infinite lengths stay infinite.
    template <math::AnyIntegerType T>
    auto divide(T scalar) noexcept -> IntegerUnitAmount & {
        *this = divided(scalar);
        return *this;
    }
    /// Create a new length by dividing this length by an unsigned scalar.
    /// Infinite lengths stay infinite.
    template <math::AnyIntegerType T>
    [[nodiscard]] constexpr auto divided(T scalar) const noexcept -> IntegerUnitAmount {
        const auto scalarValue = scalarToRawValue(scalar);
        if (scalarValue == decltype(scalarValue){0U}) {
            std::terminate();
        }
        if (isInfinite()) {
            return infinite();
        }
        return IntegerUnitAmount{math::saturatingDivideBounded(_value, scalarValue, Value{0U}, cRawMaximum)};
    }
    /// Apply modulo with an unsigned scalar.
    /// Infinite lengths stay infinite.
    template <math::AnyIntegerType T>
    auto applyModulo(T scalar) noexcept -> IntegerUnitAmount & {
        *this = modulo(scalar);
        return *this;
    }
    /// Calculate the modulo of this length and an unsigned scalar.
    /// Infinite lengths stay infinite.
    template <math::AnyIntegerType T>
    [[nodiscard]] constexpr auto modulo(T scalar) const noexcept -> IntegerUnitAmount {
        const auto scalarValue = scalarToRawValue(scalar);
        if (scalarValue == decltype(scalarValue){0U}) {
            std::terminate();
        }
        if (isInfinite()) {
            return infinite();
        }
        return IntegerUnitAmount{math::saturatingModuloBounded(_value, scalarValue, Value{0U}, cRawMaximum)};
    }

public: // conversion methods
    /// Convert this length to `std::size_t`, saturating if the raw value is too large.
    [[nodiscard]] constexpr auto toSizeT() const noexcept -> std::size_t {
        return math::saturatingCast<std::size_t>(_value);
    }
    /// Convert this length to `std::size_t`.
    /// @throws OverflowError if the raw value does not fit into `std::size_t`.
    [[nodiscard]] constexpr auto toSizeTOrThrow() const -> std::size_t {
        if (math::willCastOverflow<std::size_t>(_value)) {
            impl::throwOverflow("Length value exceeds maximum std::size_t");
        }
        return static_cast<std::size_t>(_value);
    }

public: // factory methods
    /// Return the zero length.
    [[nodiscard]] constexpr static auto zero() noexcept -> IntegerUnitAmount { return IntegerUnitAmount{0U}; }
    /// Return the length one.
    [[nodiscard]] constexpr static auto one() noexcept -> IntegerUnitAmount { return IntegerUnitAmount{1U}; }
    /// Return the smallest finite length.
    [[nodiscard]] constexpr static auto minimum() noexcept -> IntegerUnitAmount { return IntegerUnitAmount{0U}; }
    /// Return the largest finite length.
    [[nodiscard]] constexpr static auto maximum() noexcept -> IntegerUnitAmount {
        return IntegerUnitAmount{cRawMaximum};
    }
    /// Return the special infinite length.
    [[nodiscard]] constexpr static auto infinite() noexcept -> IntegerUnitAmount {
        return IntegerUnitAmount{cRawInfinite};
    }
    /// Create an index from a size_t value.
    /// Saturating if the value overflows.
    [[nodiscard]] constexpr static auto fromSizeT(const std::size_t value) noexcept -> IntegerUnitAmount {
        if constexpr (sizeof(Value) < sizeof(std::size_t)) {
            if (math::willCastOverflow<Value>(value)) {
                return maximum();
            }
        }
        return static_cast<Value>(value) > cRawMaximum ? maximum() : IntegerUnitAmount{static_cast<Value>(value)};
    }
    /// Create an index from a size_t value.
    /// @throws OverflowError if the size_t value exceeds the maximum index.
    [[nodiscard]] constexpr static auto fromSizeTOrThrow(const std::size_t value) -> IntegerUnitAmount {
        if constexpr (sizeof(Value) < sizeof(std::size_t)) {
            if (math::willCastOverflow<Value>(value)) {
                impl::throwOverflow("The size_t value exceeds maximum length.");
            }
        }
        if (static_cast<Value>(value) > cRawMaximum) {
            impl::throwOverflow("The size_t value exceeds maximum length.");
        }
        return IntegerUnitAmount{static_cast<Value>(value)};
    }

private:
    /// Convert a scalar to the unit's raw representation.
    template <math::AnyIntegerType T>
    [[nodiscard]] constexpr static auto scalarToRawValue(T scalar) noexcept -> Value {
        using Scalar = math::NativeIntegerOfT<T>;
        if constexpr (std::unsigned_integral<Scalar>) {
            return math::saturatingCast<Value>(math::toNativeInteger(scalar));
        } else {
            static_assert(
                std::unsigned_integral<Scalar>,
                "IntegerUnitAmount scalar arithmetic requires an unsigned integer scalar. Use 3U, std::size_t{3}, "
                "or an unsigned SaturatingInteger.");
            return Value{0U};
        }
    }

    Value _value{0U}; ///< The raw value of this length.
};

/// Multiply a unit amount by a scalar.
template <math::AnyIntegerType T, impl::ValidIntegerUnit tIntegerUnit>
[[nodiscard]] constexpr auto operator*(T scalar, IntegerUnitAmount<tIntegerUnit> length) noexcept
    -> IntegerUnitAmount<tIntegerUnit> {
    return length * scalar;
}

}

template <erbsland::unit::impl::ValidIntegerUnit tIntegerUnit>
struct std::hash<erbsland::unit::IntegerUnitAmount<tIntegerUnit>> {
    auto operator()(const erbsland::unit::IntegerUnitAmount<tIntegerUnit> &value) const noexcept -> std::size_t {
        return std::hash<typename erbsland::unit::IntegerUnitAmount<tIntegerUnit>::Value>{}(value.toRawValue());
    }
};
