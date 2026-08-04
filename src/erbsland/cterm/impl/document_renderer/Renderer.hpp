// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../text/TextNode.hpp"
#include "../../CursorWriter.hpp"
#include "../../TerminalDocumentStyle.hpp"

namespace erbsland::cterm::impl::document_renderer {

/// Coordinate the terminal document rendering stages and emit completed physical lines.
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

private:
    /// Get the writable display width after accounting for the writer state.
    [[nodiscard]] static auto effectiveWidth(const CursorWriter &writer) noexcept -> int;

private:
    const TerminalDocumentStyle &_style; ///< The style sheet used for rendering.
};

}
