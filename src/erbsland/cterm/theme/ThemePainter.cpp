// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ThemePainter.hpp"

#include <memory>
#include <utility>

namespace erbsland::cterm::theme {

ThemePainter::ThemePainter(WritableBuffer &buffer, ThemeAccessor theme) noexcept :
    _buffer{buffer}, _theme{std::move(theme)} {
}

void ThemePainter::fill(const bgeo::BlockRectangle rect) const noexcept {
    _buffer.fill(rect, std::make_shared<Tile9Style>(_theme.tile9Style()));
}

void ThemePainter::fillBackground(const bgeo::BlockRectangle rect) const noexcept {
    auto backgroundTheme = _theme.forPart(Part::Background);
    _buffer.fill(rect, std::make_shared<Tile9Style>(backgroundTheme.tile9Style()));
}

void ThemePainter::drawFrame(const bgeo::BlockRectangle rect) const noexcept {
    _buffer.drawFrame(rect, std::make_shared<Tile9Style>(_theme.tile9Style()));
}

}
