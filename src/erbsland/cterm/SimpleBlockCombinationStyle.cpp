// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SimpleBlockCombinationStyle.hpp"

#include "../text/StringJoins.hpp"

#include <utility>

namespace erbsland::cterm {

using namespace text;
using namespace text::literals;

SimpleBlockCombinationStyle::SimpleBlockCombinationStyle(Map map) noexcept : _map{std::move(map)} {
}

auto SimpleBlockCombinationStyle::combine(const Block &current, const Block &overlay) const noexcept -> Block {
    auto result = Block{};
    const auto key = text::join(current.toString(), overlay.toString());
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

void SimpleBlockCombinationStyle::add(const String &current, const String &overlay, const String &combined) noexcept {
    _map.set(text::join(current, overlay), combined.copy());
}

}
