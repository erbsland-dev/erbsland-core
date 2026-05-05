// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../support/TestHelper.hpp"

#include <algorithm>
#include <limits>
#include <ranges>
#include <stdexcept>
#include <string>
#include <vector>

class ReferenceBuffer final {
public:
    using Line = std::vector<Block>;

public:
    explicit ReferenceBuffer(const bgeo::BlockSize size, const Block fillChar = Block::space()) :
        _size{validateSize(size)}, _data(_size.area().toSizeT(), fillChar) {}

    [[nodiscard]] auto size() const noexcept -> bgeo::BlockSize { return _size; }

    [[nodiscard]] auto get(const bgeo::BlockPosition pos) const -> const Block & { return _data[_size.index(pos)]; }

    void set(const bgeo::BlockPosition pos, const Block &block) {
        if (!_size.contains(pos) || block.displayWidth() == 0 || block.displayWidth() > 2) {
            return;
        }
        if (block.displayWidth() == 1) {
            _data[_size.index(pos)] = block;
            return;
        }
        const auto secondPosition = pos + bgeo::BlockPosition{1, 0};
        if (!_size.contains(secondPosition)) {
            return;
        }
        _data[_size.index(secondPosition)] = Block::emptyBlock(block.style());
        _data[_size.index(pos)] = block;
    }

    void fill(const Block &fillChar) {
        for (auto &block : _data) {
            block = fillChar;
        }
    }

    void resize(const bgeo::BlockSize newSize, const BufferResizeMode mode, const Block fillChar = Block::space()) {
        const auto validatedSize = validateSize(newSize);
        if (_size == validatedSize) {
            return;
        }
        if (mode == BufferResizeMode::PreserveContent) {
            auto newData = std::vector<Block>(validatedSize.area().toSizeT(), fillChar);
            _size.limitedWith(validatedSize).forEach([&](const bgeo::BlockPosition pos) -> void {
                newData[validatedSize.index(pos)] = get(pos);
            });
            _size = validatedSize;
            _data = std::move(newData);
            return;
        }
        const auto oldArea = _data.size();
        _size = validatedSize;
        _data.resize(_size.area().toSizeT());
        if (!fillChar.isEmpty() && _data.size() > oldArea) {
            for (auto index = oldArea; index < _data.size(); ++index) {
                _data[index] = fillChar;
            }
        }
    }

    void shift(const bgeo::BlockDirection direction, const Block fillChar, const int count = 1) {
        validateDirectionalCount(direction, count);
        if (count == 0 || direction == bgeo::BlockDirection::None) {
            return;
        }
        if (direction.contains(bgeo::BlockDirection::North)) {
            eraseRows(bgeo::BlockCoordinate{0}, fillChar, count);
        }
        if (direction.contains(bgeo::BlockDirection::South)) {
            insertRows(bgeo::BlockCoordinate{0}, fillChar, count);
        }
        if (direction.contains(bgeo::BlockDirection::West)) {
            eraseColumns(bgeo::BlockCoordinate{0}, fillChar, count);
        }
        if (direction.contains(bgeo::BlockDirection::East)) {
            insertColumns(bgeo::BlockCoordinate{0}, fillChar, count);
        }
    }

    void rotate(const bgeo::BlockDirection direction, const int count = 1) {
        validateDirectionalCount(direction, count);
        if (count == 0 || direction == bgeo::BlockDirection::None) {
            return;
        }
        if (direction.contains(bgeo::BlockDirection::North)) {
            auto rows = toRows();
            rotateLines(rows, count, true);
            fromRows(rows);
        }
        if (direction.contains(bgeo::BlockDirection::South)) {
            auto rows = toRows();
            rotateLines(rows, count, false);
            fromRows(rows);
        }
        if (direction.contains(bgeo::BlockDirection::West)) {
            auto columns = toColumns();
            rotateLines(columns, count, true);
            fromColumns(columns);
        }
        if (direction.contains(bgeo::BlockDirection::East)) {
            auto columns = toColumns();
            rotateLines(columns, count, false);
            fromColumns(columns);
        }
    }

    void eraseRows(const bgeo::BlockCoordinate startRow, const Block fillChar, const int count = 1) {
        validateSpan(startRow, count, _size.height(), "startRow", "count");
        if (count == 0) {
            return;
        }
        const auto startOffset = static_cast<std::ptrdiff_t>(startRow.toRawValue());
        auto rows = toRows();
        rows.erase(rows.begin() + startOffset, rows.begin() + startOffset + count);
        rows.insert(rows.end(), static_cast<std::size_t>(count), blankRow(fillChar));
        fromRows(rows);
    }

