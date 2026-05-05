// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Printer.hpp"

#include <algorithm>

namespace erbsland::cterm::impl::paragraph {

auto Printer::print() -> int {
    auto previousWrapColor = std::optional<Color>{};
    for (const auto &line : layout().lines()) {
        const auto placement = linePlacement(line, _x1, _width);
        if (placement.textX > 0) {
            writeSpaces(placement.textX, _baseColor);
        }
        if (line.indentWidth > 0) {
            auto indentColor = _baseColor;
            if (previousWrapColor.has_value() && usesLeftFill()) {
                indentColor = backgroundFillColor(*previousWrapColor);
            }
            writeSpaces(line.indentWidth, indentColor);
        }
        auto lastColor = writeResolved(line);
        if (line.wrapsToNext) {
            if (placement.endMarkWidth > 0) {
                const auto gapWidth = placement.endMarkX - (placement.textX + placement.textWidth);
                if (gapWidth > 0) {
                    auto gapColor = _baseColor;
                    if (usesRightFill() && lastColor.has_value()) {
                        gapColor = backgroundFillColor(*lastColor);
                    }
                    writeSpaces(gapWidth, gapColor);
                }
                for (const auto &character : options().lineBreakEndMark()) {
                    if (const auto markColor = writeResolvedBlock(character); markColor.has_value()) {
                        lastColor = markColor;
                    }
                }
            }
            if (placement.endMarkWidth == 0 && usesRightFill() && lastColor.has_value()) {
                const auto fillWidth = (_x1 + _width) - (placement.textX + placement.textWidth);
                writeSpaces(fillWidth, backgroundFillColor(*lastColor));
            }
        } else if (usesRightFillForLine(line) && lastColor.has_value()) {
            const auto fillWidth = (_x1 + _width) - (placement.textX + placement.textWidth);
            writeSpaces(fillWidth, backgroundFillColor(*lastColor));
        }
        _writer.setBackground(Background::Default);
        _writer.writeLineBreak();
        if (line.wrapsToNext && usesLeftFill()) {
            previousWrapColor = lastColor;
        } else {
            previousWrapColor.reset();
        }
    }
    _writer.setColor(_baseColor);
    return static_cast<int>(layout().size());
}

auto Printer::backgroundFillColor(const Color color) const noexcept -> Color {
    return _baseColor.overlayWith(Color{Foreground::Inherited, color.bg()});
}

auto Printer::writeResolved(const LayoutLine &line) -> std::optional<Color> {
    auto lastColor = std::optional<Color>{};
    for (const auto &fragment : line.fragments) {
        if (const auto fragmentColor = writeResolved(fragment); fragmentColor.has_value()) {
            lastColor = fragmentColor;
        }
    }
    return lastColor;
}

auto Printer::writeResolved(const LayoutFragment &fragment) -> std::optional<Color> {
    switch (fragment.type()) {
    case LayoutFragment::Type::SourceRange: {
        auto lastColor = std::optional<Color>{};
        const auto &text = sourceText();
        for (
            auto index = fragment.startIndex(); index < fragment.startIndex() + fragment.length();
            index += BlockCount::one()) {
            if (const auto characterColor = writeResolvedBlock(text[index]); characterColor.has_value()) {
                lastColor = characterColor;
            }
        }
        return lastColor;
    }
    case LayoutFragment::Type::Spaces: {
        if (fragment.width() <= 0) {
            return std::nullopt;
        }
        const auto resolvedSpace = Block{U' ', fragment.color()}.withBase(_baseStyle);
        _writer.writeRepeatedResolved(resolvedSpace, fragment.width());
        return resolvedSpace.color();
    }
    case LayoutFragment::Type::LineBreakStartMark: {
        auto lastColor = std::optional<Color>{};
        for (const auto &character : options().lineBreakStartMark()) {
            if (const auto characterColor = writeResolvedBlock(character); characterColor.has_value()) {
                lastColor = characterColor;
            }
        }
        return lastColor;
    }
    case LayoutFragment::Type::WordBreakMark:
        return writeResolvedBlock(options().wordBreakMark());
    case LayoutFragment::Type::ParagraphEllipsis: {
        auto lastColor = std::optional<Color>{};
        for (const auto &character : options().paragraphEllipsisMark()) {
            if (const auto characterColor = writeResolvedBlock(character); characterColor.has_value()) {
                lastColor = characterColor;
            }
        }
        return lastColor;
    }
    }
    return std::nullopt;
}

auto Printer::writeResolvedBlock(const Block &character) -> std::optional<Color> {
    const auto resolvedCharacter = character.withBase(_baseStyle);
    _writer.writeResolved(resolvedCharacter);
    if (resolvedCharacter.isEmpty()) {
        return std::nullopt;
    }
    return resolvedCharacter.color();
}

void Printer::writeSpaces(const int count, const Color color) {
    if (count <= 0) {
        return;
    }
    _writer.writeRepeatedResolved(Block{U' ', color}.withBase(_baseStyle), count);
}

}
