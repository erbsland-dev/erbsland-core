// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerRange.hpp"
#include "SaturatingInteger.hpp"

#include <compare>
#include <limits>

namespace erbsland::math {

/// Integer value that is always bound to a fixed inclusive range.
/// Arithmetic saturates at the configured range bounds.
/// @tested{ClampedIntegerTest}
template <
    NativeInteger tValue,
    tValue tMinimum = std::numeric_limits<tValue>::min(),
    tValue tMaximum = std::numeric_limits<tValue>::max(),
    tValue tDefault = tMinimum>
class BoundedInteger {
    static_assert(tMinimum <= tMaximum);
    static_assert(tDefault >= tMinimum && tDefault <= tMaximum);

public:
    /// The native integer type.
    using NativeValue = tValue;
    /// The internal saturating value type.
    using SaturatingValue = SaturatingInteger<NativeValue>;

public:
    /// Create the configured default value.
    constexpr BoundedInteger() noexcept : _value{tDefault} {}
    /// Create a value clamped to the configured range.
    template <AnyIntegerType T>
    explicit constexpr BoundedInteger(T value) noexcept : _value{clampedRawValue(value)} {}

    // defaults
    ~BoundedInteger() = default;
    BoundedInteger(const BoundedInteger &) noexcept = default;
    auto operator=(const BoundedInteger &) noexcept -> BoundedInteger & = default;
    BoundedInteger(BoundedInteger &&) noexcept = default;
    auto operator=(BoundedInteger &&) noexcept -> BoundedInteger & = default;

public: // operators
    /// Compare two clamped integers.
    [[nodiscard]] constexpr auto operator<=>(const BoundedInteger &other) const noexcept -> std::strong_ordering {
        return _value.compare(other._value);
    }
    /// Test if two clamped integers contain the same value.
    [[nodiscard]] constexpr auto operator==(const BoundedInteger &other) const noexcept -> bool {
        return _value == other._value;
    }
    /// Add another clamped integer with range saturation.
    [[nodiscard]] auto operator+(const BoundedInteger &other) const noexcept -> BoundedInteger {
        return added(other._value);
    }
    /// Add another clamped integer in place with range saturation.
    auto operator+=(const BoundedInteger &other) noexcept -> BoundedInteger & {
        add(other._value);
        return *this;
    }
    /// Subtract another clamped integer with range saturation.
    [[nodiscard]] auto operator-(const BoundedInteger &other) const noexcept -> BoundedInteger {
        return subtracted(other._value);
    }
    /// Subtract another clamped integer in place with range saturation.
    auto operator-=(const BoundedInteger &other) noexcept -> BoundedInteger & {
        subtract(other._value);
        return *this;
    }
    /// Add an integer operand with range saturation.
    template <AnyIntegerType T>
    [[nodiscard]] auto operator+(T other) const noexcept -> BoundedInteger {
        return added(other);
    }
    /// Add an integer operand in place with range saturation.
    template <AnyIntegerType T>
    auto operator+=(T other) noexcept -> BoundedInteger & {
        add(other);
        return *this;
    }
    /// Subtract an integer operand with range saturation.
    template <AnyIntegerType T>
    [[nodiscard]] auto operator-(T other) const noexcept -> BoundedInteger {
        return subtracted(other);
    }
    /// Subtract an integer operand in place with range saturation.
    template <AnyIntegerType T>
    auto operator-=(T other) noexcept -> BoundedInteger & {
        subtract(other);
        return *this;
    }
    /// Increment with range saturation.
    auto operator++() noexcept -> BoundedInteger & {
        increment();
        return *this;
    }
    /// Increment with range saturation.
    auto operator++(int) noexcept -> BoundedInteger {
        auto result = *this;
        increment();
        return result;
    }
    /// Decrement with range saturation.
    auto operator--() noexcept -> BoundedInteger & {
        decrement();
        return *this;
    }
    /// Decrement with range saturation.
    auto operator--(int) noexcept -> BoundedInteger {
        auto result = *this;
        decrement();
        return result;
    }

public: // comparison with integers
    /// Compare with an integer operand.
    template <AnyIntegerType T>
    [[nodiscard]] constexpr auto compare(T other) const noexcept -> std::strong_ordering {
        return _value.compare(other);
    }
    /// Test if this value equals an integer operand.
    template <AnyIntegerType T>
    [[nodiscard]] constexpr auto operator==(T other) const noexcept -> bool {
        return _value == other;
    }
    /// Test if this value differs from an integer operand.
    template <AnyIntegerType T>
    [[nodiscard]] constexpr auto operator!=(T other) const noexcept -> bool {
        return _value != other;
    }
    /// Test if this value is less than an integer operand.
    template <AnyIntegerType T>
    [[nodiscard]] constexpr auto operator<(T other) const noexcept -> bool {
        return _value < other;
    }
    /// Test if this value is less than or equal to an integer operand.
    template <AnyIntegerType T>
    [[nodiscard]] constexpr auto operator<=(T other) const noexcept -> bool {
        return _value <= other;
    }
    /// Test if this value is greater than an integer operand.
    template <AnyIntegerType T>
    [[nodiscard]] constexpr auto operator>(T other) const noexcept -> bool {
        return _value > other;
    }
    /// Test if this value is greater than or equal to an integer operand.
    template <AnyIntegerType T>
    [[nodiscard]] constexpr auto operator>=(T other) const noexcept -> bool {
        return _value >= other;
    }

public: // arithmetic
    /// Return this value plus `other`, clamped to the configured range.
    template <AnyIntegerType T>
    [[nodiscard]] auto added(T other) const noexcept -> BoundedInteger {
        return BoundedInteger{_value.added(other)};
    }
    /// Add `other` in place, clamped to the configured range.
    template <AnyIntegerType T>
    void add(T other) noexcept {
        _value = _value.added(other).clamped(tMinimum, tMaximum);
    }
    /// Return this value minus `other`, clamped to the configured range.
    template <AnyIntegerType T>
    [[nodiscard]] auto subtracted(T other) const noexcept -> BoundedInteger {
        return BoundedInteger{_value.subtracted(other)};
    }
    /// Subtract `other` in place, clamped to the configured range.
    template <AnyIntegerType T>
    void subtract(T other) noexcept {
        _value = _value.subtracted(other).clamped(tMinimum, tMaximum);
    }
    /// Return this value incremented by one, clamped to the configured range.
    [[nodiscard]] auto incremented() const noexcept -> BoundedInteger { return added(NativeValue{1}); }
    /// Increment this value in place, clamped to the configured range.
    void increment() noexcept { add(NativeValue{1}); }
    /// Return this value decremented by one, clamped to the configured range.
    [[nodiscard]] auto decremented() const noexcept -> BoundedInteger { return subtracted(NativeValue{1}); }
    /// Decrement this value in place, clamped to the configured range.
    void decrement() noexcept { subtract(NativeValue{1}); }

public: // tests
    /// Check if this value is the configured minimum.
    [[nodiscard]] constexpr auto isMinimum() const noexcept -> bool { return _value == tMinimum; }
    /// Check if this value is the configured maximum.
    [[nodiscard]] constexpr auto isMaximum() const noexcept -> bool { return _value == tMaximum; }

public: // conversion
    /// Return the raw native integer value.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> NativeValue { return _value.toRawValue(); }
    /// Return the value as a saturating integer.
    [[nodiscard]] constexpr auto toValue() const noexcept -> SaturatingValue { return _value; }

public:
    /// Return the configured minimum as a raw value.
    [[nodiscard]] constexpr static auto minimumRawValue() noexcept -> NativeValue { return tMinimum; }
    /// Return the configured maximum as a raw value.
    [[nodiscard]] constexpr static auto maximumRawValue() noexcept -> NativeValue { return tMaximum; }
    /// Return the configured minimum value.
    [[nodiscard]] constexpr static auto minimum() noexcept -> BoundedInteger { return BoundedInteger{tMinimum}; }
    /// Return the configured maximum value.
    [[nodiscard]] constexpr static auto maximum() noexcept -> BoundedInteger { return BoundedInteger{tMaximum}; }
    /// Return the range accepted by this type.
    [[nodiscard]] constexpr static auto range() noexcept -> IntegerRange<NativeValue> {
        return IntegerRange<NativeValue>{tMinimum, tMaximum};
    }
    /// Test if `value` is inside the configured range.
    template <AnyIntegerType T>
    [[nodiscard]] constexpr static auto contains(T value) noexcept -> bool {
        return range().contains(value);
    }

protected:
    /// Assign a value known to be in range.
    constexpr void setRawValue(NativeValue value) noexcept { _value = SaturatingValue{value}; }

private:
    /// Clamp an arbitrary integer to this type's configured native range.
    template <AnyIntegerType T>
    [[nodiscard]] constexpr static auto clampedRawValue(T value) noexcept -> NativeValue {
        return range().clamped(value);
    }

private:
    SaturatingValue _value; ///< The clamped integer value.
};

}
