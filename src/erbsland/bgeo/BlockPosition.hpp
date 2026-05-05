// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BlockCoordinate.hpp"
#include "BlockPosition_fwd.hpp"
#include "Orientation.hpp"

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

namespace erbsland::bgeo {

/// Represents a 2D integer position or vector (x, y).
/// - Lightweight value type with default construction to (0,0).
/// - Useful both for coordinates in a grid and for 2D vector arithmetic.
/// @seedoc{/reference/bgeo/block_geometry}
/// @tested{BlockPositionTest}
class BlockPosition {
public:
    /// Default construct to (0,0).
    BlockPosition() = default;
    /// Construct from explicit coordinates.
    /// @param x The x-coordinate.
    /// @param y The y-coordinate.
    constexpr BlockPosition(const BlockCoordinate x, const BlockCoordinate y) noexcept : _x{x}, _y{y} {}
    /// @overload
    constexpr BlockPosition(const int x, const int y) noexcept : _x{x}, _y{y} {}

public: // operators
    /// Equality comparison (component-wise).
    auto operator==(const BlockPosition &other) const noexcept -> bool = default;
    /// Inequality comparison (component-wise).
    auto operator!=(const BlockPosition &other) const noexcept -> bool = default;
    /// Vector addition (component-wise).
    /// @param other The other position to add.
    /// @return A BlockPosition with coordinates (_x + other._x, _y + other._y).
    [[nodiscard]] auto operator+(const BlockPosition &other) const noexcept -> BlockPosition {
        return {_x + other._x, _y + other._y};
    }
    /// Vector subtraction (component-wise).
    /// @param other The other position to subtract.
    /// @return A BlockPosition with coordinates (_x - other._x, _y - other._y).
    [[nodiscard]] auto operator-(const BlockPosition &other) const noexcept -> BlockPosition {
        return {_x - other._x, _y - other._y};
    }
    /// Add another position to this one in-place.
    /// @param other The other position to add.
    /// @return Reference to this position.
    auto operator+=(const BlockPosition &other) noexcept -> BlockPosition & {
        _x += other._x;
        _y += other._y;
        return *this;
    }
    /// Subtract another position from this one in-place.
    /// @param other The other position to subtract.
    /// @return Reference to this position.
    auto operator-=(const BlockPosition &other) noexcept -> BlockPosition & {
        _x -= other._x;
        _y -= other._y;
        return *this;
    }

public: // attributes
    /// Get the x coordinate.
    [[nodiscard]] constexpr auto x() const noexcept -> BlockCoordinate { return _x; }
    /// Set the x coordinate.
    /// @param x New x value.
    void setX(BlockCoordinate x) noexcept;
    /// @overload
    void setX(int x) noexcept { setX(BlockCoordinate{x}); }
    /// Get the y coordinate.
    [[nodiscard]] constexpr auto y() const noexcept -> BlockCoordinate { return _y; }
    /// Set the y coordinate.
    /// @param y New y value.
    void setY(BlockCoordinate y) noexcept;
    /// @overload
    void setY(int y) noexcept { setY(BlockCoordinate{y}); }
    /// Get the coordinate for the selected orientation.
    /// @param orientation The orientation that selects the x or y coordinate.
    /// @return `x()` for `Orientation::Horizontal`, otherwise `y()`.
    [[nodiscard]] constexpr auto coordinate(const Orientation orientation) const noexcept -> BlockCoordinate {
        return orientation == Orientation::Horizontal ? _x : _y;
    }

public: // tools
    /// Get a hash for this position.
    /// This hash is designed to be fast and uniform for both 32-bit and 64-bit platforms.
    /// It is not only optimized to be used in a map but also as a source for pseudo-randomness.
    [[nodiscard]] auto hash() const noexcept -> std::size_t { return util::createHash(_x, _y); }
    /// Manhattan (L1) distance to another position.
    /// @param other The other position.
    /// @return |x - other.x| + |y - other.y|.
    [[nodiscard]] auto distanceTo(BlockPosition other) const noexcept -> BlockCoordinate;
    /// Component-wise maximum with another position.
    /// @param other The other position.
    /// @return A BlockPosition containing the max of each component.
    [[nodiscard]] auto componentMax(BlockPosition other) const noexcept -> BlockPosition {
        return {std::max(_x, other._x), std::max(_y, other._y)};
    }
    /// Component-wise minimum with another position.
    /// @param other The other position.
    /// @return A BlockPosition containing the min of each component.
    [[nodiscard]] auto componentMin(BlockPosition other) const noexcept -> BlockPosition {
        return {std::min(_x, other._x), std::min(_y, other._y)};
    }
    /// Get the four cardinal positions, relative to this one.
    /// Order: right, down, left, up
    [[nodiscard]] auto cardinalFour() const noexcept -> std::array<BlockPosition, 4> {
        return {
            BlockPosition{_x + 1, _y}, BlockPosition{_x, _y + 1}, BlockPosition{_x - 1, _y}, BlockPosition{_x, _y - 1}};
    }
    /// Get the four cardinal position deltas.
    /// Order: right, down, left, up
    [[nodiscard]] static auto cardinalFourDeltas() noexcept -> const std::array<BlockPosition, 4> &;
    /// Create a bitmask testing the four cardinal positions.
    /// @param fn The function to test each cardinal delta position, relative to this one.
    template <typename Fn>
        requires std::invocable<Fn, BlockPosition> && std::convertible_to<std::invoke_result_t<Fn, BlockPosition>, bool>
    [[nodiscard]] auto cardinalFourBitmask(Fn fn) const noexcept -> uint32_t;
    /// Get the eight positions that form a ring around this position.
    /// Clockwise order: 0:E, 1:SE, 2:S, 3:SW, 4:W, 5:NW, 6:N, 7:NE
    /// @return An array with all the eight positions.
    [[nodiscard]] auto ringEight() const noexcept -> std::array<BlockPosition, 8U> {
        return {
            BlockPosition{_x + 1, _y},
            BlockPosition{_x + 1, _y + 1},
            BlockPosition{_x, _y + 1},
            BlockPosition{_x - 1, _y + 1},
            BlockPosition{_x - 1, _y},
            BlockPosition{_x - 1, _y - 1},
            BlockPosition{_x, _y - 1},
            BlockPosition{_x + 1, _y - 1}};
    }
    /// Get the eight deltas that form a ring around this position.
    /// Clockwise order: 0:E, 1:SE, 2:S, 3:SW, 4:W, 5:NW, 6:N, 7:NE
    [[nodiscard]] static auto ringEightDeltas() noexcept -> const std::array<BlockPosition, 8U> &;

public: // useful constants
    /// Get the point with the minimum coordinates.
    static auto minimum() noexcept -> BlockPosition {
        return BlockPosition{BlockCoordinate::minimum(), BlockCoordinate::minimum()};
    }
    /// Get the point with the maximum coordinates.
    static auto maximum() noexcept -> BlockPosition {
        return BlockPosition{BlockCoordinate::maximum(), BlockCoordinate::maximum()};
    }

private:
    BlockCoordinate _x;
    BlockCoordinate _y;
};

}

#include "BlockPosition.tpp"

template <>
struct std::hash<erbsland::bgeo::BlockPosition> {
    auto operator()(const erbsland::bgeo::BlockPosition &pos) const noexcept -> std::size_t { return pos.hash(); }
};
