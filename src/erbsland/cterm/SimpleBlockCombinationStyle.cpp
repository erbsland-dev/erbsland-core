// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SimpleBlockCombinationStyle.hpp"

#include <utility>

namespace erbsland::cterm {

using namespace text::literals;

SimpleBlockCombinationStyle::SimpleBlockCombinationStyle(Map map) noexcept : _map{std::move(map)} {
}

auto SimpleBlockCombinationStyle::combine(const Block &current, const Block &overlay) const noexcept -> Block {
    auto result = Block{};
    const auto key = text::String::fromJoined({current.toString(), overlay.toString()});
    if (const auto combined = map().get(key)) {
        result = Block{*combined};
    } else {
        result = Block{overlay.toString()};
    }
    result.setStyle(result.style().withBase(current.style()).withOverlay(overlay.style()));
    return result;
}

auto SimpleBlockCombinationStyle::map() const noexcept -> const Map & {
    return _map;
}

void SimpleBlockCombinationStyle::setMap(Map map) noexcept {
    _map = std::move(map);
}

void SimpleBlockCombinationStyle::add(
    const text::String &current, const text::String &overlay, const text::String &combined) noexcept {
    _map.set(text::String::fromJoined({current, overlay}), combined.copy());
}

}
