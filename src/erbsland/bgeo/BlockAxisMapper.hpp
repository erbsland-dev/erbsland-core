// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BlockPosition.hpp"
#include "BlockSize.hpp"
#include "Orientation.hpp"

namespace erbsland::bgeo {

/// Maps main/cross-axis values to horizontal/vertical geometry values.
/// @tested{BlockAxisMapperTest}
class BlockAxisMapper final {
public:
    /// Create a mapper for the given orientation.
    /// @param orientation The main axis orientation.
    constexpr explicit BlockAxisMapper(const Orientation orientation) noexcept : _orientation{orientation} {}

public:
    /// Create a size from main and cross-axis extents.
    /// @param main The size along the main axis.
    /// @param cross The size along the cross axis.
    /// @return The corresponding width and height.
    [[nodiscard]] constexpr auto size(const BlockCoordinate main, const BlockCoordinate cross) const noexcept
        -> BlockSize {
        return _orientation == Orientation::Horizontal ? BlockSize{main, cross} : BlockSize{cross, main};
    }
    /// @overload
    [[nodiscard]] constexpr auto size(const int main, const int cross) const noexcept -> BlockSize {
        return size(BlockCoordinate{main}, BlockCoordinate{cross});
    }
    /// Create a position from main and cross-axis coordinates.
    /// @param main The coordinate along the main axis.
    /// @param cross The coordinate along the cross axis.
    /// @return The corresponding x and y position.
    [[nodiscard]] constexpr auto position(
        const BlockCoordinate main, const BlockCoordinate cross = BlockCoordinate{0}) const noexcept -> BlockPosition {
        return _orientation == Orientation::Horizontal ? BlockPosition{main, cross} : BlockPosition{cross, main};
    }
    /// @overload
    [[nodiscard]] constexpr auto position(const int main, const int cross = 0) const noexcept -> BlockPosition {
        return position(BlockCoordinate{main}, BlockCoordinate{cross});
    }
    /// Select the horizontal value from a main/cross pair.
    /// @param main The value for the main axis.
    /// @param cross The value for the cross axis.
    /// @return `main` for horizontal orientation, otherwise `cross`.
    template <typename Value>
    [[nodiscard]] constexpr auto horizontalValue(Value main, Value cross) const noexcept -> Value {
        return _orientation == Orientation::Horizontal ? main : cross;
    }
    /// Select the vertical value from a main/cross pair.
    /// @param main The value for the main axis.
    /// @param cross The value for the cross axis.
    /// @return `cross` for horizontal orientation, otherwise `main`.
    template <typename Value>
    [[nodiscard]] constexpr auto verticalValue(Value main, Value cross) const noexcept -> Value {
        return _orientation == Orientation::Horizontal ? cross : main;
    }

private:
    Orientation _orientation;
};

}
