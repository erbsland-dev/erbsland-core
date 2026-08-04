// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CodeSnippetLayoutRow.hpp"

#include <algorithm>
#include <ranges>

namespace erbsland::text::impl {

auto CodeSnippetLayoutRow::displayWidth() const noexcept -> int {
    auto result = 0;
    for (const auto &cell : _cells) {
        result += cell.width;
    }
    return result;
}

auto CodeSnippetLayoutRow::markerPlacement(const unit::ColumnRange range) const noexcept
    -> std::optional<MarkerPlacement> {
    if (range.isEmpty()) {
        if (_cells.empty()) {
            return range.index().isZero() ? std::optional{MarkerPlacement{0, 1}} : std::nullopt;
        }
        const auto first = std::ranges::find_if(_cells, [](const auto &cell) -> bool { return !cell.isEllipsis; });
        const auto last = std::ranges::find_if(
            _cells.rbegin(), _cells.rend(), [](const auto &cell) -> bool { return !cell.isEllipsis; });
        if (first == _cells.end() || last == _cells.rend() || range.index() < first->range.index() ||
            range.index() > last->range.endIndex()) {
            return std::nullopt;
        }
        auto start = 0;
        for (const auto &cell : _cells) {
            if (cell.containsOrFollows(range.index())) {
                break;
            }
            start += cell.width;
        }
        return MarkerPlacement{start, 1};
    }

    auto position = 0;
    auto result = MarkerPlacement{};
    for (const auto &cell : _cells) {
        if (cell.overlaps(range)) {
            if (result.length == 0) {
                result.start = position;
            }
            result.length += cell.width;
        }
        position += cell.width;
    }
    return result.length > 0 ? std::optional{result} : std::nullopt;
}

void CodeSnippetLayoutRow::cropLeading(const int width) {
    crop(width, true);
}

void CodeSnippetLayoutRow::cropTrailing(const int width) {
    crop(width, false);
}

void CodeSnippetLayoutRow::crop(const int width, const bool leading) {
    const auto contentWidth = std::max(width - 1, 0);
    while (!_cells.empty() && displayWidth() > contentWidth) {
        if (leading) {
            _cells.erase(_cells.begin());
        } else {
            _cells.pop_back();
        }
    }
    if (leading) {
        _cells.insert(_cells.begin(), CodeSnippetLayoutCell::ellipsis());
    } else {
        _cells.push_back(CodeSnippetLayoutCell::ellipsis());
    }
}

}
