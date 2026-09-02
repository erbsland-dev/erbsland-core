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

/// Provides a simple reference implementation for terminal buffer tests.
/// @notest{Used to validate the behavior of the tested buffer implementation.}
class ReferenceBuffer final {
public:
    /// Defines one buffer row or column.
    using Line = std::vector<Block>;

public:
    /// Create a reference buffer with a size and fill block.
    explicit ReferenceBuffer(const block::Size size, const Block fillChar = Block::space()) :
        _size{validateSize(size)}, _data(_size.area().toSizeT(), fillChar) {}

    /// Get the current buffer size.
    [[nodiscard]] auto size() const noexcept -> block::Size { return _size; }

    /// Get the block at a buffer position.
    [[nodiscard]] auto get(const block::Position pos) const -> const Block & { return _data[_size.index(pos)]; }

    /// Set a block at a buffer position.
    void set(const block::Position pos, const Block &block) {
        if (!_size.contains(pos) || block.displayWidth() == 0 || block.displayWidth() > 2) {
            return;
        }
        if (block.displayWidth() == 1) {
            _data[_size.index(pos)] = block;
            return;
        }
        const auto secondPosition = pos + block::Position{1, 0};
        if (!_size.contains(secondPosition)) {
            return;
        }
        _data[_size.index(secondPosition)] = Block::emptyBlock(block.style());
        _data[_size.index(pos)] = block;
    }

    /// Fill the complete buffer with one block.
    void fill(const Block &fillChar) {
        for (auto &block : _data) {
            block = fillChar;
        }
    }

