// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BlockSize.hpp"
#include "Orientation.hpp"

#include <algorithm>
#include <cstdint>

namespace erbsland::bgeo {

/// Represents margins (top, right, bottom, left) around a rectangle.
/// @seedoc{/reference/bgeo/block_geometry}
/// @tested{BlockMarginsTest}
class BlockMargins final {
public:
    /// The side of the margin.
    enum class Side : uint8_t { Top = 0, Right, Bottom, Left };

public:
    /// Default construct with uninitialized values (useful for aggregate-style construction).
    BlockMargins() = default;
    /// Construct margins with the same value on all sides.
    /// @param allSides Value applied to top, right, bottom and left.
    constexpr explicit BlockMargins(const BlockCoordinate allSides) :
        _top(allSides), _right(allSides), _bottom(allSides), _left(allSides) {}
    /// @overload
    constexpr explicit BlockMargins(const int allSides) : BlockMargins{BlockCoordinate{allSides}} {}
    /// Construct margins with separate horizontal and vertical values.
    /// @param horizontal Value applied to left and right.
    /// @param vertical Value applied to top and bottom.
    constexpr BlockMargins(const BlockCoordinate horizontal, const BlockCoordinate vertical) :
        _top(vertical), _right(horizontal), _bottom(vertical), _left(horizontal) {}
    /// @overload
    constexpr BlockMargins(const int horizontal, const int vertical) :
        BlockMargins{BlockCoordinate{horizontal}, BlockCoordinate{vertical}} {}
    /// Construct margins with individually specified sides.
    /// @param top Top margin.
    /// @param right Right margin.
    /// @param bottom Bottom margin.
    /// @param left Left margin.
    constexpr BlockMargins(
        const BlockCoordinate top,
        const BlockCoordinate right,
        const BlockCoordinate bottom,
        const BlockCoordinate left) :
        _top(top), _right(right), _bottom(bottom), _left(left) {}
    /// @overload
    constexpr BlockMargins(const int top, const int right, const int bottom, const int left) :
        BlockMargins{BlockCoordinate{top}, BlockCoordinate{right}, BlockCoordinate{bottom}, BlockCoordinate{left}} {}

public: // operators
    /// Equality comparison (all sides must be equal).
    auto operator==(const BlockMargins &other) const noexcept -> bool = default;
    /// Inequality comparison.
    auto operator!=(const BlockMargins &other) const noexcept -> bool = default;

