// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BlockTextPainter.hpp"

#include "BlockStringWrapper.hpp"

#include "paragraph/Layout.hpp"
#include "paragraph/Painter.hpp"

#include "../../text/Char.hpp"

namespace erbsland::cterm::impl {

using namespace bgeo;
using text::Char;
using text::String;
using text::U32String;

void BlockTextPainter::drawBlockText(BlockPosition pos, const BlockString &str) {
    if (str.isEmpty()) {
        return;
    }
    const auto x = pos.x();
    for (const auto character : str) {
        if (character == U'\n') {
            pos.setX(x);
            pos += BlockPosition{0, 1};
            continue;
        }
        if (character.displayWidth() == 0) {
            continue;
        }
        if (rect().contains(pos)) {
            set(pos, character.withBase(get(pos).color()));
        }
        pos += BlockPosition{character.displayWidth(), 0};
    }
}

void BlockTextPainter::drawBlockText(const BlockText &text, const std::size_t animationCycle) {
    drawBlockText(text.blockString(), text.rectangle(), text.blockTextOptions(), animationCycle);
}

auto BlockTextPainter::simpleBlockTextOptions(const Alignment alignment, BlockStyle style) noexcept
    -> BlockTextOptions {
    auto options = BlockTextOptions{alignment};
    options.setColor(style.color());
    options.setBlockAttributes(style.attributes());
    return options;
}

void BlockTextPainter::drawBlockText(
    const BlockString &text,
    const BlockRectangle rect,
    const BlockTextOptions &options,
    const std::size_t animationCycle) {
    const auto textRect = contentRect(rect, options.paragraphOptions());
    if (textRect.width() <= 0 || textRect.height() <= 0) {
        return;
    }
    if (options.font() != nullptr) {
        auto lines = BlockStringLines{};
        for (const auto &paragraph : text.splitLines()) {
            const auto fontLines = buildFontBlockTextLines(options, paragraph);
            lines.insert(lines.end(), fontLines.begin(), fontLines.end());
        }
        applyBlockTextLines(textRect, options, lines, animationCycle);
        return;
    }
    const auto layout =
        paragraph::Layout{
            text,
            textRect.width().toRawValue(),
            options.paragraphOptions(),
            paragraph::LayoutNewlineMode::ParagraphBreak}
            .build();
    if (!layout.valid()) {
        if (options.onError() == ParagraphOnError::Empty) {
            return;
        }
        applyBlockTextLines(
            textRect, options, buildSimpleBlockTextLines(text, textRect, options.paragraphSpacing()), animationCycle);
        return;
    }
    paragraph::Painter{
        _buffer,
        textRect,
        options.alignment(),
        layout,
        text,
        options.paragraphOptions(),
        options.backgroundMode(),
        [&](const Block &character, const BlockPosition position) -> Color {
            return colorForBlockTextPosition(options, character, position, animationCycle);
        }}
        .paint();
}

void BlockTextPainter::drawBlockText(
    const String &text,
    const BlockRectangle rect,
    const Alignment alignment,
    const BlockStyle style,
    const std::size_t animationCycle) {
    drawBlockText(BlockStringEditor{text}, rect, simpleBlockTextOptions(alignment, style), animationCycle);
}

void BlockTextPainter::drawBlockText(
    const U32String &text,
    const BlockRectangle rect,
    const Alignment alignment,
    const BlockStyle style,
    const std::size_t animationCycle) {
    drawBlockText(BlockStringEditor{text}, rect, simpleBlockTextOptions(alignment, style), animationCycle);
}

void BlockTextPainter::drawBlockText(
    const BlockString &text,
    const BlockRectangle rect,
    const Alignment alignment,
    const BlockStyle style,
    const std::size_t animationCycle) {
    drawBlockText(text, rect, simpleBlockTextOptions(alignment, style), animationCycle);
}

auto BlockTextPainter::contentRect(const BlockRectangle rect, const ParagraphOptions &options) noexcept
    -> BlockRectangle {
    return rect.insetBy(options.margins());
}

auto BlockTextPainter::buildSimpleBlockTextLines(
    const BlockString &text, const BlockRectangle rect, ParagraphSpacing spacing) const -> BlockStringLines {
    return text.wrapIntoLines(rect.width().toRawValue(), spacing);
}

auto BlockTextPainter::buildFontBlockTextLines(const BlockTextOptions &options, const BlockString &paragraph) const
    -> BlockStringLines {
    static constexpr auto pixelMap = std::array<Char, 16>{
        Char{U' '},
        Char{U'▘'},
        Char{U'▝'},
        Char{U'▀'},
        Char{U'▖'},
        Char{U'▌'},
        Char{U'▞'},
        Char{U'▛'},
        Char{U'▗'},
        Char{U'▚'},
        Char{U'▐'},
        Char{U'▜'},
        Char{U'▄'},
        Char{U'▙'},
        Char{U'▟'},
        Char{U'█'}};
    if (options.font() == nullptr) {
        return {};
    }
    const auto &font = *options.font();
    auto bitmapWidth = 0;
    auto renderedGlyphs = 0;
    for (const auto &character : paragraph) {
        if (const auto *glyph = font.glyph(character.toString()); glyph != nullptr) {
            bitmapWidth += glyph->size().width().toRawValue();
            ++renderedGlyphs;
        }
    }
    if (renderedGlyphs == 0) {
        return {};
    }
    bitmapWidth += renderedGlyphs - 1;
    auto bitmap = Bitmap{BlockSize{bitmapWidth, font.height()}};
    auto columnColors = std::vector<Color>(static_cast<std::size_t>((bitmapWidth + 1) / 2 + 1));
    auto insertX = 0;
    auto isFirstGlyph = true;
    for (const auto &character : paragraph) {
        const auto *glyph = font.glyph(character.toString());
        if (glyph == nullptr) {
            continue;
        }
        if (!isFirstGlyph) {
            ++insertX;
        }
        bitmap.draw(BlockPosition{insertX, 0}, *glyph);
        const auto startColumn = insertX / 2;
        const auto columnCount = glyph->size().width().toRawValue() / 2 + 1;
        for (auto columnIndex = 0; columnIndex < columnCount; ++columnIndex) {
            columnColors[static_cast<std::size_t>(startColumn + columnIndex)] = character.color();
        }
        insertX += glyph->size().width().toRawValue();
        isFirstGlyph = false;
    }
    const auto rowCount = std::max(1, (font.height() + 1) / 2);
    const auto columns = std::max(1, (bitmapWidth + 1) / 2);
    auto lines = BlockStringLines{};
    lines.reserve(static_cast<std::size_t>(rowCount));
    for (auto y = 0; y < rowCount; ++y) {
        auto line = BlockStringEditor{};
        line.reserve(BlockCount::fromSizeT(static_cast<std::size_t>(columns)));
        for (auto x = 0; x < columns; ++x) {
            const auto color = columnColors[static_cast<std::size_t>(x)];
            line.append(Block{pixelMap[bitmap.pixelQuad(BlockPosition{x, y})], color});
        }
        lines.emplace_back(line);
    }
    return lines;
}

void BlockTextPainter::applyBlockTextLines(
    const BlockRectangle rect,
    const BlockTextOptions &options,
    const BlockStringLines &lines,
    const std::size_t animationCycle) noexcept {
    const auto maxLines = std::min(static_cast<int>(lines.size()), rect.height().toRawValue());
    const auto alignment = options.alignment();
    auto yStart = rect.topLeft().y();
    if (alignment.isVerticalCenter()) {
        yStart += (rect.height() - maxLines) / 2;
    } else if (alignment.isBottom()) {
        yStart = rect.y2() - maxLines;
    }
    for (auto lineIndex = 0; lineIndex < maxLines; ++lineIndex) {
        const auto &line = lines[static_cast<std::size_t>(lineIndex)];
        const auto lineWidth = BlockCoordinate{std::min(line.displayWidth(), rect.width().toRawValue())};
        auto xStart = rect.topLeft().x();
        if (alignment.isRight()) {
            xStart = rect.x2() - lineWidth;
        } else if (alignment.isHorizontalCenter()) {
            xStart += (rect.width() - lineWidth) / 2;
        }
        auto pos = BlockPosition{xStart, yStart + lineIndex};
        for (const auto &character : line) {
            const auto characterWidth = character.displayWidth();
            if (characterWidth <= 0) {
                continue;
            }
            if (pos.x() + characterWidth > rect.x2()) {
                break;
            }
            // only do this calculation if we actually change the buffer.
            if (this->rect().contains(pos)) {
                auto finalColor = colorForBlockTextPosition(options, character, pos, animationCycle);
                finalColor = get(pos).color().overlayWith(finalColor);
                set(pos, character.withOverlay(finalColor));
            }
            pos = pos + BlockPosition{characterWidth, 0};
        }
    }
}

auto BlockTextPainter::colorForBlockTextPosition(
    const BlockTextOptions &options,
    const Block &character,
    const BlockPosition position,
    const std::size_t animationCycle) const noexcept -> Color {

    auto color = Color{};
    if (!options.colorSequence().empty()) {
        auto sequenceIndex = std::size_t{0};
        if (options.animation() == BlockTextAnimation::ColorDiagonal) {
            sequenceIndex =
                animationCycle + static_cast<std::size_t>(std::max(0, (position.x() + position.y()).toRawValue()));
        }
        color = options.colorSequence().color(sequenceIndex);
    }
    return color.overlayWith(character.color());
}

}
