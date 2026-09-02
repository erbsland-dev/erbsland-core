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

/// A one-dimensional half-open coordinate span represented by an origin and a non-negative extent.
/// @seedoc{/reference/block/block_geometry}
/// @tested{CoordinateSpanTest}
class CoordinateSpan final {
public:
    /// The component type used for axis mapping.
    using AxisComponent = CoordinateSpan;
    /// The number of dimensions represented by this type.
    static constexpr auto cDimensionality = geometry::Dimensionality::One;

public:
    /// Create an empty span at the origin.
    constexpr CoordinateSpan() noexcept = default;
    /// Create a span from an origin and extent.
    /// Negative extents are clamped to zero.
    /// @param origin The first coordinate in the span.
    /// @param extent The number of coordinates in the span.
    constexpr CoordinateSpan(const Coordinate origin, const Coordinate extent) noexcept :
        _origin{origin}, _extent{std::max(extent, Coordinate{0})} {}
    /// @overload
    constexpr CoordinateSpan(const int origin, const int extent) noexcept :
        CoordinateSpan{Coordinate{origin}, Coordinate{extent}} {}

public: // operators
    /// Compare two coordinate spans.
    auto operator==(const CoordinateSpan &) const noexcept -> bool = default;
    /// Compare two coordinate spans.
    auto operator!=(const CoordinateSpan &) const noexcept -> bool = default;

public: // tests
    /// Test whether this span contains no coordinates.
    [[nodiscard]] constexpr auto isEmpty() const noexcept -> bool { return _extent == 0; }
    /// Test whether a coordinate lies in this half-open span.
    /// @param coordinate The coordinate to test.
    /// @return `true` if `origin() <= coordinate < end()`.
    [[nodiscard]] auto contains(const Coordinate coordinate) const noexcept -> bool {
        return !isEmpty() && coordinate >= _origin && coordinate < end();
    }

public: // attributes
    /// Get the first coordinate in the span.
    [[nodiscard]] constexpr auto origin() const noexcept -> Coordinate { return _origin; }
    /// Set the first coordinate in the span.
    constexpr void setOrigin(const Coordinate origin) noexcept { _origin = origin; }
    /// @overload
    constexpr void setOrigin(const int origin) noexcept { setOrigin(Coordinate{origin}); }
    /// Get the non-negative extent.
    [[nodiscard]] constexpr auto extent() const noexcept -> Coordinate { return _extent; }
    /// Set the extent, clamping negative values to zero.
    constexpr void setExtent(const Coordinate extent) noexcept { _extent = std::max(extent, Coordinate{0}); }
    /// @overload
    constexpr void setExtent(const int extent) noexcept { setExtent(Coordinate{extent}); }
    /// Get the first excluded coordinate using saturated arithmetic.
    [[nodiscard]] auto end() const noexcept -> Coordinate { return _origin + _extent; }
    /// Get this one-dimensional component.
    /// @param axis The requested axis, which must be X.
    /// @return This span.
    /// @throws err::ParameterError if `axis` is not X.
    [[nodiscard]] constexpr auto component(const geometry::Axis axis) const -> CoordinateSpan {
        using namespace text::literals;
        if (axis != geometry::Axis::X) {
            throw err::ParameterError{"The axis is outside the coordinate span dimensionality."_el, "axis"_el};
        }
        return *this;
    }
    /// Get this one-dimensional component with an optional reversal.
    /// @param axis The signed axis, which must select X.
    /// @return This span, reversed when requested.
    /// @throws err::ParameterError if `axis` does not select X.
    [[nodiscard]] auto component(const geometry::SignedAxis axis) const -> CoordinateSpan {
        const auto result = component(axis.axis());
        return axis.isReversed() ? result.reversed() : result;
    }

public: // transformation
    /// Reverse this span about coordinate zero while preserving discrete cell membership.
    /// @return The reversed span with the same extent.
    [[nodiscard]] auto reversed() const noexcept -> CoordinateSpan {
        if (isEmpty()) {
            return {-_origin, Coordinate{0}};
        }
        return {-(end() - 1), _extent};
    }

private:
    Coordinate _origin{}; ///< The first coordinate in the span.
    Coordinate _extent{}; ///< The non-negative number of coordinates in the span.
};

}
