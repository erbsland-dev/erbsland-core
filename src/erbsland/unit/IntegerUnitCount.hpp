// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerUnitCount_fwd.hpp"
#include "IntegerUnitIndex_fwd.hpp"

#include "impl/Throw.hpp"
#include "impl/TypeTraits.hpp"

#include "../math/ConstexprSaturatingMath.hpp"
#include "../math/IntegerConversion.hpp"
#include "../math/IntegerTraits.hpp"
#include "../math/SaturatingMath.hpp"
#include "../util/impl/ComparisonHelper.hpp"
#include "../util/impl/LoopControl.hpp"
#include "../util/LoopResult.hpp"
#include "../util/LoopStatus.hpp"

#include <compare>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <type_traits>
#include <utility>

namespace erbsland::unit {

/// A non-negative count with an integer unit.
/// @seedoc{/reference/unit/units_and_versions}
/// @tparam tIntegerUnit The integer unit for this count.
/// @tested{IntegerUnitTest IntegerUnitRemainingTest}
template <impl::ValidIntegerUnit tIntegerUnit>
class IntegerUnitCount {
public:
    /// Tag for unchecked construction.
    struct Unchecked {};
    /// The raw unsigned integer type.
    using Value = uint64_t;
    /// The unit type.
    using Unit = tIntegerUnit;

    /// The raw value used for the special infinite length.
    static constexpr Value cRawInfinite = std::numeric_limits<Value>::max();
    /// The largest finite raw value.
    static constexpr Value cRawMaximum = cRawInfinite - 1U;

public:
    /// Create a zero length.
    constexpr IntegerUnitCount() noexcept = default;

    /// Construct a finite value from a compile-time integer.
    /// @param value An integer within the finite domain; invalid values fail compilation.
    template <math::NativeInteger T>
    explicit consteval IntegerUnitCount(T value) : _value{checkedInteger<T>(value)} {}
    /// Construct directly from a raw representation without conversion or range checks.
    /// The caller must ensure the input fits Value before conversion and represents the intended finite or special
    /// state.
    /// @param value The raw unsigned integer value.
    constexpr IntegerUnitCount(const Value value, Unchecked) noexcept : _value{value} {}

    // defaults
    ~IntegerUnitCount() = default;
    IntegerUnitCount(const IntegerUnitCount &) noexcept = default;
    constexpr auto operator=(const IntegerUnitCount &) noexcept -> IntegerUnitCount & = default;

public: // operators
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const IntegerUnitCount &other, other._value);

