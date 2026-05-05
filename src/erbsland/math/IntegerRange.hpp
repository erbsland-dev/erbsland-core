// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerConversion.hpp"
#include "IntegerMath.hpp"
#include "IntegerTraits.hpp"
#include "SaturatingMath.hpp"

#include <compare>

namespace erbsland::math {

/// Inclusive range for native integer values.
/// If constructed with reversed bounds, the bounds are ordered automatically.
/// @tested{IntegerRangeTest}
template <NativeInteger tValue>
class IntegerRange final {
public:
    /// The integer type stored by this range.
    using Value = tValue;

public:
    /// Create the single-value range `0...0`.
    constexpr IntegerRange() noexcept = default;
    /// Create an inclusive range from `minimum` to `maximum`.
    constexpr IntegerRange(Value minimum, Value maximum) noexcept :
        _minimum{minimum <= maximum ? minimum : maximum}, _maximum{minimum <= maximum ? maximum : minimum} {}

    // defaults
    ~IntegerRange() = default;
    IntegerRange(const IntegerRange &) noexcept = default;
    auto operator=(const IntegerRange &) noexcept -> IntegerRange & = default;
    IntegerRange(IntegerRange &&) noexcept = default;
    auto operator=(IntegerRange &&) noexcept -> IntegerRange & = default;

public: // operators
    /// Compare two ranges by minimum first, then maximum.
    [[nodiscard]] constexpr auto operator<=>(const IntegerRange &other) const noexcept
        -> std::strong_ordering = default;

public: // tests
    /// Test if this range contains `value`.
    template <AnyIntegerType T>
    [[nodiscard]] constexpr auto contains(T value) const noexcept -> bool {
        const auto rawValue = toNativeInteger(value);
        return mixedIntegerCompare(rawValue, _minimum) != std::strong_ordering::less &&
            mixedIntegerCompare(rawValue, _maximum) != std::strong_ordering::greater;
    }
    /// Test if this range fully contains another range.
    template <NativeInteger T>
    [[nodiscard]] constexpr auto contains(IntegerRange<T> range) const noexcept -> bool {
        return contains(range.minimum()) && contains(range.maximum());
    }

public: // conversion
    /// Return `value` clamped to this range.
    template <AnyIntegerType T>
    [[nodiscard]] constexpr auto clamped(T value) const noexcept -> Value {
        const auto rawValue = toNativeInteger(value);
        if (mixedIntegerCompare(rawValue, _minimum) == std::strong_ordering::less) {
            return _minimum;
        }
        if (mixedIntegerCompare(rawValue, _maximum) == std::strong_ordering::greater) {
            return _maximum;
        }
        return saturatingCast<Value>(rawValue);
    }
    /// Cast this range to another native integer type with saturation.
    template <NativeInteger T>
    [[nodiscard]] constexpr auto cast() const noexcept -> IntegerRange<T> {
        return IntegerRange<T>{saturatingCast<T>(_minimum), saturatingCast<T>(_maximum)};
    }

public: // accessors
    /// Get the minimum value.
    [[nodiscard]] constexpr auto minimum() const noexcept -> Value { return _minimum; }
    /// Get the maximum value.
    [[nodiscard]] constexpr auto maximum() const noexcept -> Value { return _maximum; }

private:
    Value _minimum{0}; ///< The inclusive minimum value.
    Value _maximum{0}; ///< The inclusive maximum value.
};

}
