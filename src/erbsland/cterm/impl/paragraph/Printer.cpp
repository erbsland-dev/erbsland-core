// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Printer.hpp"

#include "../../CursorWriter.hpp"

#include <algorithm>

namespace erbsland::cterm::impl::paragraph {

auto Printer::printTo(CursorWriter &writer) -> int {
    _previousWrapColor.reset();
    const WriteBlockFn writeBlock = [&writer](const Block &block) -> void { writer.writeResolved(block); };
    for (const auto &line : layout().lines()) {
        emitLine(line, writeBlock);
        writer.setBackground(Background::Default);
        writer.writeLineBreak();
    }
    writer.setColor(_baseStyle.color());
    return static_cast<int>(layout().size());
}

void Printer::emitLine(const LayoutLine &line, const WriteBlockFn &writeBlock) {
    const auto baseColor = _baseStyle.color();
    const auto placement = linePlacement(line, _x1, _width);
    emitSpaces(placement.textX, baseColor, writeBlock);
    emitSpaces(line.indentWidth, fillColor(_previousWrapColor), writeBlock);

    auto lastColor = std::optional<Color>{};
    const auto emitText = [&](const Block &block) -> void {
        const auto resolved = block.withBase(_baseStyle);
        writeBlock(resolved);
        if (!resolved.isEmpty()) {
            lastColor = resolved.color();
        }
    };
    for (const auto &fragment : line.fragments) {
        switch (fragment.type()) {
        case LayoutFragment::Type::SourceRange:
            std::ranges::for_each(sourceText().slice(BlockRange{fragment.startIndex(), fragment.length()}), emitText);
            break;
        case LayoutFragment::Type::Spaces:
            for (auto i = 0; i < fragment.width(); ++i) {
                emitText(Block{U' ', fragment.color()});
            }
            break;
        case LayoutFragment::Type::LineBreakStartMark:
            std::ranges::for_each(options().lineBreakStartMark(), emitText);
            break;
        case LayoutFragment::Type::WordBreakMark:
            emitText(options().wordBreakMark());
            break;
        case LayoutFragment::Type::ParagraphEllipsis:
            std::ranges::for_each(options().paragraphEllipsisMark(), emitText);
            break;
        }
    }

    const auto textEnd = placement.textX + placement.textWidth;
    if (placement.endMarkWidth > 0) {
        auto gapColor = baseColor;
        if (usesRightFill()) {
            gapColor = fillColor(lastColor);
        }
        emitSpaces(placement.endMarkX - textEnd, gapColor, writeBlock);
        std::ranges::for_each(options().lineBreakEndMark(), emitText);
    } else if (usesRightFillForLine(line) && lastColor.has_value()) {
        emitSpaces(_x1 + _width - textEnd, fillColor(lastColor), writeBlock);
    }
    _previousWrapColor.reset();
    if (line.wrapsToNext && usesLeftFill()) {
        _previousWrapColor = lastColor;
    }
}

auto Printer::fillColor(const std::optional<Color> &textColor) const noexcept -> Color {
    if (textColor.has_value()) {
        return _baseStyle.color().overlayWith(Color{Foreground::Inherited, textColor->bg()});
    }
    return _baseStyle.color();
}

void Printer::emitSpaces(const int count, const Color color, const WriteBlockFn &writeBlock) const {
    const auto space = Block{U' ', color}.withBase(_baseStyle);
    for (auto i = 0; i < count; ++i) {
        writeBlock(space);
    }
}

}
