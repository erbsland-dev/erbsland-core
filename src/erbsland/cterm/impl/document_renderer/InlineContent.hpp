// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../paragraph/LayoutSemantics.hpp"

#include "../../BlockString.hpp"
#include "../../BlockStringView.hpp"

#include <utility>

namespace erbsland::cterm::impl::document_renderer {

/// Styled inline text with optional semantic layout boundaries.
///
/// The text is never modified to encode layout hints. All semantic indices refer directly to `text()`.
/// @tested{TerminalDocumentRendererTest}
class InlineContent final {
public:
    InlineContent() = default;
    /// Create metadata-free content from styled text.
    /// @param text The styled text.
    explicit InlineContent(BlockString text) : _text{std::move(text)} {}
    /// Create content with source-relative semantic boundaries.
    /// @param text The styled text.
    /// @param semantics The semantic boundaries for `text`.
    InlineContent(BlockString text, paragraph::LayoutSemantics semantics) :
        _text{std::move(text)}, _semantics{std::move(semantics)} {}

public:
    /// Access the styled source text.
    /// @return The source text.
    [[nodiscard]] auto text() const noexcept -> const BlockString & { return _text; }
    /// Access the semantic layout boundaries.
    /// @return The source-relative semantic boundaries.
    [[nodiscard]] auto semantics() const noexcept -> const paragraph::LayoutSemantics & { return _semantics; }
    /// Get the source character count.
    /// @return The number of styled characters.
    [[nodiscard]] auto length() const noexcept -> BlockCount { return _text.length(); }
    /// Get the rendered cell width.
    /// @return The source display width.
    [[nodiscard]] auto displayWidth() const noexcept -> int { return _text.displayWidth(); }
    /// Test whether the source is empty.
    /// @return `true` if no styled characters are present.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return _text.isEmpty(); }

public:
    /// Append metadata-free styled text.
    /// @param text The text to append.
    void append(BlockStringView text);
    /// Append content and shift its semantic boundaries to the new source position.
    /// @param content The content to append.
    void append(const InlineContent &content);
    /// Add a soft break at the current end of the source.
    void addSoftBreak();
    /// Mark a source range as indivisible.
    /// @param range The range relative to this content.
    void addIndivisibleRange(BlockRange range);

private:
    BlockString _text;
    paragraph::LayoutSemantics _semantics;
};

}
