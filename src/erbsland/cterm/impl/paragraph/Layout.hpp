// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "LayoutContext.hpp"
#include "LayoutFragment.hpp"
#include "LayoutLine.hpp"
#include "LayoutNewlineMode.hpp"
#include "LayoutPreparedSourceLine.hpp"
#include "LayoutResult.hpp"
#include "LayoutSemantics.hpp"

#include "../../BlockRange.hpp"
#include "../../BlockString.hpp"
#include "../../ParagraphOptions.hpp"

#include <vector>

namespace erbsland::cterm::impl::paragraph {

/// Shared paragraph layout for wrapped terminal text.
class Layout final {
public:
    /// Create paragraph layout state for terminal text.
    Layout(
        const BlockString &text,
        int width,
        const ParagraphOptions &options,
        LayoutNewlineMode newlineMode,
        const LayoutSemantics *semantics = nullptr) noexcept;

    // defaults/deletions
    ~Layout() = default;
    Layout(const Layout &) = delete;
    Layout(Layout &&) = delete;
    auto operator=(const Layout &) -> Layout & = delete;
    auto operator=(Layout &&) -> Layout & = delete;

    /// Build the wrapped paragraph layout.
    /// @return The layout result.
    [[nodiscard]] auto build() -> LayoutResult;

private:
    /// Split the source text into newline-delimited ranges.
    [[nodiscard]] auto splitIntoSourceLines() const -> std::vector<BlockRange>;
    /// Lay out all source lines into visual paragraph lines.
    [[nodiscard]] auto layoutParagraph(const std::vector<BlockRange> &sourceLines, std::vector<LayoutLine> &lines)
        -> bool;
    /// Lay out a single source line into visual lines.
    [[nodiscard]] auto layoutSourceLine(BlockRange sourceLine, std::vector<LayoutLine> &lines) -> bool;
    /// Prepare source-line data for wrapping.
    [[nodiscard]] auto prepareSourceLine(BlockRange sourceLine) const -> LayoutPreparedSourceLine;

private:
    LayoutContext _context;
    LayoutNewlineMode _newlineMode;
    text::CharSet _wordSeparators;
    const LayoutSemantics *_semantics;
};

}
