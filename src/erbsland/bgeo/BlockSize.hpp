// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Alignment.hpp"
#include "BlockAnchor.hpp"
#include "BlockPosition.hpp"
#include "BlockSize_fwd.hpp"
#include "Orientation.hpp"
#include "Symmetry.hpp"

#include "../math/ConstexprSaturatingMath.hpp"

#include <algorithm>
#include <limits>

namespace erbsland::bgeo {

/// A non-negative 2D size (width × height).
/// - Width and height are clamped to be >= 0.
/// - Many operations assume a grid indexed from (0,0) to (width-1,height-1).
/// @seedoc{/reference/bgeo/block_geometry}
/// @tested{BlockSizeTest}
class BlockSize {
public:
    /// Construct a zero size (0 × 0).
    BlockSize() = default;
    /// Construct a size from explicit width and height.
    /// Negative inputs are clamped to 0.
    /// @param width The desired width (clamped to >= 0).
    /// @param height The desired height (clamped to >= 0).
    constexpr BlockSize(const BlockCoordinate width, const BlockCoordinate height) noexcept :
        _width{std::max(BlockCoordinate{0}, width)}, _height{std::max(BlockCoordinate{0}, height)} {}
    /// @overload
    constexpr BlockSize(const int width, const int height) noexcept :
        BlockSize{BlockCoordinate{width}, BlockCoordinate{height}} {}
    /// Construct a size from the axis-aligned distance between two positions.
    /// @param pos1 First position.
    /// @param pos2 Second position.
    /// @note The order of the positions does not matter; absolute differences are used.
    BlockSize(const BlockPosition pos1, const BlockPosition pos2) noexcept :
        _width{absoluteDifference(pos1.x(), pos2.x())}, _height{absoluteDifference(pos1.y(), pos2.y())} {}

public: // operators
    /// Equality comparison on width and height.
    auto operator==(const BlockSize &other) const noexcept -> bool = default;
    /// Inequality comparison on width and height.
    auto operator!=(const BlockSize &other) const noexcept -> bool = default;
    /// Add two sizes using saturated arithmetic.
    /// @param other The size to add.
    /// @return The component-wise sum, capped at the maximum coordinate value.
    auto operator+(const BlockSize &other) const noexcept -> BlockSize {
        auto result = *this;
        result.add(other);
        return result;
    }
    /// Subtract two sizes using saturated arithmetic.
    /// @param other The size to subtract.
    /// @return The component-wise difference, clamped to non-negative values.
    auto operator-(const BlockSize &other) const noexcept -> BlockSize {
        auto result = *this;
        result.subtract(other);
        return result;
    }
    /// Add a size using saturated arithmetic.
    /// @param other The size to add.
    /// @return This size.
    auto operator+=(const BlockSize &other) noexcept -> BlockSize & { return add(other); }
    /// Subtract a size using saturated arithmetic.
    /// @param other The size to subtract.
    /// @return This size.
    auto operator-=(const BlockSize &other) noexcept -> BlockSize & { return subtract(other); }

public: // attributes
    /// Get the width (>= 0).
    [[nodiscard]] constexpr auto width() const noexcept -> BlockCoordinate { return _width; }
    /// Set the width. Negative values are clamped to 0.
    /// @param width New width (clamped to >= 0).
    void setWidth(BlockCoordinate width) noexcept { _width = std::max(BlockCoordinate{0}, width); }
    /// @overload
    void setWidth(const int width) noexcept { setWidth(BlockCoordinate{width}); }
    /// Get the height (>= 0).
    [[nodiscard]] constexpr auto height() const noexcept -> BlockCoordinate { return _height; }
    /// Set the height. Negative values are clamped to 0.
    /// @param height New height (clamped to >= 0).
    void setHeight(BlockCoordinate height) noexcept { _height = std::max(BlockCoordinate{0}, height); }
    /// @overload
    void setHeight(const int height) noexcept { setHeight(BlockCoordinate{height}); }
    /// Get the size component for the selected orientation.
    /// @param orientation The orientation that selects width or height.
    /// @return `width()` for `Orientation::Horizontal`, otherwise `height()`.
    [[nodiscard]] constexpr auto coordinate(const Orientation orientation) const noexcept -> BlockCoordinate {
        return orientation == Orientation::Horizontal ? _width : _height;
    }
    /// Compute a position inside the rectangle defined by this size for a given anchor.
    /// Bottom-right resolves to (width-1, height-1), top-left to (0,0), etc.
    /// @param anchor The anchor describing the target corner/edge/center.
    /// @return The position inside the [0,width-1]×[0,height-1] grid (or (0,0) for empty dimensions).
    [[nodiscard]] auto anchor(BlockAnchor anchor) const noexcept -> BlockPosition;
    /// Compute the offset for content aligned inside this size.
    /// If `contentSize` is larger than this size on an axis, the returned offset on that axis is negative.
    /// @param contentSize The aligned content size.
    /// @param alignment The alignment for the content.
    /// @return The zero-based offset for placing the content inside this size.
    [[nodiscard]] auto alignmentOffset(BlockSize contentSize, Alignment alignment) const noexcept -> BlockPosition;

public: // tests
    /// Test if this size is zero.
    [[nodiscard]] constexpr auto isZero() const noexcept -> bool { return _width == 0 || _height == 0; }
    /// Check if this size fits completely into another size (component-wise <=).
    /// @param other The candidate container size.
    /// @return true if width <= other.width and height <= other.height.
    [[nodiscard]] auto fitsInto(const BlockSize other) const noexcept -> bool {
        return _width <= other._width && _height <= other._height;
    }
    /// Test if the width and height of this size is in the given range.
    /// @param minimum The minimum size.
    /// @param maximum The maximum size.
    /// @return true If this size is in the range `minimum`-`maximum`.
    [[nodiscard]] auto isInRange(const BlockSize minimum, const BlockSize maximum) const noexcept -> bool {
        return _width >= minimum._width && _height >= minimum._height && _width <= maximum._width &&
            _height <= maximum._height;
    }
    /// Check if a position lies strictly inside the bounds [0,width) × [0,height).
    /// @param pos The position to test.
    /// @return true if 0 <= x < width and 0 <= y < height.
    [[nodiscard]] constexpr auto contains(const BlockPosition &pos) const noexcept -> bool {
        return pos.x() >= 0 && pos.y() >= 0 && pos.x() < _width && pos.y() < _height;
    }

public: // constraints
    /// Add another size using saturated arithmetic.
    /// @param other The size to add.
    /// @return This size.
    auto add(const BlockSize other) noexcept -> BlockSize & {
        _width = _width + other._width;
        _height = _height + other._height;
        return *this;
    }
    /// Add another size only on the selected axis using saturated arithmetic.
    /// @param other The size to add.
    /// @param orientation The axis to modify.
    /// @return This size.
    auto add(const BlockSize other, const Orientation orientation) noexcept -> BlockSize & {
        if (orientation == Orientation::Horizontal) {
            _width = _width + other._width;
        } else {
            _height = _height + other._height;
        }
        return *this;
    }
    /// Subtract another size using saturated arithmetic and clamp the result to zero.
    /// @param other The size to subtract.
    /// @return This size.
    auto subtract(const BlockSize other) noexcept -> BlockSize & {
        setWidth(_width - other._width);
        setHeight(_height - other._height);
        return *this;
    }
    /// Subtract another size only on the selected axis using saturated arithmetic and clamp the result to zero.
    /// @param other The size to subtract.
    /// @param orientation The axis to modify.
    /// @return This size.
    auto subtract(const BlockSize other, const Orientation orientation) noexcept -> BlockSize & {
        if (orientation == Orientation::Horizontal) {
            setWidth(_width - other._width);
        } else {
            setHeight(_height - other._height);
        }
        return *this;
    }
    /// Expand this size so it is at least the given size.
    /// @param other The minimum size to cover.
    /// @return This size.
    auto expandTo(const BlockSize other) noexcept -> BlockSize & {
        _width = std::max(_width, other._width);
        _height = std::max(_height, other._height);
        return *this;
    }
    /// Expand this size so it is at least the given size on the selected axis.
    /// @param other The minimum size to cover.
    /// @param orientation The axis to modify.
    /// @return This size.
    auto expandTo(const BlockSize other, const Orientation orientation) noexcept -> BlockSize & {
        if (orientation == Orientation::Horizontal) {
            _width = std::max(_width, other._width);
        } else {
            _height = std::max(_height, other._height);
        }
        return *this;
    }
    /// Limit this size so it is at most the given size.
    /// @param other The maximum size to respect.
    /// @return This size.
    auto limitTo(const BlockSize other) noexcept -> BlockSize & {
        _width = std::min(_width, other._width);
        _height = std::min(_height, other._height);
        return *this;
    }
    /// Limit this size so it is at most the given size on the selected axis.
    /// @param other The maximum size to respect.
    /// @param orientation The axis to modify.
    /// @return This size.
    auto limitTo(const BlockSize other, const Orientation orientation) noexcept -> BlockSize & {
        if (orientation == Orientation::Horizontal) {
            _width = std::min(_width, other._width);
        } else {
            _height = std::min(_height, other._height);
        }
        return *this;
    }
    /// Create a copy expanded so it is at least the given size.
    /// @param other The minimum size to cover.
    /// @return The expanded size.
    [[nodiscard]] auto expandedWith(const BlockSize other) const noexcept -> BlockSize {
        auto result = *this;
        result.expandTo(other);
        return result;
    }
    /// Create a copy expanded so it is at least the given size on the selected axis.
    /// @param other The minimum size to cover.
    /// @param orientation The axis to modify.
    /// @return The expanded size.
    [[nodiscard]] auto expandedWith(const BlockSize other, const Orientation orientation) const noexcept -> BlockSize {
        auto result = *this;
        result.expandTo(other, orientation);
        return result;
    }
    /// Create a copy limited so it is at most the given size.
    /// @param other The maximum size to respect.
    /// @return The limited size.
    [[nodiscard]] auto limitedWith(const BlockSize other) const noexcept -> BlockSize {
        auto result = *this;
        result.limitTo(other);
        return result;
    }
    /// Create a copy limited so it is at most the given size on the selected axis.
    /// @param other The maximum size to respect.
    /// @param orientation The axis to modify.
    /// @return The limited size.
    [[nodiscard]] auto limitedWith(const BlockSize other, const Orientation orientation) const noexcept -> BlockSize {
        auto result = *this;
        result.limitTo(other, orientation);
        return result;
    }
    /// Component-wise clamp with a minimum and maximum size.
    /// @warning If minimum.width > maximum.width or minimum.height > maximum.height, the behavior is *undefined*.
    /// @param minimum The minimum size.
    /// @param maximum The maximum size.
    /// @return a size (min.width <= width <= max.width, min.height <= height <= max.height)
    [[nodiscard]] auto clampTo(const BlockSize minimum, const BlockSize maximum) const noexcept -> BlockSize {
        return {_width.clamped(minimum._width, maximum._width), _height.clamped(minimum._height, maximum._height)};
    }
    /// Clamp a position *inside* this size.
    /// The resulting position is *at least* (0, 0) and *less than* (width, height).
    [[nodiscard]] auto clamp(BlockPosition position) const noexcept -> BlockPosition {
        return {
            position.x().clamped(BlockCoordinate{0}, widthForPosition()),
            position.y().clamped(BlockCoordinate{0}, heightForPosition())};
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
    [[nodiscard]] auto index(const BlockPosition &pos) const noexcept -> std::size_t {
        return (pos.y() * _width + pos.x()).toSizeT();
    }

public: // tools
    /// Rotate a local position counter-clockwise inside this size.
    /// Positions outside the size are transformed by the same affine mapping. Rotation is normalized to 90 degree
    /// steps, so negative values rotate clockwise.
    /// @param pos The local position to rotate.
    /// @param rotation The number of 90 degree counter-clockwise rotation steps.
    /// @return The transformed local position.
    [[nodiscard]] auto rotateCCW(const BlockPosition &pos, int rotation) const noexcept -> BlockPosition;
    /// Mirror a local position horizontally or vertically inside this size.
    /// Horizontal mirroring exchanges left and right. Vertical mirroring exchanges top and bottom.
    /// @param pos The local position to mirror.
    /// @param orientation The mirror orientation.
    /// @return The transformed local position.
    [[nodiscard]] auto mirror(const BlockPosition &pos, Orientation orientation) const noexcept -> BlockPosition;
    /// Transform a local position using a block symmetry.
    /// @param pos The local position to transform.
    /// @param symmetry The symmetry to apply.
    /// @return The transformed local position.
    [[nodiscard]] auto transform(const BlockPosition &pos, Symmetry symmetry) const noexcept -> BlockPosition;
    /// Visit all positions inside the size in row-major order.
    /// @tparam Fn A callable taking BlockPosition.
    /// @param fn The function to invoke for each BlockPosition (x from 0..width-1, y from 0..height-1).
    template <typename Fn>
    void forEach(Fn fn) const;

public: // factory methods
    /// Get the maximum size that can be represented by this type.
    [[nodiscard]] static constexpr auto maximum() noexcept -> BlockSize {
        return BlockSize{BlockCoordinate::maximum(), BlockCoordinate::maximum()};
    }
    /// Get the minimum size that can be represented by this type.
    [[nodiscard]] static constexpr auto minimum() noexcept -> BlockSize {
        return BlockSize{BlockCoordinate{0}, BlockCoordinate{0}};
    }
    /// Get a size.
    [[nodiscard]] static constexpr auto zero() noexcept -> BlockSize {
        return BlockSize{BlockCoordinate{0}, BlockCoordinate{0}};
    }

private:
    /// Compute the absolute difference between two integer values.
    [[nodiscard]] static auto absoluteDifference(BlockCoordinate value1, BlockCoordinate value2) noexcept
        -> BlockCoordinate {
        return (value1 >= value2) ? (value1 - value2) : (value2 - value1);
    }
    /// The maximum valid x coordinate inside the size, or 0 if width == 0.
    [[nodiscard]] auto widthForPosition() const noexcept -> BlockCoordinate {
        return std::max(BlockCoordinate{0}, _width - 1);
    }
    /// Half of the valid x range endpoint used for horizontal center positioning.
    [[nodiscard]] auto halfWidthForPosition() const noexcept -> BlockCoordinate { return widthForPosition() / 2; }
    /// The maximum valid y coordinate inside the size, or 0 if height == 0.
    [[nodiscard]] auto heightForPosition() const noexcept -> BlockCoordinate {
        return std::max(BlockCoordinate{0}, _height - 1);
    }
    /// Half of the valid y range endpoint used for vertical center positioning.
    [[nodiscard]] auto halfHeightForPosition() const noexcept -> BlockCoordinate { return heightForPosition() / 2; }

private:
    BlockCoordinate _width;
    BlockCoordinate _height;
};

template <typename Fn>
void BlockSize::forEach(Fn fn) const {
    for (auto y = BlockCoordinate{0}; y < height(); ++y) {
        for (auto x = BlockCoordinate{0}; x < width(); ++x) {
            fn(BlockPosition{x, y});
        }
    }
}

}
