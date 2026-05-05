// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LayoutHelper.hpp"

#include <vector>

namespace erbsland::cterm::theme::layout {

auto stylePaddingAndMarginsImpl(const BlockStringView &text, const ThemeAccessor &themeAccessor) noexcept
    -> BlockStringWithMargins {
    const auto margins = themeAccessor.margins();
    const auto padding = themeAccessor.padding();
    if (padding.left() == 0 && padding.right() == 0) {
        return BlockStringWithMargins{BlockString{text}, margins};
    }
    const auto paddingLeftBlock = themeAccessor.block(BlockRole::LeftPadding);
    const auto paddingRightBlock = themeAccessor.block(BlockRole::RightPadding);
    auto result = BlockString{};
    result.append(BlockCount::fromSizeT(padding.left().toSizeT()), paddingLeftBlock);
    result.append(text);
    result.append(BlockCount::fromSizeT(padding.right().toSizeT()), paddingRightBlock);
    return BlockStringWithMargins{result, margins};
}

auto stylePaddingAndMargins(const BlockStringView &text, const ThemeAccessor &themeAccessor, Part textPart) noexcept
    -> BlockStringWithMargins {
    const auto textTheme = textPart != Part::None ? themeAccessor.forPart(textPart) : themeAccessor;
    return stylePaddingAndMarginsImpl(text.withBase(textTheme.style()), textTheme);
}

auto stylePaddingAndMargins(const text::StringView &text, const ThemeAccessor &themeAccessor, Part textPart) noexcept
    -> BlockStringWithMargins {
    const auto textTheme = textPart != Part::None ? themeAccessor.forPart(textPart) : themeAccessor;
    return stylePaddingAndMarginsImpl(BlockString{text, textTheme.style()}, textTheme);
}

auto stylePaddingCropAndMargins(
    const BlockStringView &text,
    bgeo::BlockCoordinate displayWidth,
    const ThemeAccessor &themeAccessor,
    const Part textPart,
    const Part ellipsisPart,
    const bgeo::Alignment textAlignmentForEllipsis) noexcept -> BlockStringWithMargins {

    const auto textTheme = textPart != Part::None ? themeAccessor.forPart(textPart) : themeAccessor;
    const auto ellipsisTheme = ellipsisPart != Part::None ? themeAccessor.forPart(ellipsisPart) : themeAccessor;
    const auto margins = textTheme.margins();
    if (text.isEmpty() || displayWidth <= 0) {
        return BlockStringWithMargins{{}, margins};
    }
    const auto padding = textTheme.padding();
    const auto displayWidthWithPadding = text.displayWidth() + padding.horizontalExtent();
    if (displayWidthWithPadding <= displayWidth) {
        return stylePaddingAndMarginsImpl(text, textTheme);
    }
    const auto targetWidth = displayWidth - 1 - padding.horizontalExtent();
    if (targetWidth < 0) {
        return BlockStringWithMargins{{}, margins};
    }
    const auto paddingLeftBlock = textTheme.block(BlockRole::LeftPadding);
    const auto paddingRightBlock = textTheme.block(BlockRole::RightPadding);
    const auto ellipsisBlock = ellipsisTheme.block(BlockRole::Main);
    if (targetWidth == 0) {
        auto result = BlockString{};
        result.append(BlockCount::fromSizeT(padding.left().toSizeT()), paddingLeftBlock);
        result.append(ellipsisBlock);
        result.append(BlockCount::fromSizeT(padding.right().toSizeT()), paddingRightBlock);
        return BlockStringWithMargins{result, margins};
    }
    auto result = BlockString{};
    result.append(BlockCount::fromSizeT(padding.left().toSizeT()), paddingLeftBlock);
    if (textAlignmentForEllipsis.isLeft()) {
        result.append(text.croppedToDisplayWidth(targetWidth, textAlignmentForEllipsis));
        result.append(ellipsisBlock);
    } else if (textAlignmentForEllipsis.isRight()) {
        result.append(ellipsisBlock);
        result.append(text.croppedToDisplayWidth(targetWidth, textAlignmentForEllipsis));
    } else {
        const auto leftTargetWidth = targetWidth / 2;
        const auto rightTargetWidth = targetWidth - leftTargetWidth;
        result.append(text.croppedToDisplayWidth(leftTargetWidth, bgeo::Alignment::Left));
        result.append(ellipsisBlock);
        result.append(text.croppedToDisplayWidth(rightTargetWidth, bgeo::Alignment::Right));
    }
    result.append(BlockCount::fromSizeT(padding.right().toSizeT()), paddingRightBlock);
    return BlockStringWithMargins{result, margins};
}

template <typename tString>
    requires std::is_same_v<std::remove_cvref_t<tString>, text::String> ||
    std::is_same_v<std::remove_cvref_t<tString>, text::StringView> ||
    std::is_same_v<std::remove_cvref_t<tString>, BlockString> ||
    std::is_same_v<std::remove_cvref_t<tString>, BlockStringView>
auto encloseInBracketsImpl(
    std::span<tString> texts, const ThemeAccessor &themeAccessor, const Part bracketPart, const Part textPart) noexcept
    -> BlockStringWithMargins {

    if (texts.empty()) {
        return BlockStringWithMargins{};
    }
    const auto bracketTheme = bracketPart != Part::None ? themeAccessor.forPart(bracketPart) : themeAccessor;
    const auto bracketPadding = bracketTheme.padding();
    const auto textTheme = textPart != Part::None ? themeAccessor.forPart(textPart) : themeAccessor;
    const auto textStyle = textPart != Part::None ? textTheme.style() : BlockStyle{};
    const auto textPadding = textPart != Part::None ? textTheme.padding() : bgeo::BlockMargins{};
    const auto textLeftPaddingBlock = textTheme.block(BlockRole::LeftPadding);
    const auto textRightPaddingBlock = textTheme.block(BlockRole::RightPadding);
    const auto leftBracketBlock = bracketTheme.block(BlockRole::LeftBracket);
    const auto middleBracketBlock = bracketTheme.block(BlockRole::MiddleBracket);
    const auto rightBracketBlock = bracketTheme.block(BlockRole::RightBracket);
    const auto leftBracketPaddingBlock = bracketTheme.block(BlockRole::LeftOuterPadding);
    const auto rightBracketPaddingBlock = bracketTheme.block(BlockRole::RightOuterPadding);
    const auto leftInnerPaddingBlock = bracketTheme.block(BlockRole::LeftInnerPadding);
    const auto rightInnerPaddingBlock = bracketTheme.block(BlockRole::RightInnerPadding);
    auto result = BlockString{};
    result.append(leftBracketBlock);
    result.append(BlockCount::fromSizeT(bracketPadding.left().toSizeT()), leftBracketPaddingBlock);
    for (std::size_t i = 0; i < texts.size(); ++i) {
        result.append(BlockCount::fromSizeT(textPadding.left().toSizeT()), textLeftPaddingBlock);
        result.appendStyled(texts[i], textStyle);
        result.append(BlockCount::fromSizeT(textPadding.right().toSizeT()), textRightPaddingBlock);
        if (i < texts.size() - 1) {
            result.append(BlockCount::fromSizeT(bracketPadding.right().toSizeT()), rightInnerPaddingBlock);
            result.append(middleBracketBlock);
            result.append(BlockCount::fromSizeT(bracketPadding.left().toSizeT()), leftInnerPaddingBlock);
        }
    }
    result.append(BlockCount::fromSizeT(bracketPadding.right().toSizeT()), rightBracketPaddingBlock);
    result.append(rightBracketBlock);
    const auto margins = bracketTheme.margins().expandedWith(textTheme.margins());
    return BlockStringWithMargins{result, margins};
}

auto encloseInBrackets(
    std::span<BlockString> texts,
    const ThemeAccessor &themeAccessor,
    const Part bracketPart,
    const Part textPart) noexcept -> BlockStringWithMargins {
    return encloseInBracketsImpl(texts, themeAccessor, bracketPart, textPart);
}

auto encloseInBrackets(
    std::span<BlockStringView> texts,
    const ThemeAccessor &themeAccessor,
    const Part bracketPart,
    const Part textPart) noexcept -> BlockStringWithMargins {
    return encloseInBracketsImpl(texts, themeAccessor, bracketPart, textPart);
}

auto encloseInBrackets(
    std::span<text::String> texts,
    const ThemeAccessor &themeAccessor,
    const Part bracketPart,
    const Part textPart) noexcept -> BlockStringWithMargins {
    return encloseInBracketsImpl(texts, themeAccessor, bracketPart, textPart);
}

auto encloseInBrackets(
    const text::StringList &texts,
    const ThemeAccessor &themeAccessor,
    const Part bracketPart,
    const Part textPart) noexcept -> BlockStringWithMargins {
    const auto &rawTexts = texts.toRawValue();
    return encloseInBracketsImpl(std::span{rawTexts.begin(), rawTexts.size()}, themeAccessor, bracketPart, textPart);
}

auto encloseInBrackets(
    const BlockStringView &text,
    const ThemeAccessor &themeAccessor,
    const Part bracketPart,
    const Part textPart) noexcept -> BlockStringWithMargins {
    return encloseInBracketsImpl(std::span{&text, 1}, themeAccessor, bracketPart, textPart);
}

auto encloseInBrackets(
    const text::StringView &text,
    const ThemeAccessor &themeAccessor,
    const Part bracketPart,
    const Part textPart) noexcept -> BlockStringWithMargins {
    return encloseInBracketsImpl(std::span{&text, 1}, themeAccessor, bracketPart, textPart);
}

}
