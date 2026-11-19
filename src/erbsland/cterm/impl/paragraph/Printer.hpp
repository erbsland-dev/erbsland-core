// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "RendererBase.hpp"

#include "../../CursorWriter_fwd.hpp"

#include <functional>
#include <optional>

namespace erbsland::cterm::impl::paragraph {

/// Emit the styled blocks of a paragraph in physical line order.
/// @tested{ParagraphRenderTest TerminalDocumentRendererTest}
class Printer final : public RendererBase {
public:
    /// Receives one block in output order.
    using WriteBlockFn = std::function<void(const Block &)>;

    /// Create a printer using the destination's base style to resolve inherited attributes and colors.
    /// @param x1 The starting column.
    /// @param width The width available for text.
    /// @param layout The completed paragraph layout.
    /// @param sourceText The text referenced by the layout.
    /// @param options The options used to lay out the paragraph.
    /// @param baseStyle The style inherited by paragraph content.
    Printer(
        int x1,
        int width,
        const LayoutResult &layout,
        const BlockString &sourceText,
        const ParagraphOptions &options,
        BlockStyle baseStyle) noexcept :
        RendererBase{options.alignment(), layout, sourceText, options, options.backgroundMode()},
        _x1{x1},
        _width{width},
        _baseStyle{baseStyle} {}

public:
    /// Print all lines with explicit line breaks.
    /// @param writer The destination cursor writer.
    /// @return The number of physical lines written.
    [[nodiscard]] auto printTo(CursorWriter &writer) -> int;
    /// Emit the resolved blocks of the next physical line, carrying its wrap color forward.
    /// @param line The next line in layout order.
    /// @param writeBlock Receives each styled block, excluding the final line break.
    void emitLine(const LayoutLine &line, const WriteBlockFn &writeBlock);

private:
    /// Inherit the background of adjacent text when filling a wrap.
    [[nodiscard]] auto fillColor(const std::optional<Color> &textColor) const noexcept -> Color;
    /// Emit padding with the paragraph's base attributes.
    void emitSpaces(int count, Color color, const WriteBlockFn &writeBlock) const;

private:
    int _x1;
    int _width;
    BlockStyle _baseStyle;
    std::optional<Color> _previousWrapColor; ///< Last text color of the preceding wrapped line.
};

}
