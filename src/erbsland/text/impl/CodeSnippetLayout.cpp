// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CodeSnippetLayout.hpp"

#include "../Literals.hpp"
#include "../StringCharReader.hpp"

#include "../../unit/ColumnCount.hpp"
#include "../../unit/ColumnIndex.hpp"
#include "../../unit/CpIndex.hpp"
#include "../../unit/CpLength.hpp"
#include "../../unit/CpRange.hpp"

#include <algorithm>
#include <utility>

namespace erbsland::text::impl {

using namespace literals;

CodeSnippetLayout::CodeSnippetLayout(
    const String &source, const std::vector<CodeSnippetLayoutMarker> &markers, const int requestedWidth) {
    const auto width = std::max(requestedWidth, 1);
    auto cells = sourceCells(source);
    applyMarkers(cells, markers);
    if (markers.empty()) {
        auto row = CodeSnippetLayoutRow{std::move(cells)};
        if (row.displayWidth() > width) {
            row.cropTrailing(width);
        }
        _rows.push_back(std::move(row));
        return;
    }

    auto rows = wrap(cells, width);
    if (rows.size() <= cMaximumMarkedRows) {
        _rows = std::move(rows);
        return;
    }

    auto firstMarkerRow = std::size_t{0};
    auto foundMarker = false;
    for (auto rowIndex = std::size_t{0}; rowIndex < rows.size() && !foundMarker; ++rowIndex) {
        for (const auto &marker : markers) {
            if (rows[rowIndex].markerPlacement(marker.range).has_value()) {
                firstMarkerRow = rowIndex;
                foundMarker = true;
                break;
            }
        }
    }
    const auto firstRow = firstMarkerRow > 0 ? firstMarkerRow - 1 : 0;
    const auto lastRow = std::min(firstRow + cMaximumMarkedRows, rows.size());
    _rows = std::vector<CodeSnippetLayoutRow>{
        rows.begin() + static_cast<std::vector<CodeSnippetLayoutRow>::difference_type>(firstRow),
        rows.begin() + static_cast<std::vector<CodeSnippetLayoutRow>::difference_type>(lastRow)};
    if (firstRow > 0) {
        _rows.front().cropLeading(width);
    }
    if (lastRow < rows.size()) {
        _rows.back().cropTrailing(width);
    }
}

auto CodeSnippetLayout::isLastMarkerRow(const std::size_t rowIndex, const unit::ColumnRange markerRange) const noexcept
    -> bool {
    for (auto following = rowIndex + 1; following < _rows.size(); ++following) {
        if (_rows[following].markerPlacement(markerRange).has_value()) {
            return false;
        }
    }
    return true;
}

auto CodeSnippetLayout::sourceCells(const String &source) -> std::vector<CodeSnippetLayoutCell> {
    auto result = std::vector<CodeSnippetLayoutCell>{};
    auto reader = StringCharReader{source};
    auto logicalColumn = unit::ColumnIndex::zero();
    while (!reader.isAtEnd()) {
        const auto character = reader.read();
        const auto nextColumn = logicalColumn.advanced(unit::ColumnCount::one());
        if (!character.isControlOrFormat() && character.displayWidth() == 0 && !result.empty()) {
            result.back().text = String::fromJoined({result.back().text, String::fromCharacter(character)});
            result.back().range = unit::ColumnRange{result.back().range.index(), nextColumn};
        } else if (character.isControlOrFormat() || character.displayWidth() <= 0) {
            result.push_back(CodeSnippetLayoutCell{"?"_el, {logicalColumn, nextColumn}, 1, {}, false});
        } else {
            result.push_back(
                CodeSnippetLayoutCell{
                    source.slice(
                        unit::CpRange{unit::CpIndex::fromSizeT(logicalColumn.toSizeT()), unit::CpLength::one()}),
                    {logicalColumn, nextColumn},
                    character.displayWidth(),
                    {},
                    false});
        }
        logicalColumn = nextColumn;
    }
    return result;
}

void CodeSnippetLayout::applyMarkers(
    std::vector<CodeSnippetLayoutCell> &cells, const std::vector<CodeSnippetLayoutMarker> &markers) {
    for (auto markerIndex = std::size_t{0}; markerIndex < markers.size(); ++markerIndex) {
        for (auto &cell : cells) {
            if (cell.overlaps(markers[markerIndex].range)) {
                cell.marker = markerIndex;
            }
        }
    }
}

auto CodeSnippetLayout::wrap(const std::vector<CodeSnippetLayoutCell> &cells, const int width)
    -> std::vector<CodeSnippetLayoutRow> {
    auto result = std::vector<CodeSnippetLayoutRow>{};
    auto rowCells = std::vector<CodeSnippetLayoutCell>{};
    auto rowWidth = 0;
    for (const auto &cell : cells) {
        if (!rowCells.empty() && rowWidth + cell.width > width) {
            result.emplace_back(std::move(rowCells));
            rowCells = {};
            rowWidth = 0;
        }
        rowCells.push_back(cell);
        rowWidth += cell.width;
    }
    if (!rowCells.empty() || cells.empty()) {
        result.emplace_back(std::move(rowCells));
    }
    return result;
}

}
