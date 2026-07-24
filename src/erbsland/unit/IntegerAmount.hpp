// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerAmount_fwd.hpp"

#include "impl/IntegerAmountTraits.hpp"
#include "impl/Throw.hpp"

#include "../math/SaturatingInteger.hpp"
#include "../util/impl/ComparisonHelper.hpp"

#include <compare>
#include <cstdint>
#include <functional>
#include <ratio>
#include <type_traits>

namespace erbsland::unit {

/// A signed integer amount in a ratio-scaled unit.
///
/// This type is intended for values such as seconds, milliseconds, meters, or kilometers where signed arithmetic,
/// ratio conversion, and saturating integer behavior are desired.
/// @seedoc{/reference/unit/integer_unit_types}
/// @tparam tUnit The unit tag that prevents accidental mixing with unrelated amounts.
/// @tparam tRatio The ratio of this amount to its base unit.
/// @tparam tValue The signed saturating integer type used to store the amount.
/// @tested{IntegerAmountTest}
template <typename tUnit, typename tRatio, typename tValue>
class IntegerAmount {
    static_assert(std::is_class_v<tUnit>, "The unit tag must be a class or struct type.");
    static_assert(
        impl::ValidIntegerAmountRatio<tRatio>,
        "The ratio must be a positive std::ratio type where either numerator or denominator is one.");
    static_assert(
        impl::ValidIntegerAmountValue<tValue>, "IntegerAmount requires a signed saturating integer value type.");

public:
    /// The unit tag.
    using Unit = tUnit;
    /// The ratio to the base unit.
    using Ratio = tRatio;
    /// The saturating integer value type.
    using Value = tValue;
    /// The native signed integer type wrapped by `Value`.
    using NativeValue = math::NativeIntegerOfT<Value>;

    /// Marker used by concepts to identify integer amount types.
    static constexpr auto cIsIntegerAmount = true;

public:
    /// Create a zero amount.
    constexpr IntegerAmount() noexcept = default;
    /// Create an amount from a saturating integer value.
    explicit constexpr IntegerAmount(Value value) noexcept : _value{value} {}
    /// Create an amount from a native signed integer value.
    explicit constexpr IntegerAmount(NativeValue value) noexcept : _value{value} {}
    /// Do not create signed amounts from unsigned integer operands.
    template <math::AnyIntegerType tOther>
        requires std::unsigned_integral<math::NativeIntegerOfT<tOther>>
    explicit IntegerAmount(tOther value) noexcept = delete;
    /// Create an amount from another signed integer operand, saturating if needed.
    template <impl::SignedIntegerAmountOperand tOther>
        requires(
            !std::same_as<std::remove_cvref_t<tOther>, Value> &&
            !std::same_as<std::remove_cvref_t<tOther>, NativeValue>)
    explicit constexpr IntegerAmount(tOther value) noexcept : _value{valueFromInteger(value)} {}

    // defaults
    ~IntegerAmount() = default;
    IntegerAmount(const IntegerAmount &) noexcept = default;
    auto operator=(const IntegerAmount &) noexcept -> IntegerAmount & = default;
    IntegerAmount(IntegerAmount &&) noexcept = default;
    auto operator=(IntegerAmount &&) noexcept -> IntegerAmount & = default;

public: // operators
    /// Compare this amount with another amount of the exact same type.
    [[nodiscard]] constexpr auto operator<=>(const IntegerAmount &other) const noexcept -> std::strong_ordering {
        return compare(other);
    }
    ERBSLAND_CORE_CONSTEXPR_COMPARE_FROM_SPACESHIP(const IntegerAmount &other, other);