    /// Add another length and return the saturated result.
    [[nodiscard]] constexpr auto operator+(const IntegerUnitCount other) const noexcept -> IntegerUnitCount {
        return added(other);
    }
    /// Add another length to this length with saturation.
    constexpr auto operator+=(const IntegerUnitCount other) noexcept -> IntegerUnitCount & { return add(other); }
    /// Subtract another length and return the saturated result.
    [[nodiscard]] constexpr auto operator-(const IntegerUnitCount other) const noexcept -> IntegerUnitCount {
        return subtracted(other);
    }
    /// Subtract another length from this length with saturation.
    constexpr auto operator-=(const IntegerUnitCount other) noexcept -> IntegerUnitCount & { return subtract(other); }
    /// Multiply this length by an unsigned scalar and return the saturated result.
    template <math::AnyIntegerType T>
        requires std::unsigned_integral<math::NativeIntegerOfT<T>>
    [[nodiscard]] constexpr auto operator*(T scalar) const noexcept -> IntegerUnitCount {
        return multiplied(scalar);
    }
    /// Multiply this length by an unsigned scalar with saturation.
    template <math::AnyIntegerType T>
        requires std::unsigned_integral<math::NativeIntegerOfT<T>>
    constexpr auto operator*=(T scalar) noexcept -> IntegerUnitCount & {
        return multiply(scalar);
    }
    /// Divide this length by an unsigned scalar.
    template <math::AnyIntegerType T>
        requires std::unsigned_integral<math::NativeIntegerOfT<T>>
    [[nodiscard]] constexpr auto operator/(T scalar) const noexcept -> IntegerUnitCount {
        return divided(scalar);
    }
    /// Divide this length by an unsigned scalar.
    template <math::AnyIntegerType T>
        requires std::unsigned_integral<math::NativeIntegerOfT<T>>
    constexpr auto operator/=(T scalar) noexcept -> IntegerUnitCount & {
        return divide(scalar);
    }
    /// Calculate the modulo of this length and an unsigned scalar.
    template <math::AnyIntegerType T>
        requires std::unsigned_integral<math::NativeIntegerOfT<T>>
    [[nodiscard]] constexpr auto operator%(T scalar) const noexcept -> IntegerUnitCount {
        return modulo(scalar);
    }
    /// Apply modulo with an unsigned scalar.
    template <math::AnyIntegerType T>
        requires std::unsigned_integral<math::NativeIntegerOfT<T>>
    constexpr auto operator%=(T scalar) noexcept -> IntegerUnitCount & {
        return applyModulo(scalar);
    }
    /// Increment this length by one with saturation.
    constexpr auto operator++() noexcept -> IntegerUnitCount & { return increment(); }
    /// Increment this length by one with saturation and return the previous value.
    constexpr auto operator++(int) noexcept -> IntegerUnitCount {
        auto result = *this;
        increment();
        return result;
    }
    /// Decrement this length by one with saturation.
    constexpr auto operator--() noexcept -> IntegerUnitCount & { return decrement(); }
    /// Decrement this length by one with saturation and return the previous value.
    constexpr auto operator--(int) noexcept -> IntegerUnitCount {
        auto result = *this;
        decrement();
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
    /// Return the amount remaining from an index measured from zero.
    /// An absent index or a position at or beyond the end returns zero, including noIndex() for infinite amounts.
    /// @param index The inclusive beginning of the remaining amount.
    /// @return The remaining amount, or infinite for a valid index within an infinite amount.
    [[nodiscard]] constexpr auto remainingFrom(const IntegerUnitIndex<tIntegerUnit> index) const noexcept
        -> IntegerUnitCount {
        if (!index.isWithin(*this)) {
            return zero();
        }
        return subtracted(index.distanceFromZero());
    }
    /// Add another length, saturating at the largest finite length or infinite length.
    /// If either operand is infinite, the result is infinite.
    constexpr auto add(const IntegerUnitCount other) noexcept -> IntegerUnitCount & {
        *this = added(other);
        return *this;
    }
    /// Create a new length by adding another length to it.
    /// If either operand is infinite, the result is infinite.
    [[nodiscard]] constexpr auto added(const IntegerUnitCount other) const noexcept -> IntegerUnitCount {
        if (isInfinite() || other.isInfinite()) {
            return infinite();
        }
        if (other._value > cRawMaximum - _value) {
            return maximum();
        }
        return uncheckedAdded(other);
    }
    /// Add another length.
    /// @throws OverflowError if either operand is infinite or the finite result exceeds maximum().
    constexpr auto addOrThrow(const IntegerUnitCount other) -> IntegerUnitCount & {
        *this = addedOrThrow(other);
        return *this;
    }
    /// Create a new length by adding another length to it.
    /// @throws OverflowError if either operand is infinite or the finite result exceeds maximum().
    [[nodiscard]] constexpr auto addedOrThrow(const IntegerUnitCount other) const -> IntegerUnitCount {
        if (isInfinite() || other._value > cRawMaximum - _value) {
            impl::throwOverflow("Length addition would exceed finite length bounds");
        }
        return {static_cast<Value>(_value + other._value), Unchecked{}};
    }
    /// Subtract another length, saturating at zero.
    /// If this length is infinite, it remains infinite. Subtracting infinite from a finite length produces zero.
    constexpr auto subtract(const IntegerUnitCount other) noexcept -> IntegerUnitCount & {
        *this = subtracted(other);
        return *this;
    }
    /// Create a new length by subtracting another length from it.
    /// If this length is infinite, the result is infinite. Subtracting infinite from a finite length produces zero.
    [[nodiscard]] constexpr auto subtracted(const IntegerUnitCount other) const noexcept -> IntegerUnitCount {
        if (isInfinite()) {
            return infinite();
        }
        if (other._value > _value) {
            return zero();
        }
        return uncheckedSubtracted(other);
    }
    /// Subtract another length.
    /// @throws OverflowError if either operand is infinite or the finite result would be below zero.
    constexpr auto subtractOrThrow(const IntegerUnitCount other) -> IntegerUnitCount & {
        *this = subtractedOrThrow(other);
        return *this;
    }
    /// Create a new length by subtracting another length from it.
    /// @throws OverflowError if either operand is infinite or the finite result would be below zero.
    [[nodiscard]] constexpr auto subtractedOrThrow(const IntegerUnitCount other) const -> IntegerUnitCount {
        if (isInfinite() || other._value > _value) {
            impl::throwOverflow("Length subtraction would exceed finite length bounds");
        }
        return {static_cast<Value>(_value - other._value), Unchecked{}};
    }
    /// Multiply this length by an unsigned scalar with saturation.
    template <math::AnyIntegerType T>
        requires std::unsigned_integral<math::NativeIntegerOfT<T>>
    constexpr auto multiply(T scalar) noexcept -> IntegerUnitCount & {
        *this = multiplied(scalar);
        return *this;
    }
    /// Create a new length by multiplying this length with an unsigned scalar.
    /// Infinite lengths stay infinite.
    template <math::AnyIntegerType T>
        requires std::unsigned_integral<math::NativeIntegerOfT<T>>
    [[nodiscard]] constexpr auto multiplied(T scalar) const noexcept -> IntegerUnitCount {
        const auto scalarValue = scalarToRawValue(scalar);
        if (isInfinite()) {
            return infinite();
        }
        if (_value == Value{0U} || scalarValue == decltype(scalarValue){0U}) {
            return zero();
        }
        // The native limit lets compilers use a multiply-overflow instruction instead of division.
        if (_value > cRawInfinite / scalarValue) {
            return maximum();
        }
        const auto result = _value * scalarValue;
        if (result == cRawInfinite) {
            return maximum();
        }
        return {result, Unchecked{}};
    }
    /// Divide this length by an unsigned scalar.
    /// Infinite lengths stay infinite.
    template <math::AnyIntegerType T>
        requires std::unsigned_integral<math::NativeIntegerOfT<T>>
    constexpr auto divide(T scalar) noexcept -> IntegerUnitCount & {
        *this = divided(scalar);
        return *this;
    }
    /// Create a new length by dividing this length by an unsigned scalar.
    /// Infinite lengths stay infinite.
    template <math::AnyIntegerType T>
        requires std::unsigned_integral<math::NativeIntegerOfT<T>>
    [[nodiscard]] constexpr auto divided(T scalar) const noexcept -> IntegerUnitCount {
        const auto scalarValue = scalarToRawValue(scalar);
        if (scalarValue == decltype(scalarValue){0U}) {
            std::terminate();
        }
        if (isInfinite()) {
            return infinite();
        }
        return {_value / scalarValue, Unchecked{}};
    }
    /// Apply modulo with an unsigned scalar.
    /// Infinite lengths stay infinite.
    template <math::AnyIntegerType T>
        requires std::unsigned_integral<math::NativeIntegerOfT<T>>
    constexpr auto applyModulo(T scalar) noexcept -> IntegerUnitCount & {
        *this = modulo(scalar);
        return *this;
    }
    /// Calculate the modulo of this length and an unsigned scalar.
    /// Infinite lengths stay infinite.
    template <math::AnyIntegerType T>
        requires std::unsigned_integral<math::NativeIntegerOfT<T>>
    [[nodiscard]] constexpr auto modulo(T scalar) const noexcept -> IntegerUnitCount {
        const auto scalarValue = scalarToRawValue(scalar);
        if (scalarValue == decltype(scalarValue){0U}) {
            std::terminate();
        }
        if (isInfinite()) {
            return infinite();
        }
        return {_value % scalarValue, Unchecked{}};
    }

    /// Increment this length by one, preserving infinity and saturating at the finite bounds.
    constexpr auto increment() noexcept -> IntegerUnitCount & {
        if (_value < cRawMaximum) {
            ++_value;
        }
        return *this;
    }
    /// Decrement this length by one, preserving infinity and saturating at the finite bounds.
    constexpr auto decrement() noexcept -> IntegerUnitCount & {
        if (_value != 0U && !isInfinite()) {
            --_value;
        }
        return *this;
    }

public: // unchecked fast-track methods
    /// Add a length without checks; both operands and the result must be finite and representable.
    /// @param other The length to add.
    constexpr auto uncheckedAdd(const IntegerUnitCount other) noexcept -> IntegerUnitCount & {
        _value += other._value;
        return *this;
    }
    /// Return the exact result without checks; both operands and the result must be finite and representable.
    /// @param other The length to add.
    /// @return The exact result.
    [[nodiscard]] constexpr auto uncheckedAdded(const IntegerUnitCount other) const noexcept -> IntegerUnitCount {
        return {_value + other._value, Unchecked{}};
    }
    /// Subtract a length without checks; both operands and the result must be finite and representable.
    /// @param other The length to subtract.
    constexpr auto uncheckedSubtract(const IntegerUnitCount other) noexcept -> IntegerUnitCount & {
        _value -= other._value;
        return *this;
    }
    /// Return the exact result without checks; both operands and the result must be finite and representable.
    /// @param other The length to subtract.
    /// @return The exact result.
    [[nodiscard]] constexpr auto uncheckedSubtracted(const IntegerUnitCount other) const noexcept -> IntegerUnitCount {
        return {_value - other._value, Unchecked{}};
    }
    /// Increment without checks; this length and the result must be finite and representable.
    constexpr auto uncheckedIncrement() noexcept -> IntegerUnitCount & {
        ++_value;
        return *this;
    }
    /// Decrement without checks; this length and the result must be finite and representable.
    constexpr auto uncheckedDecrement() noexcept -> IntegerUnitCount & {
        --_value;
        return *this;
    }

public: // conversion methods
    /// Convert this length to `std::size_t`, saturating if the raw value is too large.
    [[nodiscard]] constexpr auto toSizeT() const noexcept -> std::size_t {
        if constexpr (sizeof(Value) > sizeof(std::size_t)) {
            return math::saturatingCast<std::size_t>(_value);
        } else {
            return static_cast<std::size_t>(_value);
        }
    }
    /// Convert this length to `std::size_t`.
    /// @throws OverflowError if the raw value does not fit into `std::size_t`.
    [[nodiscard]] constexpr auto toSizeTOrThrow() const -> std::size_t {
        if constexpr (sizeof(Value) > sizeof(std::size_t)) {
            if (math::willCastOverflow<std::size_t>(_value)) {
                impl::throwOverflow("Length value exceeds maximum std::size_t");
            }
        }
        return static_cast<std::size_t>(_value);
    }

public: // iteration
    /// Call a function for each index in this range.
    /// Raw index values are incremented internally, avoiding saturation checks for every step.
    /// @param loopFn The function to call with each index. It may return `void` or `util::LoopStatus`.
    /// @return The result of the loop.
    template <typename Fn>
    auto forEach(Fn &&loopFn) const -> util::LoopResult {
        if (isInfinite()) {
            return util::LoopResult::Error;
        }
        if (isZero()) {
            return util::LoopResult::Success;
        }
        const auto end = _value;
        for (auto index = Value{0U}; index < end; ++index) {
            auto current = IntegerUnitIndex<tIntegerUnit>{index, typename IntegerUnitIndex<tIntegerUnit>::Unchecked{}};
            if constexpr (std::is_void_v<std::invoke_result_t<Fn &, IntegerUnitIndex<tIntegerUnit>>>) {
                std::invoke(loopFn, std::move(current));
            } else {
                const auto status = util::impl::invokeLoopFunction(loopFn, std::move(current));
                if (status != util::LoopStatus::Continue) {
                    return util::impl::loopStatusToResult(status);
                }
            }
        }
        return util::LoopResult::Success;
    }

public: // factory methods
    /// Return the zero length.
    [[nodiscard]] constexpr static auto zero() noexcept -> IntegerUnitCount { return {0U, Unchecked{}}; }
    /// Return the length one.
    [[nodiscard]] constexpr static auto one() noexcept -> IntegerUnitCount { return {1U, Unchecked{}}; }
    /// Return the smallest finite length.
    [[nodiscard]] constexpr static auto minimum() noexcept -> IntegerUnitCount { return {0U, Unchecked{}}; }
    /// Return the largest finite length.
    [[nodiscard]] constexpr static auto maximum() noexcept -> IntegerUnitCount { return {cRawMaximum, Unchecked{}}; }
    /// Return the special infinite length.
    [[nodiscard]] constexpr static auto infinite() noexcept -> IntegerUnitCount { return {cRawInfinite, Unchecked{}}; }
    /// Create a finite value from a native size, saturating at maximum().
    [[nodiscard]] constexpr static auto fromSizeT(std::size_t value) noexcept -> IntegerUnitCount {
        return fromInteger(value);
    }
    /// Create a finite value from a native size.
    /// @throws err::OverflowError if the value exceeds maximum().
    [[nodiscard]] constexpr static auto fromSizeTOrThrow(std::size_t value) -> IntegerUnitCount {
        return fromIntegerOrThrow(value);
    }

    /// Convert an integer, saturating into the finite domain.
    /// @param value The integer to convert.
    /// @return The bounded finite value.
    template <math::NativeInteger T>
    [[nodiscard]] constexpr static auto fromInteger(T value) noexcept -> IntegerUnitCount {
        // NativeInteger guarantees max 64bit, therefore  T is same size or a smaller than Value.
        if constexpr (std::is_signed_v<T>) {
            if (value < 0) {
                return minimum();
            }
            return {static_cast<Value>(value), Unchecked{}};
        } else if constexpr (sizeof(T) < sizeof(Value)) {
            return {static_cast<Value>(value), Unchecked{}};
        } else {
            const auto raw = static_cast<Value>(value);
            if (raw > cRawMaximum) {
                return maximum();
            }
            return {raw, Unchecked{}};
        }
    }
    /// Convert an integer exactly into the finite domain.
    /// @param value The integer to convert.
    /// @return The finite value.
    /// @throws err::OverflowError if value is outside the finite domain.
    template <math::NativeInteger T>
    [[nodiscard]] constexpr static auto fromIntegerOrThrow(T value) -> IntegerUnitCount {
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
        const auto raw = static_cast<Value>(value);
        if (raw > cRawMaximum) {
            impl::throwOverflow("Integer is outside the unit's finite range");
        }
        return raw;
    }
    /// Convert a scalar to the unit's raw representation.
    template <math::AnyIntegerType T>
        requires std::unsigned_integral<math::NativeIntegerOfT<T>>
    [[nodiscard]] constexpr static auto scalarToRawValue(T scalar) noexcept -> Value {
        return static_cast<Value>(math::toNativeInteger(scalar));
    }

    Value _value{0U}; ///< The raw value of this length.
};

/// Multiply a unit amount by a scalar.
template <math::AnyIntegerType T, impl::ValidIntegerUnit tIntegerUnit>
    requires std::unsigned_integral<math::NativeIntegerOfT<T>>
[[nodiscard]] constexpr auto operator*(T scalar, IntegerUnitCount<tIntegerUnit> length) noexcept
    -> IntegerUnitCount<tIntegerUnit> {
    return length * scalar;
}

}

template <erbsland::unit::impl::ValidIntegerUnit tIntegerUnit>
struct std::hash<erbsland::unit::IntegerUnitCount<tIntegerUnit>> {
    auto operator()(const erbsland::unit::IntegerUnitCount<tIntegerUnit> &value) const noexcept -> std::size_t {
        return std::hash<typename erbsland::unit::IntegerUnitCount<tIntegerUnit>::Value>{}(value.toRawValue());
    }
};
