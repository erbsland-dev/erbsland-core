// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CodeSnippetLayoutMarker.hpp"
#include "CodeSnippetLayoutRow.hpp"

#include "../String.hpp"

#include "../../unit/ColumnRange.hpp"

#include <cstddef>
#include <vector>

namespace erbsland::text::impl {

/// Renderer-neutral layout for one code-snippet source line.
/// @tested{CodeSnippetLayoutTest}
class CodeSnippetLayout final {
public:
    /// Build physical source rows for one source line.
    /// @param source The original source line.
    /// @param markers The markers attached to the line.
    /// @param width The available source width in terminal cells.
    CodeSnippetLayout(const String &source, const std::vector<CodeSnippetLayoutMarker> &markers, int width);

public:
    /// Access the physical source rows.
    [[nodiscard]] auto rows() const noexcept -> const std::vector<CodeSnippetLayoutRow> & { return _rows; }
    /// Test whether a row is the final visible row intersecting a marker.
    /// @param rowIndex The row containing the marker.
    /// @param markerRange The logical marker range.
    /// @return `true` if no following row contains the marker.
    [[nodiscard]] auto isLastMarkerRow(std::size_t rowIndex, unit::ColumnRange markerRange) const noexcept -> bool;

private:
    static constexpr auto cMaximumMarkedRows = std::size_t{5}; ///< Maximum rows retained around a marker.

    /// Split source text into displayable layout cells.
    [[nodiscard]] static auto sourceCells(const String &source) -> std::vector<CodeSnippetLayoutCell>;
    /// Apply logical markers to layout cells.
    static void applyMarkers(
        std::vector<CodeSnippetLayoutCell> &cells, const std::vector<CodeSnippetLayoutMarker> &markers);
    /// Wrap layout cells into display rows of the requested width.
    [[nodiscard]] static auto wrap(const std::vector<CodeSnippetLayoutCell> &cells, int width)
        -> std::vector<CodeSnippetLayoutRow>;

private:
    std::vector<CodeSnippetLayoutRow> _rows; ///< The completed physical rows.
};

}
