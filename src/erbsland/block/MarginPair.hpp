// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Coordinate.hpp"

#include "../err/ParameterError.hpp"
#include "../geometry/Axis.hpp"
#include "../geometry/Dimensionality.hpp"
#include "../geometry/SignedAxis.hpp"
#include "../text/Literals.hpp"

#include <algorithm>

namespace erbsland::block {

/// Leading and trailing margins along one axis.
/// @seedoc{/reference/block/block_geometry}
/// @tested{MarginPairTest}
class MarginPair final {
public:
    /// The component type used for axis mapping.
    using AxisComponent = MarginPair;
    /// The number of dimensions represented by this type.
    static constexpr auto cDimensionality = geometry::Dimensionality::One;

public:
    /// Create zero leading and trailing margins.
    constexpr MarginPair() noexcept = default;
    /// Create equal leading and trailing margins.
    /// @param both The value for both margins.
    constexpr explicit MarginPair(const Coordinate both) noexcept : _leading{both}, _trailing{both} {}
    /// @overload
    constexpr explicit MarginPair(const int both) noexcept : MarginPair{Coordinate{both}} {}
    /// Create separate leading and trailing margins.
    /// @param leading The leading margin.
    /// @param trailing The trailing margin.
    constexpr MarginPair(const Coordinate leading, const Coordinate trailing) noexcept :
        _leading{leading}, _trailing{trailing} {}
    /// @overload
    constexpr MarginPair(const int leading, const int trailing) noexcept :
        MarginPair{Coordinate{leading}, Coordinate{trailing}} {}

public: // operators
    /// Compare two margin pairs.
    auto operator==(const MarginPair &) const noexcept -> bool = default;
    /// Compare two margin pairs.
    auto operator!=(const MarginPair &) const noexcept -> bool = default;
    /// Negate both margins.
    [[nodiscard]] constexpr auto operator-() const noexcept -> MarginPair { return {-_leading, -_trailing}; }

public: // attributes
    /// Get the leading margin.
    [[nodiscard]] constexpr auto leading() const noexcept -> Coordinate { return _leading; }
    /// Set the leading margin.
    constexpr void setLeading(const Coordinate leading) noexcept { _leading = leading; }
    /// @overload
    constexpr void setLeading(const int leading) noexcept { setLeading(Coordinate{leading}); }
    /// Get the trailing margin.
    [[nodiscard]] constexpr auto trailing() const noexcept -> Coordinate { return _trailing; }
    /// Set the trailing margin.
    constexpr void setTrailing(const Coordinate trailing) noexcept { _trailing = trailing; }
    /// @overload
    constexpr void setTrailing(const int trailing) noexcept { setTrailing(Coordinate{trailing}); }
    /// Get this one-dimensional component.
    /// @param axis The requested axis, which must be X.
    /// @return This pair.
    /// @throws err::ParameterError if `axis` is not X.
    [[nodiscard]] constexpr auto component(const geometry::Axis axis) const -> MarginPair {
        using namespace text::literals;
        if (axis != geometry::Axis::X) {
            throw err::ParameterError{"The axis is outside the margin pair dimensionality."_el, "axis"_el};
        }
        return *this;
    }
    /// Get this one-dimensional component with an optional reversal.
    /// @param axis The signed axis, which must select X.
    /// @return This pair, reversed when requested.
    /// @throws err::ParameterError if `axis` does not select X.
    [[nodiscard]] constexpr auto component(const geometry::SignedAxis axis) const -> MarginPair {
        const auto result = component(axis.axis());
        return axis.isReversed() ? result.reversed() : result;
    }

public: // calculations
    /// Get the space consumed by positive margins.
    [[nodiscard]] auto extent() const noexcept -> Coordinate {
        return std::max(_leading, Coordinate{0}) + std::max(_trailing, Coordinate{0});
    }
    /// Get the signed size delta caused by both margins.
    [[nodiscard]] auto delta() const noexcept -> Coordinate { return _leading + _trailing; }
    /// Get the greatest positive margin.
    [[nodiscard]] constexpr auto spacing() const noexcept -> Coordinate {
        return std::max<Coordinate>({Coordinate{0}, _leading, _trailing});
    }
    /// Exchange the leading and trailing margins.
    [[nodiscard]] constexpr auto reversed() const noexcept -> MarginPair { return {_trailing, _leading}; }

public: // constraints
    /// Expand both margins to at least the matching margins in another pair.
    /// @param other The minimum margins.
    /// @return This pair.
    constexpr auto expandTo(const MarginPair other) noexcept -> MarginPair & {
        _leading = std::max(_leading, other._leading);
        _trailing = std::max(_trailing, other._trailing);
        return *this;
    }
    /// Limit both margins to the matching margins in another pair.
    /// @param other The maximum margins.
    /// @return This pair.
    constexpr auto limitTo(const MarginPair other) noexcept -> MarginPair & {
        _leading = std::min(_leading, other._leading);
        _trailing = std::min(_trailing, other._trailing);
        return *this;
    }
    /// Clamp both margins to zero or positive values.
    /// @return This pair.
    constexpr auto expandPositive() noexcept -> MarginPair & { return expandTo(MarginPair{}); }
    /// Create a copy expanded to at least the matching margins in another pair.
    /// @param other The minimum margins.
    /// @return The expanded pair.
    [[nodiscard]] constexpr auto expandedWith(const MarginPair other) const noexcept -> MarginPair {
        auto result = *this;
        result.expandTo(other);
        return result;
    }
    /// Create a copy limited to the matching margins in another pair.
    /// @param other The maximum margins.
    /// @return The limited pair.
    [[nodiscard]] constexpr auto limitedWith(const MarginPair other) const noexcept -> MarginPair {
        auto result = *this;
        result.limitTo(other);
        return result;
    }
    /// Create a copy clamped to zero or positive values.
    [[nodiscard]] constexpr auto expandedPositive() const noexcept -> MarginPair {
        auto result = *this;
        result.expandPositive();
        return result;
    }

private:
    Coordinate _leading;  ///< The leading margin.
    Coordinate _trailing; ///< The trailing margin.
};

}
