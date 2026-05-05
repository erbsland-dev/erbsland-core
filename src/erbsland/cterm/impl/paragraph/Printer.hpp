// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "RendererBase.hpp"

#include "../../CursorWriter.hpp"

#include <cstddef>
#include <optional>

namespace erbsland::cterm::impl::paragraph {

/// Print a shared paragraph layout sequentially to a cursor writer.
class Printer final : public RendererBase {
public:
    Printer(
        CursorWriter &writer,
        const int x1,
        const int width,
        const bgeo::Alignment alignment,
        const LayoutResult &layout,
        const BlockStringView &sourceText,
        const ParagraphOptions &options,
        const ParagraphBackgroundMode backgroundMode) noexcept :
        RendererBase{alignment, layout, sourceText, options, backgroundMode},
        _writer{writer},
        _x1{x1},
        _width{width},
        _baseStyle{writer.style()},
        _baseColor{writer.color()} {}

    // defaults/deletions
    ~Printer() = default;
    Printer(const Printer &) = delete;
    Printer(Printer &&) = delete;
    auto operator=(const Printer &) -> Printer & = delete;
    auto operator=(Printer &&) -> Printer & = delete;

public:
    /// Print the paragraph layout line by line.
    /// @return The number of physical lines written.
    [[nodiscard]] auto print() -> int;

private:
    [[nodiscard]] auto backgroundFillColor(Color color) const noexcept -> Color;
    [[nodiscard]] auto writeResolved(const LayoutLine &line) -> std::optional<Color>;
    [[nodiscard]] auto writeResolved(const LayoutFragment &fragment) -> std::optional<Color>;
    [[nodiscard]] auto writeResolvedBlock(const Block &character) -> std::optional<Color>;
    void writeSpaces(int count, Color color);

private:
    CursorWriter &_writer;
    int _x1;
    int _width;
    BlockStyle _baseStyle;
    Color _baseColor;
};

}