    /// Resize the buffer with an optional content-preservation mode.
    void resize(const block::Size newSize, const BufferResizeMode mode, const Block fillChar = Block::space()) {
        const auto validatedSize = validateSize(newSize);
        if (_size == validatedSize) {
            return;
        }
        if (mode == BufferResizeMode::PreserveContent) {
            auto newData = std::vector<Block>(validatedSize.area().toSizeT(), fillChar);
            _size.limitedWith(validatedSize).forEach([&](const block::Position pos) -> void {
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

    /// Shift rows or columns toward a direction and fill the exposed area.
    void shift(const block::Direction direction, const Block fillChar, const int count = 1) {
        validateDirectionalCount(direction, count);
        if (count == 0 || direction == block::Direction::None) {
            return;
        }
        if (direction.contains(block::Direction::North)) {
            eraseRows(block::Coordinate{0}, fillChar, count);
        }
        if (direction.contains(block::Direction::South)) {
            insertRows(block::Coordinate{0}, fillChar, count);
        }
        if (direction.contains(block::Direction::West)) {
            eraseColumns(block::Coordinate{0}, fillChar, count);
        }
        if (direction.contains(block::Direction::East)) {
            insertColumns(block::Coordinate{0}, fillChar, count);
        }
    }

    /// Rotate rows or columns toward a direction.
    void rotate(const block::Direction direction, const int count = 1) {
        validateDirectionalCount(direction, count);
        if (count == 0 || direction == block::Direction::None) {
            return;
        }
        if (direction.contains(block::Direction::North)) {
            auto rows = toRows();
            rotateLines(rows, count, true);
            fromRows(rows);
        }
        if (direction.contains(block::Direction::South)) {
            auto rows = toRows();
            rotateLines(rows, count, false);
            fromRows(rows);
        }
        if (direction.contains(block::Direction::West)) {
            auto columns = toColumns();
            rotateLines(columns, count, true);
            fromColumns(columns);
        }
        if (direction.contains(block::Direction::East)) {
            auto columns = toColumns();
            rotateLines(columns, count, false);
            fromColumns(columns);
        }
    }

    /// Erase rows and append filled rows at the opposite edge.
    void eraseRows(const block::Coordinate startRow, const Block fillChar, const int count = 1) {
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

    /// Erase columns and append filled columns at the opposite edge.
    void eraseColumns(const block::Coordinate startColumn, const Block fillChar, const int count = 1) {
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

    /// Insert filled rows and remove rows from the opposite edge.
    void insertRows(const block::Coordinate startRow, const Block fillChar, const int count = 1) {
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

    /// Insert filled columns and remove columns from the opposite edge.
    void insertColumns(const block::Coordinate startColumn, const Block fillChar, const int count = 1) {
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

    /// Move a contiguous range of rows and fill any newly exposed space.
    void moveRows(
        const block::Coordinate startRow, const int count, const block::Coordinate delta, const Block fillChar) {
        validateSpan(startRow, count, _size.height(), "startRow", "count");
        if (count == 0 || delta == 0) {
            return;
        }
        auto rows = toRows();
        rows = moveLines(std::move(rows), startRow, count, delta, blankRow(fillChar));
        fromRows(rows);
    }

    /// Move a contiguous range of columns and fill any newly exposed space.
    void moveColumns(
        const block::Coordinate startColumn, const int count, const block::Coordinate delta, const Block fillChar) {
        validateSpan(startColumn, count, _size.width(), "startColumn", "count");
        if (count == 0 || delta == 0) {
            return;
        }
        auto columns = toColumns();
        columns = moveLines(std::move(columns), startColumn, count, delta, blankColumn(fillChar));
        fromColumns(columns);
    }

private:
    /// Validate a buffer size supported by the reference implementation.
    [[nodiscard]] static auto validateSize(const block::Size size) -> block::Size {
        if (size.width() < 1 || size.height() < 1) {
            throw std::invalid_argument("Buffer size must be at least 1x1");
        }
        if (!size.fitsInto(block::Size{10'000, 10'000})) {
            throw std::invalid_argument("Buffer size must not exceed 10'000x10'000");
        }
        return size;
    }

    /// Validate a coordinate span against an axis limit.
    static void validateSpan(
        const block::Coordinate start,
        const int count,
        const block::Coordinate limit,
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

    /// Validate a directional operation count for the current buffer size.
    void validateDirectionalCount(const block::Direction direction, const int count) const {
        if (count < 0) {
            throw std::invalid_argument("count is invalid");
        }
        if ((direction.contains(block::Direction::North) || direction.contains(block::Direction::South)) &&
            count > _size.height().toRawValue()) {
            throw std::invalid_argument("count is invalid");
        }
        if ((direction.contains(block::Direction::West) || direction.contains(block::Direction::East)) &&
            count > _size.width().toRawValue()) {
            throw std::invalid_argument("count is invalid");
        }
    }

    /// Create a filled row for the current width.
    [[nodiscard]] auto blankRow(const Block &fillChar) const -> Line { return Line(_size.width().toSizeT(), fillChar); }

    /// Create a filled column for the current height.
    [[nodiscard]] auto blankColumn(const Block &fillChar) const -> Line {
        return Line(_size.height().toSizeT(), fillChar);
    }

    /// Convert the buffer storage to rows.
    [[nodiscard]] auto toRows() const -> std::vector<Line> {
        auto rows = std::vector<Line>{};
        rows.reserve(_size.height().toSizeT());
        for (auto y = block::Coordinate{0}; y < _size.height(); ++y) {
            auto row = Line{};
            row.reserve(_size.width().toSizeT());
            for (auto x = block::Coordinate{0}; x < _size.width(); ++x) {
                row.push_back(get(block::Position{x, y}));
            }
            rows.push_back(std::move(row));
        }
        return rows;
    }

    /// Replace buffer storage from rows.
    void fromRows(const std::vector<Line> &rows) {
        for (auto y = block::Coordinate{0}; y < _size.height(); ++y) {
            for (auto x = block::Coordinate{0}; x < _size.width(); ++x) {
                _data[_size.index(block::Position{x, y})] = rows[y.toSizeT()][x.toSizeT()];
            }
        }
    }

    /// Convert the buffer storage to columns.
    [[nodiscard]] auto toColumns() const -> std::vector<Line> {
        auto columns = std::vector<Line>{};
        columns.reserve(_size.width().toSizeT());
        for (auto x = block::Coordinate{0}; x < _size.width(); ++x) {
            auto column = Line{};
            column.reserve(_size.height().toSizeT());
            for (auto y = block::Coordinate{0}; y < _size.height(); ++y) {
                column.push_back(get(block::Position{x, y}));
            }
            columns.push_back(std::move(column));
        }
        return columns;
    }

    /// Replace buffer storage from columns.
    void fromColumns(const std::vector<Line> &columns) {
        for (auto x = block::Coordinate{0}; x < _size.width(); ++x) {
            for (auto y = block::Coordinate{0}; y < _size.height(); ++y) {
                _data[_size.index(block::Position{x, y})] = columns[x.toSizeT()][y.toSizeT()];
            }
        }
    }

    /// Rotate a collection of rows or columns.
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

    /// Move a range of rows or columns within a collection.
    [[nodiscard]] static auto moveLines(
        std::vector<Line> lines, block::Coordinate start, int count, block::Coordinate delta, const Line &blankLine)
        -> std::vector<Line> {
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
    block::Size _size;        ///< The current buffer dimensions.
    std::vector<Block> _data; ///< The row-major buffer blocks.
};
