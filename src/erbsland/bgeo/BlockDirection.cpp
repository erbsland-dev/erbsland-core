// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BlockDirection.hpp"

#include "../text/Char.hpp"
#include "../text/Literals.hpp"

#include <algorithm>
#include <array>
#include <tuple>

namespace erbsland::bgeo {

using namespace erbsland::text::literals;

auto BlockDirection::contains(BlockDirection direction) const noexcept -> bool {
    switch (_value) {
    case North:
        return direction == North;
    case NorthEast:
        return direction == NorthEast || direction == North || direction == East;
    case East:
        return direction == East;
    case SouthEast:
        return direction == SouthEast || direction == South || direction == East;
    case South:
        return direction == South;
    case SouthWest:
        return direction == SouthWest || direction == South || direction == West;
    case West:
        return direction == West;
    case NorthWest:
        return direction == NorthWest || direction == North || direction == West;
    default:
        return false;
    }
}

auto BlockDirection::toDelta() const noexcept -> BlockPosition {
    auto it = std::ranges::find_if(directionToDeltaMap(), [this](const DirectionToDeltaEntry &entry) -> bool {
        return std::get<0>(entry) == _value;
    });
    if (it == directionToDeltaMap().end()) {
        return {};
    }
    return std::get<1>(*it);
}

auto BlockDirection::fromDelta(const BlockPosition delta) noexcept -> BlockDirection {
    const auto mask = (delta.x() > 0 ? 0b0001 : 0) | (delta.x() < 0 ? 0b0010 : 0) | (delta.y() > 0 ? 0b0100 : 0) |
        (delta.y() < 0 ? 0b1000 : 0);
    switch (mask) {
    case 0b0001:
        return East;
    case 0b0010:
        return West;
    case 0b0100:
        return South;
    case 0b0101:
        return SouthEast;
    case 0b0110:
        return SouthWest;
    case 0b1000:
        return North;
    case 0b1001:
        return NorthEast;
    case 0b1010:
        return NorthWest;
    default:
        return None;
    }
}

auto BlockDirection::toString() const noexcept -> text::StringView {
    auto it = std::ranges::find_if(directionToStringMap(), [this](const DirectionToStringEntry &entry) -> bool {
        return std::get<0>(entry) == _value;
    });
    if (it == directionToStringMap().end()) {
        return "none"_el;
    }
    return std::get<1>(*it);
}

auto BlockDirection::isValidString(const text::StringView text) noexcept -> bool {
    auto direction = BlockDirection{};
    return findStringDirection(text, direction);
}

auto BlockDirection::fromString(const text::StringView text) noexcept -> BlockDirection {
    auto direction = BlockDirection{};
    if (findStringDirection(text, direction)) {
        return direction;
    }
    return None;
}

auto BlockDirection::directionToDeltaMap() noexcept -> const DirectionToDeltaMap & {
    static const std::array<DirectionToDeltaEntry, _EnumCount> map = {
        std::make_tuple(None, BlockPosition{0, 0}),
        std::make_tuple(North, BlockPosition{0, -1}),
        std::make_tuple(NorthEast, BlockPosition{1, -1}),
        std::make_tuple(East, BlockPosition{1, 0}),
        std::make_tuple(SouthEast, BlockPosition{1, 1}),
        std::make_tuple(South, BlockPosition{0, 1}),
        std::make_tuple(SouthWest, BlockPosition{-1, 1}),
        std::make_tuple(West, BlockPosition{-1, 0}),
        std::make_tuple(NorthWest, BlockPosition{-1, -1}),
    };
    return map;
}

auto BlockDirection::directionToStringMap() noexcept -> const DirectionToStringMap & {
    static const std::array<DirectionToStringEntry, _EnumCount> map = {
        std::make_tuple(None, "none"_el),
        std::make_tuple(North, "north"_el),
        std::make_tuple(NorthEast, "north_east"_el),
        std::make_tuple(East, "east"_el),
        std::make_tuple(SouthEast, "south_east"_el),
        std::make_tuple(South, "south"_el),
        std::make_tuple(SouthWest, "south_west"_el),
        std::make_tuple(West, "west"_el),
        std::make_tuple(NorthWest, "north_west"_el),
    };
    return map;
}

auto BlockDirection::stringToDirectionMap() noexcept -> const StringToDirectionMap & {
    static const std::array<StringToDirectionEntry, 22> map = {
        std::make_tuple(""_el, None),
        std::make_tuple("none"_el, None),
        std::make_tuple("n"_el, North),
        std::make_tuple("north"_el, North),
        std::make_tuple("ne"_el, NorthEast),
        std::make_tuple("northeast"_el, NorthEast),
        std::make_tuple("north_east"_el, NorthEast),
        std::make_tuple("e"_el, East),
        std::make_tuple("east"_el, East),
        std::make_tuple("se"_el, SouthEast),
        std::make_tuple("southeast"_el, SouthEast),
        std::make_tuple("south_east"_el, SouthEast),
        std::make_tuple("s"_el, South),
        std::make_tuple("south"_el, South),
        std::make_tuple("sw"_el, SouthWest),
        std::make_tuple("southwest"_el, SouthWest),
        std::make_tuple("south_west"_el, SouthWest),
        std::make_tuple("w"_el, West),
        std::make_tuple("west"_el, West),
        std::make_tuple("nw"_el, NorthWest),
        std::make_tuple("northwest"_el, NorthWest),
        std::make_tuple("north_west"_el, NorthWest),
    };
    return map;
}

auto BlockDirection::findStringDirection(const text::StringView text, BlockDirection &direction) noexcept -> bool {
    auto it = std::ranges::find_if(stringToDirectionMap(), [text](const StringToDirectionEntry &entry) -> bool {
        return text.compare(text::StringView{std::get<0>(entry)}, text::Char::compareIdentifier) ==
            std::strong_ordering::equal;
    });
    if (it == stringToDirectionMap().end()) {
        return false;
    }
    direction = std::get<1>(*it);
    return true;
}

}
