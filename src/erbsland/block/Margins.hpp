// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "MarginPair.hpp"
#include "Size.hpp"

#include "../err/ParameterError.hpp"
#include "../geometry/Axis.hpp"
#include "../geometry/Dimensionality.hpp"
#include "../geometry/Orientation.hpp"
#include "../geometry/SignedAxis.hpp"
#include "../text/Literals.hpp"

#include <algorithm>
#include <cstdint>

namespace erbsland::block {

/// Represents margins (top, right, bottom, left) around a rectangle.
/// @seedoc{/reference/block/block_geometry}
/// @tested{MarginsTest}
class Margins final {
public:
    /// The component type used for axis mapping.
    using AxisComponent = MarginPair;
    /// The number of dimensions represented by this type.
    static constexpr auto cDimensionality = geometry::Dimensionality::Two;
    /// The side of the margin.
    enum class Side : uint8_t {
        Top = 0, ///< The top side.
        Right,   ///< The right side.
        Bottom,  ///< The bottom side.
        Left,    ///< The left side.
    };

public:
    /// Create zero margins.
    constexpr Margins() noexcept = default;
    /// Construct margins with the same value on all sides.
    /// @param allSides Value applied to top, right, bottom and left.
    constexpr explicit Margins(const Coordinate allSides) noexcept :
        _top{allSides}, _right{allSides}, _bottom{allSides}, _left{allSides} {}
    /// @overload
    constexpr explicit Margins(const int allSides) noexcept : Margins{Coordinate{allSides}} {}
    /// Construct margins with separate horizontal and vertical values.
    /// @param horizontal Value applied to left and right.
    /// @param vertical Value applied to top and bottom.
    constexpr Margins(const Coordinate horizontal, const Coordinate vertical) noexcept :
        _top{vertical}, _right{horizontal}, _bottom{vertical}, _left{horizontal} {}
    /// @overload
    constexpr Margins(const int horizontal, const int vertical) noexcept :
        Margins{Coordinate{horizontal}, Coordinate{vertical}} {}
    /// Construct margins from horizontal and vertical margin pairs.
    /// @param horizontal The left/right margins as leading/trailing values.
    /// @param vertical The top/bottom margins as leading/trailing values.
    constexpr Margins(const MarginPair horizontal, const MarginPair vertical) noexcept :
        _top{vertical.leading()},
        _right{horizontal.trailing()},
        _bottom{vertical.trailing()},
        _left{horizontal.leading()} {}
    /// Construct margins with individually specified sides.
    /// @param top Top margin.
    /// @param right Right margin.
    /// @param bottom Bottom margin.
    /// @param left Left margin.
    constexpr Margins(
        const Coordinate top, const Coordinate right, const Coordinate bottom, const Coordinate left) noexcept :
        _top{top}, _right{right}, _bottom{bottom}, _left{left} {}
    /// @overload
    constexpr Margins(const int top, const int right, const int bottom, const int left) noexcept :
        Margins{Coordinate{top}, Coordinate{right}, Coordinate{bottom}, Coordinate{left}} {}

public: // operators
    /// Compare two margin sets.
    auto operator==(const Margins &) const noexcept -> bool = default;
    /// Compare two margin sets.
    auto operator!=(const Margins &) const noexcept -> bool = default;
    /// Negate every margin.
    [[nodiscard]] constexpr auto operator-() const noexcept -> Margins { return {-_top, -_right, -_bottom, -_left}; }
    /// Get the margin at a side.
    [[nodiscard]] constexpr auto operator[](const Side side) const noexcept -> Coordinate { return at(side); }

public: // side accessors
    /// Get the top margin.
    [[nodiscard]] constexpr auto top() const noexcept -> Coordinate { return _top; }
    /// Set the top margin.
    constexpr void setTop(const Coordinate value) noexcept { _top = value; }
    /// @overload
    constexpr void setTop(const int value) noexcept { setTop(Coordinate{value}); }
    /// Get the right margin.
    [[nodiscard]] constexpr auto right() const noexcept -> Coordinate { return _right; }
    /// Set the right margin.
    constexpr void setRight(const Coordinate value) noexcept { _right = value; }
    /// @overload
    constexpr void setRight(const int value) noexcept { setRight(Coordinate{value}); }
    /// Get the bottom margin.
    [[nodiscard]] constexpr auto bottom() const noexcept -> Coordinate { return _bottom; }
    /// Set the bottom margin.
    constexpr void setBottom(const Coordinate value) noexcept { _bottom = value; }
    /// @overload
    constexpr void setBottom(const int value) noexcept { setBottom(Coordinate{value}); }
    /// Get the left margin.
    [[nodiscard]] constexpr auto left() const noexcept -> Coordinate { return _left; }
    /// Set the left margin.
    constexpr void setLeft(const Coordinate value) noexcept { _left = value; }
    /// @overload
    constexpr void setLeft(const int value) noexcept { setLeft(Coordinate{value}); }
    /// Get the margin at a side.
    [[nodiscard]] constexpr auto at(const Side side) const noexcept -> Coordinate {
        switch (side) {
        case Side::Top:
            return _top;
        case Side::Right:
            return _right;
        case Side::Bottom:
            return _bottom;
        case Side::Left:
            return _left;
        }
        return Coordinate{0};
    }
    /// Set the margin at a side.
    constexpr void set(const Side side, const Coordinate value) noexcept {
        switch (side) {
        case Side::Top:
            _top = value;
            break;
        case Side::Right:
            _right = value;
            break;
        case Side::Bottom:
            _bottom = value;
            break;
        case Side::Left:
            _left = value;
            break;
        }
    }
    /// @overload
    constexpr void set(const Side side, const int value) noexcept { set(side, Coordinate{value}); }

public: // axis components
    /// Get the horizontal left/right margin pair.
    [[nodiscard]] constexpr auto horizontal() const noexcept -> MarginPair { return {_left, _right}; }
    /// Set the horizontal left/right margin pair.
    constexpr void setHorizontal(const MarginPair margins) noexcept {
        _left = margins.leading();
        _right = margins.trailing();
    }
    /// Get the vertical top/bottom margin pair.
    [[nodiscard]] constexpr auto vertical() const noexcept -> MarginPair { return {_top, _bottom}; }
    /// Set the vertical top/bottom margin pair.
    constexpr void setVertical(const MarginPair margins) noexcept {
        _top = margins.leading();
        _bottom = margins.trailing();
    }
    /// Get the margin pair for an orientation.
    [[nodiscard]] constexpr auto component(const geometry::Orientation orientation) const noexcept -> MarginPair {
        return orientation == geometry::Orientation::Horizontal ? horizontal() : vertical();
    }
    /// Set the margin pair for an orientation.
    constexpr void setComponent(const geometry::Orientation orientation, const MarginPair margins) noexcept {
        if (orientation == geometry::Orientation::Horizontal) {
            setHorizontal(margins);
        } else {
            setVertical(margins);
        }
    }
    /// Get the margin pair for a physical axis.
    /// @param axis The physical axis to select.
    /// @return The selected margin pair.
    /// @throws err::ParameterError if `axis` is Z.
    [[nodiscard]] constexpr auto component(const geometry::Axis axis) const -> MarginPair {
        using namespace text::literals;
        switch (axis) {
        case geometry::Axis::X:
            return horizontal();
        case geometry::Axis::Y:
            return vertical();
        default:
            throw err::ParameterError{"The axis is outside the margins dimensionality."_el, "axis"_el};
        }
    }
    /// Set the margin pair for a physical axis.
    /// @param axis The physical axis to select.
    /// @param margins The new margin pair.
    /// @throws err::ParameterError if `axis` is Z.
    constexpr void setComponent(const geometry::Axis axis, const MarginPair margins) {
        using namespace text::literals;
        switch (axis) {
        case geometry::Axis::X:
            setHorizontal(margins);
            break;
        case geometry::Axis::Y:
            setVertical(margins);
            break;
        default:
            throw err::ParameterError{"The axis is outside the margins dimensionality."_el, "axis"_el};
        }
    }
    /// Get the margin pair for a signed physical axis.
    /// @param axis The signed physical axis to select.
    /// @return The selected pair, reversed when requested.
    /// @throws err::ParameterError if `axis` selects Z.
    [[nodiscard]] constexpr auto component(const geometry::SignedAxis axis) const -> MarginPair {
        const auto result = component(axis.axis());
        return axis.isReversed() ? result.reversed() : result;
    }

public: // calculations
    /// Get the positive horizontal and vertical extents.
    [[nodiscard]] auto extent() const noexcept -> Size { return {horizontal().extent(), vertical().extent()}; }
    /// Get the horizontal and vertical spacing.
    [[nodiscard]] constexpr auto spacing() const noexcept -> Size {
        return {horizontal().spacing(), vertical().spacing()};
    }

public: // constraints
    /// Expand all sides to at least the matching sides in another margin set.
    /// @param other The minimum margins.
    /// @return This margin set.
    constexpr auto expandTo(const Margins other) noexcept -> Margins & {
        setHorizontal(horizontal().expandedWith(other.horizontal()));
        setVertical(vertical().expandedWith(other.vertical()));
        return *this;
    }
    /// Expand both sides of one orientation.
    /// @param other The minimum margins.
    /// @param orientation The orientation to modify.
    /// @return This margin set.
    constexpr auto expandTo(const Margins other, const geometry::Orientation orientation) noexcept -> Margins & {
        setComponent(orientation, component(orientation).expandedWith(other.component(orientation)));
        return *this;
    }
    /// Expand one side.
    /// @param other The minimum margins.
    /// @param side The side to modify.
    /// @return This margin set.
    constexpr auto expandTo(const Margins other, const Side side) noexcept -> Margins & {
        set(side, std::max(at(side), other.at(side)));
        return *this;
    }
    /// Clamp all margins to zero or positive values.
    /// @return This margin set.
    constexpr auto expandPositive() noexcept -> Margins & {
        setHorizontal(horizontal().expandedPositive());
        setVertical(vertical().expandedPositive());
        return *this;
    }
    /// Limit all sides to the matching sides in another margin set.
    /// @param other The maximum margins.
    /// @return This margin set.
    constexpr auto limitTo(const Margins other) noexcept -> Margins & {
        setHorizontal(horizontal().limitedWith(other.horizontal()));
        setVertical(vertical().limitedWith(other.vertical()));
        return *this;
    }
    /// Limit both sides of one orientation.
    /// @param other The maximum margins.
    /// @param orientation The orientation to modify.
    /// @return This margin set.
    constexpr auto limitTo(const Margins other, const geometry::Orientation orientation) noexcept -> Margins & {
        setComponent(orientation, component(orientation).limitedWith(other.component(orientation)));
        return *this;
    }
    /// Limit one side.
    /// @param other The maximum margins.
    /// @param side The side to modify.
    /// @return This margin set.
    constexpr auto limitTo(const Margins other, const Side side) noexcept -> Margins & {
        set(side, std::min(at(side), other.at(side)));
        return *this;
    }
    /// Create a copy expanded to at least the matching sides in another margin set.
    /// @param other The minimum margins.
    /// @return The expanded margins.
    [[nodiscard]] constexpr auto expandedWith(const Margins other) const noexcept -> Margins {
        auto result = *this;
        result.expandTo(other);
        return result;
    }
    /// Create a copy expanded along one orientation.
    /// @param other The minimum margins.
    /// @param orientation The orientation to modify.
    /// @return The expanded margins.
    [[nodiscard]] constexpr auto expandedWith(
        const Margins other, const geometry::Orientation orientation) const noexcept -> Margins {
        auto result = *this;
        result.expandTo(other, orientation);
        return result;
    }
    /// Create a copy expanded on one side.
    /// @param other The minimum margins.
    /// @param side The side to modify.
    /// @return The expanded margins.
    [[nodiscard]] constexpr auto expandedWith(const Margins other, const Side side) const noexcept -> Margins {
        auto result = *this;
        result.expandTo(other, side);
        return result;
    }
    /// Create a copy clamped to zero or positive values.
    [[nodiscard]] constexpr auto expandedPositive() const noexcept -> Margins {
        auto result = *this;
        result.expandPositive();
        return result;
    }
    /// Create a copy limited to the matching sides in another margin set.
    /// @param other The maximum margins.
    /// @return The limited margins.
    [[nodiscard]] constexpr auto limitedWith(const Margins other) const noexcept -> Margins {
        auto result = *this;
        result.limitTo(other);
        return result;
    }
    /// Create a copy limited along one orientation.
    /// @param other The maximum margins.
    /// @param orientation The orientation to modify.
    /// @return The limited margins.
    [[nodiscard]] constexpr auto limitedWith(
        const Margins other, const geometry::Orientation orientation) const noexcept -> Margins {
        auto result = *this;
        result.limitTo(other, orientation);
        return result;
    }
    /// Create a copy limited on one side.
    /// @param other The maximum margins.
    /// @param side The side to modify.
    /// @return The limited margins.
    [[nodiscard]] constexpr auto limitedWith(const Margins other, const Side side) const noexcept -> Margins {
        auto result = *this;
        result.limitTo(other, side);
        return result;
    }

private:
    Coordinate _top;    ///< The top margin.
    Coordinate _right;  ///< The right margin.
    Coordinate _bottom; ///< The bottom margin.
    Coordinate _left;   ///< The left margin.
};

}
