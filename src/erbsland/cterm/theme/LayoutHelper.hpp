// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BlockStringWithMargins.hpp"
#include "ThemeAccessor.hpp"

#include "../../text/String.hpp"
#include "../../text/StringList.hpp"
#include "../../text/StringView.hpp"

namespace erbsland::cterm::theme::layout {

/// Apply padding and margins to a string.
/// @param text The text to style and surround with padding.
/// @param themeAccessor The theme accessor to use for styling (selects element, states, and tags).
/// @param textPart Optional part to use for the text styling, padding, and margins.
///    If Part::None is passed, the themeAccessor is used as part theme.
/// @return The styled text with padding applied, and margins returned.
auto stylePaddingAndMargins(
    const BlockStringView &text, const ThemeAccessor &themeAccessor, Part textPart = Part::None) noexcept
    -> BlockStringWithMargins;
/// @overload
auto stylePaddingAndMargins(
    const text::StringView &text, const ThemeAccessor &themeAccessor, Part textPart = Part::None) noexcept
    -> BlockStringWithMargins;

/// Apply style, padding, margins, and crop to fit to a given width.
/// @param text The text to style and surround with padding.
/// @param displayWidth The maximum width the text, including the ellipsis, will be cropped to.
/// @param themeAccessor The theme accessor to use for styling (selects element, states, and tags).
/// @param textPart Part to use for the text styling, padding, and margins.
///     If Part::None is passed, the themeAccessor is used as part theme.
/// @param ellipsisPart Part to use for the ellipsis block.
///     If Part::None is passed, the themeAccessor is used as part theme.
/// @param textAlignmentForEllipsis The text alignment for the ellipsis block.
///     If the text is left aligned, the ellipsis will be placed at the end of the text.
///     If the text is right aligned, the ellipsis will be placed at the beginning of the text.
///     If the text is center aligned, the ellipsis will be placed in the middle of the text.
///     Only the `Left`, `HCenter`, and `Right` alignments are tested. Vertical alignments are ignored.
auto stylePaddingCropAndMargins(
    const BlockStringView &text,
    bgeo::BlockCoordinate displayWidth,
    const ThemeAccessor &themeAccessor,
    Part textPart = Part::Text,
    Part ellipsisPart = Part::Ellipsis,
    bgeo::Alignment textAlignmentForEllipsis = bgeo::Alignment::Left) noexcept -> BlockStringWithMargins;

/// Enclose a sequence of text parts in brackets using the specified theme accessors.
/// This will create a string using this structure:
/// <code>
/// Elements:
///     LB: left bracket
///     MB: middle bracket (separator)
///     RB: right bracket
///     BlockText: one of the text elements
/// Attributes:
///     LM: left margin     RM: right margin
///     LP: left padding    RP: right padding
///     (padding always rendered as spaces in the given theme char style)
/// Theme style:
///     [B: ...] bracketPart
///     [T: ...] textPart
///
/// [B:LM] collapsed with [T:LM] -> returned as left margin
/// [B: LB + LP]
///     [T: LP + BlockText + RP]
/// [B: RP + MB + LP]
///     [T: LP + BlockText + RP]
/// [B: RP + RB]
/// [B:RM] collapsed with [T:RM] -> returned as right margin
/// </code>
/// @param texts The text to join with surrounding brackets.
/// @param themeAccessor The theme accessor to use for styling (selects element, states and tags).
/// @param bracketPart The part to use for the bracket styling.
/// @param textPart The part to use for the text styling. If set to Part::None, no text styling is applied.
/// @return The joined text with surrounding brackets, styled according to the theme accessor.
auto encloseInBrackets(
    std::span<BlockString> texts, const ThemeAccessor &themeAccessor, Part bracketPart, Part textPart) noexcept
    -> BlockStringWithMargins;
/// @overload
auto encloseInBrackets(
    std::span<BlockStringView> texts, const ThemeAccessor &themeAccessor, Part bracketPart, Part textPart) noexcept
    -> BlockStringWithMargins;
/// @overload
auto encloseInBrackets(
    std::span<text::String> texts, const ThemeAccessor &themeAccessor, Part bracketPart, Part textPart) noexcept
    -> BlockStringWithMargins;
/// @overload
auto encloseInBrackets(
    const text::StringList &texts, const ThemeAccessor &themeAccessor, Part bracketPart, Part textPart) noexcept
    -> BlockStringWithMargins;
/// @overload
auto encloseInBrackets(
    const BlockStringView &text, const ThemeAccessor &themeAccessor, Part bracketPart, Part textPart) noexcept
    -> BlockStringWithMargins;
/// @overload
auto encloseInBrackets(
    const text::StringView &text, const ThemeAccessor &themeAccessor, Part bracketPart, Part textPart) noexcept
    -> BlockStringWithMargins;

}
