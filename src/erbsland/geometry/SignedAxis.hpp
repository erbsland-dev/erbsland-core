// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Axis.hpp"

#include <cstdint>
#include <limits>

namespace erbsland::geometry {

/// A physical geometry axis with an optional direction reversal.
/// @seedoc{/reference/geometry/geometry}
/// @tested{AxisMapperTest}
class SignedAxis final {
public:
    /// The signed-axis values.
    enum Value : uint8_t {
        PositiveX = 0, ///< The positive x-axis.
        PositiveY,     ///< The positive y-axis.
        PositiveZ,     ///< The positive z-axis.
        NegativeX,     ///< The reversed x-axis.
        NegativeY,     ///< The reversed y-axis.
        NegativeZ,     ///< The reversed z-axis.
    };

public:
    /// Create the positive x-axis.
    constexpr SignedAxis() noexcept = default;
    /// Create a signed axis from a value.
    /// @param value The signed-axis value.
    constexpr SignedAxis(const Value value) noexcept : _value{value} {} // NOLINT(*-explicit-constructor)
    /// Create a positive signed axis from a physical axis.
    /// @param axis The physical axis.
    constexpr SignedAxis(const Axis axis) noexcept :
        _value{
            static_cast<uint8_t>(axis) <= static_cast<uint8_t>(Axis::Z)
                ? static_cast<Value>(axis)
                : static_cast<Value>(std::numeric_limits<uint8_t>::max())} {} // NOLINT(*-explicit-constructor)

public:                                                                       // operators
    /// Compare two signed axes.
    auto operator==(const SignedAxis &) const noexcept -> bool = default;
    /// Compare two signed axes.
    auto operator!=(const SignedAxis &) const noexcept -> bool = default;

public: // tests
    /// Test whether the stored value is a defined signed axis.
    [[nodiscard]] constexpr auto isValid() const noexcept -> bool {
        return static_cast<uint8_t>(_value) <= static_cast<uint8_t>(NegativeZ);
    }
    /// Test whether this axis reverses its component.
    [[nodiscard]] constexpr auto isReversed() const noexcept -> bool {
        return isValid() && static_cast<uint8_t>(_value) >= static_cast<uint8_t>(NegativeX);
    }

public: // attributes
    /// Get the physical axis without its direction.
    [[nodiscard]] constexpr auto axis() const noexcept -> Axis {
        if (!isValid()) {
            return static_cast<Axis>(std::numeric_limits<uint8_t>::max());
        }
        const auto value = static_cast<uint8_t>(_value);
        return static_cast<Axis>(value >= static_cast<uint8_t>(NegativeX) ? value - 3U : value);
    }
    /// Get the stored value.
    [[nodiscard]] constexpr auto value() const noexcept -> Value { return _value; }

private:
    Value _value{PositiveX}; ///< The stored signed-axis value.
};

}
