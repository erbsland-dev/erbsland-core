// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Alignment.hpp"
#include "BlockAlignedSource_fwd.hpp"
#include "BlockDirection.hpp"
#include "BlockMargins.hpp"
#include "BlockPosition.hpp"
#include "BlockPositionList_fwd.hpp"
#include "BlockRectangle_fwd.hpp"
#include "BlockSize.hpp"

#include <cstdint>
#include <functional>
#include <type_traits>
#include <vector>

namespace erbsland::bgeo {

/// Axis-aligned rectangle represented by a top-left position and size.
/// Provides geometry utilities such as containment tests, expansion and iteration.
/// @seedoc{/reference/bgeo/block_geometry}
/// @tested{BlockRectTest}
class BlockRectangle {
public:
    /// Construct an empty rectangle at (0,0).
    BlockRectangle() = default;
    /// Construct from explicit position and size values.
    /// @param x X-coordinate of the top-left corner.
    /// @param y Y-coordinate of the top-left corner.
    /// @param width BlockRect width.
    /// @param height BlockRect height.
    constexpr BlockRectangle(
        BlockCoordinate x, BlockCoordinate y, BlockCoordinate width, BlockCoordinate height) noexcept :
        _pos{x, y}, _size{width, height} {}
    /// @overload
    constexpr BlockRectangle(const int x, const int y, const int width, const int height) noexcept :
        BlockRectangle{BlockCoordinate{x}, BlockCoordinate{y}, BlockCoordinate{width}, BlockCoordinate{height}} {}
    /// Construct from position and size objects.
    /// @param pos Top-left corner position.
    /// @param size BlockRect size.
    constexpr BlockRectangle(BlockPosition pos, BlockSize size) noexcept : _pos{pos}, _size{size} {}
    /// Construct from a top-left and bottom-right (exclusive) corner position.
    /// If the bottom right is left or above the top-left corner, the rectangle will be empty.
    /// @param topLeft The top-left corner *inside* the new rectangle.
    /// @param bottomRight The bottom-right corner *outside* the new rectangle.
    BlockRectangle(const BlockPosition topLeft, const BlockPosition bottomRight) noexcept :
        _pos{topLeft}, _size{bottomRight.x() - topLeft.x(), bottomRight.y() - topLeft.y()} {}

public: // operators
    /// Compare two rectangles.
    auto operator==(const BlockRectangle &other) const noexcept -> bool = default;
    /// Compare two rectangles.
    auto operator!=(const BlockRectangle &other) const noexcept -> bool = default;
    /// Merge two rectangles into a larger one that holds both rectangles.
    auto operator|(const BlockRectangle &other) const noexcept -> BlockRectangle;
    /// Expand this rectangle to include another rectangle.
    /// @param other The other rectangle to merge.
    /// @return Reference to this rectangle.
    auto operator|=(const BlockRectangle &other) noexcept -> BlockRectangle &;
    /// Intersect two rectangles to get only the overlapping part.
    /// If the two rectangles don't overlap, return an empty rectangle.
    auto operator&(const BlockRectangle &other) const noexcept -> BlockRectangle;
    /// Change this rectangle to the intersection of both rectangles.
    /// If the two rectangles don't overlap, its size will be (0,0).
    auto operator&=(const BlockRectangle &other) noexcept -> BlockRectangle &;

public: // attributes
    /// Get the top-left corner position.
    [[nodiscard]] constexpr auto pos() const noexcept -> BlockPosition { return _pos; }
    /// Set the top-left corner position.
    /// @param pos New position value.
    void setPos(BlockPosition pos) noexcept { _pos = pos; }
    /// Get the size of the rectangle.
    [[nodiscard]] constexpr auto size() const noexcept -> BlockSize { return _size; }
    /// Set the size of the rectangle.
    /// @param size New size value.
    void setSize(BlockSize size) noexcept { _size = size; }

public: // accessors
    /// Left x-coordinate.
    [[nodiscard]] constexpr auto x1() const noexcept -> BlockCoordinate { return _pos.x(); }
    /// Top y-coordinate.
    [[nodiscard]] constexpr auto y1() const noexcept -> BlockCoordinate { return _pos.y(); }
    /// Right x-coordinate (exclusive).
    [[nodiscard]] auto x2() const noexcept -> BlockCoordinate { return _pos.x() + _size.width(); }
    /// Bottom y-coordinate (exclusive).
    [[nodiscard]] auto y2() const noexcept -> BlockCoordinate { return _pos.y() + _size.height(); }
    /// Top-left corner position.
    /// Is equivalent to `pos()`.
    [[nodiscard]] constexpr auto topLeft() const noexcept -> BlockPosition { return _pos; }
    /// Top-right corner position (x-coordinate exclusive).
    [[nodiscard]] auto topRight() const noexcept -> BlockPosition { return {x2(), y1()}; }
    /// Bottom-left corner position (y-coordinate exclusive).
    [[nodiscard]] auto bottomLeft() const noexcept -> BlockPosition { return {x1(), y2()}; }
    /// Bottom-right corner position (x-coordinate exclusive, y-coordinate exclusive).
    [[nodiscard]] auto bottomRight() const noexcept -> BlockPosition { return {x2(), y2()}; }
    /// BlockRect width.
    [[nodiscard]] constexpr auto width() const noexcept -> BlockCoordinate { return _size.width(); }
    /// BlockRect height.
    [[nodiscard]] constexpr auto height() const noexcept -> BlockCoordinate { return _size.height(); }
    /// BlockPosition of a given anchor within this rectangle.
    /// @param anchor BlockAnchor to query.
    /// @return The position *inside* this rectangle matching the requested anchor.
    [[nodiscard]] auto anchor(BlockAnchor anchor = BlockAnchor::TopLeft) const noexcept -> BlockPosition;
    /// Center position.
    /// This is equal to the position of the `BlockAnchor::Center` anchor.
    [[nodiscard]] auto center() const noexcept -> BlockPosition { return anchor(BlockAnchor::Center); }

public: // tests
    /// Check if a position is inside the rectangle.
    /// @param testedPosition BlockPosition to test.
    /// @return `true` if the position lies inside the rectangle bounds.
    [[nodiscard]] auto contains(BlockPosition testedPosition) const noexcept -> bool;
    /// Checks if another rectangle fits into this one.
    /// Only `true` if every position of the tested rectangle is inside this one.
    /// @param testedRectangle The rectangle to test for containment.
    /// @return `true` if the tested rectangle is fully contained within this one.
    [[nodiscard]] auto contains(BlockRectangle testedRectangle) const noexcept -> bool;
    /// Check if another rectangle overlaps this one.
    /// Overlapping is when both rectangles share at least one position.
    /// @param testedRectangle The rectangle to test for overlap.
    [[nodiscard]] auto overlaps(BlockRectangle testedRectangle) const noexcept -> bool;
    /// Check if a position lies on the rectangle frame.
    /// @param testedPosition BlockPosition to test.
    /// @return `true` if the position lies on the outer frame of the rectangle.
    [[nodiscard]] auto isFrame(BlockPosition testedPosition) const noexcept -> bool;

public: // tools
    /// Get a hash for this rectangle.
    [[nodiscard]] auto hash() const noexcept -> std::size_t { return util::createHash(x1(), y1(), width(), height()); }
    /// Clamp a position to this rectangle.
    /// @param position The position to clamp.
    /// @return The clamped position where (x1 <= position.x <= x2) && (y1 <= position.y <= y2)
    [[nodiscard]] auto clamp(BlockPosition position) const noexcept -> BlockPosition;
    /// Create a rectangle expanded by the provided margins.
    /// @param margins BlockMargins to apply; positive values expand outward.
    /// @return The expanded rectangle.
    [[nodiscard]] auto expandedBy(BlockMargins margins) const noexcept -> BlockRectangle;
    /// Create a rectangle inset by the provided margins.
    /// @param margins BlockMargins to remove from each side.
    /// @return The inset rectangle.
    [[nodiscard]] auto insetBy(BlockMargins margins) const noexcept -> BlockRectangle;
    /// Create a sub-rectangle inside this rectangle.
    /// @param anchor The anchor of the rectangle.
    /// @param size The size. Zero means full width/height.
    /// @param margins The margins around the sub rectangle.
    /// @return The aligned sub-rectangle.
    [[nodiscard]] auto subRectangle(BlockAnchor anchor, BlockSize size, BlockMargins margins) const noexcept
        -> BlockRectangle;
    /// Compute the position for content aligned inside this rectangle.
    /// If `contentSize` is larger than this rectangle on an axis, the returned position on that axis lies before
    /// `topLeft()` on that axis.
    /// @param contentSize The aligned content size.
    /// @param alignment The alignment for the content.
    /// @return The aligned content position relative to the global coordinate space.
    [[nodiscard]] auto alignmentOffset(BlockSize contentSize, Alignment alignment) const noexcept -> BlockPosition;
    /// Align a source rectangle inside this rectangle and crop the larger side according to the alignment.
    /// If the source is smaller than this rectangle on an axis, the returned target rectangle is moved inside this
    /// rectangle. If the source is larger on an axis, the returned source rectangle is cropped on that axis.
    /// @param sourceRect The source rectangle before alignment and cropping.
    /// @param alignment The alignment used for placement or cropping.
    /// @return The effective target rectangle and source rectangle after alignment.
    [[nodiscard]] auto alignedSource(BlockRectangle sourceRect, Alignment alignment) const noexcept
        -> BlockAlignedSource;
    /// Get the clockwise border index for a frame position.
    /// The top-left corner has index `0`, then the index increases clockwise around the perimeter.
    /// Degenerate rectangles with width or height `1` still produce a continuous index sequence.
    /// @param testedPosition The position on the frame.
    /// @return The clockwise border index, or `-1` if the position is not on the frame.
    [[nodiscard]] auto frameIndex(BlockPosition testedPosition) const noexcept -> int64_t;
    /// Get the frame direction for a given position in this rectangle.
    /// @return The direction of the frame at the given position, or BlockDirection::None if the position is not on the
    /// frame.
    [[nodiscard]] auto frameDirection(BlockPosition testedPosition) const noexcept -> BlockDirection;
    /// Divide this rectangle into equally spaced grid cells.
    /// Each cell must be at least 1x1 in size, if this isn't possible, `std::invalid_argument` is thrown.
    /// @param rows The number of rows. Minimum 1.
    /// @param columns The number of columns. Minimum 1
    /// @param horizontalSpacing The spacing between cells horizontally.
    /// @param verticalSpacing The spacing between cells vertically.
    /// @return A vector of rectangles representing the grid cells from left to right, top to bottom.
    /// @throws std::invalid_argument if rows or columns are less than 1 or the chosen division is impossible.
    [[nodiscard]] auto gridCells(
        int rows,
        int columns,
        BlockCoordinate horizontalSpacing = BlockCoordinate{0},
        BlockCoordinate verticalSpacing = BlockCoordinate{0}) const -> std::vector<BlockRectangle>;
    /// @overload
    [[nodiscard]] auto gridCells(
        const int rows, const int columns, const int horizontalSpacing, const int verticalSpacing) const
        -> std::vector<BlockRectangle> {
        return gridCells(rows, columns, BlockCoordinate{horizontalSpacing}, BlockCoordinate{verticalSpacing});
    }
    /// Rotate a global position counter-clockwise around this rectangle.
    /// Positions outside the rectangle are transformed by the same affine mapping. Rotation is normalized to 90 degree
    /// steps, so negative values rotate clockwise.
    /// @param pos The global position to rotate.
    /// @param rotation The number of 90 degree counter-clockwise rotation steps.
    /// @return The transformed global position.
    [[nodiscard]] auto rotateCCW(const BlockPosition &pos, int rotation) const noexcept -> BlockPosition;
    /// Mirror a global position horizontally or vertically inside this rectangle.
    /// Horizontal mirroring exchanges left and right. Vertical mirroring exchanges top and bottom.
    /// @param pos The global position to mirror.
    /// @param orientation The mirror orientation.
    /// @return The transformed global position.
    [[nodiscard]] auto mirror(const BlockPosition &pos, Orientation orientation) const noexcept -> BlockPosition;
    /// Transform a global position using a block symmetry.
    /// @param pos The global position to transform.
    /// @param symmetry The symmetry to apply.
    /// @return The transformed global position.
    [[nodiscard]] auto transform(const BlockPosition &pos, Symmetry symmetry) const noexcept -> BlockPosition;
    /// Call a function for each position contained in the rectangle.
    /// @tparam Fn A callable with signature `void(BlockPosition)`.
    /// The function definition must be `void fn(BlockPosition pos)`.
    template <typename Fn>
    void forEach(Fn fn) const;
    /// Call a function for each position around the frame, clockwise with index.
    /// @tparam Fn A callable with signature `void(BlockPosition, int)`.
    /// The function definition must be `void fn(BlockPosition pos, int index)`.
    template <typename Fn>
        requires(std::is_invocable_r_v<void, Fn, BlockPosition, int> || std::is_invocable_r_v<void, Fn, BlockPosition>)
    void forEachInFrame(Fn fn) const;
    /// Get the bounds from the given positions.
    /// @param positions The positions to get the bounds from.
    /// @return A rectangle that contains all positions.
    [[nodiscard]] static auto bounds(const BlockPositionList &positions) noexcept -> BlockRectangle;

private:
    BlockPosition _pos;
    BlockSize _size;
};

}

#include "BlockRectangle.tpp"

template <>
struct std::hash<erbsland::bgeo::BlockRectangle> {
    auto operator()(const erbsland::bgeo::BlockRectangle &rect) const noexcept -> std::size_t { return rect.hash(); }
};
