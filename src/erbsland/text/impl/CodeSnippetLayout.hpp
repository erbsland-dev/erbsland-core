// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CodeSnippetLayoutMarker.hpp"
#include "CodeSnippetLayoutRow.hpp"

#include "../StringView.hpp"

#include <vector>

namespace erbsland::text::impl {

/// Renderer-neutral layout for one code-snippet source line.
/// @tested{TextDocumentTest TerminalDocumentRendererTest}
class CodeSnippetLayout final {
public:
    /// Build physical source rows for one source line.
    /// @param source The original source line.
    /// @param markers The markers attached to the line.
    /// @param width The available source width in terminal cells.
    /// @return The source rows after cropping or character-level wrapping.
    [[nodiscard]] static auto build(StringView source, const std::vector<CodeSnippetLayoutMarker> &markers, int width)
        -> std::vector<CodeSnippetLayoutRow>;

    /// Test whether a marker has visible content in a source row.
    [[nodiscard]] static auto markerIntersects(
        const CodeSnippetLayoutRow &row, const CodeSnippetLayoutMarker &marker) noexcept -> bool;
    /// Get the display-cell start of a marker in a source row.
    [[nodiscard]] static auto markerStart(
        const CodeSnippetLayoutRow &row, const CodeSnippetLayoutMarker &marker) noexcept -> int;
    /// Get the display-cell length of a range marker in a source row.
    [[nodiscard]] static auto markerLength(
        const CodeSnippetLayoutRow &row, const CodeSnippetLayoutMarker &marker) noexcept -> int;
    /// Get the display width of a source row without crop ellipses.
    [[nodiscard]] static auto rowWidth(const CodeSnippetLayoutRow &row) noexcept -> int;

private:
    [[nodiscard]] static auto sourceCells(StringView source) -> std::vector<CodeSnippetLayoutCell>;
    static void applyMarkers(
        std::vector<CodeSnippetLayoutCell> &cells, const std::vector<CodeSnippetLayoutMarker> &markers);
    [[nodiscard]] static auto wrap(const std::vector<CodeSnippetLayoutCell> &cells, int width)
        -> std::vector<CodeSnippetLayoutRow>;
    static void cropRow(CodeSnippetLayoutRow &row, int width, bool leading, bool trailing);
};

}
