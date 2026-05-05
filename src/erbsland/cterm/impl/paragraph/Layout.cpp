// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Layout.hpp"

#include "LineBuilder.hpp"

#include <utility>

namespace erbsland::cterm::impl::paragraph {

Layout::Layout(
    const BlockStringView &text,
    const int width,
    const ParagraphOptions &options,
    const LayoutNewlineMode newlineMode) noexcept :
    _context{text, width, options, options.alignment().isLeft()},
    _newlineMode{newlineMode},
    _wordSeparators{_context.options().wordSeparatorSet()} {
}

auto Layout::build() -> LayoutResult {
    if (_context.width() <= 0) {
        return LayoutResult::invalid();
    }
    auto result = LayoutResult::create();
    const auto sourceLines = splitIntoSourceLines();
    if (_newlineMode == LayoutNewlineMode::HardLineBreak) {
        if (!layoutParagraph(sourceLines, result.lines())) {
            return LayoutResult::invalid();
        }
        return result;
    }
    auto isFirstParagraph = true;
    for (const auto sourceLine : sourceLines) {
        if (!isFirstParagraph && _context.options().paragraphSpacing() == ParagraphSpacing::DoubleLine) {
            result.appendEmptyLine();
        }
        if (!layoutSourceLine(sourceLine, result.lines())) {
            return LayoutResult::invalid();
        }
        isFirstParagraph = false;
    }
    return result;
}

auto Layout::splitIntoSourceLines() const -> std::vector<BlockRange> {
    const auto &text = _context.blockString();
    if (text.isEmpty()) {
        return {};
    }
    auto result = std::vector<BlockRange>{};
    auto lineStartIndex = BlockIndex{};
    const auto textEndIndex = BlockIndex::end(text.length());
    for (auto index = BlockIndex{}; index < textEndIndex; index += BlockCount::one()) {
        if (text[index] == text::Char{U'\n'}) {
            result.push_back(BlockRange{lineStartIndex, lineStartIndex.absoluteDistanceTo(index)});
            lineStartIndex = index + BlockCount::one();
        }
    }
    if (lineStartIndex < textEndIndex) {
        result.push_back(BlockRange{lineStartIndex, lineStartIndex.absoluteDistanceTo(textEndIndex)});
    }
    return result;
}

auto Layout::layoutParagraph(const std::vector<BlockRange> &sourceLines, std::vector<LayoutLine> &lines) -> bool {
    if (sourceLines.empty()) {
        return true;
    }
    for (const auto &sourceLine : sourceLines) {
        if (!layoutSourceLine(sourceLine, lines)) {
            return false;
        }
    }
    return true;
}

auto Layout::layoutSourceLine(const BlockRange sourceLine, std::vector<LayoutLine> &lines) -> bool {
    const auto preparedLine = prepareSourceLine(sourceLine);
    if (preparedLine.empty()) {
        lines.emplace_back();
        return true;
    }
    auto lineBuilder = LineBuilder{_context, preparedLine};
    return lineBuilder.appendLinesTo(lines);
}

auto Layout::prepareSourceLine(const BlockRange sourceLine) const -> LayoutPreparedSourceLine {
    auto result = LayoutPreparedSourceLine{sourceLine};
    const auto leftAligned = _context.leftAligned();
    const auto &text = _context.blockString();
    auto currentWordStartIndex = BlockIndex{};
    auto currentWordLength = BlockCount{};
    auto currentWordWidth = 0;
    auto finishCurrentWord = [&]() -> void {
        result.appendWord(currentWordStartIndex, currentWordLength, currentWordWidth);
        currentWordStartIndex = BlockIndex{};
        currentWordLength = BlockCount{};
        currentWordWidth = 0;
    };
    for (auto index = sourceLine.index(); index < sourceLine.endIndex(); index += BlockCount::one()) {
        const auto &character = text[index];
        const auto codePoint = text::Char{character.singleCodePoint()};
        if (leftAligned && codePoint == text::Char{U'\t'}) {
            finishCurrentWord();
            result.appendPendingTab(index);
            continue;
        }
        if (_wordSeparators.contains(codePoint)) {
            finishCurrentWord();
            result.appendPendingSeparatorSpace(index);
            continue;
        }
        if (currentWordLength.isZero()) {
            currentWordStartIndex = index;
        }
        currentWordLength += BlockCount::one();
        currentWordWidth += character.displayWidth();
    }
    finishCurrentWord();
    result.finish();
    return result;
}

}
