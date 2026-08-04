// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BlockCombinationStyle.hpp"

#include "../text/String.hpp"
#include "../text/StringMap.hpp"

namespace erbsland::cterm {

/// Combine block characters through a map of current/overlay character pairs.
/// @tested{BlockCombinationStyleTest}
class SimpleBlockCombinationStyle : public BlockCombinationStyle {
public:
    /// The map, where the key is `<current>+<overlay>`, and the value is the combined character.
    using Map = text::StringMap<text::String>;

public:
    /// Create an empty combination style.
    SimpleBlockCombinationStyle() = default;
    /// Create a new instance from the given map.
    explicit SimpleBlockCombinationStyle(Map map) noexcept;

public: // implement BlockCombinationStyle
    [[nodiscard]] auto combine(const Block &current, const Block &overlay) const noexcept -> Block override;

public:
    /// Access the current map.
    [[nodiscard]] auto map() const noexcept -> const Map &;
    /// Replace the map.
    void setMap(Map map) noexcept;
    /// Add a new entry to the map.
    void add(const text::String &current, const text::String &overlay, const text::String &combined) noexcept;

private:
    Map _map;
};

}