    /// Add another amount, saturating on overflow.
    [[nodiscard]] auto operator+(IntegerAmount other) const noexcept -> IntegerAmount {
        return IntegerAmount{_value.added(other._value)};
    }
    /// Add another amount in place, saturating on overflow.
    auto operator+=(IntegerAmount other) noexcept -> IntegerAmount & {
        _value.add(other._value);
        return *this;
    }
    /// Subtract another amount, saturating on underflow.
    [[nodiscard]] auto operator-(IntegerAmount other) const noexcept -> IntegerAmount {
        return IntegerAmount{_value.subtracted(other._value)};
    }
    /// Subtract another amount in place, saturating on underflow.
    auto operator-=(IntegerAmount other) noexcept -> IntegerAmount & {
        _value.subtract(other._value);
        return *this;
    }
    /// Return the negated amount, saturating the minimum value to the maximum value.
    [[nodiscard]] auto operator-() const noexcept -> IntegerAmount { return negated(); }
    /// Increment this amount by one, saturating at maximum().
    auto operator++() noexcept -> IntegerAmount & {
        ++_value;
        return *this;
    }
    /// Increment this amount by one and return the previous value.
    auto operator++(int) noexcept -> IntegerAmount {
        auto result = *this;
        ++*this;
        return result;
    }
    /// Decrement this amount by one, saturating at minimum().
    auto operator--() noexcept -> IntegerAmount & {
        --_value;
        return *this;
    }
    /// Decrement this amount by one and return the previous value.
    auto operator--(int) noexcept -> IntegerAmount {
        auto result = *this;
        --*this;
        return result;
    }
    /// Multiply this amount by a signed integer factor.
    template <impl::SignedIntegerAmountOperand tFactor>
    [[nodiscard]] auto operator*(tFactor factor) const noexcept -> IntegerAmount {
        return IntegerAmount{_value.multiplied(factor)};
    }
    /// Multiply a signed integer factor by an amount.
    template <impl::SignedIntegerAmountOperand tFactor>
    friend auto operator*(tFactor factor, IntegerAmount amount) noexcept -> IntegerAmount {
        return amount * factor;
    }
    /// Multiply this amount in place by a signed integer factor.
    template <impl::SignedIntegerAmountOperand tFactor>
    auto operator*=(tFactor factor) noexcept -> IntegerAmount & {
        _value.multiply(factor);
        return *this;
    }
    /// Divide this amount by a signed integer divisor.
    template <impl::SignedIntegerAmountOperand tDivisor>
    [[nodiscard]] auto operator/(tDivisor divisor) const noexcept -> IntegerAmount {
        return IntegerAmount{_value.divided(divisor)};
    }
    /// Divide this amount in place by a signed integer divisor.
    template <impl::SignedIntegerAmountOperand tDivisor>
    auto operator/=(tDivisor divisor) noexcept -> IntegerAmount & {
        _value.divide(divisor);
        return *this;
    }
    /// Modulo this amount by a signed integer factor.
    template <impl::SignedIntegerAmountOperand tModulus>
    [[nodiscard]] auto operator%(tModulus modulus) const noexcept -> IntegerAmount {
        return IntegerAmount{_value.modulo(modulus)};
    }
    /// Modulo an amount by another amount.
    template <impl::SignedIntegerAmountOperand tModulus>
    auto operator%=(tModulus modulus) noexcept -> IntegerAmount & {
        _value.applyModulo(modulus);
        return *this;
    }

public: // tests
    /// Test if this amount is zero.
    [[nodiscard]] constexpr auto isZero() const noexcept -> bool { return _value.isZero(); }
    /// Test if this amount is one.
    [[nodiscard]] constexpr auto isOne() const noexcept -> bool { return _value.isOne(); }
    /// Test if this amount is minus one.
    [[nodiscard]] constexpr auto isMinusOne() const noexcept -> bool { return _value == NativeValue{-1}; }
    /// Test if this amount is positive.
    [[nodiscard]] constexpr auto isPositive() const noexcept -> bool { return _value > NativeValue{0}; }
    /// Test if this amount is negative.
    [[nodiscard]] constexpr auto isNegative() const noexcept -> bool { return _value.isNegative(); }
    /// Test if this amount has the minimum representable value.
    [[nodiscard]] constexpr auto isMinimum() const noexcept -> bool { return _value.isMinimum(); }
    /// Test if this amount has the maximum representable value.
    [[nodiscard]] constexpr auto isMaximum() const noexcept -> bool { return _value.isMaximum(); }

public: // accessors
    /// Access the raw native signed integer value.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> NativeValue { return _value.toRawValue(); }
    /// Access the saturating integer value.
    [[nodiscard]] constexpr auto toValue() const noexcept -> Value { return _value; }
    /// Compare this amount with another amount of the exact same type.
    [[nodiscard]] constexpr auto compare(const IntegerAmount &other) const noexcept -> std::strong_ordering {
        return _value.compare(other._value);
    }

public: // conversion
    /// Convert this amount to the base unit for its unit tag.
    [[nodiscard]] auto toBaseUnit() const noexcept -> IntegerAmount<Unit, std::ratio<1>, Value> {
        return converted<IntegerAmount<Unit, std::ratio<1>, Value>>();
    }
    /// Convert this amount to the base unit for its unit tag.
    /// @throws err::OverflowError if the conversion would saturate.
    [[nodiscard]] auto toBaseUnitOrThrow() const -> IntegerAmount<Unit, std::ratio<1>, Value> {
        return convertedOrThrow<IntegerAmount<Unit, std::ratio<1>, Value>>();
    }
    /// Test if conversion to another ratio with the same unit tag would saturate.
    template <typename tTarget>
        requires impl::CompatibleIntegerAmount<tTarget, Unit> &&
        impl::SimpleIntegerAmountRatio<std::ratio_divide<Ratio, typename tTarget::Ratio>>
    [[nodiscard]] auto wouldConvertSaturate() const noexcept -> bool {
        using ConversionRatio = std::ratio_divide<Ratio, typename tTarget::Ratio>;
        using TargetValue = typename tTarget::Value;
        auto result = TargetValue{_value};
        if (Value{result} != _value) {
            return true;
        }
        if constexpr (ConversionRatio::num != 1) {
            if (result.wouldMultiplySaturate(ConversionRatio::num)) {
                return true;
            }
        }
        return false;
    }
    /// Convert this amount to another ratio with the same unit tag.
    template <typename tTarget>
        requires impl::CompatibleIntegerAmount<tTarget, Unit> &&
        impl::SimpleIntegerAmountRatio<std::ratio_divide<Ratio, typename tTarget::Ratio>>
    [[nodiscard]] auto converted() const noexcept -> tTarget {
        using ConversionRatio = std::ratio_divide<Ratio, typename tTarget::Ratio>;
        using TargetValue = typename tTarget::Value;
        auto result = TargetValue{_value};
        if constexpr (ConversionRatio::num != 1) {
            result.multiply(ConversionRatio::num);
        }
        if constexpr (ConversionRatio::den != 1) {
            result.divide(ConversionRatio::den);
        }
        return tTarget{result};
    }
    /// Convert this amount to another ratio with the same unit tag.
    /// @throws err::OverflowError if the conversion would saturate.
    template <typename tTarget>
        requires impl::CompatibleIntegerAmount<tTarget, Unit> &&
        impl::SimpleIntegerAmountRatio<std::ratio_divide<Ratio, typename tTarget::Ratio>>
    [[nodiscard]] auto convertedOrThrow() const -> tTarget {
        if (wouldConvertSaturate<tTarget>()) {
            impl::throwOverflow("Integer amount conversion would exceed target bounds");
        }
        return converted<tTarget>();
    }
    /// Compatibility alias for `converted`.
    template <typename tTarget>
        requires impl::CompatibleIntegerAmount<tTarget, Unit> &&
        impl::SimpleIntegerAmountRatio<std::ratio_divide<Ratio, typename tTarget::Ratio>>
    [[nodiscard]] auto convert() const noexcept -> tTarget {
        return converted<tTarget>();
    }
    /// Compatibility alias for `convertedOrThrow`.
    /// @throws err::OverflowError if the conversion would saturate.
    template <typename tTarget>
        requires impl::CompatibleIntegerAmount<tTarget, Unit> &&
        impl::SimpleIntegerAmountRatio<std::ratio_divide<Ratio, typename tTarget::Ratio>>
    [[nodiscard]] auto convertOrThrow() const -> tTarget {
        return convertedOrThrow<tTarget>();
    }

public: // manipulation
    /// Extract a larger whole unit and keep the signed remainder in this amount.
    template <typename tTarget>
        requires impl::CompatibleIntegerAmount<tTarget, Unit> &&
        (std::ratio_divide<typename tTarget::Ratio, Ratio>::den == 1) &&
        (std::ratio_divide<typename tTarget::Ratio, Ratio>::num > 1)
    [[nodiscard]] auto extract() noexcept -> tTarget {
        using ExtractionRatio = std::ratio_divide<typename tTarget::Ratio, Ratio>;
        return tTarget{_value.divideKeepRemainder(ExtractionRatio::num)};
    }
    /// Return the negated amount, saturating the minimum value to the maximum value.
    [[nodiscard]] auto negated() const noexcept -> IntegerAmount { return IntegerAmount{_value.negated()}; }
    /// Negate this amount in place, saturating the minimum value to the maximum value.
    void negate() noexcept { _value.negate(); }
    /// Return this amount clamped to the given inclusive range.
    [[nodiscard]] auto clamped(IntegerAmount first, IntegerAmount last) const noexcept -> IntegerAmount {
        return IntegerAmount{_value.clamped(first._value, last._value)};
    }
    /// Clamp this amount to the given inclusive range.
    void clamp(IntegerAmount first, IntegerAmount last) noexcept { _value.clamp(first._value, last._value); }
    /// Return the absolute value of this amount.
    [[nodiscard]] auto toAbsolute() const noexcept -> IntegerAmount { return IntegerAmount{_value.toAbsolute()}; }

public: // factory methods
    /// Return the zero amount.
    [[nodiscard]] static constexpr auto zero() noexcept -> IntegerAmount { return IntegerAmount{}; }
    /// Return the amount one.
    [[nodiscard]] static constexpr auto one() noexcept -> IntegerAmount { return IntegerAmount{Value{1}}; }
    /// Return the amount minus one.
    [[nodiscard]] static constexpr auto minusOne() noexcept -> IntegerAmount { return IntegerAmount{Value{-1}}; }
    /// Return the smallest representable amount.
    [[nodiscard]] static constexpr auto minimum() noexcept -> IntegerAmount { return IntegerAmount{Value::minimum()}; }
    /// Return the largest representable amount.
    [[nodiscard]] static constexpr auto maximum() noexcept -> IntegerAmount { return IntegerAmount{Value::maximum()}; }
    /// Return the ratio numerator.
    [[nodiscard]] static constexpr auto ratioNumerator() noexcept -> std::intmax_t { return Ratio::num; }
    /// Return the ratio denominator.
    [[nodiscard]] static constexpr auto ratioDenominator() noexcept -> std::intmax_t { return Ratio::den; }
    /// Test if this amount is measured in the base unit.
    [[nodiscard]] static constexpr auto isBaseUnit() noexcept -> bool {
        return std::ratio_equal_v<Ratio, std::ratio<1>>;
    }

private:
    template <impl::SignedIntegerAmountOperand tOther>
    [[nodiscard]] static constexpr auto valueFromInteger(tOther value) noexcept -> Value {
        return Value{value};
    }

private:
    Value _value{}; ///< The saturating amount value.
};

}

template <typename tUnit, typename tRatio, erbsland::unit::impl::ValidIntegerAmountValue tValue>
struct std::hash<erbsland::unit::IntegerAmount<tUnit, tRatio, tValue>> {
    auto operator()(const erbsland::unit::IntegerAmount<tUnit, tRatio, tValue> &value) const noexcept -> std::size_t {
        return std::hash<typename erbsland::unit::IntegerAmount<tUnit, tRatio, tValue>::NativeValue>{}(
            value.toRawValue());
    }
};
