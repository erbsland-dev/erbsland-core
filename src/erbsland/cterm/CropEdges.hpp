// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../block/Direction.hpp"
#include "../block/Rectangle.hpp"

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
    [[nodiscard]] auto isSet(const block::Direction direction) const noexcept -> bool {
        if (direction == block::Direction::None) {
            return false;
        }
        return _flags.test(indexFromDirection(direction));
    }
    /// Set a crop edge.
    void set(const block::Direction direction, bool value = true) noexcept {
        if (direction == block::Direction::None) {
            return;
        }
        _flags.set(indexFromDirection(direction), value);
    }
    /// Clear a crop edge.
    void clear(const block::Direction direction) noexcept {
        if (direction == block::Direction::None) {
            return;
        }
        _flags.reset(indexFromDirection(direction));
    }
    /// Reset all crop edges.
    void reset() noexcept { _flags.reset(); }

private:
    /// Convert a cardinal direction to its backing flag index.
    [[nodiscard]] auto indexFromDirection(const block::Direction direction) const noexcept -> std::size_t {
        return std::min(static_cast<std::size_t>(direction) - 1, _flags.size());
    }

public: // tools
    /// Test if a frame position in a view rectangle matches a given crop direction.
    /// This function automatically handles the corners of the view rectangle correctly.
    /// Corner directions, like `block::Direction::NorthEast` are only returned if `block::Direction::North`
    /// and `block::Direction::East` are set. Otherwise, the corner direction matches the main direction.
    [[nodiscard]] auto edgeForView(const block::Position pos, const block::Rectangle viewRect) const noexcept
        -> block::Direction {
        const auto frameDirection = viewRect.frameDirection(pos);
        if (frameDirection == block::Direction::None) {
            return block::Direction::None;
        }
        const bool north = frameDirection.contains(block::Direction::North) && isSet(block::Direction::North);
        const bool east = frameDirection.contains(block::Direction::East) && isSet(block::Direction::East);
        const bool south = frameDirection.contains(block::Direction::South) && isSet(block::Direction::South);
        const bool west = frameDirection.contains(block::Direction::West) && isSet(block::Direction::West);

        if (north && east) {
            return block::Direction::NorthEast;
        }
        if (south && east) {
            return block::Direction::SouthEast;
        }
        if (south && west) {
            return block::Direction::SouthWest;
        }
        if (north && west) {
            return block::Direction::NorthWest;
        }
        if (north) {
            return block::Direction::North;
        }
        if (east) {
            return block::Direction::East;
        }
        if (south) {
            return block::Direction::South;
        }
        if (west) {
            return block::Direction::West;
        }
        return block::Direction::None;
    }

    /// Create a crop edge instance for the given view/content situation.
    /// @param viewRect The view rectangle.
    /// @param contentRect The content rectangle.
    [[nodiscard]] static auto fromView(const block::Rectangle viewRect, const block::Rectangle contentRect)
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
        result.set(block::Direction::North, north);
        result.set(block::Direction::East, east);
        result.set(block::Direction::South, south);
        result.set(block::Direction::West, west);
        result.set(block::Direction::NorthEast, northEast);
        result.set(block::Direction::NorthWest, northWest);
        result.set(block::Direction::SouthEast, southEast);
        result.set(block::Direction::SouthWest, southWest);
        return result;
    }

private:
    /// The flags for crop edges and corners.
    /// Bits are clockwise, starting with north.
    /// 0:N, 1:NE, 2:E, 3:SE, 4:S, 5:SW, 6:W, 7:NW
    Flags _flags{0};
};

}
