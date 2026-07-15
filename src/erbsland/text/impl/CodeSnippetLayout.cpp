// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CodeSnippetLayout.hpp"

#include "../Char.hpp"
#include "../Literals.hpp"
#include "../StringBuilder.hpp"
#include "../StringCharReader.hpp"

#include "../../unit/ColumnCount.hpp"
#include "../../unit/ColumnIndex.hpp"

#include <algorithm>
#include <ranges>

namespace erbsland::text::impl {

using namespace literals;

namespace {

constexpr auto cMaximumMarkedRows = std::size_t{5};

}

auto CodeSnippetLayoutCell::ellipsis() -> CodeSnippetLayoutCell {
    return {String::fromCharacter(Char{U'…'}), {}, 1, {}, true};
}

auto CodeSnippetLayoutCell::overlaps(const unit::ColumnRange other) const noexcept -> bool {
    return !isEllipsis && !other.isEmpty() && range.index() < other.endIndex() && other.index() < range.endIndex();
}

auto CodeSnippetLayoutCell::containsOrFollows(const unit::ColumnIndex point) const noexcept -> bool {
    return !isEllipsis && (point <= range.index() || range.contains(point));
}

auto CodeSnippetLayout::build(
    const StringView source, const std::vector<CodeSnippetLayoutMarker> &markers, const int requestedWidth)
    -> std::vector<CodeSnippetLayoutRow> {
    const auto width = std::max(requestedWidth, 1);
    auto cells = sourceCells(source);
    applyMarkers(cells, markers);
    if (markers.empty()) {
        auto row = CodeSnippetLayoutRow{std::move(cells)};
        cropRow(row, width, false, rowWidth(row) > width);
        return {std::move(row)};
    }

    auto rows = wrap(cells, width);
    if (rows.size() <= cMaximumMarkedRows) {
        return rows;
    }

    auto firstMarkerRow = std::size_t{0};
    auto foundMarker = false;
    for (auto rowIndex = std::size_t{0}; rowIndex < rows.size() && !foundMarker; ++rowIndex) {
        for (const auto &marker : markers) {
            if (markerIntersects(rows[rowIndex], marker)) {
                firstMarkerRow = rowIndex;
                foundMarker = true;
                break;
            }
        }
    }
    const auto firstRow = firstMarkerRow > 0 ? firstMarkerRow - 1 : 0;
    const auto lastRow = std::min(firstRow + cMaximumMarkedRows, rows.size());
    auto result = std::vector<CodeSnippetLayoutRow>{
        rows.begin() + static_cast<std::vector<CodeSnippetLayoutRow>::difference_type>(firstRow),
        rows.begin() + static_cast<std::vector<CodeSnippetLayoutRow>::difference_type>(lastRow)};
    if (firstRow > 0) {
        cropRow(result.front(), width, true, false);
    }
    if (lastRow < rows.size()) {
        cropRow(result.back(), width, false, true);
    }
    return result;
}

auto CodeSnippetLayout::markerIntersects(
    const CodeSnippetLayoutRow &row, const CodeSnippetLayoutMarker &marker) noexcept -> bool {
    if (marker.range.isEmpty()) {
        if (row.cells.empty()) {
            return marker.range.index().isZero();
        }
        const auto first = std::ranges::find_if(row.cells, [](const auto &cell) { return !cell.isEllipsis; });
        const auto last = std::ranges::find_if(
            row.cells.rbegin(), row.cells.rend(), [](const auto &cell) { return !cell.isEllipsis; });
        if (first == row.cells.end() || last == row.cells.rend()) {
            return false;
        }
        const auto point = marker.range.index();
        return point >= first->range.index() && point <= last->range.endIndex();
    }
    return std::ranges::any_of(row.cells, [&marker](const auto &cell) { return cell.overlaps(marker.range); });
}

auto CodeSnippetLayout::markerStart(const CodeSnippetLayoutRow &row, const CodeSnippetLayoutMarker &marker) noexcept
    -> int {
    auto result = 0;
    for (const auto &cell : row.cells) {
        if ((marker.range.isEmpty() && cell.containsOrFollows(marker.range.index())) || cell.overlaps(marker.range)) {
            return result;
        }
        result += cell.width;
    }
    return result;
}

auto CodeSnippetLayout::markerLength(const CodeSnippetLayoutRow &row, const CodeSnippetLayoutMarker &marker) noexcept
    -> int {
    if (marker.range.isEmpty()) {
        return 1;
    }
    auto result = 0;
    for (const auto &cell : row.cells) {
        if (cell.overlaps(marker.range)) {
            result += cell.width;
        }
    }
    return result;
}

auto CodeSnippetLayout::rowWidth(const CodeSnippetLayoutRow &row) noexcept -> int {
    auto result = 0;
    for (const auto &cell : row.cells) {
        result += cell.width;
    }
    return result;
}

auto CodeSnippetLayout::sourceCells(const StringView source) -> std::vector<CodeSnippetLayoutCell> {
    auto result = std::vector<CodeSnippetLayoutCell>{};
    auto reader = StringCharReader{source};
    auto logicalColumn = unit::ColumnIndex::zero();
    while (!reader.isAtEnd()) {
        const auto character = reader.read();
        const auto nextColumn = logicalColumn.advanced(unit::ColumnCount::one());
        if (!character.isControlOrFormat() && character.displayWidth() == 0 && !result.empty()) {
            auto builder = StringBuilder::basedOn(result.back().text);
            builder.append(character);
            result.back().text = builder.toString();
            result.back().range = unit::ColumnRange{result.back().range.index(), nextColumn};
        } else if (character.isControlOrFormat() || character.displayWidth() <= 0) {
            result.push_back(CodeSnippetLayoutCell{String{"?"_el}, {logicalColumn, nextColumn}, 1, {}, false});
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
    auto row = CodeSnippetLayoutRow{};
    auto rowWidth = 0;
    for (const auto &cell : cells) {
        if (!row.cells.empty() && rowWidth + cell.width > width) {
            result.push_back(std::move(row));
            row = {};
            rowWidth = 0;
        }
        row.cells.push_back(cell);
        rowWidth += cell.width;
    }
    if (!row.cells.empty() || cells.empty()) {
        result.push_back(std::move(row));
    }
    return result;
}

void CodeSnippetLayout::cropRow(CodeSnippetLayoutRow &row, const int width, const bool leading, const bool trailing) {
    const auto ellipsisCount = static_cast<int>(leading) + static_cast<int>(trailing);
    const auto contentWidth = std::max(width - ellipsisCount, 0);
    while (!row.cells.empty() && rowWidth(row) > contentWidth) {
        if (leading) {
            row.cells.erase(row.cells.begin());
        } else {
            row.cells.pop_back();
        }
    }
    if (leading) {
        row.cells.insert(row.cells.begin(), CodeSnippetLayoutCell::ellipsis());
        row.hasLeadingEllipsis = true;
    }
    if (trailing) {
        row.cells.push_back(CodeSnippetLayoutCell::ellipsis());
        row.hasTrailingEllipsis = true;
    }
}

}
