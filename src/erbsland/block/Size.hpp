// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Position.hpp"
#include "Size_fwd.hpp"

#include "../err/ParameterError.hpp"
#include "../geometry/Alignment.hpp"
#include "../geometry/Anchor.hpp"
#include "../geometry/Axis.hpp"
#include "../geometry/Dimensionality.hpp"
#include "../geometry/Orientation.hpp"
#include "../geometry/SignedAxis.hpp"
#include "../geometry/Symmetry.hpp"
#include "../math/ConstexprSaturatingMath.hpp"
#include "../text/Literals.hpp"

#include <algorithm>
#include <limits>

namespace erbsland::block {

/// A non-negative 2D size (width × height).
/// - Width and height are clamped to be >= 0.
/// - Many operations assume a grid indexed from (0,0) to (width-1,height-1).
/// @seedoc{/reference/block/block_geometry}
/// @tested{SizeTest}
class Size {
public:
    /// The component type used for axis mapping.
    using AxisComponent = Coordinate;
    /// The number of dimensions represented by this type.
    static constexpr auto cDimensionality = geometry::Dimensionality::Two;

public:
    /// Construct a zero size (0 × 0).
    Size() = default;
    /// Construct a size from explicit width and height.
    /// Negative inputs are clamped to 0.
    /// @param width The desired width (clamped to >= 0).
    /// @param height The desired height (clamped to >= 0).
    constexpr Size(const Coordinate width, const Coordinate height) noexcept :
        _width{std::max(Coordinate{0}, width)}, _height{std::max(Coordinate{0}, height)} {}
    /// @overload
    constexpr Size(const int width, const int height) noexcept : Size{Coordinate{width}, Coordinate{height}} {}
    /// Construct a size from the axis-aligned distance between two positions.
    /// @param pos1 First position.
    /// @param pos2 Second position.
    /// @note The order of the positions does not matter; absolute differences are used.
    Size(const Position pos1, const Position pos2) noexcept :
        _width{pos1.x().absoluteDifference(pos2.x())}, _height{pos1.y().absoluteDifference(pos2.y())} {}

public: // operators
    /// Equality comparison on width and height.
    auto operator==(const Size &other) const noexcept -> bool = default;
    /// Inequality comparison on width and height.
    auto operator!=(const Size &other) const noexcept -> bool = default;
    /// Add two sizes using saturated arithmetic.
    /// @param other The size to add.
    /// @return The component-wise sum, capped at the maximum coordinate value.
    auto operator+(const Size &other) const noexcept -> Size {
        auto result = *this;
        result.add(other);
        return result;
    }
    /// Subtract two sizes using saturated arithmetic.
    /// @param other The size to subtract.
    /// @return The component-wise difference, clamped to non-negative values.
    auto operator-(const Size &other) const noexcept -> Size {
        auto result = *this;
        result.subtract(other);
        return result;
    }
    /// Add a size using saturated arithmetic.
    /// @param other The size to add.
    /// @return This size.
    auto operator+=(const Size &other) noexcept -> Size & { return add(other); }
    /// Subtract a size using saturated arithmetic.
    /// @param other The size to subtract.
    /// @return This size.
    auto operator-=(const Size &other) noexcept -> Size & { return subtract(other); }

public: // attributes
    /// Get the width (>= 0).
    [[nodiscard]] constexpr auto width() const noexcept -> Coordinate { return _width; }
    /// Set the width. Negative values are clamped to 0.
    /// @param width New width (clamped to >= 0).
    void setWidth(Coordinate width) noexcept { _width = std::max(Coordinate{0}, width); }
    /// @overload
    void setWidth(const int width) noexcept { setWidth(Coordinate{width}); }
    /// Get the height (>= 0).
    [[nodiscard]] constexpr auto height() const noexcept -> Coordinate { return _height; }
    /// Set the height. Negative values are clamped to 0.
    /// @param height New height (clamped to >= 0).
    void setHeight(Coordinate height) noexcept { _height = std::max(Coordinate{0}, height); }
    /// @overload
    void setHeight(const int height) noexcept { setHeight(Coordinate{height}); }
    /// Get the component for the selected orientation.
    /// @param orientation The orientation that selects width or height.
    /// @return `width()` for `geometry::Orientation::Horizontal`, otherwise `height()`.
    [[nodiscard]] constexpr auto component(const geometry::Orientation orientation) const noexcept -> Coordinate {
        return orientation == geometry::Orientation::Horizontal ? _width : _height;
    }
    /// Get the extent for a physical axis.
    /// @param axis The physical axis to select.
    /// @return The selected non-negative extent.
    /// @throws err::ParameterError if `axis` is Z.
    [[nodiscard]] constexpr auto component(const geometry::Axis axis) const -> Coordinate {
        using namespace text::literals;
        switch (axis) {
        case geometry::Axis::X:
            return _width;
        case geometry::Axis::Y:
            return _height;
        default:
            throw err::ParameterError{"The axis is outside the size dimensionality."_el, "axis"_el};
        }
    }
    /// Get the extent for a signed physical axis.
    /// Reversing an axis does not change its non-negative extent.
    /// @param axis The signed physical axis to select.
    /// @return The selected non-negative extent.
    /// @throws err::ParameterError if `axis` selects Z.
    [[nodiscard]] constexpr auto component(const geometry::SignedAxis axis) const -> Coordinate {
        return component(axis.axis());
    }
    /// Compute a position inside the rectangle defined by this size for a given anchor.
    /// Bottom-right resolves to (width-1, height-1), top-left to (0,0), etc.
    /// @param anchor The anchor describing the target corner/edge/center.
    /// @return The position inside the [0,width-1]×[0,height-1] grid (or (0,0) for empty dimensions).
    [[nodiscard]] auto anchor(geometry::Anchor anchor) const noexcept -> Position;
    /// Compute the offset for content aligned inside this size.
    /// If `contentSize` is larger than this size on an axis, the returned offset on that axis is negative.
    /// @param contentSize The aligned content size.
    /// @param alignment The alignment for the content.
    /// @return The zero-based offset for placing the content inside this size.
    [[nodiscard]] auto alignmentOffset(Size contentSize, geometry::Alignment alignment) const noexcept -> Position;

public: // tests
    /// Test if this size is zero.
    [[nodiscard]] constexpr auto isZero() const noexcept -> bool { return _width == 0 || _height == 0; }
    /// Check if this size fits completely into another size (component-wise <=).
    /// @param other The candidate container size.
    /// @return true if width <= other.width and height <= other.height.
    [[nodiscard]] auto fitsInto(const Size other) const noexcept -> bool {
        return _width <= other._width && _height <= other._height;
    }
    /// Test if the width and height of this size is in the given range.
    /// @param minimum The minimum size.
    /// @param maximum The maximum size.
    /// @return true If this size is in the range `minimum`-`maximum`.
    [[nodiscard]] auto isInRange(const Size minimum, const Size maximum) const noexcept -> bool {
        return _width >= minimum._width && _height >= minimum._height && _width <= maximum._width &&
            _height <= maximum._height;
    }
    /// Check if a position lies strictly inside the bounds [0,width) × [0,height).
    /// @param pos The position to test.
    /// @return true if 0 <= x < width and 0 <= y < height.
    [[nodiscard]] constexpr auto contains(const Position &pos) const noexcept -> bool {
        return pos.x() >= 0 && pos.y() >= 0 && pos.x() < _width && pos.y() < _height;
    }

public: // constraints
    /// Add another size using saturated arithmetic.
    /// @param other The size to add.
    /// @return This size.
    auto add(const Size other) noexcept -> Size & {
        _width = _width + other._width;
        _height = _height + other._height;
        return *this;
    }
    /// Add another size only on the selected axis using saturated arithmetic.
    /// @param other The size to add.
    /// @param orientation The axis to modify.
    /// @return This size.
    auto add(const Size other, const geometry::Orientation orientation) noexcept -> Size & {
        if (orientation == geometry::Orientation::Horizontal) {
            _width = _width + other._width;
        } else {
            _height = _height + other._height;
        }
        return *this;
    }
    /// Subtract another size using saturated arithmetic and clamp the result to zero.
    /// @param other The size to subtract.
    /// @return This size.
    auto subtract(const Size other) noexcept -> Size & {
        setWidth(_width - other._width);
        setHeight(_height - other._height);
        return *this;
    }
    /// Subtract another size only on the selected axis using saturated arithmetic and clamp the result to zero.
    /// @param other The size to subtract.
    /// @param orientation The axis to modify.
    /// @return This size.
    auto subtract(const Size other, const geometry::Orientation orientation) noexcept -> Size & {
        if (orientation == geometry::Orientation::Horizontal) {
            setWidth(_width - other._width);
        } else {
            setHeight(_height - other._height);
        }
        return *this;
    }
    /// Expand this size so it is at least the given size.
    /// @param other The minimum size to cover.
    /// @return This size.
    auto expandTo(const Size other) noexcept -> Size & {
        _width = std::max(_width, other._width);
        _height = std::max(_height, other._height);
        return *this;
    }
    /// Expand this size so it is at least the given size on the selected axis.
    /// @param other The minimum size to cover.
    /// @param orientation The axis to modify.
    /// @return This size.
    auto expandTo(const Size other, const geometry::Orientation orientation) noexcept -> Size & {
        if (orientation == geometry::Orientation::Horizontal) {
            _width = std::max(_width, other._width);
        } else {
            _height = std::max(_height, other._height);
        }
        return *this;
    }
    /// Limit this size so it is at most the given size.
    /// @param other The maximum size to respect.
    /// @return This size.
    auto limitTo(const Size other) noexcept -> Size & {
        _width = std::min(_width, other._width);
        _height = std::min(_height, other._height);
        return *this;
    }
    /// Limit this size so it is at most the given size on the selected axis.
    /// @param other The maximum size to respect.
    /// @param orientation The axis to modify.
    /// @return This size.
    auto limitTo(const Size other, const geometry::Orientation orientation) noexcept -> Size & {
        if (orientation == geometry::Orientation::Horizontal) {
            _width = std::min(_width, other._width);
        } else {
            _height = std::min(_height, other._height);
        }
        return *this;
    }
    /// Create a copy expanded so it is at least the given size.
    /// @param other The minimum size to cover.
    /// @return The expanded size.
    [[nodiscard]] auto expandedWith(const Size other) const noexcept -> Size {
        auto result = *this;
        result.expandTo(other);
        return result;
    }
    /// Create a copy expanded so it is at least the given size on the selected axis.
    /// @param other The minimum size to cover.
    /// @param orientation The axis to modify.
    /// @return The expanded size.
    [[nodiscard]] auto expandedWith(const Size other, const geometry::Orientation orientation) const noexcept -> Size {
        auto result = *this;
        result.expandTo(other, orientation);
        return result;
    }
    /// Create a copy limited so it is at most the given size.
    /// @param other The maximum size to respect.
    /// @return The limited size.
    [[nodiscard]] auto limitedWith(const Size other) const noexcept -> Size {
        auto result = *this;
        result.limitTo(other);
        return result;
    }
    /// Create a copy limited so it is at most the given size on the selected axis.
    /// @param other The maximum size to respect.
    /// @param orientation The axis to modify.
    /// @return The limited size.
    [[nodiscard]] auto limitedWith(const Size other, const geometry::Orientation orientation) const noexcept -> Size {
        auto result = *this;
        result.limitTo(other, orientation);
        return result;
    }
    /// Component-wise clamp with a minimum and maximum size.
    /// @warning If minimum.width > maximum.width or minimum.height > maximum.height, the behavior is *undefined*.
    /// @param minimum The minimum size.
    /// @param maximum The maximum size.
    /// @return a size (min.width <= width <= max.width, min.height <= height <= max.height)
    [[nodiscard]] auto clampTo(const Size minimum, const Size maximum) const noexcept -> Size {
        return {_width.clamped(minimum._width, maximum._width), _height.clamped(minimum._height, maximum._height)};
    }
    /// Clamp a position *inside* this size.
    /// The resulting position is *at least* (0, 0) and *less than* (width, height).
    [[nodiscard]] auto clamp(Position position) const noexcept -> Position {
        return {
            position.x().clamped(Coordinate{0}, widthForPosition()),
            position.y().clamped(Coordinate{0}, heightForPosition())};
    }

public: // conversion
    /// Compute the area (width * height).
    /// @return The area. Note: returns 0 if either dimension is 0.
    [[nodiscard]] constexpr auto area() const noexcept -> math::SatInt32 {
        return math::SatInt32{math::saturatingMultiplyBounded(
            _width.toRawValue(),
            _height.toRawValue(),
            math::SatInt32::minimum().toRawValue(),
            math::SatInt32::maximum().toRawValue())};
    }
    /// Convert a 2D position to a row-major linear index.
    /// @param pos The position. Behavior is undefined if not contained by this size.
    /// @return y * width + x.
    [[nodiscard]] auto index(const Position &pos) const noexcept -> std::size_t {
        return (pos.y() * _width + pos.x()).toSizeT();
    }

public: // tools
    /// Rotate a local position counter-clockwise inside this size.
    /// Positions outside the size are transformed by the same affine mapping. Rotation is normalized to 90 degree
    /// steps, so negative values rotate clockwise.
    /// @param pos The local position to rotate.
    /// @param rotation The number of 90 degree counter-clockwise rotation steps.
    /// @return The transformed local position.
    [[nodiscard]] auto rotateCCW(const Position &pos, int rotation) const noexcept -> Position;
    /// Mirror a local position horizontally or vertically inside this size.
    /// Horizontal mirroring exchanges left and right. Vertical mirroring exchanges top and bottom.
    /// @param pos The local position to mirror.
    /// @param orientation The mirror orientation.
    /// @return The transformed local position.
    [[nodiscard]] auto mirror(const Position &pos, geometry::Orientation orientation) const noexcept -> Position;
    /// Transform a local position using a block symmetry.
    /// @param pos The local position to transform.
    /// @param symmetry The symmetry to apply.
    /// @return The transformed local position.
    [[nodiscard]] auto transform(const Position &pos, geometry::Symmetry symmetry) const noexcept -> Position;
    /// Visit all positions inside the size in row-major order.
    /// @tparam Fn A callable taking Position.
    /// @param fn The function to invoke for each Position (x from 0..width-1, y from 0..height-1).
    template <typename Fn>
    void forEach(Fn fn) const;

public: // factory methods
    /// Get the maximum size that can be represented by this type.
    [[nodiscard]] static constexpr auto maximum() noexcept -> Size {
        return Size{Coordinate::maximum(), Coordinate::maximum()};
    }
    /// Get the minimum size that can be represented by this type.
    [[nodiscard]] static constexpr auto minimum() noexcept -> Size { return Size{Coordinate{0}, Coordinate{0}}; }
    /// Get a size.
    [[nodiscard]] static constexpr auto zero() noexcept -> Size { return Size{Coordinate{0}, Coordinate{0}}; }

private:
    /// Calculate the horizontal content offset for an alignment.
    [[nodiscard]] static auto horizontalAlignmentOffset(
        Coordinate availableWidth, Coordinate contentWidth, geometry::Alignment alignment) noexcept -> Coordinate;
    /// Calculate the vertical content offset for an alignment.
    [[nodiscard]] static auto verticalAlignmentOffset(
        Coordinate availableHeight, Coordinate contentHeight, geometry::Alignment alignment) noexcept -> Coordinate;
    /// The maximum valid x coordinate inside the size, or 0 if width == 0.
    [[nodiscard]] auto widthForPosition() const noexcept -> Coordinate { return std::max(Coordinate{0}, _width - 1); }
    /// Half of the valid x range endpoint used for horizontal center positioning.
    [[nodiscard]] auto halfWidthForPosition() const noexcept -> Coordinate { return widthForPosition() / 2; }
    /// The maximum valid y coordinate inside the size, or 0 if height == 0.
    [[nodiscard]] auto heightForPosition() const noexcept -> Coordinate { return std::max(Coordinate{0}, _height - 1); }
    /// Half of the valid y range endpoint used for vertical center positioning.
    [[nodiscard]] auto halfHeightForPosition() const noexcept -> Coordinate { return heightForPosition() / 2; }

private:
    Coordinate _width;
    Coordinate _height;
};

}

#include "Size.tpp"
