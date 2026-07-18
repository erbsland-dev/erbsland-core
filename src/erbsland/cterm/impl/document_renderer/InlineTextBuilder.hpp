// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "InlineContent.hpp"

#include "../BlockStringBuilder.hpp"

#include "../../../text/String.hpp"
#include "../../Block.hpp"
#include "../../BlockString.hpp"
#include "../../BlockStringEditor.hpp"
#include "../../BlockStyle.hpp"

#include <vector>

namespace erbsland::cterm::impl::document_renderer {

/// Incremental builder for normalized inline terminal text.
/// @tested{TerminalDocumentRendererTest}
class InlineTextBuilder final {
public:
    /// Reset this builder while keeping reusable storage.
    void reset() noexcept;
    /// Append plain text with optional whitespace preservation.
    /// @param text The text to append.
    /// @param style The style for appended characters.
    /// @param preserveWhitespace Preserve whitespace as-is when `true`.
    void appendText(const text::String &text, BlockStyle style, bool preserveWhitespace);
    /// Append a terminal-string decoration.
    /// @param decoration The decoration to append.
    /// @param style The base style for decoration characters.
    /// @param preserveWhitespace Preserve whitespace as-is when `true`.
    void appendDecoration(BlockString decoration, BlockStyle style, bool preserveWhitespace);
    /// Append an explicit line break.
    /// @param style The style used for the line-break character.
    void appendLineBreak(BlockStyle style);
    /// Add a semantic soft break at the current source position.
    void addSoftBreak();
    /// Get the current source character count.
    /// @return The number of characters built so far.
    [[nodiscard]] auto length() const noexcept -> BlockCount { return _builder.length(); }
    /// Move the built string out and reset inline whitespace state.
    /// @return The built string.
    [[nodiscard]] auto takeString() -> BlockString;
    /// Move the built content and its semantic boundaries out.
    /// @return The built inline content.
    [[nodiscard]] auto takeContent() -> InlineContent;
    /// Mark a source range as indivisible.
    /// @param range The source range relative to this builder.
    void addIndivisibleRange(BlockRange range);

private:
    void appendTrimmedCharacter(const Block &character);
    void flushPendingWhitespace();

private:
    BlockStringBuilder _builder;           ///< Reusable output builder.
    std::vector<Block> _pendingWhitespace; ///< Whitespace delayed until the next non-space character.
    bool _atLineStart{true};               ///< `true` while trimming leading whitespace.
    paragraph::LayoutSemantics _semantics; ///< Semantic boundaries for the built source.
};

}
