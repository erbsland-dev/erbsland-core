// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../bgeo/BlockDirection.hpp"
#include "../bgeo/BlockRectangle.hpp"

#include <bitset>
#include <cstdint>

namespace erbsland::cterm {

/// Flags for crop edges and corners.
class CropEdges {
public:
    /// Bitset type storing the eight crop-edge directions.
    using Flags = std::bitset<8>;

public:
    /// No crop edges.
    CropEdges() = default;

    // defaults
    ~CropEdges() = default;
    CropEdges(const CropEdges &) = default;
    CropEdges(CropEdges &&) noexcept = default;
    auto operator=(const CropEdges &) -> CropEdges & = default;
    auto operator=(CropEdges &&) -> CropEdges & = default;

public:
    /// Compare two crop-edges objects.
    auto operator==(const CropEdges &) const noexcept -> bool = default;
    /// Compare two crop-edges objects.
    auto operator!=(const CropEdges &) const noexcept -> bool = default;

public:
    /// Get the raw flags.
    [[nodiscard]] auto flags() const noexcept -> Flags { return _flags; }
    /// Test if a crop edge is set.
    [[nodiscard]] auto isSet(const bgeo::BlockDirection direction) const noexcept -> bool {
        if (direction == bgeo::BlockDirection::None) {
            return false;
        }
        return _flags.test(indexFromDirection(direction));
    }
    /// Set a crop edge.
    void set(const bgeo::BlockDirection direction, bool value = true) noexcept {
        if (direction == bgeo::BlockDirection::None) {
            return;
        }
        _flags.set(indexFromDirection(direction), value);
    }
    /// Clear a crop edge.
    void clear(const bgeo::BlockDirection direction) noexcept {
        if (direction == bgeo::BlockDirection::None) {
            return;
        }
        _flags.reset(indexFromDirection(direction));
    }
    /// Reset all crop edges.
    void reset() noexcept { _flags.reset(); }

private:
    /// Convert a cardinal direction to its backing flag index.
    [[nodiscard]] auto indexFromDirection(const bgeo::BlockDirection direction) const noexcept -> std::size_t {
        return std::min(static_cast<std::size_t>(direction) - 1, _flags.size());
    }

public: // tools
    /// Test if a frame position in a view rectangle matches a given crop direction.
    /// This function automatically handles the corners of the view rectangle correctly.
    /// Corner directions, like `bgeo::BlockDirection::NorthEast` are only returned if `bgeo::BlockDirection::North` and
    /// `bgeo::BlockDirection::East` are set. Otherwise, the corner direction matches the main direction.
    [[nodiscard]] auto edgeForView(const bgeo::BlockPosition pos, const bgeo::BlockRectangle viewRect) const noexcept
        -> bgeo::BlockDirection {
        const auto frameDirection = viewRect.frameDirection(pos);
        if (frameDirection == bgeo::BlockDirection::None) {
            return bgeo::BlockDirection::None;
        }
        const bool north = frameDirection.contains(bgeo::BlockDirection::North) && isSet(bgeo::BlockDirection::North);
        const bool east = frameDirection.contains(bgeo::BlockDirection::East) && isSet(bgeo::BlockDirection::East);
        const bool south = frameDirection.contains(bgeo::BlockDirection::South) && isSet(bgeo::BlockDirection::South);
        const bool west = frameDirection.contains(bgeo::BlockDirection::West) && isSet(bgeo::BlockDirection::West);

        if (north && east) {
            return bgeo::BlockDirection::NorthEast;
        }
        if (south && east) {
            return bgeo::BlockDirection::SouthEast;
        }
        if (south && west) {
            return bgeo::BlockDirection::SouthWest;
        }
        if (north && west) {
            return bgeo::BlockDirection::NorthWest;
        }
        if (north) {
            return bgeo::BlockDirection::North;
        }
        if (east) {
            return bgeo::BlockDirection::East;
        }
        if (south) {
            return bgeo::BlockDirection::South;
        }
        if (west) {
            return bgeo::BlockDirection::West;
        }
        return bgeo::BlockDirection::None;
    }

    /// Create a crop edge instance for the given view/content situation.
    /// @param viewRect The view rectangle.
    /// @param contentRect The content rectangle.
    [[nodiscard]] static auto fromView(const bgeo::BlockRectangle viewRect, const bgeo::BlockRectangle contentRect)
        -> CropEdges {
        CropEdges result;
        const bool north = viewRect.y1() > contentRect.y1();
        const bool east = viewRect.x2() < contentRect.x2();
        const bool south = viewRect.y2() < contentRect.y2();
        const bool west = viewRect.x1() > contentRect.x1();
        const bool northEast = north && east;
        const bool northWest = north && west;
        const bool southEast = south && east;
        const bool southWest = south && west;
        result.set(bgeo::BlockDirection::North, north);
        result.set(bgeo::BlockDirection::East, east);
        result.set(bgeo::BlockDirection::South, south);
        result.set(bgeo::BlockDirection::West, west);
        result.set(bgeo::BlockDirection::NorthEast, northEast);
        result.set(bgeo::BlockDirection::NorthWest, northWest);
        result.set(bgeo::BlockDirection::SouthEast, southEast);
        result.set(bgeo::BlockDirection::SouthWest, southWest);
        return result;
    }

private:
    /// The flags for crop edges and corners.
    /// Bits are clockwise, starting with north.
    /// 0:N, 1:NE, 2:E, 3:SE, 4:S, 5:SW, 6:W, 7:NW
    Flags _flags{0};
};

}
