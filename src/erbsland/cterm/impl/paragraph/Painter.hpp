// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "RendererBase.hpp"

#include "../../ParagraphBackgroundMode.hpp"
#include "../../WritableBuffer.hpp"

#include <functional>
#include <optional>

namespace erbsland::cterm::impl::paragraph {

/// Paint a shared paragraph layout into a writable buffer.
class Painter final : public RendererBase {
public:
    using ColorResolver = std::function<Color(const Block &, bgeo::BlockPosition)>;

public:
    /// Create a paragraph painter for a target buffer.
    Painter(
        WritableBuffer &buffer,
        const bgeo::BlockRectangle rect,
        const bgeo::Alignment alignment,
        const LayoutResult &layout,
        const BlockString &sourceText,
        const ParagraphOptions &options,
        const ParagraphBackgroundMode backgroundMode,
        const ColorResolver &colorResolver = {}) noexcept :
        RendererBase{alignment, layout, sourceText, options, backgroundMode},
        _buffer{buffer},
        _rect{rect},
        _colorResolver{colorResolver} {}

    // defaults/deletions
    ~Painter() = default;
    Painter(const Painter &) = delete;
    Painter(Painter &&) = delete;
    auto operator=(const Painter &) -> Painter & = delete;
    auto operator=(Painter &&) -> Painter & = delete;

public:
    /// Paint the given paragraph layout into a rectangle.
    void paint();

private:
    /// Draw one laid-out line and return its trailing color.
    [[nodiscard]] auto drawLine(const LayoutLine &line, bgeo::BlockPosition pos) -> std::optional<Color>;
    /// Draw one layout fragment and return its trailing color.
    [[nodiscard]] auto drawFragment(const LayoutFragment &fragment, bgeo::BlockPosition &pos) -> std::optional<Color>;
    /// Draw one terminal block and return its color.
    [[nodiscard]] auto drawBlock(const Block &character, bgeo::BlockPosition &pos) -> std::optional<Color>;
    /// Fill a horizontal background range.
    void fillBackgroundRange(int y, int x1, int x2, Color color);

private:
    WritableBuffer &_buffer;
    bgeo::BlockRectangle _rect;
    const ColorResolver &_colorResolver;
};

}