    void eraseColumns(const bgeo::BlockCoordinate startColumn, const Block fillChar, const int count = 1) {
        validateSpan(startColumn, count, _size.width(), "startColumn", "count");
        if (count == 0) {
            return;
        }
        const auto startOffset = static_cast<std::ptrdiff_t>(startColumn.toRawValue());
        auto columns = toColumns();
        columns.erase(columns.begin() + startOffset, columns.begin() + startOffset + count);
        columns.insert(columns.end(), static_cast<std::size_t>(count), blankColumn(fillChar));
        fromColumns(columns);
    }

    void insertRows(const bgeo::BlockCoordinate startRow, const Block fillChar, const int count = 1) {
        validateSpan(startRow, count, _size.height(), "startRow", "count");
        if (count == 0) {
            return;
        }
        const auto startOffset = static_cast<std::ptrdiff_t>(startRow.toRawValue());
        auto rows = toRows();
        rows.insert(rows.begin() + startOffset, static_cast<std::size_t>(count), blankRow(fillChar));
        rows.erase(rows.end() - count, rows.end());
        fromRows(rows);
    }

    void insertColumns(const bgeo::BlockCoordinate startColumn, const Block fillChar, const int count = 1) {
        validateSpan(startColumn, count, _size.width(), "startColumn", "count");
        if (count == 0) {
            return;
        }
        const auto startOffset = static_cast<std::ptrdiff_t>(startColumn.toRawValue());
        auto columns = toColumns();
        columns.insert(columns.begin() + startOffset, static_cast<std::size_t>(count), blankColumn(fillChar));
        columns.erase(columns.end() - count, columns.end());
        fromColumns(columns);
    }

    void moveRows(
        const bgeo::BlockCoordinate startRow,
        const int count,
        const bgeo::BlockCoordinate delta,
        const Block fillChar) {
        validateSpan(startRow, count, _size.height(), "startRow", "count");
        if (count == 0 || delta == 0) {
            return;
        }
        auto rows = toRows();
        rows = moveLines(std::move(rows), startRow, count, delta, blankRow(fillChar));
        fromRows(rows);
    }

    void moveColumns(
        const bgeo::BlockCoordinate startColumn,
        const int count,
        const bgeo::BlockCoordinate delta,
        const Block fillChar) {
        validateSpan(startColumn, count, _size.width(), "startColumn", "count");
        if (count == 0 || delta == 0) {
            return;
        }
        auto columns = toColumns();
        columns = moveLines(std::move(columns), startColumn, count, delta, blankColumn(fillChar));
        fromColumns(columns);
    }

private:
    [[nodiscard]] static auto validateSize(const bgeo::BlockSize size) -> bgeo::BlockSize {
        if (size.width() < 1 || size.height() < 1) {
            throw std::invalid_argument("Buffer size must be at least 1x1");
        }
        if (!size.fitsInto(bgeo::BlockSize{10'000, 10'000})) {
            throw std::invalid_argument("Buffer size must not exceed 10'000x10'000");
        }
        return size;
    }

    static void validateSpan(
        const bgeo::BlockCoordinate start,
        const int count,
        const bgeo::BlockCoordinate limit,
        const char *startName,
        const char *countName) {
        if (count < 0 || count > limit) {
            throw std::invalid_argument(std::string{countName} + " is invalid");
        }
        if (count == 0) {
            if (start < 0 || start > limit) {
                throw std::invalid_argument(std::string{startName} + " is invalid");
            }
            return;
        }
        if (start < 0 || start >= limit || start + count > limit) {
            throw std::invalid_argument(std::string{startName} + " is invalid");
        }
    }

    void validateDirectionalCount(const bgeo::BlockDirection direction, const int count) const {
        if (count < 0) {
            throw std::invalid_argument("count is invalid");
        }
        if ((direction.contains(bgeo::BlockDirection::North) || direction.contains(bgeo::BlockDirection::South)) &&
            count > _size.height().toRawValue()) {
            throw std::invalid_argument("count is invalid");
        }
        if ((direction.contains(bgeo::BlockDirection::West) || direction.contains(bgeo::BlockDirection::East)) &&
            count > _size.width().toRawValue()) {
            throw std::invalid_argument("count is invalid");
        }
    }

    [[nodiscard]] auto blankRow(const Block &fillChar) const -> Line { return Line(_size.width().toSizeT(), fillChar); }

    [[nodiscard]] auto blankColumn(const Block &fillChar) const -> Line {
        return Line(_size.height().toSizeT(), fillChar);
    }