    /// Unary negation producing margins with all sides negated.
    /// Useful for reversing an inset/expansion operation.
    auto operator-() const noexcept -> BlockMargins { return {-_top, -_right, -_bottom, -_left}; }
    /// Get the margin at the given side.
    [[nodiscard]] constexpr auto operator[](const Side side) const noexcept -> BlockCoordinate { return at(side); }

public: // accessors
    /// Get top margin.
    [[nodiscard]] constexpr auto top() const noexcept -> BlockCoordinate { return _top; }
    /// Set the top margin.
    constexpr void setTop(const BlockCoordinate value) noexcept { _top = value; }
    /// @overload
    constexpr void setTop(const int value) noexcept { setTop(BlockCoordinate{value}); }
    /// Get the right margin.
    [[nodiscard]] constexpr auto right() const noexcept -> BlockCoordinate { return _right; }
    /// Set the right margin.
    constexpr void setRight(const BlockCoordinate value) noexcept { _right = value; }
    /// @overload
    constexpr void setRight(const int value) noexcept { setRight(BlockCoordinate{value}); }
    /// Get the bottom margin.
    [[nodiscard]] constexpr auto bottom() const noexcept -> BlockCoordinate { return _bottom; }
    /// Set the bottom margin.
    constexpr void setBottom(const BlockCoordinate value) noexcept { _bottom = value; }
    /// @overload
    constexpr void setBottom(const int value) noexcept { setBottom(BlockCoordinate{value}); }
    /// Get the left margin.
    [[nodiscard]] constexpr auto left() const noexcept -> BlockCoordinate { return _left; }
    /// Set the left margin.
    constexpr void setLeft(const BlockCoordinate value) noexcept { _left = value; }
    /// @overload
    constexpr void setLeft(const int value) noexcept { setLeft(BlockCoordinate{value}); }
    /// Get the margin at the given side.
    [[nodiscard]] constexpr auto at(const Side side) const noexcept -> BlockCoordinate {
        switch (side) {
        case Side::Top:
            return top();
        case Side::Right:
            return right();
        case Side::Bottom:
            return bottom();
        case Side::Left:
            return left();
        }
        return BlockCoordinate{0};
    }
    /// Set the margin at the given side.
    constexpr void set(const Side side, BlockCoordinate value) noexcept {
        switch (side) {
        case Side::Top:
            _top = value;
            return;
        case Side::Right:
            _right = value;
            return;
        case Side::Bottom:
            _bottom = value;
            return;
        case Side::Left:
            _left = value;
            return;
        default:
            break;
        }
    }
    /// @overload
    constexpr void set(const Side side, const int value) noexcept { set(side, BlockCoordinate{value}); }
    /// Get the leading side for an orientation.
    [[nodiscard]] static constexpr auto leadingSide(const Orientation orientation) noexcept -> Side {
        return orientation == Orientation::Horizontal ? Side::Left : Side::Top;
    }
    /// Get the trailing side for an orientation.
    [[nodiscard]] static constexpr auto trailingSide(const Orientation orientation) noexcept -> Side {
        return orientation == Orientation::Horizontal ? Side::Right : Side::Bottom;
    }
    /// Get the leading cross-axis side for an orientation.
    [[nodiscard]] static constexpr auto crossLeadingSide(const Orientation orientation) noexcept -> Side {
        return leadingSide(orientation.crossed());
    }
    /// Get the trailing cross-axis side for an orientation.
    [[nodiscard]] static constexpr auto crossTrailingSide(const Orientation orientation) noexcept -> Side {
        return trailingSide(orientation.crossed());
    }
    /// Get the leading margin for an orientation.
    [[nodiscard]] constexpr auto leading(const Orientation orientation) const noexcept -> BlockCoordinate {
        return at(leadingSide(orientation));
    }
    /// Get the trailing margin for an orientation.
    [[nodiscard]] constexpr auto trailing(const Orientation orientation) const noexcept -> BlockCoordinate {
        return at(trailingSide(orientation));
    }
    /// Get the leading cross-axis margin for an orientation.
    [[nodiscard]] constexpr auto crossLeading(const Orientation orientation) const noexcept -> BlockCoordinate {
        return at(crossLeadingSide(orientation));
    }
    /// Get the trailing cross-axis margin for an orientation.
    [[nodiscard]] constexpr auto crossTrailing(const Orientation orientation) const noexcept -> BlockCoordinate {
        return at(crossTrailingSide(orientation));
    }
    /// Get a new object with only the horizontal margins preserved.
    [[nodiscard]] constexpr auto horizontal() const noexcept -> BlockMargins {
        return {BlockCoordinate{0}, _right, BlockCoordinate{0}, _left};
    }
    /// Get a new object with only the vertical margins preserved.
    [[nodiscard]] constexpr auto vertical() const noexcept -> BlockMargins {
        return {_top, BlockCoordinate{0}, _bottom, BlockCoordinate{0}};
    }
    /// Get the positive horizontal extent consumed by the margins.
    /// @return `left() + right()` with negative sides treated as zero.
    [[nodiscard]] auto horizontalExtent() const noexcept -> BlockCoordinate {
        return std::max(_left, BlockCoordinate{0}) + std::max(_right, BlockCoordinate{0});
    }
    /// Get the positive vertical extent consumed by the margins.
    /// @return `top() + bottom()` with negative sides treated as zero.
    [[nodiscard]] auto verticalExtent() const noexcept -> BlockCoordinate {
        return std::max(_top, BlockCoordinate{0}) + std::max(_bottom, BlockCoordinate{0});
    }
    /// Get the positive extent consumed by the margins.
    /// @return A size containing the horizontal and vertical extents.
    [[nodiscard]] auto extent() const noexcept -> BlockSize { return {horizontalExtent(), verticalExtent()}; }
    /// Get the extent for the selected orientation.
    /// @param orientation The orientation that selects horizontal or vertical extent.
    /// @return `horizontalExtent()` for `Horizontal`, otherwise `verticalExtent()`.
    [[nodiscard]] auto extent(const Orientation orientation) const noexcept -> BlockCoordinate {
        return orientation == Orientation::Horizontal ? horizontalExtent() : verticalExtent();
    }
    /// Get the horizontal spacing represented by the larger positive horizontal side.
    [[nodiscard]] constexpr auto horizontalSpacing() const noexcept -> BlockCoordinate {
        return std::max<BlockCoordinate>({BlockCoordinate{0}, _left, _right});
    }
    /// Get the vertical spacing represented by the larger positive vertical side.
    [[nodiscard]] constexpr auto verticalSpacing() const noexcept -> BlockCoordinate {
        return std::max<BlockCoordinate>({BlockCoordinate{0}, _top, _bottom});
    }
    /// Get the spacing represented by the margins.
    /// @return A size containing horizontal and vertical spacing.
    [[nodiscard]] constexpr auto spacing() const noexcept -> BlockSize {
        return {horizontalSpacing(), verticalSpacing()};
    }
    /// Get the spacing for the selected orientation.
    /// @param orientation The orientation that selects horizontal or vertical spacing.
    /// @return `horizontalSpacing()` for `Horizontal`, otherwise `verticalSpacing()`.
    [[nodiscard]] constexpr auto spacing(const Orientation orientation) const noexcept -> BlockCoordinate {
        return orientation == Orientation::Horizontal ? horizontalSpacing() : verticalSpacing();
    }
    /// Get the horizontal delta caused by positive or negative margins.
    /// Saturation addition limits the value to the `BlockCoordinate` range.
    [[nodiscard]] auto horizontalDelta() const noexcept -> BlockCoordinate { return _left + _right; }
    /// Get the vertical delta caused by positive or negative margins.
    /// Saturation addition limits the value to the `BlockCoordinate` range.
    [[nodiscard]] auto verticalDelta() const noexcept -> BlockCoordinate { return _top + _bottom; }
    /// Get the delta for the given axis.
    [[nodiscard]] auto delta(const Orientation orientation) const noexcept {
        return orientation == Orientation::Horizontal ? horizontalDelta() : verticalDelta();
    }
    /// Expand all sides so they are at least the matching sides in another margin set.
    /// @param other The minimum margins to cover.
    /// @return This margin set.
    constexpr auto expandTo(const BlockMargins other) noexcept -> BlockMargins & {
        _top = std::max(_top, other._top);
        _right = std::max(_right, other._right);
        _bottom = std::max(_bottom, other._bottom);
        _left = std::max(_left, other._left);
        return *this;
    }
    /// Expand the margins along one axis so they are at least the matching sides in another margin set.
    /// @param other The minimum margins to cover.
    /// @param orientation The axis to modify.
    /// @return This margin set.
    constexpr auto expandTo(const BlockMargins other, const Orientation orientation) noexcept -> BlockMargins & {
        expandTo(other, leadingSide(orientation));
        expandTo(other, trailingSide(orientation));
        return *this;
    }
    /// Expand one side so it is at least the matching side in another margin set.
    /// @param other The minimum margins to cover.
    /// @param side The side to modify.
    /// @return This margin set.
    constexpr auto expandTo(const BlockMargins other, const Side side) noexcept -> BlockMargins & {
        set(side, std::max(at(side), other.at(side)));
        return *this;
    }
    /// Limit all sides to zero or positive values.
    /// @return This margin set.
    constexpr auto expandPositive() -> BlockMargins & {
        expandTo(BlockMargins{});
        return *this;
    }
    /// Limit all sides so they are at most the matching sides in another margin set.
    /// @param other The maximum margins to respect.
    /// @return This margin set.
    constexpr auto limitTo(const BlockMargins other) noexcept -> BlockMargins & {
        _top = std::min(_top, other._top);
        _right = std::min(_right, other._right);
        _bottom = std::min(_bottom, other._bottom);
        _left = std::min(_left, other._left);
        return *this;
    }
    /// Limit the margins along one axis so they are at most the matching sides in another margin set.
    /// @param other The maximum margins to respect.
    /// @param orientation The axis to modify.
    /// @return This margin set.
    constexpr auto limitTo(const BlockMargins other, const Orientation orientation) noexcept -> BlockMargins & {
        limitTo(other, leadingSide(orientation));
        limitTo(other, trailingSide(orientation));
        return *this;
    }
    /// Limit one side so it is at most the matching side in another margin set.
    /// @param other The maximum margins to respect.
    /// @param side The side to modify.
    /// @return This margin set.
    constexpr auto limitTo(const BlockMargins other, const Side side) noexcept -> BlockMargins & {
        set(side, std::min(at(side), other.at(side)));
        return *this;
    }
    /// Create a copy expanded so all sides are at least the matching sides in another margin set.
    /// @param other The minimum margins to cover.
    /// @return The expanded margins.
    [[nodiscard]] constexpr auto expandedWith(const BlockMargins other) const noexcept -> BlockMargins {
        auto result = *this;
        result.expandTo(other);
        return result;
    }
    /// Create a copy expanded on one axis so both sides are at least the matching sides in another margin set.
    /// @param other The minimum margins to cover.
    /// @param orientation The axis to modify.
    /// @return The expanded margins.
    [[nodiscard]] constexpr auto expandedWith(const BlockMargins other, const Orientation orientation) const noexcept
        -> BlockMargins {
        auto result = *this;
        result.expandTo(other, orientation);
        return result;
    }
    /// Create a copy expanded on one side so it is at least the matching side in another margin set.
    /// @param other The minimum margins to cover.
    /// @param side The side to modify.
    /// @return The expanded margins.
    [[nodiscard]] constexpr auto expandedWith(const BlockMargins other, const Side side) const noexcept
        -> BlockMargins {
        auto result = *this;
        result.expandTo(other, side);
        return result;
    }
    /// Create a copy expanding with all sides to zero or positive values.
    [[nodiscard]] constexpr auto expandedPositive() const noexcept -> BlockMargins {
        auto result = *this;
        result.expandPositive();
        return result;
    }
    /// Create a copy limited so all sides are at most the matching sides in another margin set.
    /// @param other The maximum margins to respect.
    /// @return The limited margins.
    [[nodiscard]] constexpr auto limitedWith(const BlockMargins other) const noexcept -> BlockMargins {
        auto result = *this;
        result.limitTo(other);
        return result;
    }
    /// Create a copy limited on one axis so both sides are at most the matching sides in another margin set.
    /// @param other The maximum margins to respect.
    /// @param orientation The axis to modify.
    /// @return The limited margins.
    [[nodiscard]] constexpr auto limitedWith(const BlockMargins other, const Orientation orientation) const noexcept
        -> BlockMargins {
        auto result = *this;
        result.limitTo(other, orientation);
        return result;
    }
    /// Create a copy limited on one side so it is at most the matching side in another margin set.
    /// @param other The maximum margins to respect.
    /// @param side The side to modify.
    /// @return The limited margins.
    [[nodiscard]] constexpr auto limitedWith(const BlockMargins other, const Side side) const noexcept -> BlockMargins {
        auto result = *this;
        result.limitTo(other, side);
        return result;
    }

private:
    BlockCoordinate _top{};
    BlockCoordinate _right{};
    BlockCoordinate _bottom{};
    BlockCoordinate _left{};
};

}
