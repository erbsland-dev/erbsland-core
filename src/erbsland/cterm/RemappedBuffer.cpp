// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RemappedBuffer.hpp"

#include "../err/ParameterError.hpp"

#include <algorithm>
#include <cassert>
#include <ranges>

namespace erbsland::cterm {

RemappedBuffer::RemappedBuffer() : RemappedBuffer{bgeo::BlockSize{1, 1}, bgeo::Orientation::Vertical, Block{U' '}} {
}

RemappedBuffer::RemappedBuffer(const bgeo::BlockSize size, bgeo::Orientation orientation, const Block fillChar) :
    _size{validatedBufferSize(size)},
    _orientation{orientation},
    _buffer(_size.area().toSizeT(), fillChar),
    _rowRemap(linearIndex(_size.height().toSizeT())),
    _columnRemap(linearIndex(_size.width().toSizeT())) {
}

auto RemappedBuffer::size() const noexcept -> bgeo::BlockSize {
    return _size;
}

auto RemappedBuffer::rect() const noexcept -> bgeo::BlockRectangle {
    return bgeo::BlockRectangle{bgeo::BlockPosition{0, 0}, _size};
}

auto RemappedBuffer::get(const bgeo::BlockPosition pos) const noexcept -> const Block & {
    assert(_size.contains(pos));
    if (!_size.contains(pos)) {
        return Block::space();
    }
    return _buffer[bufferIndex(pos.x(), pos.y())];
}

auto RemappedBuffer::clone() const -> WritableBufferPtr {
    return std::make_shared<RemappedBuffer>(*this);
}

void RemappedBuffer::resize(const bgeo::BlockSize newSize) {
    resize(newSize, BufferResizeMode::Fast, Block{});
}

void RemappedBuffer::resize(const bgeo::BlockSize newSize, const BufferResizeMode mode, const Block fillChar) {
    const auto validatedSize = validatedBufferSize(newSize);
    if (_size == validatedSize) {
        return;
    }
    if (mode == BufferResizeMode::PreserveContent) {
        if (isPrimaryAxisOnlyResize(validatedSize)) {
            primaryAxisResize(validatedSize, fillChar);
        } else {
            reorderedResize(validatedSize, fillChar);
        }
        return;
    }
    fastResize(validatedSize, fillChar);
}

void RemappedBuffer::set(const bgeo::BlockPosition pos, const Block &block) noexcept {
    const auto displayWidth = block.displayWidth();
    if (!_size.contains(pos) || displayWidth == 0 || displayWidth > 2) {
        return;
    }
    if (displayWidth == 1) {
        _buffer[bufferIndex(pos.x(), pos.y())] = block;
        return;
    }
    const auto secondPosition = pos + bgeo::BlockPosition{1, 0};
    if (!_size.contains(secondPosition)) {
        return;
    }
    // The continuation cell for a wide character must stay logically empty while preserving the style.
    _buffer[bufferIndex(secondPosition.x(), secondPosition.y())] = Block::emptyBlock(block.style());
    _buffer[bufferIndex(pos.x(), pos.y())] = block;
}

void RemappedBuffer::reserve(bgeo::BlockSize size) noexcept {
    _buffer.reserve(size.area().toSizeT());
    _rowRemap.reserve(size.height().toSizeT());
    _columnRemap.reserve(size.width().toSizeT());
}

void RemappedBuffer::shift(const bgeo::BlockDirection direction, const Block fillChar, const int count) {
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

void RemappedBuffer::rotate(const bgeo::BlockDirection direction, const int count) {
    validateDirectionalCount(direction, count);
    if (count == 0 || direction == bgeo::BlockDirection::None) {
        return;
    }
    if (direction.contains(bgeo::BlockDirection::North)) {
        rotateMap(_rowRemap, count, true);
    }
    if (direction.contains(bgeo::BlockDirection::South)) {
        rotateMap(_rowRemap, count, false);
    }
    if (direction.contains(bgeo::BlockDirection::West)) {
        rotateMap(_columnRemap, count, true);
    }
    if (direction.contains(bgeo::BlockDirection::East)) {
        rotateMap(_columnRemap, count, false);
    }
}

void RemappedBuffer::eraseRows(const bgeo::BlockCoordinate startRow, const Block fillChar, const int count) {
    validateExistingSpan(startRow, count, _size.height().toRawValue(), "startRow", "count");
    if (count == 0) {
        return;
    }
    fillStoredRows(eraseFromMap(_rowRemap, startRow, count), fillChar);
}

void RemappedBuffer::eraseColumns(const bgeo::BlockCoordinate startColumn, const Block fillChar, const int count) {
    validateExistingSpan(startColumn, count, _size.width().toRawValue(), "startColumn", "count");
    if (count == 0) {
        return;
    }
    fillStoredColumns(eraseFromMap(_columnRemap, startColumn, count), fillChar);
}

void RemappedBuffer::insertRows(const bgeo::BlockCoordinate startRow, const Block fillChar, const int count) {
    validateInsertArguments(startRow, count, _size.height().toRawValue(), "startRow", "count");
    if (count == 0) {
        return;
    }
    fillStoredRows(insertIntoMap(_rowRemap, startRow, count), fillChar);
}

void RemappedBuffer::insertColumns(const bgeo::BlockCoordinate startColumn, const Block fillChar, const int count) {
    validateInsertArguments(startColumn, count, _size.width().toRawValue(), "startColumn", "count");
    if (count == 0) {
        return;
    }
    fillStoredColumns(insertIntoMap(_columnRemap, startColumn, count), fillChar);
}

void RemappedBuffer::moveRows(
    const bgeo::BlockCoordinate startRow, const int count, const bgeo::BlockCoordinate delta, const Block fillChar) {
    validateExistingSpan(startRow, count, _size.height().toRawValue(), "startRow", "count");
    if (count == 0 || delta == 0) {
        return;
    }
    fillStoredRows(moveInMap(_rowRemap, startRow, count, delta), fillChar);
}

void RemappedBuffer::moveColumns(
    const bgeo::BlockCoordinate startColumn, const int count, const bgeo::BlockCoordinate delta, const Block fillChar) {
    validateExistingSpan(startColumn, count, _size.width().toRawValue(), "startColumn", "count");
    if (count == 0 || delta == 0) {
        return;
    }
    fillStoredColumns(moveInMap(_columnRemap, startColumn, count, delta), fillChar);
}

void RemappedBuffer::fill(const Block &fillBlock) noexcept {
    for (auto &character : _buffer) {
        character = fillBlock;
    }
    for (std::size_t i = 0; i < _rowRemap.size(); ++i) {
        _rowRemap[i] = bgeo::BlockCoordinate{i};
    }
    for (std::size_t i = 0; i < _columnRemap.size(); ++i) {
        _columnRemap[i] = bgeo::BlockCoordinate{i};
    }
}

auto RemappedBuffer::validatedBufferSize(const bgeo::BlockSize size) -> bgeo::BlockSize {
    if (size.width() < 1 || size.height() < 1) {
        throw err::ParameterError{"Buffer size must be at least 1x1.", "size"};
    }
    if (!size.fitsInto(cMaximumSize)) {
        throw err::ParameterError{"Buffer size must not exceed 10'000x10'000.", "size"};
    }
    return size;
}

auto RemappedBuffer::linearIndex(std::size_t size) -> CoordinateMap {
    CoordinateMap result;
    result.resize(size);
    for (std::size_t index = 0; index < size; ++index) {
        result[index] = bgeo::BlockCoordinate{index};
    }
    return result;
}

void RemappedBuffer::validateCount(const int count, const int maximum, const std::string_view parameterName) {
    if (count < 0) {
        throw err::ParameterError{"The count must not be negative.", parameterName};
    }
    if (count > maximum) {
        throw err::ParameterError{"The count must not exceed the buffer dimension.", parameterName};
    }
}

void RemappedBuffer::validateExistingSpan(
    const bgeo::BlockCoordinate start,
    const int count,
    const int limit,
    const std::string_view startName,
    const std::string_view countName) {
    validateCount(count, limit, countName);
    if (count == 0) {
        if (start < 0 || start > limit) {
            throw err::ParameterError{"The start coordinate is out of bounds.", startName};
        }
        return;
    }
    if (start < 0 || start >= limit) {
        throw err::ParameterError{"The start coordinate is out of bounds.", startName};
    }
    if (start + count > limit) {
        throw err::ParameterError{"The count exceeds the remaining buffer dimension.", countName};
    }
}

void RemappedBuffer::validateInsertArguments(
    const bgeo::BlockCoordinate start,
    const int count,
    const int limit,
    const std::string_view startName,
    const std::string_view countName) {
    validateCount(count, limit, countName);
    if (count == 0) {
        if (start < 0 || start > limit) {
            throw err::ParameterError{"The start coordinate is out of bounds.", startName};
        }
        return;
    }
    if (start < 0 || start >= limit) {
        throw err::ParameterError{"The start coordinate is out of bounds.", startName};
    }
    if (start + count > limit) {
        throw err::ParameterError{"The count exceeds the remaining buffer dimension.", countName};
    }
}

void RemappedBuffer::validateDirectionalCount(const bgeo::BlockDirection direction, const int count) const {
    if (count < 0) {
        throw err::ParameterError{"The count must not be negative.", "count"};
    }
    if (direction.contains(bgeo::BlockDirection::North) || direction.contains(bgeo::BlockDirection::South)) {
        validateCount(count, _size.height().toRawValue(), "count");
    }
    if (direction.contains(bgeo::BlockDirection::West) || direction.contains(bgeo::BlockDirection::East)) {
        validateCount(count, _size.width().toRawValue(), "count");
    }
}

auto RemappedBuffer::bufferIndex(
    const bgeo::BlockPosition pos, const bgeo::BlockSize size, const bgeo::Orientation orientation) noexcept
    -> std::size_t {
    const auto crossAxis = orientation.crossed();
    return (pos.coordinate(orientation) * size.coordinate(crossAxis) + pos.coordinate(crossAxis)).toSizeT();
}

void RemappedBuffer::rotateMap(CoordinateMap &map, const int count, const bool towardFront) noexcept {
    if (map.empty() || count == 0) {
        return;
    }
    const auto normalizedCount = static_cast<std::size_t>(count) % map.size();
    if (normalizedCount == 0) {
        return;
    }
    if (towardFront) {
        std::ranges::rotate(map, map.begin() + static_cast<std::ptrdiff_t>(normalizedCount));
        return;
    }
    std::ranges::rotate(map, map.end() - static_cast<std::ptrdiff_t>(normalizedCount));
}

auto RemappedBuffer::eraseFromMap(CoordinateMap &map, const bgeo::BlockCoordinate start, const int count)
    -> std::span<const bgeo::BlockCoordinate> {
    const auto recycledSize = static_cast<std::size_t>(count);
    const auto first = map.begin() + start.toRawValue();
    const auto last = first + count;
    std::ranges::rotate(first, last, map.end());
    return {map.data() + (map.size() - recycledSize), recycledSize};
}

auto RemappedBuffer::insertIntoMap(CoordinateMap &map, const bgeo::BlockCoordinate start, const int count)
    -> std::span<const bgeo::BlockCoordinate> {
    const auto recycledSize = static_cast<std::size_t>(count);
    std::ranges::rotate(map.begin() + start.toRawValue(), map.end() - count, map.end());
    return {map.data() + start.toSizeT(), recycledSize};
}

auto RemappedBuffer::moveInMap(
    CoordinateMap &map, const bgeo::BlockCoordinate start, const int count, const bgeo::BlockCoordinate delta)
    -> std::span<const bgeo::BlockCoordinate> {
    const auto targetStart = start + delta;
    const auto droppedBefore = std::clamp((-targetStart).toRawValue(), 0, count);
    const auto droppedAfter =
        std::clamp((targetStart + count - bgeo::BlockCoordinate{map.size()}).toRawValue(), 0, count);

    if (droppedBefore > 0) {
        const auto recycledSize = static_cast<std::size_t>(droppedBefore);
        const auto keptSize = static_cast<std::ptrdiff_t>(count - droppedBefore);
        std::ranges::rotate(
            map.begin() + start.toRawValue(), map.begin() + start.toRawValue() + droppedBefore, map.end());
        std::ranges::rotate(map.begin(), map.begin() + start.toRawValue(), map.begin() + start.toRawValue() + keptSize);
        return {map.data() + (map.size() - recycledSize), recycledSize};
    }

    if (droppedAfter > 0) {
        const auto recycledSize = static_cast<std::size_t>(droppedAfter);
        const auto keptSize = static_cast<std::ptrdiff_t>(count - droppedAfter);
        std::ranges::rotate(
            map.begin(), map.begin() + start.toRawValue() + keptSize, map.begin() + start.toRawValue() + count);
        std::ranges::rotate(
            map.begin() + static_cast<std::ptrdiff_t>(recycledSize) + start.toRawValue(),
            map.begin() + static_cast<std::ptrdiff_t>(recycledSize) + start.toRawValue() + keptSize,
            map.end());
        return {map.data(), recycledSize};
    }

    if (delta > 0) {
        std::ranges::rotate(
            map.begin() + start.toRawValue(),
            map.begin() + start.toRawValue() + count,
            map.begin() + start.toRawValue() + count + delta.toRawValue());
        return {};
    }

    std::ranges::rotate(
        map.begin() + targetStart.toRawValue(),
        map.begin() + start.toRawValue(),
        map.begin() + start.toRawValue() + count);
    return {};
}

void RemappedBuffer::fillStoredRows(const std::span<const bgeo::BlockCoordinate> rows, const Block &fillChar) noexcept {
    for (const auto storedRow : rows) {
        for (auto x = bgeo::BlockCoordinate{0}; x < _size.width(); ++x) {
            _buffer[storedBufferIndex(x, storedRow)] = fillChar;
        }
    }
}

void RemappedBuffer::fillStoredColumns(
    const std::span<const bgeo::BlockCoordinate> columns, const Block &fillChar) noexcept {
    for (const auto storedColumn : columns) {
        for (auto y = bgeo::BlockCoordinate{0}; y < _size.height(); ++y) {
            _buffer[storedBufferIndex(storedColumn, y)] = fillChar;
        }
    }
}

void RemappedBuffer::fastResize(const bgeo::BlockSize newSize, const Block &fillChar) {
    const auto oldArea = _buffer.size();
    _size = newSize;
    _buffer.resize(_size.area().toSizeT());
    if (!fillChar.isEmpty() && _buffer.size() > oldArea) {
        for (auto index = oldArea; index < _buffer.size(); ++index) {
            _buffer[index] = fillChar;
        }
    }
    _rowRemap = linearIndex(_size.height().toSizeT());
    _columnRemap = linearIndex(_size.width().toSizeT());
}

void RemappedBuffer::primaryAxisResize(const bgeo::BlockSize newSize, const Block &fillChar) {
    const auto oldArea = _buffer.size();
    const auto oldPrimarySize = _size.coordinate(_orientation);
    const auto newPrimarySize = newSize.coordinate(_orientation);
    auto &map = primaryMap();

    if (newPrimarySize > oldPrimarySize) {
        _size = newSize;
        _buffer.resize(_size.area().toSizeT());
        if (!fillChar.isEmpty() && _buffer.size() > oldArea) {
            for (auto index = oldArea; index < _buffer.size(); ++index) {
                _buffer[index] = fillChar;
            }
        }
        map.resize(newPrimarySize.toSizeT());
        for (auto index = oldPrimarySize; index < newPrimarySize; ++index) {
            map[index.toSizeT()] = index;
        }
        return;
    }

    auto availableDestinations = CoordinateMap{};
    availableDestinations.reserve((oldPrimarySize - newPrimarySize).toSizeT());
    auto destinationUsed = std::vector<bool>(newPrimarySize.toSizeT(), false);
    for (auto index = bgeo::BlockCoordinate{0}; index < newPrimarySize; ++index) {
        const auto storedCoordinate = map[index.toSizeT()];
        if (storedCoordinate < newPrimarySize) {
            destinationUsed[storedCoordinate.toSizeT()] = true;
        }
    }
    for (auto index = bgeo::BlockCoordinate{0}; index < newPrimarySize; ++index) {
        if (!destinationUsed[index.toSizeT()]) {
            availableDestinations.push_back(index);
        }
    }

    auto destinationIndex = std::size_t{0};
    for (auto index = bgeo::BlockCoordinate{0}; index < newPrimarySize; ++index) {
        auto &storedCoordinate = map[index.toSizeT()];
        if (storedCoordinate < newPrimarySize) {
            continue;
        }
        const auto destination = availableDestinations[destinationIndex++];
        copyStoredPrimaryLine(storedCoordinate, destination);
        storedCoordinate = destination;
    }

    _size = newSize;
    _buffer.resize(_size.area().toSizeT());
    map.resize(newPrimarySize.toSizeT());
}

void RemappedBuffer::reorderedResize(const bgeo::BlockSize newSize, const Block &fillChar) {
    auto newBuffer = std::vector<Block>(newSize.area().toSizeT(), fillChar);
    const auto copySize = _size.limitedWith(newSize);
    copySize.forEach(
        [&](const bgeo::BlockPosition pos) -> void { newBuffer[bufferIndex(pos, newSize, _orientation)] = get(pos); });
    _size = newSize;
    _buffer = std::move(newBuffer);
    _rowRemap = linearIndex(_size.height().toSizeT());
    _columnRemap = linearIndex(_size.width().toSizeT());
}

auto RemappedBuffer::isPrimaryAxisOnlyResize(const bgeo::BlockSize newSize) const noexcept -> bool {
    return newSize.coordinate(_orientation.crossed()) == _size.coordinate(_orientation.crossed());
}

auto RemappedBuffer::primaryMap() noexcept -> CoordinateMap & {
    return _orientation == bgeo::Orientation::Vertical ? _rowRemap : _columnRemap;
}

auto RemappedBuffer::primaryMap() const noexcept -> const CoordinateMap & {
    return _orientation == bgeo::Orientation::Vertical ? _rowRemap : _columnRemap;
}

void RemappedBuffer::copyStoredPrimaryLine(
    const bgeo::BlockCoordinate source, const bgeo::BlockCoordinate destination) noexcept {
    if (source == destination) {
        return;
    }
    if (_orientation == bgeo::Orientation::Vertical) {
        for (auto x = bgeo::BlockCoordinate{0}; x < _size.width(); ++x) {
            _buffer[storedBufferIndex(x, destination)] = _buffer[storedBufferIndex(x, source)];
        }
        return;
    }
    for (auto y = bgeo::BlockCoordinate{0}; y < _size.height(); ++y) {
        _buffer[storedBufferIndex(destination, y)] = _buffer[storedBufferIndex(source, y)];
    }
}

}
