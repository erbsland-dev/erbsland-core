// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "RenderBlock.hpp"

namespace erbsland::cterm::impl::document_renderer {

/// Prepared indentation and marker state for one rendered list item.
/// @tested{TerminalDocumentRendererTest}
struct ListItemLayout final {
    BlockString prefix;        ///< The rendered list marker.
    int firstLineOffset{0};    ///< Paragraph first-line offset for the marker.
    int continuationOffset{0}; ///< Paragraph continuation-line offset.

    /// Get the marker width.
    /// @return The marker width in terminal cells.
    [[nodiscard]] auto prefixWidth() const noexcept -> int { return prefix.displayWidth(); }
    /// Apply the marker and first-block list indentation.
    /// @param block The block to adjust.
    void applyPrefixTo(RenderBlock &block) const;
    /// Apply continuation indentation to later blocks in the item.
    /// @param block The block to adjust.
    void applyContinuationTo(RenderBlock &block) const;
};

}
