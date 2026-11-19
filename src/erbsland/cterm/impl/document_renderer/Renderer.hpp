// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../text/AnyStringBuilder.hpp"
#include "../../../text/TextNode.hpp"
#include "../../CursorWriter.hpp"
#include "../../TerminalDocumentStyle.hpp"

namespace erbsland::cterm::impl::document_renderer {

/// Lay out a document and emit its completed physical lines.
/// @tested{TerminalDocumentRendererTest TextDocumentTest}
class Renderer final {
public:
    /// Create a renderer using the given style.
    /// @param style The style sheet to use.
    explicit Renderer(const TerminalDocumentStyle &style) noexcept;

public:
    /// Render a node tree directly to a cursor writer.
    /// @param writer The output writer.
    /// @param node The root node to render.
    void renderTo(CursorWriter &writer, const text::TextNode &node);

    /// Append a 120-column rendering without terminal styling or trailing padding.
    /// @param builder Destination for the rendered characters.
    /// @param node The root of the document.
    void appendPlainText(text::AnyStringBuilder &builder, const text::TextNode &node) const;

private:
    /// Lay out the document at the given display width.
    [[nodiscard]] auto layout(const text::TextNode &node, int width) const -> BlockStringLines;
    /// Get the writable display width after accounting for the writer state.
    [[nodiscard]] static auto effectiveWidth(const CursorWriter &writer) noexcept -> int;

private:
    const TerminalDocumentStyle &_style; ///< The style sheet used for rendering.
};

}
