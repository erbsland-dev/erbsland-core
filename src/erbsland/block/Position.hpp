// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Coordinate.hpp"
#include "Position_fwd.hpp"

#include "../err/ParameterError.hpp"
#include "../geometry/Axis.hpp"
#include "../geometry/Dimensionality.hpp"
#include "../geometry/Orientation.hpp"
#include "../geometry/SignedAxis.hpp"
#include "../text/Literals.hpp"
#include "../util/HashHelper.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <limits>
#include <numeric>
#include <string>
#include <string_view>
#include <utility>

namespace erbsland::block {

/// Represents a 2D integer position or vector (x, y).
/// - Lightweight value type with default construction to (0,0).
/// - Useful both for coordinates in a grid and for 2D vector arithmetic.
/// @seedoc{/reference/block/block_geometry}
/// @tested{PositionTest}
class Position {
public:
    /// The component type used for axis mapping.
    using AxisComponent = Coordinate;
    /// The number of dimensions represented by this type.
    static constexpr auto cDimensionality = geometry::Dimensionality::Two;

public:
    /// Default construct to (0,0).
    Position() = default;
    /// Construct from explicit coordinates.
    /// @param x The x-coordinate.
    /// @param y The y-coordinate.
    constexpr Position(const Coordinate x, const Coordinate y) noexcept : _x{x}, _y{y} {}
    /// @overload
    constexpr Position(const int x, const int y) noexcept : _x{x}, _y{y} {}

public: // operators
    /// Equality comparison (component-wise).
    auto operator==(const Position &other) const noexcept -> bool = default;
    /// Inequality comparison (component-wise).
    auto operator!=(const Position &other) const noexcept -> bool = default;
    /// Vector addition (component-wise).
    /// @param other The other position to add.
    /// @return A Position with coordinates (_x + other._x, _y + other._y).
    [[nodiscard]] auto operator+(const Position &other) const noexcept -> Position {
        return {_x + other._x, _y + other._y};
    }
    /// Vector subtraction (component-wise).
    /// @param other The other position to subtract.
    /// @return A Position with coordinates (_x - other._x, _y - other._y).
    [[nodiscard]] auto operator-(const Position &other) const noexcept -> Position {
        return {_x - other._x, _y - other._y};
    }
    /// Add another position to this one in-place.
    /// @param other The other position to add.
    /// @return Reference to this position.
    auto operator+=(const Position &other) noexcept -> Position & {
        _x += other._x;
        _y += other._y;
        return *this;
    }
    /// Subtract another position from this one in-place.
    /// @param other The other position to subtract.
    /// @return Reference to this position.
    auto operator-=(const Position &other) noexcept -> Position & {
        _x -= other._x;
        _y -= other._y;
        return *this;
    }

public: // attributes
    /// Get the x coordinate.
    [[nodiscard]] constexpr auto x() const noexcept -> Coordinate { return _x; }
    /// Set the x coordinate.
    /// @param x New x value.
    void setX(Coordinate x) noexcept;
    /// @overload
    void setX(int x) noexcept { setX(Coordinate{x}); }
    /// Get the y coordinate.
    [[nodiscard]] constexpr auto y() const noexcept -> Coordinate { return _y; }
    /// Set the y coordinate.
    /// @param y New y value.
    void setY(Coordinate y) noexcept;
    /// @overload
    void setY(int y) noexcept { setY(Coordinate{y}); }
    /// Get the component for the selected orientation.
    /// @param orientation The orientation that selects the x or y component.
    /// @return `x()` for `geometry::Orientation::Horizontal`, otherwise `y()`.
    [[nodiscard]] constexpr auto component(const geometry::Orientation orientation) const noexcept -> Coordinate {
        return orientation == geometry::Orientation::Horizontal ? _x : _y;
    }
    /// Get the component for a physical axis.
    /// @param axis The physical axis to select.
    /// @return The selected coordinate.
    /// @throws err::ParameterError if `axis` is Z.
    [[nodiscard]] constexpr auto component(const geometry::Axis axis) const -> Coordinate {
        using namespace text::literals;
        switch (axis) {
        case geometry::Axis::X:
            return _x;
        case geometry::Axis::Y:
            return _y;
        default:
            throw err::ParameterError{"The axis is outside the position dimensionality."_el, "axis"_el};
        }
    }
    /// Get the component for a signed physical axis.
    /// @param axis The signed physical axis to select.
    /// @return The selected coordinate, negated for a reversed axis.
    /// @throws err::ParameterError if `axis` selects Z.
    [[nodiscard]] constexpr auto component(const geometry::SignedAxis axis) const -> Coordinate {
        const auto result = component(axis.axis());
        return axis.isReversed() ? -result : result;
    }

public: // tools
    /// Get a hash for this position.
    /// This hash is designed to be fast and uniform for both 32-bit and 64-bit platforms.
    /// It is not only optimized to be used in a map but also as a source for pseudo-randomness.
    [[nodiscard]] auto hash() const noexcept -> std::size_t { return util::createHash(_x, _y); }
    /// Manhattan (L1) distance to another position.
    /// @param other The other position.
    /// @return |x - other.x| + |y - other.y|.
    [[nodiscard]] auto distanceTo(Position other) const noexcept -> Coordinate;
    /// Component-wise maximum with another position.
    /// @param other The other position.
    /// @return A Position containing the max of each component.
    [[nodiscard]] auto componentMax(Position other) const noexcept -> Position {
        return {std::max(_x, other._x), std::max(_y, other._y)};
    }
    /// Component-wise minimum with another position.
    /// @param other The other position.
    /// @return A Position containing the min of each component.
    [[nodiscard]] auto componentMin(Position other) const noexcept -> Position {
        return {std::min(_x, other._x), std::min(_y, other._y)};
    }
    /// Get the four cardinal positions, relative to this one.
    /// Order: right, down, left, up
    [[nodiscard]] auto cardinalFour() const noexcept -> std::array<Position, 4> {
        return {Position{_x + 1, _y}, Position{_x, _y + 1}, Position{_x - 1, _y}, Position{_x, _y - 1}};
    }
    /// Get the four cardinal position deltas.
    /// Order: right, down, left, up
    [[nodiscard]] static auto cardinalFourDeltas() noexcept -> const std::array<Position, 4> &;
    /// Create a bitmask testing the four cardinal positions.
    /// @param fn The function to test each cardinal delta position, relative to this one.
    template <typename Fn>
        requires std::invocable<Fn, Position> && std::convertible_to<std::invoke_result_t<Fn, Position>, bool>
    [[nodiscard]] auto cardinalFourBitmask(Fn fn) const noexcept -> uint32_t;
    /// Get the eight positions that form a ring around this position.
    /// Clockwise order: 0:E, 1:SE, 2:S, 3:SW, 4:W, 5:NW, 6:N, 7:NE
    /// @return An array with all the eight positions.
    [[nodiscard]] auto ringEight() const noexcept -> std::array<Position, 8U> {
        return {
            Position{_x + 1, _y},
            Position{_x + 1, _y + 1},
            Position{_x, _y + 1},
            Position{_x - 1, _y + 1},
            Position{_x - 1, _y},
            Position{_x - 1, _y - 1},
            Position{_x, _y - 1},
            Position{_x + 1, _y - 1}};
    }
    /// Get the eight deltas that form a ring around this position.
    /// Clockwise order: 0:E, 1:SE, 2:S, 3:SW, 4:W, 5:NW, 6:N, 7:NE
    [[nodiscard]] static auto ringEightDeltas() noexcept -> const std::array<Position, 8U> &;

public: // useful constants
    /// Get the point with the minimum coordinates.
    static auto minimum() noexcept -> Position { return Position{Coordinate::minimum(), Coordinate::minimum()}; }
    /// Get the point with the maximum coordinates.
    static auto maximum() noexcept -> Position { return Position{Coordinate::maximum(), Coordinate::maximum()}; }

private:
    Coordinate _x;
    Coordinate _y;
};

}

#include "Position.tpp"

template <>
struct std::hash<erbsland::block::Position> {
    auto operator()(const erbsland::block::Position &pos) const noexcept -> std::size_t { return pos.hash(); }
};
