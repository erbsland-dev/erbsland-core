// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Buffer.hpp"

#include "Block16Style.hpp"
#include "Tile9Style.hpp"

#include "../err/ParameterError.hpp"
#include "../text/EncodingErrorMode.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <utility>

namespace erbsland::cterm {

using namespace bgeo;

Buffer::Buffer() : _size{1, 1}, _data(1U, Block{U' '}) {
}

Buffer::Buffer(const BlockSize size, const Block fillChar) :
    _size{validatedBufferSize(size)}, _data(_size.area().toSizeT(), fillChar) {
}

auto Buffer::size() const noexcept -> BlockSize {
    return _size;
}

auto Buffer::rect() const noexcept -> BlockRectangle {
    return BlockRectangle{BlockPosition{0, 0}, _size};
}

auto Buffer::get(const BlockPosition pos) const noexcept -> const Block & {
    assert(_size.contains(pos));
    if (!_size.contains(pos)) {
        return Block::space();
    }
    return _data[_size.index(pos)];
}

void Buffer::resize(const BlockSize newSize) {
    resize(newSize, BufferResizeMode::Fast, Block{}); // fastest possible resize
}

void Buffer::resize(const BlockSize size, const BufferResizeMode mode, const Block fillChar) {
    if (_size == size) {
        return;
    }
    const auto validatedSize = validatedBufferSize(size);
    if (mode != BufferResizeMode::Fast) {
        if (validatedSize.area() > _size.area()) {
            // if the data expands, do resize before reordering.
            _data.resize(validatedSize.area().toSizeT());
        }
        if (validatedSize.width() < _size.width()) {
            // shrinking horizontally: forward copy should be safe.
            validatedSize.forEach([&](const BlockPosition pos) -> void {
                if (_size.contains(pos)) {
                    _data[validatedSize.index(pos)] = _data[_size.index(pos)];
                } else {
                    _data[validatedSize.index(pos)] = fillChar;
                }
            });
        } else {
            // expanding: reverse copy should be safe.
            for (auto y = validatedSize.height(); y > 0; --y) {
                for (auto x = validatedSize.width(); x > 0; --x) {
                    const auto pos = BlockPosition{x - 1, y - 1};
                    if (_size.contains(pos)) {
                        _data[validatedSize.index(pos)] = _data[_size.index(pos)];
                    } else {
                        _data[validatedSize.index(pos)] = fillChar;
                    }
                }
            }
        }
        if (validatedSize.area() < _size.area()) {
            // if the data shrinks, do resize after reordering.
            _data.resize(validatedSize.area().toSizeT());
        }
    } else {
        _data.resize(validatedSize.area().toSizeT());
        if (!fillChar.isEmpty() && validatedSize.area() > _size.area()) {
            for (auto i = _size.area().toSizeT(); i < _data.size(); ++i) {
                _data[i] = fillChar;
            }
        }
    }
    _size = validatedSize;
}

void Buffer::set(const BlockPosition pos, const Block &block) noexcept {
    // faster
    if (!_size.contains(pos) || block.displayWidth() == 0 || block.displayWidth() > 2) {
        return;
    }
    if (block.displayWidth() == 1) {
        _data[_size.index(pos)] = block;
    } else {
        const auto secondPosition = pos + BlockPosition{1, 0};
        if (!_size.contains(secondPosition)) {
            return;
        }
        // The continuation cell for a wide character must stay logically empty while preserving the style.
        _data[_size.index(secondPosition)] = Block::emptyBlock(block.style());
        _data[_size.index(pos)] = block;
    }
}

void Buffer::fill(const Block &fillBlock) noexcept {
    // faster
    for (auto &dataBlock : _data) {
        dataBlock = fillBlock;
    }
}

auto Buffer::clone() const -> WritableBufferPtr {
    return std::make_shared<Buffer>(*this);
}

void Buffer::setAndResizeFrom(const ReadableBuffer &other) {
    // if we do a copy `Buffer -> Buffer`, use a fast path.
    if (const auto bufferImpl = dynamic_cast<Buffer const *>(&other); bufferImpl != nullptr) {
        _size = validatedBufferSize(bufferImpl->_size);
        _data.resize(bufferImpl->_data.size());
        std::ranges::copy(bufferImpl->_data, _data.begin());
        return;
    }
    // fallback to the original safe implementation.
    WritableBuffer::setAndResizeFrom(other);
}

auto Buffer::fromLinesInString(const BlockString &text) -> Buffer {
    if (text.isEmpty()) {
        throw err::ParameterError{"Text must not be empty.", "text"};
    }
    auto lines = BlockStringLines{};
    for (const auto &line : text.splitLines()) {
        lines.emplace_back(line);
    }
    return fromLines(lines);
}

auto Buffer::fromLines(const BlockStringLines &lines) -> Buffer {
    if (lines.empty()) {
        throw err::ParameterError{"Lines must not be empty.", "lines"};
    }
    BlockSize size{BlockCoordinate{1}, BlockCoordinate{lines.size()}};
    for (const auto &line : lines) {
        if (size.width() < line.displayWidth()) {
            size.setWidth(line.displayWidth());
        }
    }
    auto buffer = Buffer{size};
    BlockPosition pos{0, 0};
    for (const auto &line : lines) {
        buffer.set(pos, line);
        pos += BlockPosition{0, 1};
    }
    return buffer;
}

auto Buffer::validatedBufferSize(const BlockSize size) -> BlockSize {
    if (size.width() < 1 || size.height() < 1) {
        throw err::ParameterError{"Buffer size must be at least 1x1.", "size"};
    }
    if (!size.fitsInto(cMaximumSize)) {
        throw err::ParameterError{"Buffer size must not exceed 10'000x10'000.", "size"};
    }
    return size;
}

void Buffer::drawBlockText(
    const text::String &text,
    const Alignment alignment,
    const BlockRectangle rect,
    const Color color,
    const std::size_t animationCycle) {

    auto renderedText = BlockText{BlockStringEditor{text, text::EncodingErrorMode::Replace}, rect, alignment};
    renderedText.setColor(color);
    drawBlockText(renderedText, animationCycle);
}

}
