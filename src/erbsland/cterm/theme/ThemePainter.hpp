// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ThemeAccessor.hpp"

#include "../WritableBuffer.hpp"

namespace erbsland::cterm::theme {

/// Thin convenience painter for rendering themed blocks into a writable buffer.
class ThemePainter final {
public:
    /// Create a themed painter.
    /// @param buffer The target buffer.
    /// @param theme The theme accessor to render.
    ThemePainter(WritableBuffer &buffer, ThemeAccessor theme) noexcept;

public:
    /// Fill a rectangle using a tile-9 pattern from the accessor.
    /// @param rect The rectangle to fill.
    void fill(bgeo::BlockRectangle rect) const noexcept;
    /// Fill the background using `Part::Background`, and a tile-9 pattern from accessor.
    void fillBackground(bgeo::BlockRectangle rect) const noexcept;
    /// Draw only the edge blocks for a rectangle.
    /// @param rect The rectangle to draw.
    void drawFrame(bgeo::BlockRectangle rect) const noexcept;

private:
    WritableBuffer &_buffer; ///< The target buffer.
    ThemeAccessor _theme;    ///< The themed part to render.
};

}
