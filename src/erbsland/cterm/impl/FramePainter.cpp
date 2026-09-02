// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "FramePainter.hpp"

#include "../Block16Style.hpp"
#include "../Tile9Style.hpp"

namespace erbsland::cterm::impl {

using block::Coordinate;
using block::Position;
using block::Rectangle;

void FramePainter::drawFrame(
    const Rectangle rect, const Block &frameBlock, const BlockCombinationStylePtr &combinationStyle) noexcept {

    rect.forEachInFrame([&, this](const Position pos) -> void { set(pos, frameBlock, combinationStyle); });
}

void FramePainter::drawFrame(
    Rectangle rect,
    const Block16StylePtr &frameStyle,
    const BlockCombinationStylePtr &combinationStyle,
    const Color frameColor) noexcept {

    rect.forEachInFrame([&, this](const Position pos) -> void {
        set(pos, blockForFrame(rect, pos, frameStyle).withBase(frameColor), combinationStyle);
    });
}

void FramePainter::drawFrame(
    const Rectangle rect,
    const Tile9StylePtr &style,
    const Color frameColor,
    const BlockCombinationStylePtr &combinationStyle) noexcept {

    if (style == nullptr) {
        return;
    }
    rect.forEachInFrame([&, this](const Position pos) -> void {
        set(pos, style->block(rect, pos).withBase(frameColor), combinationStyle);
    });
}

void FramePainter::drawFrame(const Rectangle rect, const FrameStyle frameStyle, const Color frameColor) noexcept {
    if (const auto tile9Style = Tile9Style::forStyle(frameStyle); tile9Style != nullptr) {
        drawFrame(rect, tile9Style, frameColor, BlockCombinationStyle::commonBoxFrame());
        return;
    }
    if (const auto style = Block16Style::forStyle(frameStyle); style != nullptr) {
        drawFrame(rect, style, BlockCombinationStyle::commonBoxFrame(), frameColor);
    }
}

void FramePainter::drawFrame(
    const Rectangle rect, const FrameDrawOptions &options, const std::size_t animationCycle) noexcept {
    if (rect.width() <= 0 || rect.height() <= 0) {
        return;
    }
    Tile9StylePtr tile9Style;
    Block16StylePtr block16Style;
    if (options.tile9Style() != nullptr) {
        tile9Style = options.tile9Style();
    } else if (options.block16Style() != nullptr) {
        block16Style = options.block16Style();
    } else if (tile9Style = Tile9Style::forStyle(options.style()); tile9Style == nullptr) {
        block16Style = Block16Style::forStyle(options.style());
    }
    if (block16Style == nullptr && tile9Style == nullptr) {
        return;
    }
    rect.forEach([&, this](const Position pos) -> void {
        const auto frameBaseColor = colorForFramePosition(
            options.frameColor(), options.frameColorMode(), rect, pos, animationCycle, options.animationOffset());
        if (rect.isFrame(pos)) {
            if (tile9Style != nullptr) {
                drawFrameBlock(pos, tile9Style->block(rect, pos), frameBaseColor, options.combinationStyle());
            } else {
                drawFrameBlock(pos, blockForFrame(rect, pos, block16Style), frameBaseColor, options.combinationStyle());
            }
            return;
        }
        if (tile9Style != nullptr) {
            const auto fillBaseColor = frameBaseColor.overlayWith(colorForFramePosition(
                options.fillColor(), options.fillColorMode(), rect, pos, animationCycle, options.animationOffset()));
            drawFrameBlock(pos, tile9Style->block(rect, pos), fillBaseColor, options.combinationStyle());
            return;
        }
        if (options.fillBlock().displayWidth() <= 0) {
            return;
        }
        const auto fillBaseColor = frameBaseColor.overlayWith(colorForFramePosition(
            options.fillColor(), options.fillColorMode(), rect, pos, animationCycle, options.animationOffset()));
        drawFrameBlock(pos, options.fillBlock(), fillBaseColor, options.combinationStyle());
    });
}

void FramePainter::drawGridLayout(const Position pos, const GridLayout &layout, const FrameBorder &border) noexcept {

    const auto layoutSize = layout.size(border);
    const auto &topBorder = border.border(FrameBorder::Element::Top);
    const auto &bottomBorder = border.border(FrameBorder::Element::Bottom);
    const auto &leftBorder = border.border(FrameBorder::Element::Left);
    const auto &rightBorder = border.border(FrameBorder::Element::Right);
    const auto &hLineBorder = border.border(FrameBorder::Element::HLine);
    const auto &vLineBorder = border.border(FrameBorder::Element::VLine);

    auto horizontalLines = FrameLineList{};
    auto verticalLines = FrameLineList{};
    horizontalLines.reserve(layout.rowCount() + 1);
    verticalLines.reserve(layout.columnCount() + 1);
    addGridLine(horizontalLines, pos.y(), topBorder);
    addGridLine(verticalLines, pos.x(), leftBorder);

    auto lineY = pos.y() + lineSize(topBorder);
    const auto &rowHeights = layout.rowHeights();
    for (std::size_t row = 0; row + 1 < rowHeights.size(); ++row) {
        lineY += rowHeights[row];
        addGridLine(horizontalLines, lineY, hLineBorder);
        lineY += lineSize(hLineBorder);
    }
    addGridLine(horizontalLines, pos.y() + layoutSize.height() - 1, bottomBorder);

    auto lineX = pos.x() + lineSize(leftBorder);
    const auto &columnWidths = layout.columnWidths();
    for (std::size_t column = 0; column + 1 < columnWidths.size(); ++column) {
        lineX += columnWidths[column];
        addGridLine(verticalLines, lineX, vLineBorder);
        lineX += lineSize(vLineBorder);
    }
    addGridLine(verticalLines, pos.x() + layoutSize.width() - 1, rightBorder);

    const auto x1 = pos.x();
    const auto x2 = pos.x() + layoutSize.width() - 1;
    const auto y1 = pos.y();
    const auto y2 = pos.y() + layoutSize.height() - 1;
    for (const auto &horizontalLine : horizontalLines) {
        drawHorizontalGridLine(horizontalLine, verticalLines, x1, x2, y1, y2);
    }
    for (const auto &verticalLine : verticalLines) {
        drawVerticalGridLine(verticalLine, horizontalLines, y1, y2);
    }
}

void FramePainter::drawFilledFrame(
    Rectangle rect,
    const Block &frameBlock,
    const Block &fillBlock,
    const BlockCombinationStylePtr &combinationStyle) noexcept {

    rect.forEach([&, this](const Position pos) -> void {
        if (rect.isFrame(pos)) {
            set(pos, frameBlock, combinationStyle);
        } else {
            set(pos, fillBlock, combinationStyle);
        }
    });
}

void FramePainter::drawFilledFrame(
    Rectangle rect,
    const Block16StylePtr &frameStyle,
    const Block &fillBlock,
    const BlockCombinationStylePtr &combinationStyle,
    const Color frameColor) noexcept {

    rect.forEach([&, this](const Position pos) -> void {
        if (rect.isFrame(pos)) {
            set(pos, blockForFrame(rect, pos, frameStyle).withBase(frameColor), combinationStyle);
        } else {
            set(pos, fillBlock, combinationStyle);
        }
    });
}

void FramePainter::drawFilledFrame(
    Rectangle rect,
    const Tile9StylePtr &style,
    const Block &fillBlock,
    const BlockCombinationStylePtr &combinationStyle,
    const Color frameColor) noexcept {

    if (style == nullptr) {
        fill(rect, fillBlock, combinationStyle);
        return;
    }
    rect.forEach([&, this](const Position pos) -> void {
        if (rect.isFrame(pos)) {
            set(pos, style->block(rect, pos).withBase(frameColor), combinationStyle);
        } else {
            set(pos, fillBlock, combinationStyle);
        }
    });
}

void FramePainter::drawFilledFrame(
    const Rectangle rect, const FrameStyle frameStyle, const Block &fillBlock, const Color frameColor) noexcept {
    if (const auto tile9Style = Tile9Style::forStyle(frameStyle); tile9Style != nullptr) {
        drawFilledFrame(rect, tile9Style, fillBlock, BlockCombinationStyle::commonBoxFrame(), frameColor);
        return;
    }
    if (const auto style = Block16Style::forStyle(frameStyle); style != nullptr) {
        drawFilledFrame(rect, style, fillBlock, BlockCombinationStyle::commonBoxFrame(), frameColor);
    }
}

auto FramePainter::blockForFrame(const Rectangle rect, const Position pos, const Block16StylePtr &frameStyle) -> Block {

    const auto bitMask =
        pos.cardinalFourBitmask([&](const Position deltaPos) -> bool { return rect.isFrame(deltaPos); });
    return frameStyle->block(bitMask);
}

auto FramePainter::blockForGridLine(const FrameBorder::Border &border, const uint32_t bitMask) noexcept -> Block {
    if (const auto style = Block16Style::forStyle(border.style); style != nullptr) {
        return style->block(bitMask);
    }
    return {};
}

auto FramePainter::lineSize(const FrameBorder::Border &border) noexcept -> Coordinate {
    return Coordinate{border.style != FrameStyle::None && FrameBorder::isLineStyle(border.style) ? 1 : 0};
}

auto FramePainter::lineAt(const FrameLineList &lines, const Coordinate coordinate, std::size_t &index) noexcept
    -> OptionalFrameBorderReference {

    while (index < lines.size() && lines[index].coordinate < coordinate) {
        ++index;
    }
    if (index < lines.size() && lines[index].coordinate == coordinate) {
        return lines[index].border;
    }
    return std::nullopt;
}

void FramePainter::addGridLine(
    FrameLineList &lines, const Coordinate coordinate, const FrameBorder::Border &border) noexcept {

    if (lineSize(border) == 0) {
        return;
    }
    lines.push_back(FrameLine{.coordinate = coordinate, .border = std::cref(border)});
}

void FramePainter::drawHorizontalGridLine(
    const FrameLine &horizontalLine,
    const FrameLineList &verticalLines,
    const Coordinate x1,
    const Coordinate x2,
    const Coordinate y1,
    const Coordinate y2) noexcept {

    const auto &horizontalBorder = horizontalLine.border.get();
    const auto block = blockForGridLine(horizontalBorder, 0b0101);
    auto verticalLineIndex = std::size_t{0};
    for (auto x = x1; x <= x2; ++x) {
        const auto verticalBorder = lineAt(verticalLines, x, verticalLineIndex);
        if (!verticalBorder.has_value()) {
            drawFrameBlock(Position{x, horizontalLine.coordinate}, block, horizontalBorder.color, {});
            continue;
        }
        auto eastBorder = OptionalFrameBorderReference{};
        auto southBorder = OptionalFrameBorderReference{};
        auto westBorder = OptionalFrameBorderReference{};
        auto northBorder = OptionalFrameBorderReference{};
        if (x < x2) {
            eastBorder = horizontalLine.border;
        }
        if (horizontalLine.coordinate < y2) {
            southBorder = verticalBorder;
        }
        if (x > x1) {
            westBorder = horizontalLine.border;
        }
        if (horizontalLine.coordinate > y1) {
            northBorder = verticalBorder;
        }
        drawGridBlock(Position{x, horizontalLine.coordinate}, eastBorder, southBorder, westBorder, northBorder);
    }
}

void FramePainter::drawVerticalGridLine(
    const FrameLine &verticalLine,
    const FrameLineList &horizontalLines,
    const Coordinate y1,
    const Coordinate y2) noexcept {

    const auto &verticalBorder = verticalLine.border.get();
    const auto block = blockForGridLine(verticalBorder, 0b1010);
    auto horizontalLineIndex = std::size_t{0};
    for (auto y = y1; y <= y2; ++y) {
        if (lineAt(horizontalLines, y, horizontalLineIndex).has_value()) {
            continue;
        }
        drawFrameBlock(Position{verticalLine.coordinate, y}, block, verticalBorder.color, {});
    }
}

void FramePainter::drawGridBlock(
    const Position pos,
    const OptionalFrameBorderReference east,
    const OptionalFrameBorderReference south,
    const OptionalFrameBorderReference west,
    const OptionalFrameBorderReference north) noexcept {

    static const auto cNone = FrameBorder::Border{};
    drawFrameBlock(
        pos,
        FrameBorder::cornerChar(
            east.has_value() ? east.value().get() : cNone,
            south.has_value() ? south.value().get() : cNone,
            west.has_value() ? west.value().get() : cNone,
            north.has_value() ? north.value().get() : cNone),
        {},
        {});
}

void FramePainter::drawFrameBlock(
    const Position pos,
    const Block &block,
    const Color baseColor,
    const BlockCombinationStylePtr &combinationStyle) noexcept {

    if (!rect().contains(pos)) {
        return;
    }
    const auto finalColor = get(pos).color().overlayWith(baseColor).overlayWith(block.color());
    set(pos, block.withColorReplaced(finalColor), combinationStyle);
}

auto FramePainter::colorForFramePosition(
    const ColorSequence &colorSequence,
    const FrameColorMode colorMode,
    const Rectangle rect,
    const Position pos,
    const std::size_t animationCycle,
    const std::size_t animationOffset) noexcept -> Color {

    if (colorSequence.empty()) {
        return {};
    }
    auto sequenceIndex = static_cast<int64_t>(animationCycle + animationOffset);
    const auto relativePos = pos - rect.topLeft();
    switch (colorMode) {
    case FrameColorMode::VerticalStripes:
        sequenceIndex += relativePos.x().toRawValue();
        break;
    case FrameColorMode::HorizontalStripes:
        sequenceIndex += relativePos.y().toRawValue();
        break;
    case FrameColorMode::ForwardDiagonalStripes:
        sequenceIndex += (relativePos.x() + relativePos.y()).toRawValue();
        break;
    case FrameColorMode::BackwardDiagonalStripes:
        sequenceIndex += (relativePos.y() - relativePos.x()).toRawValue();
        break;
    case FrameColorMode::ChasingBorderCW:
        if (!rect.isFrame(pos)) {
            break;
        }
        sequenceIndex = rect.frameIndex(pos) - sequenceIndex;
        break;
    case FrameColorMode::ChasingBorderCCW:
        if (!rect.isFrame(pos)) {
            break;
        }
        sequenceIndex += rect.frameIndex(pos);
        break;
    case FrameColorMode::OneColor:
    default:
        break;
    }
    const auto sequenceLength = static_cast<int64_t>(colorSequence.sequenceLength());
    const auto wrappedSequenceIndex = ((sequenceIndex % sequenceLength) + sequenceLength) % sequenceLength;
    return colorSequence.color(static_cast<std::size_t>(wrappedSequenceIndex));
}

}
