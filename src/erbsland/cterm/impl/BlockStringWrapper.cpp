// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BlockStringWrapper.hpp"

namespace erbsland::cterm::impl {

auto BlockStringWrapper::wrapIntoLines(const int width, const ParagraphSpacing paragraphSpacing) const noexcept
    -> std::vector<BlockString> {
    if (width <= 0) {
        return {};
    }
    auto lines = BlockStringLines{};
    auto isFirstParagraph = true;
    for (const auto &paragraph : splitLines()) {
        if (!isFirstParagraph && paragraphSpacing == ParagraphSpacing::DoubleLine) {
            lines.emplace_back();
        }
        const auto paragraphLines = BlockStringWrapper{paragraph}.wrapParagraphIntoLines(width);
        if (paragraphLines.empty()) {
            lines.emplace_back();
        } else {
            lines.insert(lines.end(), paragraphLines.begin(), paragraphLines.end());
        }
        isFirstParagraph = false;
    }
    return lines;
}

auto BlockStringWrapper::splitLines() const noexcept -> std::vector<BlockString> {
    if (_str.isEmpty()) {
        return {};
    }
    auto result = std::vector<BlockString>{};
    result.reserve((_str.count(text::Char{U'\n'}) + BlockCount::one()).toSizeT());
    auto lineStartIndex = BlockIndex{};
    while (lineStartIndex < BlockIndex::end(_str.length())) {
        const auto lineEndIndex = _str.indexOf(text::Char{U'\n'}, lineStartIndex);
        if (lineEndIndex.isNoIndex()) {
            result.emplace_back(BlockString{_str.slice(BlockRange{lineStartIndex, BlockCount::infinite()})});
            break;
        }
        result.emplace_back(
            BlockString{_str.slice(BlockRange{lineStartIndex, lineStartIndex.absoluteDistanceTo(lineEndIndex)})});
        lineStartIndex = lineEndIndex + BlockCount::one();
    }
    return result;
}

void BlockStringWrapper::clearPendingSpacing(BlockString &pendingSpacing, int &pendingSpacingWidth) noexcept {
    pendingSpacing.clear();
    pendingSpacingWidth = 0;
}

void BlockStringWrapper::finishWrappedLine(
    BlockString &line,
    int &lineWidth,
    BlockString &pendingSpacing,
    int &pendingSpacingWidth,
    std::vector<BlockString> &lines) noexcept {
    if (!line.isEmpty()) {
        lines.emplace_back(std::move(line));
        line.clear();
    }
    lineWidth = 0;
    clearPendingSpacing(pendingSpacing, pendingSpacingWidth);
}

void BlockStringWrapper::appendSpacingToken(
    BlockString &spacing,
    const int spacingWidth,
    const BlockString &line,
    BlockString &pendingSpacing,
    int &pendingSpacingWidth) noexcept {
    if (line.isEmpty()) {
        spacing.clear();
        return;
    }
    if (pendingSpacing.isEmpty()) {
        pendingSpacing = std::move(spacing);
    } else {
        pendingSpacing += spacing;
    }
    pendingSpacingWidth += spacingWidth;
    spacing.clear();
}

void BlockStringWrapper::appendWordToken(
    BlockString &word,
    const int wordWidth,
    const int width,
    BlockString &line,
    int &lineWidth,
    BlockString &pendingSpacing,
    int &pendingSpacingWidth,
    std::vector<BlockString> &lines) noexcept {
    if (line.isEmpty()) {
        clearPendingSpacing(pendingSpacing, pendingSpacingWidth);
        startWrappedLine(word, wordWidth, width, line, lineWidth, lines);
        return;
    }
    if (lineWidth + pendingSpacingWidth + wordWidth <= width) {
        line += pendingSpacing;
        line += word;
        lineWidth += pendingSpacingWidth + wordWidth;
        clearPendingSpacing(pendingSpacing, pendingSpacingWidth);
        return;
    }
    finishWrappedLine(line, lineWidth, pendingSpacing, pendingSpacingWidth, lines);
    startWrappedLine(word, wordWidth, width, line, lineWidth, lines);
}

void BlockStringWrapper::startWrappedLine(
    BlockString &word,
    const int wordWidth,
    const int width,
    BlockString &line,
    int &lineWidth,
    std::vector<BlockString> &lines) noexcept {
    if (wordWidth > width) {
        splitWordIntoWrappedLines(word, wordWidth, width, lines);
        line.clear();
        lineWidth = 0;
        word.clear();
        return;
    }
    line = std::move(word);
    lineWidth = wordWidth;
    word.clear();
}

void BlockStringWrapper::splitWordIntoWrappedLines(
    const BlockString &word, const int wordWidth, const int width, std::vector<BlockString> &lines) noexcept {
    const auto estimatedLineCount = static_cast<std::size_t>((wordWidth + width - 1) / width);
    lines.reserve(lines.size() + estimatedLineCount);
    auto startIndex = BlockIndex{};
    while (startIndex < BlockIndex::end(word.length())) {
        const auto endIndex = findWrappedWordSplitIndex(word, startIndex, width);
        lines.emplace_back(word.slice(BlockRange{startIndex, startIndex.absoluteDistanceTo(endIndex)}));
        startIndex = endIndex;
    }
}

auto BlockStringWrapper::findWrappedWordSplitIndex(
    const BlockStringView &word, const BlockIndex startIndex, const int width) noexcept -> BlockIndex {
    auto lineWidth = 0;
    auto index = startIndex;
    while (index < BlockIndex::end(word.length())) {
        const auto characterWidth = word.at(index).displayWidth();
        if (lineWidth > 0 && lineWidth + characterWidth > width) {
            break;
        }
        lineWidth += characterWidth;
        index += BlockCount::one();
        if (lineWidth >= width) {
            break;
        }
    }
    return index;
}

auto BlockStringWrapper::wrapParagraphIntoLines(const int width) const noexcept -> std::vector<BlockString> {
    if (width <= 0) {
        return {};
    }
    auto lines = BlockStringLines{};
    auto line = BlockString{};
    auto lineWidth = 0;
    auto pendingSpacing = BlockString{};
    auto pendingSpacingWidth = 0;
    auto token = BlockString{};
    auto isSpacingToken = false;
    for (const auto &character : _str) {
        const auto currentIsSpacing = character.isSpacing();
        if (token.isEmpty()) {
            token.append(character);
            isSpacingToken = currentIsSpacing;
            continue;
        }
        if (currentIsSpacing == isSpacingToken) {
            token.append(character);
            continue;
        }
        const auto tokenWidth = token.displayWidth();
        if (isSpacingToken) {
            appendSpacingToken(token, tokenWidth, line, pendingSpacing, pendingSpacingWidth);
        } else {
            appendWordToken(token, tokenWidth, width, line, lineWidth, pendingSpacing, pendingSpacingWidth, lines);
        }
        token.clear();
        token.append(character);
        isSpacingToken = currentIsSpacing;
    }
    if (!token.isEmpty()) {
        const auto tokenWidth = token.displayWidth();
        if (isSpacingToken) {
            appendSpacingToken(token, tokenWidth, line, pendingSpacing, pendingSpacingWidth);
        } else {
            appendWordToken(token, tokenWidth, width, line, lineWidth, pendingSpacing, pendingSpacingWidth, lines);
        }
    }
    finishWrappedLine(line, lineWidth, pendingSpacing, pendingSpacingWidth, lines);
    return lines;
}

}
