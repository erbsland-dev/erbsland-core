// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RendererBase.hpp"

namespace erbsland::cterm::impl::paragraph {

auto RendererBase::linePlacement(const LayoutLine &line, const int x1, const int width) const noexcept
    -> LinePlacement {
    const auto endMarkWidth = line.wrapsToNext ? options().lineBreakEndMark().displayWidth() : 0;
    const auto availableWidth = width - endMarkWidth;
    const auto textWidth = line.textWidth();
    auto textX = x1;
    if (_alignment.isRight()) {
        textX = x1 + availableWidth - textWidth;
    } else if (_alignment.isHorizontalCenter()) {
        textX = x1 + (availableWidth - textWidth) / 2;
    }
    return LinePlacement{
        .textX = textX, .textWidth = textWidth, .endMarkX = x1 + width - endMarkWidth, .endMarkWidth = endMarkWidth};
}

}
