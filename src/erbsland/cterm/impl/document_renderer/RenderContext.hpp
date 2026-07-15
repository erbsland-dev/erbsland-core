// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../BlockStyle.hpp"
#include "../../ParagraphIndents.hpp"
#include "../../TerminalDocumentStyleRule.hpp"

namespace erbsland::cterm::impl::document_renderer {

/// Inherited rendering context for nested document nodes.
/// @tested{TerminalDocumentRendererTest}
struct RenderContext final {
    BlockStyle textStyle; ///< The inherited text-style overlay.

    /// Create the child context for a container style rule.
    /// @param rule The resolved container style.
    /// @return The context inherited by child nodes.
    [[nodiscard]] auto withContainer(const TerminalDocumentStyleRule &rule) const -> RenderContext {
        return {textStyle.withOverlay(rule.textStyle())};
    }

    /// Resolve the text style for a block or inline rule.
    /// @param baseTextStyle The document base text style.
    /// @param rule The rule to apply.
    /// @return The resolved style.
    [[nodiscard]] auto resolvedTextStyle(
        const BlockStyle baseTextStyle, const TerminalDocumentStyleRule &rule) const noexcept -> BlockStyle {
        return baseTextStyle.withOverlay(textStyle).withOverlay(rule.textStyle());
    }

    /// Resolve the paragraph indents for one block.
    /// @param rule The rule to apply.
    /// @return The resolved paragraph indents.
    [[nodiscard]] auto resolvedIndents(const TerminalDocumentStyleRule &rule) const noexcept -> ParagraphIndents {
        return rule.indents();
    }
};

}
