// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BlockStyle.hpp"

namespace erbsland::cterm {

auto BlockStyle::withOverlay(const BlockStyle overlay) const noexcept -> BlockStyle {
    return BlockStyle{_color.overlayWith(overlay._color), overlay._attributes.withBase(_attributes)};
}

auto BlockStyle::withBase(const BlockStyle base) const noexcept -> BlockStyle {
    return BlockStyle{base._color.overlayWith(_color), _attributes.withBase(base._attributes)};
}

}
