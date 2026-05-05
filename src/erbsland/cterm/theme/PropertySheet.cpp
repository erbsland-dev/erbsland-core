// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PropertySheet.hpp"

namespace erbsland::cterm::theme {

PropertySheet::PropertySheet() noexcept {
    _blocks.fill(U' ');
}

PropertySheet::PropertySheet(const PropertySheet &base, const Properties &properties) noexcept :
    _color{base._color},
    _colorSequence{base._colorSequence},
    _attributes{base._attributes},
    _blocks{base._blocks},
    _margins{base._margins},
    _padding{base._padding} {
    if (properties.color().has_value()) {
        _color = *properties.color();
        _colorSequence = ColorSequence{};
    }
    if (properties.colorSequence().has_value()) {
        _colorSequence = *properties.colorSequence();
    }
    if (properties.attributes().has_value()) {
        _attributes = *properties.attributes();
    }
    if (properties.blocks().has_value()) {
        const auto &blocks = *properties.blocks();
        for (auto index = std::size_t{0}; index < _blocks.size(); ++index) {
            if (blocks[index] != Properties::cInheritedBlock) {
                _blocks[index] = blocks[index];
            }
        }
    }
    if (properties.margins().has_value()) {
        _margins = *properties.margins();
    }
    if (properties.padding().has_value()) {
        _padding = *properties.padding();
    }
}

auto PropertySheet::color(const std::size_t animationCycle) const noexcept -> Color {
    if (!_colorSequence.empty()) {
        return _colorSequence.color(animationCycle);
    }
    return _color;
}

auto PropertySheet::style(const std::size_t animationCycle) const noexcept -> BlockStyle {
    return BlockStyle{color(animationCycle), _attributes};
}

auto PropertySheet::block(const BlockRole role, const std::size_t animationCycle) const noexcept -> Block {
    return Block{_blocks[static_cast<std::size_t>(role)], style(animationCycle)};
}

auto PropertySheet::tile9Style(const std::size_t animationCycle) const noexcept -> Tile9Style {
    return Tile9Style{_blocks, style(animationCycle)};
}

}
