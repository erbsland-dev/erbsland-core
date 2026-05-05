// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LineBuilder.hpp"

#include <utility>

namespace erbsland::cterm::impl::paragraph {

LineBuilder::LineBuilder(const LayoutContext &context, const LayoutPreparedSourceLine &preparedSourceLine) noexcept :
    _context{context}, _preparedSourceLine{preparedSourceLine} {
}

auto LineBuilder::appendLinesTo(std::vector<LayoutLine> &lines) -> bool {
    while (_state.tokenIndex < _preparedSourceLine.tokens.size()) {
        if (auto lastLineBuild = tryBuildLastLine(); lastLineBuild.has_value()) {
            _state = lastLineBuild->nextState;
            lines.push_back(std::move(lastLineBuild->line));
            return true;
        }
        if (_context.options().maximumLineWraps() > 0 && _wrappedLineCount >= _context.options().maximumLineWraps()) {
            auto truncatedLineBuild = buildTruncatedLine();
            if (!truncatedLineBuild.has_value()) {
                return false;
            }
            _state = truncatedLineBuild->nextState;
            lines.push_back(std::move(truncatedLineBuild->line));
            return true;
        }
        auto wrappedLine = buildWrappedLine();
        if (!wrappedLine.has_value()) {
            return false;
        }
        _state = wrappedLine->nextState;
        lines.push_back(std::move(wrappedLine->line));
        _isFirstPhysicalLine = false;
        _wrappedLineCount += 1;
    }
    return true;
}

auto LineBuilder::tryBuildLastLine() const -> std::optional<BuildResult> {
    auto builtLine = buildLine(0, false, false);
    if (!builtLine.has_value() || !builtLine->consumedAll) {
        return std::nullopt;
    }
    return builtLine;
}

auto LineBuilder::buildWrappedLine() const -> std::optional<BuildResult> {
    return buildLine(_context.lineBreakEndMarkWidth(), true, false);
}

auto LineBuilder::buildTruncatedLine() const -> std::optional<BuildResult> {
    return buildLine(_context.paragraphEllipsisWidth(), false, true);
}

auto LineBuilder::buildLine(const int reservedSuffixWidth, const bool addEndMark, const bool addEllipsis) const
    -> std::optional<BuildResult> {

    const auto maximumTextWidth = _context.width() - reservedSuffixWidth;
    auto line = LayoutLine{};
    line.wrappedFromPrevious = !_isFirstPhysicalLine;
    line.indentWidth = _context.calculateIndentWidth(_isFirstPhysicalLine);
    auto prefixWidth = line.indentWidth;
    if (!_isFirstPhysicalLine && _context.leftAligned() && !_context.options().lineBreakStartMark().isEmpty()) {
        line.fragments.appendLiteral(LayoutFragment::Type::LineBreakStartMark, _context.lineBreakStartMarkWidth());
        prefixWidth += _context.lineBreakStartMarkWidth();
    }
    auto usedWidth = prefixWidth;
    auto state = _state;
    auto consumedAnySourceCell = false;
    while (state.tokenIndex < _preparedSourceLine.tokens.size()) {
        auto spacingRun = SpacingRun{0, state.tokenIndex, state.tabStopIndex, SpacingRun::Action::Continue};
        const auto spacingTabStopIndex = state.tabStopIndex;
        auto wordTokenIndex = state.tokenIndex;
        if (_preparedSourceLine.tokens[state.tokenIndex].type() != LayoutLineToken::Type::Word) {
            spacingRun = evaluateSpacingRun(state.tokenIndex, state.tabStopIndex, usedWidth, usedWidth == prefixWidth);
            if (spacingRun.action == SpacingRun::Action::LineBreak) {
                state.tokenIndex = spacingRun.nextTokenIndex;
                state.tabStopIndex = spacingRun.nextTabStopIndex;
                consumedAnySourceCell = true;
                break;
            }
            if (spacingRun.action == SpacingRun::Action::EndOfTokens) {
                state.tokenIndex = spacingRun.nextTokenIndex;
                state.tabStopIndex = spacingRun.nextTabStopIndex;
                if (!consumedAnySourceCell) {
                    consumedAnySourceCell = true;
                }
                break;
            }
            wordTokenIndex = spacingRun.nextTokenIndex;
        }
        const auto &wordToken = _preparedSourceLine.tokens[wordTokenIndex];
        const auto remainingWidth = wordToken.remainingWidth(_context.blockString(), state.wordOffset);
        if (usedWidth + spacingRun.width + remainingWidth <= maximumTextWidth) {
            appendSpacingRun(
                line.fragments,
                state.tokenIndex,
                wordTokenIndex,
                spacingTabStopIndex,
                usedWidth,
                usedWidth == prefixWidth);
            line.fragments.appendSourceRange(
                wordToken.sourceIndex(state.wordOffset), wordToken.length() - state.wordOffset, remainingWidth);
            usedWidth += spacingRun.width + remainingWidth;
            state.tokenIndex = wordTokenIndex + 1;
            state.wordOffset = BlockCount{};
            state.tabStopIndex = spacingRun.nextTabStopIndex;
            consumedAnySourceCell = true;
            continue;
        }
        if (usedWidth > prefixWidth) {
            break;
        }
        auto splitMarkerWidth = 0;
        if (addEndMark && _context.wordBreakMarkWidth() > 0) {
            splitMarkerWidth = _context.wordBreakMarkWidth();
        }
        const auto splitWord = wordToken.split(
            _context.blockString(),
            state.wordOffset,
            maximumTextWidth - usedWidth - spacingRun.width,
            splitMarkerWidth);
        if (!splitWord.has_value()) {
            return std::nullopt;
        }
        appendSpacingRun(
            line.fragments, state.tokenIndex, wordTokenIndex, spacingTabStopIndex, usedWidth, usedWidth == prefixWidth);
        line.fragments.appendSourceRange(
            wordToken.sourceIndex(state.wordOffset), splitWord->sourceCharacterCount, splitWord->sourceWidth);
        if (addEndMark && state.wordOffset + splitWord->sourceCharacterCount < wordToken.length() &&
            _context.wordBreakMarkWidth() > 0) {
            line.fragments.appendLiteral(LayoutFragment::Type::WordBreakMark, _context.wordBreakMarkWidth());
        }
        usedWidth += spacingRun.width + splitWord->width;
        state.tokenIndex = wordTokenIndex;
        state.tabStopIndex = spacingRun.nextTabStopIndex;
        state.wordOffset += splitWord->sourceCharacterCount;
        consumedAnySourceCell = consumedAnySourceCell || !splitWord->sourceCharacterCount.isZero();
        if (state.wordOffset >= wordToken.length()) {
            state.tokenIndex += 1;
            state.wordOffset = BlockCount{};
        }
        break;
    }
    if (!consumedAnySourceCell) {
        return std::nullopt;
    }
    if (addEllipsis && _context.paragraphEllipsisWidth() > 0) {
        line.fragments.appendLiteral(LayoutFragment::Type::ParagraphEllipsis, _context.paragraphEllipsisWidth());
    }
    if (addEndMark) {
        line.wrapsToNext = true;
    }
    return BuildResult{std::move(line), state, state.tokenIndex >= _preparedSourceLine.tokens.size()};
}

auto LineBuilder::evaluateSpacingRun(
    const std::size_t tokenIndex, const std::size_t tabStopIndex, int currentColumn, const bool isLineStart) const
    -> SpacingRun {
    auto result = SpacingRun{0, tokenIndex, tabStopIndex, SpacingRun::Action::Continue};
    auto ignoreSeparatorSpaces = isLineStart;
    for (auto index = tokenIndex; index < _preparedSourceLine.tokens.size(); ++index) {
        const auto &token = _preparedSourceLine.tokens[index];
        if (token.type() == LayoutLineToken::Type::Word) {
            result.nextTokenIndex = index;
            return result;
        }
        result.nextTokenIndex = index + 1;
        if (token.type() == LayoutLineToken::Type::SeparatorSpace) {
            if (ignoreSeparatorSpaces) {
                continue;
            }
            result.width += token.displayWidth();
            currentColumn += token.displayWidth();
            continue;
        }
        const auto tabStop = _context.resolveTabStop(result.nextTabStopIndex);
        if (tabStop > currentColumn) {
            ignoreSeparatorSpaces = false;
            result.width += tabStop - currentColumn;
            currentColumn = tabStop;
            result.nextTabStopIndex += 1;
            continue;
        }
        switch (_context.options().tabOverflowBehavior()) {
        case TabOverflowBehavior::AddSpace:
            ignoreSeparatorSpaces = false;
            result.width += 1;
            currentColumn += 1;
            result.nextTabStopIndex += 1;
            continue;
        case TabOverflowBehavior::LineBreak:
            result.nextTabStopIndex += 1;
            if (isLineStart && result.width == 0) {
                continue;
            }
            result.action = SpacingRun::Action::LineBreak;
            return result;
        }
    }
    result.action = SpacingRun::Action::EndOfTokens;
    return result;
}

void LineBuilder::appendSpacingRun(
    LayoutFragments &fragments,
    const std::size_t startTokenIndex,
    const std::size_t endTokenIndex,
    std::size_t tabStopIndex,
    int currentColumn,
    const bool isLineStart) const {
    auto ignoreSeparatorSpaces = isLineStart;
    for (auto index = startTokenIndex; index < endTokenIndex; ++index) {
        const auto &token = _preparedSourceLine.tokens[index];
        if (token.type() == LayoutLineToken::Type::SeparatorSpace) {
            if (ignoreSeparatorSpaces) {
                continue;
            }
            fragments.appendSpaces(token.displayWidth(), _context.blockString()[token.startIndex()].color());
            currentColumn += token.displayWidth();
            continue;
        }
        const auto tabStop = _context.resolveTabStop(tabStopIndex);
        if (tabStop > currentColumn) {
            fragments.appendSpaces(tabStop - currentColumn, _context.blockString()[token.startIndex()].color());
            currentColumn = tabStop;
            tabStopIndex += 1;
            ignoreSeparatorSpaces = false;
            continue;
        }
        if (_context.options().tabOverflowBehavior() == TabOverflowBehavior::AddSpace) {
            fragments.appendSpaces(1, _context.blockString()[token.startIndex()].color());
            currentColumn += 1;
        }
        tabStopIndex += 1;
        ignoreSeparatorSpaces = false;
    }
}

}
