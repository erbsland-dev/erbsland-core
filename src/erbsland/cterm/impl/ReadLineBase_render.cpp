// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ReadLineBase.hpp"

#include "ReadLineLayoutTypes.hpp"

#include "../Block.hpp"
#include "../BlockString.hpp"
#include "../BlockStringEditor.hpp"
#include "../BlockStyle.hpp"
#include "../Buffer.hpp"
#include "../FrameBorder.hpp"
#include "../Terminal.hpp"

#include "../../geometry/Anchor.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringEditor.hpp"
#include "../../unit/CpRange.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <utility>

namespace erbsland::cterm::impl {

using namespace text::literals;
using block::Coordinate;
using block::Position;
using block::Size;

auto ReadLineBase::horizontalBlock(const FrameBorder &border, const FrameBorder::Element element) noexcept -> Block {
    const auto line = border.border(element);
    return FrameBorder::cornerChar(line, {}, line, {});
}

auto ReadLineBase::verticalBlock(const FrameBorder &border, const FrameBorder::Element element) noexcept -> Block {
    const auto line = border.border(element);
    return FrameBorder::cornerChar({}, line, {}, line);
}

void ReadLineBase::setClipped(Buffer &buffer, const int x, const int y, const Block &block) noexcept {
    if (x < 0 || y < 0 || x >= buffer.size().width().toRawValue() || y >= buffer.size().height().toRawValue()) {
        return;
    }
    buffer.set(Position{x, y}, block);
}

void ReadLineBase::drawBlockString(
    Buffer &buffer, int x, const int y, const int maximumWidth, const BlockString &text, const BlockStyle style) {
    const auto endX = x + std::max(0, maximumWidth);
    for (const auto &sourceBlock : text) {
        auto block = sourceBlock.withBase(style);
        const auto width = block.displayWidth();
        if (width <= 0) {
            continue;
        }
        if (x + width > endX) {
            break;
        }
        setClipped(buffer, x, y, block);
        x += width;
    }
}

auto ReadLineBase::titleWithCountdown(const ReadLineOptions &options, const std::int64_t countdown) -> BlockString {
    auto result = BlockStringEditor{options.title()};
    if (!options.timeoutDisplayThreshold().isZero() && countdown >= 0 &&
        countdown <= options.timeoutDisplayThreshold().toRawValue()) {
        if (!result.isEmpty()) {
            result.append(Block{U' '});
        }
        auto countdownText = text::StringEditor{"["_el};
        countdownText.append(text::String::fromInteger(countdown));
        countdownText.append("s]"_el);
        result.append(BlockString{countdownText});
    }
    return result;
}

auto ReadLineBase::createLayout() const -> ReadLineLayout {
    auto result = ReadLineLayout{};
    result.terminalWidth = std::max(1, _terminal->size().width().toRawValue());
    const auto hasVerticalFrame = _options.displayStyle() == ReadLineDisplayStyle::Frame;
    const auto frameInset = hasVerticalFrame && result.terminalWidth >= 2 ? 1 : 0;
    result.contentLeft = std::min(result.terminalWidth - 1, frameInset + _options.padding().leading().toRawValue());
    result.contentRight = std::max(
        result.contentLeft + 1, result.terminalWidth - frameInset - _options.padding().trailing().toRawValue());
    const auto contentWidth = std::max(1, result.contentRight - result.contentLeft);
    result.promptWidth = std::clamp(_options.prompt().displayWidth(), 0, std::max(0, contentWidth - 1));
    result.textColumn = result.contentLeft + result.promptWidth;
    result.editWidth = std::max(1, contentWidth - result.promptWidth);

    const auto &textValue = displayText();
    const auto textEnd = unit::CpIndex::end(textValue.length());
    result.cursorPositions.resize(textValue.length().toSizeT() + 1U);
    result.rows.emplace_back();
    result.rows.back().boundaries.push_back(ReadLineBoundary{0, {}});
    result.cursorPositions[0U] = ReadLineLayout::CursorPosition{0U, 0};

    auto rowIndex = std::size_t{0U};
    auto column = 0;
    auto index = unit::CpIndex{};
    const auto addBoundary = [&](const unit::CpIndex boundaryIndex, const int boundaryColumn) -> void {
        result.rows[rowIndex].boundaries.push_back(ReadLineBoundary{boundaryColumn, boundaryIndex});
        result.cursorPositions[boundaryIndex.toSizeT()] = ReadLineLayout::CursorPosition{rowIndex, boundaryColumn};
    };
    const auto startRow = [&](const unit::CpIndex boundaryIndex) -> void {
        result.rows.emplace_back();
        rowIndex += 1U;
        column = 0;
        addBoundary(boundaryIndex, column);
    };

    while (index < textEnd) {
        const auto character = textValue.charAt(index);
        if (character == U'\n') {
            addBoundary(index, column);
            ++index;
            startRow(index);
            continue;
        }

        const auto endIndex = nextUnitEnd(index);
        auto block = Block{text::U32String{textValue.slice(unit::CpRange{index, index.absoluteDistanceTo(endIndex)})}};
        block = block.withBase(_options.textStyle().withBase(_options.backgroundStyle()));
        auto width = std::max(1, block.displayWidth());
        if (width > result.editWidth) {
            block = Block{U'\uFFFD', _options.textStyle()};
            width = 1;
        }
        if (column > 0 && column + width > result.editWidth) {
            startRow(index);
        }
        result.rows[rowIndex].cells.push_back(ReadLineCell{column, index, endIndex, block});
        result.cursorPositions[index.toSizeT()] = ReadLineLayout::CursorPosition{rowIndex, column};
        column += width;
        index = endIndex;
        addBoundary(index, column);
        if (column >= result.editWidth && index == textEnd) {
            startRow(index);
        }
    }
    return result;
}

auto ReadLineBase::closestBoundary(const ReadLineLayoutRow &row, const int column) noexcept -> unit::CpIndex {
    auto result = row.boundaries.front().index;
    auto bestDistance = std::numeric_limits<int>::max();
    for (const auto &boundary : row.boundaries) {
        const auto distance = std::abs(boundary.column - column);
        if (distance < bestDistance) {
            bestDistance = distance;
            result = boundary.index;
        }
    }
    return result;
}

auto ReadLineBase::cursorRow(const ReadLineLayout &layout) const noexcept -> std::size_t {
    return layout.cursorPositions[std::min(_cursorIndex.toSizeT(), layout.cursorPositions.size() - 1U)].row;
}

auto ReadLineBase::cursorColumn(const ReadLineLayout &layout) const noexcept -> int {
    return layout.cursorPositions[std::min(_cursorIndex.toSizeT(), layout.cursorPositions.size() - 1U)].column;
}

auto ReadLineBase::moveHome(const ReadLineLayout &layout) noexcept -> bool {
    const auto row = cursorRow(layout);
    const auto target = layout.rows[row].boundaries.front().index;
    if (target == _cursorIndex) {
        return false;
    }
    _cursorIndex = target;
    resetPreferredColumn();
    return true;
}

auto ReadLineBase::moveEnd(const ReadLineLayout &layout) noexcept -> bool {
    const auto row = cursorRow(layout);
    const auto target = layout.rows[row].boundaries.back().index;
    if (target == _cursorIndex) {
        return false;
    }
    _cursorIndex = target;
    resetPreferredColumn();
    return true;
}

auto ReadLineBase::moveUp(const ReadLineLayout &layout) -> bool {
    const auto row = cursorRow(layout);
    if (row == 0U) {
        return selectPreviousHistory();
    }
    const auto column = _preferredColumn.value_or(cursorColumn(layout));
    _preferredColumn = column;
    _cursorIndex = closestBoundary(layout.rows[row - 1U], column);
    return true;
}

auto ReadLineBase::moveDown(const ReadLineLayout &layout) -> bool {
    const auto row = cursorRow(layout);
    if (row + 1U >= layout.rows.size()) {
        return selectNextHistory();
    }
    const auto column = _preferredColumn.value_or(cursorColumn(layout));
    _preferredColumn = column;
    _cursorIndex = closestBoundary(layout.rows[row + 1U], column);
    return true;
}

auto ReadLineBase::handleNavigationKey(const Key &key) -> bool {
    const auto layout = createLayout();
    switch (key.type()) {
    case Key::Left:
        moveLeft();
        return true;
    case Key::Right:
        moveRight();
        return true;
    case Key::Home:
        moveHome(layout);
        return true;
    case Key::End:
        moveEnd(layout);
        return true;
    case Key::Up:
        moveUp(layout);
        return true;
    case Key::Down:
        moveDown(layout);
        return true;
    default:
        return false;
    }
}

void ReadLineBase::render(const bool showCursor) {
    const auto layout = createLayout();
    const auto title = titleWithCountdown(_options, _lastCountdown);
    const auto maximumRows = std::max<std::size_t>(1U, _options.maximumDisplayLines().toSizeT());
    const auto visibleRowCount = std::min(maximumRows, layout.rows.size());
    const auto activeCursorRow = cursorRow(layout);
    auto firstVisibleRow = std::size_t{0U};
    if (activeCursorRow >= visibleRowCount) {
        firstVisibleRow = activeCursorRow - visibleRowCount + 1U;
    }
    if (firstVisibleRow + visibleRowCount > layout.rows.size()) {
        firstVisibleRow = layout.rows.size() - visibleRowCount;
    }

    const auto hasCompactTitle = (_options.displayStyle() == ReadLineDisplayStyle::Compact ||
                                     _options.displayStyle() == ReadLineDisplayStyle::HorizontalSpace) &&
        !title.isEmpty();
    const auto hasFrameRows = _options.displayStyle() == ReadLineDisplayStyle::HorizontalFrame ||
        _options.displayStyle() == ReadLineDisplayStyle::Frame;
    const auto topRows = hasFrameRows || hasCompactTitle ? 1 : 0;
    const auto bottomRows =
        hasFrameRows ? 1 : (_options.displayStyle() == ReadLineDisplayStyle::HorizontalSpace ? 1 : 0);
    const auto actualHeight = topRows + static_cast<int>(visibleRowCount) + bottomRows;
    const auto fill = Block{U' ', _options.backgroundStyle()};
    auto area = Buffer{Size{layout.terminalWidth, actualHeight}, fill};

    if (hasFrameRows) {
        const auto &border = _options.frameBorder();
        const auto topLine = horizontalBlock(border, FrameBorder::Element::Top).withBase(_options.backgroundStyle());
        const auto bottomLine =
            horizontalBlock(border, FrameBorder::Element::Bottom).withBase(_options.backgroundStyle());
        for (auto x = 0; x < layout.terminalWidth; ++x) {
            setClipped(area, x, 0, topLine);
            setClipped(area, x, actualHeight - 1, bottomLine);
        }
        if (_options.displayStyle() == ReadLineDisplayStyle::Frame && layout.terminalWidth >= 2) {
            setClipped(area, 0, 0, border.corner(geometry::Anchor::TopLeft).withBase(_options.backgroundStyle()));
            setClipped(
                area,
                layout.terminalWidth - 1,
                0,
                border.corner(geometry::Anchor::TopRight).withBase(_options.backgroundStyle()));
            setClipped(
                area,
                0,
                actualHeight - 1,
                border.corner(geometry::Anchor::BottomLeft).withBase(_options.backgroundStyle()));
            setClipped(
                area,
                layout.terminalWidth - 1,
                actualHeight - 1,
                border.corner(geometry::Anchor::BottomRight).withBase(_options.backgroundStyle()));
            const auto leftLine =
                verticalBlock(border, FrameBorder::Element::Left).withBase(_options.backgroundStyle());
            const auto rightLine =
                verticalBlock(border, FrameBorder::Element::Right).withBase(_options.backgroundStyle());
            for (auto y = 1; y < actualHeight - 1; ++y) {
                setClipped(area, 0, y, leftLine);
                setClipped(area, layout.terminalWidth - 1, y, rightLine);
            }
        }
        if (!title.isEmpty()) {
            const auto titleX = _options.displayStyle() == ReadLineDisplayStyle::Frame ? 3 : 4;
            if (titleX < layout.terminalWidth - 1) {
                const auto titleStyle = _options.titleStyle().withBase(_options.backgroundStyle());
                const auto titleWidth = std::max(0, layout.terminalWidth - titleX - 2);
                setClipped(area, titleX - 1, 0, Block{U' ', titleStyle});
                drawBlockString(area, titleX, 0, titleWidth, title, titleStyle);
                const auto titleEnd = titleX + std::min(titleWidth, std::max(0, title.displayWidth()));
                setClipped(area, titleEnd, 0, Block{U' ', titleStyle});
            }
        }
    } else if (hasCompactTitle) {
        drawBlockString(
            area, 0, 0, layout.terminalWidth, title, _options.titleStyle().withBase(_options.backgroundStyle()));
    }

    const auto editStartY = topRows;
    for (auto visibleIndex = std::size_t{0U}; visibleIndex < visibleRowCount; ++visibleIndex) {
        const auto sourceRowIndex = firstVisibleRow + visibleIndex;
        const auto y = editStartY + static_cast<int>(visibleIndex);
        const auto &row = layout.rows[sourceRowIndex];
        if (sourceRowIndex == 0U && layout.promptWidth > 0) {
            drawBlockString(
                area,
                layout.contentLeft,
                y,
                layout.promptWidth,
                _options.prompt(),
                _options.promptStyle().withBase(_options.backgroundStyle()));
        }
        for (const auto &cell : row.cells) {
            setClipped(area, layout.textColumn + cell.column, y, cell.block);
        }
        if (displayText().isEmpty() && sourceRowIndex == 0U) {
            drawBlockString(
                area,
                layout.textColumn,
                y,
                layout.editWidth,
                _options.placeholder(),
                _options.placeholderStyle().withBase(_options.backgroundStyle()));
        }
    }

    if (showCursor && activeCursorRow >= firstVisibleRow && activeCursorRow < firstVisibleRow + visibleRowCount) {
        const auto y = editStartY + static_cast<int>(activeCursorRow - firstVisibleRow);
        const auto x = layout.textColumn + cursorColumn(layout);
        if (x < layout.contentRight) {
            const auto existing = area.get(Position{x, y});
            if (!existing.isEmpty() && existing != U' ') {
                setClipped(area, x, y, existing.withOverlay(_options.cursorStyle()));
            } else {
                setClipped(
                    area,
                    x,
                    y,
                    _options.cursorBlock().withBase(_options.textStyle()).withBase(_options.backgroundStyle()));
            }
        }
    }

    const auto writeHeight = std::max(actualHeight, _renderedHeight);
    if (writeHeight == actualHeight) {
        writeRenderedBuffer(area, actualHeight);
        return;
    }
    auto expanded = Buffer{Size{layout.terminalWidth, writeHeight}, Block{U' ', BlockStyle::reset()}};
    expanded.setFrom(area, Block{U' ', BlockStyle::reset()});
    writeRenderedBuffer(expanded, actualHeight);
}

void ReadLineBase::writeRenderedBuffer(const ReadableBuffer &buffer, const int actualHeight) {
    _terminal->setAutoWrap(false);
    _terminal->write(buffer);
    _terminal->moveUp(Coordinate{buffer.size().height().toRawValue()});
    _terminal->setAutoWrap(true);
    _terminal->flush();
    _renderedHeight = actualHeight;
    _renderedWidth = buffer.size().width().toRawValue();
}

void ReadLineBase::renderFinal() noexcept {
    try {
        render(false);
        _terminal->moveDown(Coordinate{_renderedHeight});
        _terminal->flush();
    } catch (...) {}
}

void ReadLineBase::clearRenderedArea() noexcept {
    if (_renderedHeight <= 0) {
        return;
    }
    try {
        const auto width = std::max(1, _terminal->size().width().toRawValue());
        auto clearBuffer = Buffer{Size{width, _renderedHeight}, Block{U' ', BlockStyle::reset()}};
        writeRenderedBuffer(clearBuffer, 0);
    } catch (...) {}
}

}
