// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BufferRenderer.hpp"

#include "AnsiSequence.hpp"

#include "../../text/Literals.hpp"

#include <algorithm>
#include <utility>

namespace erbsland::cterm::impl {

using namespace text::literals;
using block::Coordinate;
using block::Position;

BufferRenderer::BufferRenderer(const ReadableBuffer &buffer, BufferRenderOptions options) :
    _buffer{buffer}, _options{std::move(options)} {
}

auto BufferRenderer::cell(const Position position) const -> std::optional<Block> {
    auto result = _buffer.get(position);
    if (result.displayWidth() == 0) {
        if (position.x() > 0 && _buffer.get(position - Position{1, 0}).displayWidth() == 2) {
            return {};
        }
        return Block::space().withBase(result.style());
    }
    if (result.displayWidth() == 2 && position.x() + 1 >= _buffer.size().width()) {
        return Block::space().withBase(result.style());
    }
    return result;
}

auto BufferRenderer::contentWidth(const Coordinate row) const -> Coordinate {
    auto width = Coordinate{0};
    for (auto column = Coordinate{0}; column < _buffer.size().width(); ++column) {
        if (const auto block = cell({column, row})) {
            if (block->singleOrNull() != U' ' || block->style().withBase(BlockStyle::reset()) != BlockStyle::reset()) {
                width = std::min(column + block->displayWidth(), _buffer.size().width());
            }
        }
    }
    return width;
}

auto BufferRenderer::measure() -> Coordinate {
    auto lastNonemptyRow = Coordinate{0};
    for (auto row = Coordinate{0}; row < _buffer.size().height(); ++row) {
        const auto width = contentWidth(row);
        if (width > 0) {
            lastNonemptyRow = row + 1;
        }
        if (_options.trimTrailingSpaces()) {
            _widths.push_back(width);
        } else {
            _widths.push_back(_buffer.size().width());
        }
    }
    if (_options.trimTrailingEmptyLines()) {
        return lastNonemptyRow;
    }
    return _buffer.size().height();
}

void BufferRenderer::appendStyle(const BlockStyle style) {
    if (style.color() != _style.color()) {
        _output.append(ansi_sequence::color(style.fg().ansiCode(), style.bg().ansiCode()));
    }
    _output.append(ansi_sequence::attributes(_style.attributes(), style.attributes()));
    _style = style;
}

void BufferRenderer::appendRow(const Coordinate row) {
    const auto width = _widths[static_cast<std::size_t>(row.toRawValue())];
    for (auto column = Coordinate{0}; column < width; ++column) {
        if (const auto block = cell({column, row})) {
            if (_options.format() == RenderFormat::Ansi) {
                appendStyle(block->style().withBase(BlockStyle::reset()));
            }
            _output.append(block->toString());
        }
    }
}

auto BufferRenderer::render() -> text::String {
    _widths.clear();
    _output.clear();
    _style = BlockStyle::reset();
    const auto rows = measure();
    if (_options.format() == RenderFormat::Ansi && rows > 0) {
        _output.append("\x1b[0m"_el);
    }
    for (auto row = Coordinate{0}; row < rows; ++row) {
        if (row > 0) {
            _output.append(U'\n');
        }
        appendRow(row);
    }
    if (_options.format() == RenderFormat::Ansi && rows > 0) {
        _output.append("\x1b[0m"_el);
    }
    return _output;
}

}
