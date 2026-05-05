// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../math/BoundedInteger.hpp"
#include "../../util/impl/ComparisonHelper.hpp"

#include <compare>
#include <cstdint>

namespace erbsland::time::impl {

/// A clamped integer base for date/time components.
///
/// This CRTP base class wraps a bounded integer with a defined range and provides arithmetic operations that clamp
/// to the valid range. It is an implementation detail for concrete public time part types.
/// @tested{TimeCoreTest}
template <typename tDerived, typename tValue, tValue tMinimum, tValue tMaximum, tValue tDefault = tMinimum>
class TimePart : public math::BoundedInteger<tValue, tMinimum, tMaximum, tDefault> {
    using Base = math::BoundedInteger<tValue, tMinimum, tMaximum, tDefault>;

public:
    using Value = tValue;
    using SaturatingValue = Base::SaturatingValue;

public:
    /// Create the default component value.
    constexpr TimePart() noexcept = default;
    /// Create a clamped component value, clamping `value` to the supported range.
    template <math::AnyIntegerType T>
    explicit constexpr TimePart(T value) noexcept : Base{value} {}

public: // operators
    /// Compare two components using three-way comparison.
    friend auto operator<=>(const tDerived &left, const tDerived &right) noexcept -> std::strong_ordering {
        return static_cast<const Base &>(left) <=> static_cast<const Base &>(right);
    }
    /// Test if two components have the same value.
    friend auto operator==(const tDerived &left, const tDerived &right) noexcept -> bool {
        return static_cast<const Base &>(left) == static_cast<const Base &>(right);
    }
    /// Add another component with clamping.
    [[nodiscard]] auto operator+(const tDerived &other) const noexcept -> tDerived { return added(other); }
    /// Add another component in place with clamping.
    auto operator+=(const tDerived &other) noexcept -> tDerived & {
        add(other);
        return static_cast<tDerived &>(*this);
    }
    /// Subtract another component with clamping.
    [[nodiscard]] auto operator-(const tDerived &other) const noexcept -> tDerived { return subtracted(other); }
    /// Subtract another component in place with clamping.
    auto operator-=(const tDerived &other) noexcept -> tDerived & {
        subtract(other);
        return static_cast<tDerived &>(*this);
    }
    /// Add an integer operand with clamping.
    template <math::AnyIntegerType T>
    [[nodiscard]] auto operator+(T other) const noexcept -> tDerived {
        return added(other);
    }
    /// Add an integer operand in place with clamping.
    template <math::AnyIntegerType T>
    auto operator+=(T other) noexcept -> tDerived & {
        add(other);
        return static_cast<tDerived &>(*this);
    }
    /// Subtract an integer operand with clamping.
    template <math::AnyIntegerType T>
    [[nodiscard]] auto operator-(T other) const noexcept -> tDerived {
        return subtracted(other);
    }
    /// Subtract an integer operand in place with clamping.
    template <math::AnyIntegerType T>
    auto operator-=(T other) noexcept -> tDerived & {
        subtract(other);
        return static_cast<tDerived &>(*this);
    }
    /// Pre-increment with clamping.
    auto operator++() noexcept -> tDerived & {
        increment();
        return static_cast<tDerived &>(*this);
    }
    /// Post-increment with clamping.
    auto operator++(int) noexcept -> tDerived {
        auto result = static_cast<tDerived &>(*this);
        ++(*this);
        return result;
    }
    /// Pre-decrement with clamping.
    auto operator--() noexcept -> tDerived & {
        decrement();
        return static_cast<tDerived &>(*this);
    }
    /// Post-decrement with clamping.
    auto operator--(int) noexcept -> tDerived {
        auto result = static_cast<tDerived &>(*this);
        --(*this);
        return result;
    }

public: // tests
    /// Test if this is the first (minimum) value.
    [[nodiscard]] constexpr auto isFirst() const noexcept -> bool { return this->isMinimum(); }
    /// Test if this is the last (maximum) value.
    [[nodiscard]] constexpr auto isLast() const noexcept -> bool { return this->isMaximum(); }

public: // arithmetic
    /// Return this value plus another component, clamped to the supported range.
    [[nodiscard]] auto added(const tDerived &other) const noexcept -> tDerived {
        return tDerived{Base::added(static_cast<const Base &>(other)).toValue()};
    }
    /// Add another component in place, clamped to the supported range.
    void add(const tDerived &other) noexcept { *this = added(other); }
    /// Return this value minus another component, clamped to the supported range.
    [[nodiscard]] auto subtracted(const tDerived &other) const noexcept -> tDerived {
        return tDerived{Base::subtracted(static_cast<const Base &>(other)).toValue()};
    }
    /// Subtract another component in place, clamped to the supported range.
    void subtract(const tDerived &other) noexcept { *this = subtracted(other); }
    /// Return this value plus `other`, clamped to the supported range.
    template <math::AnyIntegerType T>
    [[nodiscard]] auto added(T other) const noexcept -> tDerived {
        return tDerived{Base::added(other).toValue()};
    }
    /// Add `other` in place, clamped to the supported range.
    template <math::AnyIntegerType T>
    void add(T other) noexcept {
        *this = added(other);
    }
    /// Return this value minus `other`, clamped to the supported range.
    template <math::AnyIntegerType T>
    [[nodiscard]] auto subtracted(T other) const noexcept -> tDerived {
        return tDerived{Base::subtracted(other).toValue()};
    }
    /// Subtract `other` in place, clamped to the supported range.
    template <math::AnyIntegerType T>
    void subtract(T other) noexcept {
        *this = subtracted(other);
    }
    /// Return this value incremented by one, clamped to the supported range.
    [[nodiscard]] auto incremented() const noexcept -> tDerived { return added(Value{1}); }
    /// Increment this value in place, clamped to the supported range.
    void increment() noexcept { add(Value{1}); }
    /// Return this value decremented by one, clamped to the supported range.
    [[nodiscard]] auto decremented() const noexcept -> tDerived { return subtracted(Value{1}); }
    /// Decrement this value in place, clamped to the supported range.
    void decrement() noexcept { subtract(Value{1}); }

public:
    /// Return the first (minimum) value of this component type.
    [[nodiscard]] static auto first() noexcept -> tDerived { return tDerived{tMinimum}; }
    /// Return the last (maximum) value of this component type.
    [[nodiscard]] static auto last() noexcept -> tDerived { return tDerived{tMaximum}; }
    /// Return the minimum value (alias for `first()`).
    [[nodiscard]] static auto minimum() noexcept -> tDerived { return first(); }
    /// Return the maximum value (alias for `last()`).
    [[nodiscard]] static auto maximum() noexcept -> tDerived { return last(); }
    /// Return the accepted range of values for this component type.
    [[nodiscard]] constexpr static auto range() noexcept -> math::IntegerRange<Value> { return Base::range(); }
    /// Test if a raw value is in range.
    template <math::AnyIntegerType T>
    [[nodiscard]] constexpr static auto contains(T value) noexcept -> bool {
        return Base::contains(value);
    }
};

}