    [[nodiscard]] auto toRows() const -> std::vector<Line> {
        auto rows = std::vector<Line>{};
        rows.reserve(_size.height().toSizeT());
        for (auto y = bgeo::BlockCoordinate{0}; y < _size.height(); ++y) {
            auto row = Line{};
            row.reserve(_size.width().toSizeT());
            for (auto x = bgeo::BlockCoordinate{0}; x < _size.width(); ++x) {
                row.push_back(get(bgeo::BlockPosition{x, y}));
            }
            rows.push_back(std::move(row));
        }
        return rows;
    }

    void fromRows(const std::vector<Line> &rows) {
        for (auto y = bgeo::BlockCoordinate{0}; y < _size.height(); ++y) {
            for (auto x = bgeo::BlockCoordinate{0}; x < _size.width(); ++x) {
                _data[_size.index(bgeo::BlockPosition{x, y})] = rows[y.toSizeT()][x.toSizeT()];
            }
        }
    }

    [[nodiscard]] auto toColumns() const -> std::vector<Line> {
        auto columns = std::vector<Line>{};
        columns.reserve(_size.width().toSizeT());
        for (auto x = bgeo::BlockCoordinate{0}; x < _size.width(); ++x) {
            auto column = Line{};
            column.reserve(_size.height().toSizeT());
            for (auto y = bgeo::BlockCoordinate{0}; y < _size.height(); ++y) {
                column.push_back(get(bgeo::BlockPosition{x, y}));
            }
            columns.push_back(std::move(column));
        }
        return columns;
    }

    void fromColumns(const std::vector<Line> &columns) {
        for (auto x = bgeo::BlockCoordinate{0}; x < _size.width(); ++x) {
            for (auto y = bgeo::BlockCoordinate{0}; y < _size.height(); ++y) {
                _data[_size.index(bgeo::BlockPosition{x, y})] = columns[x.toSizeT()][y.toSizeT()];
            }
        }
    }

    static void rotateLines(std::vector<Line> &lines, const int count, const bool towardFront) {
        if (lines.empty() || count == 0) {
            return;
        }
        const auto normalizedCount = static_cast<std::size_t>(count) % lines.size();
        if (normalizedCount == 0) {
            return;
        }
        if (towardFront) {
            std::ranges::rotate(lines, lines.begin() + static_cast<std::ptrdiff_t>(normalizedCount));
            return;
        }
        std::ranges::rotate(lines, lines.end() - static_cast<std::ptrdiff_t>(normalizedCount));
    }

    [[nodiscard]] static auto moveLines(
        std::vector<Line> lines,
        bgeo::BlockCoordinate start,
        int count,
        bgeo::BlockCoordinate delta,
        const Line &blankLine) -> std::vector<Line> {
        const auto startOffset = static_cast<std::ptrdiff_t>(start.toRawValue());
        auto moved = std::vector<Line>{};
        moved.reserve(static_cast<std::size_t>(count));
        moved.insert(moved.end(), lines.begin() + startOffset, lines.begin() + startOffset + count);

        auto remaining = std::vector<Line>{};
        remaining.reserve(lines.size() - static_cast<std::size_t>(count));
        remaining.insert(remaining.end(), lines.begin(), lines.begin() + startOffset);
        remaining.insert(remaining.end(), lines.begin() + startOffset + count, lines.end());

        const auto targetStart = start.toRawValue() + delta.toRawValue();
        const auto droppedBefore = std::clamp(-targetStart, 0, count);
        const auto droppedAfter = std::clamp(targetStart + count - static_cast<int>(lines.size()), 0, count);
        const auto keptBegin = static_cast<std::size_t>(droppedBefore);
        const auto keptEnd = static_cast<std::size_t>(count - droppedAfter);

        auto kept = std::vector<Line>{};
        kept.reserve(keptEnd - keptBegin);
        kept.insert(kept.end(), moved.begin() + static_cast<std::ptrdiff_t>(keptBegin), moved.begin() + keptEnd);

        auto result = std::vector<Line>{};
        result.reserve(lines.size());
        if (droppedAfter > 0) {
            result.insert(result.end(), static_cast<std::size_t>(droppedAfter), blankLine);
        }
        const auto insertIndex =
            static_cast<std::size_t>(std::clamp(targetStart, 0, static_cast<int>(remaining.size())));
        result.insert(result.end(), remaining.begin(), remaining.begin() + static_cast<std::ptrdiff_t>(insertIndex));
        result.insert(result.end(), kept.begin(), kept.end());
        result.insert(result.end(), remaining.begin() + static_cast<std::ptrdiff_t>(insertIndex), remaining.end());
        if (droppedBefore > 0) {
            result.insert(result.end(), static_cast<std::size_t>(droppedBefore), blankLine);
        }
        return result;
    }

private:
    bgeo::BlockSize _size;
    std::vector<Block> _data;
};
