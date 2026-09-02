// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AlignedSource_fwd.hpp"
#include "CoordinateSpan.hpp"
#include "Direction.hpp"
#include "Margins.hpp"
#include "Position.hpp"
#include "PositionList_fwd.hpp"
#include "Rectangle_fwd.hpp"
#include "Size.hpp"

#include "../err/ParameterError.hpp"
#include "../geometry/Alignment.hpp"
#include "../geometry/Anchor.hpp"
#include "../geometry/Axis.hpp"
#include "../geometry/Dimensionality.hpp"
#include "../geometry/Orientation.hpp"
#include "../geometry/SignedAxis.hpp"
#include "../text/Literals.hpp"

#include <cstdint>
#include <functional>
#include <type_traits>
#include <vector>

namespace erbsland::block {

/// Axis-aligned rectangle represented by a top-left position and size.
/// Provides geometry utilities such as containment tests, expansion and iteration.
/// @seedoc{/reference/block/block_geometry}
/// @tested{RectangleTest}
class Rectangle {
public:
    /// The component type used for axis mapping.
    using AxisComponent = CoordinateSpan;
    /// The number of dimensions represented by this type.
    static constexpr auto cDimensionality = geometry::Dimensionality::Two;

public:
    /// Construct an empty rectangle at (0,0).
    Rectangle() = default;
    /// Construct from explicit position and size values.
    /// @param x X-coordinate of the top-left corner.
    /// @param y Y-coordinate of the top-left corner.
    /// @param width BlockRect width.
    /// @param height BlockRect height.
    constexpr Rectangle(Coordinate x, Coordinate y, Coordinate width, Coordinate height) noexcept :
        _pos{x, y}, _size{width, height} {}
    /// @overload
    constexpr Rectangle(const int x, const int y, const int width, const int height) noexcept :
        Rectangle{Coordinate{x}, Coordinate{y}, Coordinate{width}, Coordinate{height}} {}
    /// Construct from position and size objects.
    /// @param pos Top-left corner position.
    /// @param size BlockRect size.
    constexpr Rectangle(Position pos, Size size) noexcept : _pos{pos}, _size{size} {}
    /// Construct from horizontal and vertical coordinate spans.
    /// @param horizontal The horizontal origin and extent.
    /// @param vertical The vertical origin and extent.
    constexpr Rectangle(const CoordinateSpan horizontal, const CoordinateSpan vertical) noexcept :
        _pos{horizontal.origin(), vertical.origin()}, _size{horizontal.extent(), vertical.extent()} {}
    /// Construct from a top-left and bottom-right (exclusive) corner position.
    /// If the bottom right is left or above the top-left corner, the rectangle will be empty.
    /// @param topLeft The top-left corner *inside* the new rectangle.
    /// @param bottomRight The bottom-right corner *outside* the new rectangle.
    Rectangle(const Position topLeft, const Position bottomRight) noexcept :
        _pos{topLeft}, _size{bottomRight.x() - topLeft.x(), bottomRight.y() - topLeft.y()} {}

public: // operators
    /// Compare two rectangles.
    auto operator==(const Rectangle &other) const noexcept -> bool = default;
    /// Compare two rectangles.
    auto operator!=(const Rectangle &other) const noexcept -> bool = default;
    /// Merge two rectangles into a larger one that holds both rectangles.
    auto operator|(const Rectangle &other) const noexcept -> Rectangle;
    /// Expand this rectangle to include another rectangle.
    /// @param other The other rectangle to merge.
    /// @return Reference to this rectangle.
    auto operator|=(const Rectangle &other) noexcept -> Rectangle &;
    /// Intersect two rectangles to get only the overlapping part.
    /// If the two rectangles don't overlap, return an empty rectangle.
    auto operator&(const Rectangle &other) const noexcept -> Rectangle;
    /// Change this rectangle to the intersection of both rectangles.
    /// If the two rectangles don't overlap, its size will be (0,0).
    auto operator&=(const Rectangle &other) noexcept -> Rectangle &;

public: // attributes
    /// Get the top-left corner position.
    [[nodiscard]] constexpr auto pos() const noexcept -> Position { return _pos; }
    /// Set the top-left corner position.
    /// @param pos New position value.
    void setPos(Position pos) noexcept { _pos = pos; }
    /// Get the size of the rectangle.
    [[nodiscard]] constexpr auto size() const noexcept -> Size { return _size; }
    /// Set the size of the rectangle.
    /// @param size New size value.
    void setSize(Size size) noexcept { _size = size; }

public: // accessors
    /// Left x-coordinate.
    [[nodiscard]] constexpr auto x1() const noexcept -> Coordinate { return _pos.x(); }
    /// Top y-coordinate.
    [[nodiscard]] constexpr auto y1() const noexcept -> Coordinate { return _pos.y(); }
    /// Right x-coordinate (exclusive).
    [[nodiscard]] auto x2() const noexcept -> Coordinate { return _pos.x() + _size.width(); }
    /// Bottom y-coordinate (exclusive).
    [[nodiscard]] auto y2() const noexcept -> Coordinate { return _pos.y() + _size.height(); }
    /// Top-left corner position.
    /// Is equivalent to `pos()`.
    [[nodiscard]] constexpr auto topLeft() const noexcept -> Position { return _pos; }
    /// Top-right corner position (x-coordinate exclusive).
    [[nodiscard]] auto topRight() const noexcept -> Position { return {x2(), y1()}; }
    /// Bottom-left corner position (y-coordinate exclusive).
    [[nodiscard]] auto bottomLeft() const noexcept -> Position { return {x1(), y2()}; }
    /// Bottom-right corner position (x-coordinate exclusive, y-coordinate exclusive).
    [[nodiscard]] auto bottomRight() const noexcept -> Position { return {x2(), y2()}; }
    /// BlockRect width.
    [[nodiscard]] constexpr auto width() const noexcept -> Coordinate { return _size.width(); }
    /// BlockRect height.
    [[nodiscard]] constexpr auto height() const noexcept -> Coordinate { return _size.height(); }
    /// Get the coordinate span for an orientation.
    /// @param orientation The orientation to select.
    /// @return The selected horizontal or vertical span.
    [[nodiscard]] constexpr auto component(const geometry::Orientation orientation) const noexcept -> CoordinateSpan {
        return orientation == geometry::Orientation::Horizontal ? CoordinateSpan{x1(), width()}
                                                                : CoordinateSpan{y1(), height()};
    }
    /// Get the coordinate span for a physical axis.
    /// @param axis The physical axis to select.
    /// @return The selected coordinate span.
    /// @throws err::ParameterError if `axis` is Z.
    [[nodiscard]] constexpr auto component(const geometry::Axis axis) const -> CoordinateSpan {
        using namespace text::literals;
        switch (axis) {
        case geometry::Axis::X:
            return {x1(), width()};
        case geometry::Axis::Y:
            return {y1(), height()};
        default:
            throw err::ParameterError{"The axis is outside the rectangle dimensionality."_el, "axis"_el};
        }
    }
    /// Get the coordinate span for a signed physical axis.
    /// @param axis The signed physical axis to select.
    /// @return The selected span, reversed when requested.
    /// @throws err::ParameterError if `axis` selects Z.
    [[nodiscard]] constexpr auto component(const geometry::SignedAxis axis) const -> CoordinateSpan {
        const auto result = component(axis.axis());
        return axis.isReversed() ? result.reversed() : result;
    }
    /// Position of a given anchor within this rectangle.
    /// @param anchor Anchor to query.
    /// @return The position *inside* this rectangle matching the requested anchor.
    [[nodiscard]] auto anchor(geometry::Anchor anchor = geometry::Anchor::TopLeft) const noexcept -> Position;
    /// Center position.
    /// This is equal to the position of the `geometry::Anchor::Center` anchor.
    [[nodiscard]] auto center() const noexcept -> Position { return anchor(geometry::Anchor::Center); }

public: // tests
    /// Check if a position is inside the rectangle.
    /// @param testedPosition Position to test.
    /// @return `true` if the position lies inside the rectangle bounds.
    [[nodiscard]] auto contains(Position testedPosition) const noexcept -> bool;
    /// Checks if another rectangle fits into this one.
    /// Only `true` if every position of the tested rectangle is inside this one.
    /// @param testedRectangle The rectangle to test for containment.
    /// @return `true` if the tested rectangle is fully contained within this one.
    [[nodiscard]] auto contains(Rectangle testedRectangle) const noexcept -> bool;
    /// Check if another rectangle overlaps this one.
    /// Overlapping is when both rectangles share at least one position.
    /// @param testedRectangle The rectangle to test for overlap.
    [[nodiscard]] auto overlaps(Rectangle testedRectangle) const noexcept -> bool;
    /// Check if a position lies on the rectangle frame.
    /// @param testedPosition Position to test.
    /// @return `true` if the position lies on the outer frame of the rectangle.
    [[nodiscard]] auto isFrame(Position testedPosition) const noexcept -> bool;

public: // tools
    /// Get a hash for this rectangle.
    [[nodiscard]] auto hash() const noexcept -> std::size_t { return util::createHash(x1(), y1(), width(), height()); }
    /// Clamp a position to this rectangle.
    /// @param position The position to clamp.
    /// @return The clamped position where (x1 <= position.x <= x2) && (y1 <= position.y <= y2)
    [[nodiscard]] auto clamp(Position position) const noexcept -> Position;
    /// Create a rectangle expanded by the provided margins.
    /// @param margins Margins to apply; positive values expand outward.
    /// @return The expanded rectangle.
    [[nodiscard]] auto expandedBy(Margins margins) const noexcept -> Rectangle;
    /// Create a rectangle inset by the provided margins.
    /// @param margins Margins to remove from each side.
    /// @return The inset rectangle.
    [[nodiscard]] auto insetBy(Margins margins) const noexcept -> Rectangle;
    /// Create a sub-rectangle inside this rectangle.
    /// @param anchor The anchor of the rectangle.
    /// @param size The size. Zero means full width/height.
    /// @param margins The margins around the sub rectangle.
    /// @return The aligned sub-rectangle.
    [[nodiscard]] auto subRectangle(geometry::Anchor anchor, Size size, Margins margins) const noexcept -> Rectangle;
    /// Compute the position for content aligned inside this rectangle.
    /// If `contentSize` is larger than this rectangle on an axis, the returned position on that axis lies before
    /// `topLeft()` on that axis.
    /// @param contentSize The aligned content size.
    /// @param alignment The alignment for the content.
    /// @return The aligned content position relative to the global coordinate space.
    [[nodiscard]] auto alignmentOffset(Size contentSize, geometry::Alignment alignment) const noexcept -> Position;
    /// Align a source rectangle inside this rectangle and crop the larger side according to the alignment.
    /// If the source is smaller than this rectangle on an axis, the returned target rectangle is moved inside this
    /// rectangle. If the source is larger on an axis, the returned source rectangle is cropped on that axis.
    /// @param sourceRect The source rectangle before alignment and cropping.
    /// @param alignment The alignment used for placement or cropping.
    /// @return The effective target rectangle and source rectangle after alignment.
    [[nodiscard]] auto alignedSource(Rectangle sourceRect, geometry::Alignment alignment) const noexcept
        -> AlignedSource;
    /// Get the clockwise border index for a frame position.
    /// The top-left corner has index `0`, then the index increases clockwise around the perimeter.
    /// Degenerate rectangles with width or height `1` still produce a continuous index sequence.
    /// @param testedPosition The position on the frame.
    /// @return The clockwise border index, or `-1` if the position is not on the frame.
    [[nodiscard]] auto frameIndex(Position testedPosition) const noexcept -> int64_t;
    /// Get the frame direction for a given position in this rectangle.
    /// @return The direction of the frame at the given position, or Direction::None if the position is not on the
    /// frame.
    [[nodiscard]] auto frameDirection(Position testedPosition) const noexcept -> Direction;
    /// Divide this rectangle into equally spaced grid cells.
    /// Each cell must be at least 1x1 in size, if this isn't possible, `err::ParameterError` is thrown.
    /// @param rows The number of rows. Minimum 1.
    /// @param columns The number of columns. Minimum 1
    /// @param horizontalSpacing The spacing between cells horizontally.
    /// @param verticalSpacing The spacing between cells vertically.
    /// @return A vector of rectangles representing the grid cells from left to right, top to bottom.
    /// @throws err::ParameterError if rows or columns are less than 1 or the chosen division is impossible.
    [[nodiscard]] auto gridCells(
        int rows,
        int columns,
        Coordinate horizontalSpacing = Coordinate{0},
        Coordinate verticalSpacing = Coordinate{0}) const -> std::vector<Rectangle>;
    /// @overload
    [[nodiscard]] auto gridCells(
        const int rows, const int columns, const int horizontalSpacing, const int verticalSpacing) const
        -> std::vector<Rectangle> {
        return gridCells(rows, columns, Coordinate{horizontalSpacing}, Coordinate{verticalSpacing});
    }
    /// Rotate a global position counter-clockwise around this rectangle.
    /// Positions outside the rectangle are transformed by the same affine mapping. Rotation is normalized to 90 degree
    /// steps, so negative values rotate clockwise.
    /// @param pos The global position to rotate.
    /// @param rotation The number of 90 degree counter-clockwise rotation steps.
    /// @return The transformed global position.
    [[nodiscard]] auto rotateCCW(const Position &pos, int rotation) const noexcept -> Position;
    /// Mirror a global position horizontally or vertically inside this rectangle.
    /// Horizontal mirroring exchanges left and right. Vertical mirroring exchanges top and bottom.
    /// @param pos The global position to mirror.
    /// @param orientation The mirror orientation.
    /// @return The transformed global position.
    [[nodiscard]] auto mirror(const Position &pos, geometry::Orientation orientation) const noexcept -> Position;
    /// Transform a global position using a block symmetry.
    /// @param pos The global position to transform.
    /// @param symmetry The symmetry to apply.
    /// @return The transformed global position.
    [[nodiscard]] auto transform(const Position &pos, geometry::Symmetry symmetry) const noexcept -> Position;
    /// Call a function for each position contained in the rectangle.
    /// @tparam Fn A callable with signature `void(Position)`.
    /// The function definition must be `void fn(Position pos)`.
    template <typename Fn>
    void forEach(Fn fn) const;
    /// Call a function for each position around the frame, clockwise with index.
    /// @tparam Fn A callable with signature `void(Position, int)`.
    /// The function definition must be `void fn(Position pos, int index)`.
    template <typename Fn>
        requires(std::is_invocable_r_v<void, Fn, Position, int> || std::is_invocable_r_v<void, Fn, Position>)
    void forEachInFrame(Fn fn) const;
    /// Get the bounds from the given positions.
    /// @param positions The positions to get the bounds from.
    /// @return A rectangle that contains all positions.
    [[nodiscard]] static auto bounds(const PositionList &positions) noexcept -> Rectangle;

private:
    Position _pos;
    Size _size;
};

}

#include "Rectangle.tpp"

template <>
struct std::hash<erbsland::block::Rectangle> {
    auto operator()(const erbsland::block::Rectangle &rect) const noexcept -> std::size_t { return rect.hash(); }
};
