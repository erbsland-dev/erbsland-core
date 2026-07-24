// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <cstdint>

namespace erbsland::bgeo {

/// Represents the orientation of a layout or a direction.
/// @seedoc{/reference/bgeo/alignment_and_orientation}
/// @tested{OrientationTest}
class Orientation final {
public:
    /// The stored orientation value.
    enum Value : uint8_t {
        Horizontal, ///< Horizontal orientation, e.g. for horizontal layouts or horizontal scrolling.
        Vertical,   ///< Vertical orientation, e.g. for vertical layouts or vertical scrolling.
    };

public:
    /// Construct the default horizontal orientation.
    constexpr Orientation() noexcept = default;
    /// Construct an orientation from a value.
    /// @param value The orientation value.
    constexpr Orientation(const Value value) noexcept : _value{value} {}

public: // operators
    /// Equality comparison.
    auto operator==(const Orientation &other) const noexcept -> bool = default;
    /// Inequality comparison.
    auto operator!=(const Orientation &other) const noexcept -> bool = default;

public: // attributes
    /// Get the stored value.
    /// @return The wrapped orientation value.
    [[nodiscard]] constexpr auto value() const noexcept -> Value { return _value; }

public: // tools
    /// Get the crossed orientation.
    /// @return `Vertical` for `Horizontal`, otherwise `Horizontal`.
    [[nodiscard]] constexpr auto crossed() const noexcept -> Orientation {
        return _value == Horizontal ? Vertical : Horizontal;
    }

private:
    Value _value{Horizontal};
};

}
