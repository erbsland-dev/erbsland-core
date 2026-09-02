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
    /// Create a paragraph printer for a cursor writer.
    Printer(
        CursorWriter &writer,
        const int x1,
        const int width,
        const geometry::Alignment alignment,
        const LayoutResult &layout,
        const BlockString &sourceText,
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
    /// Resolve the background fill color for an output color.
    [[nodiscard]] auto backgroundFillColor(Color color) const noexcept -> Color;
    /// Write one resolved layout line.
    [[nodiscard]] auto writeResolved(const LayoutLine &line) -> std::optional<Color>;
    /// Write one resolved layout fragment.
    [[nodiscard]] auto writeResolved(const LayoutFragment &fragment) -> std::optional<Color>;
    /// Write one resolved terminal block.
    [[nodiscard]] auto writeResolvedBlock(const Block &character) -> std::optional<Color>;
    /// Write filled spaces with a color.
    void writeSpaces(int count, Color color);

private:
    CursorWriter &_writer;
    int _x1;
    int _width;
    BlockStyle _baseStyle;
    Color _baseColor;
};

}
