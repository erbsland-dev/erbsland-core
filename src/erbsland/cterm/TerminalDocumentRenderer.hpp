// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CursorWriter.hpp"
#include "TerminalDocumentStyle.hpp"

#include "../text/TextDocument.hpp"
#include "../text/TextNode.hpp"

namespace erbsland::cterm {

/// Render a `text::TextDocument` tree as styled terminal output.
///
/// A reported writer width of at least 60 cells is used as-is. Widths from 1 through 59, unknown widths, and zero
/// widths use an 80-column layout; a narrow destination may wrap the completed physical lines naturally. The complete
/// document layout is materialized before output begins, so allocation or layout failures do not partially write the
/// document.
/// @tested{TerminalDocumentRendererTest}
class TerminalDocumentRenderer final {
public:
    /// Create a renderer with the plain default style.
    TerminalDocumentRenderer();
    /// Create a renderer with the given style sheet.
    /// @param style The document style to use.
    explicit TerminalDocumentRenderer(TerminalDocumentStyle style);

    // defaults
    ~TerminalDocumentRenderer() = default;
    TerminalDocumentRenderer(const TerminalDocumentRenderer &) = default;
    TerminalDocumentRenderer(TerminalDocumentRenderer &&) noexcept = default;
    auto operator=(const TerminalDocumentRenderer &) -> TerminalDocumentRenderer & = default;
    auto operator=(TerminalDocumentRenderer &&) noexcept -> TerminalDocumentRenderer & = default;

public:
    /// Access the current document style.
    /// @return The current document style.
    [[nodiscard]] auto style() const noexcept -> const TerminalDocumentStyle & { return _style; }
    /// Replace the current document style.
    /// @param style The new document style.
    void setStyle(TerminalDocumentStyle style) noexcept;
    /// Render a document to a cursor writer.
    ///
    /// The writer receives one completed styled line followed by one line break for every physical document line.
    /// @param writer The output writer.
    /// @param document The document to render.
    /// @throws std::bad_alloc If intermediate layout storage cannot be allocated. The writer remains unchanged.
    void renderTo(CursorWriter &writer, const text::TextDocument &document) const;
    /// Render a node tree to a cursor writer.
    ///
    /// The writer receives one completed styled line followed by one line break for every physical document line.
    /// @param writer The output writer.
    /// @param node The root node to render.
    /// @throws std::bad_alloc If intermediate layout storage cannot be allocated. The writer remains unchanged.
    void renderTo(CursorWriter &writer, const text::TextNode &node) const;

private:
    TerminalDocumentStyle _style; ///< The style sheet used for rendering.
};

}
