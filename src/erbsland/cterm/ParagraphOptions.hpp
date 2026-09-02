// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Block.hpp"
#include "BlockString.hpp"
#include "ParagraphBackgroundMode.hpp"
#include "ParagraphIndents.hpp"
#include "ParagraphOnError.hpp"
#include "TabOverflowBehavior.hpp"

#include "../geometry/Alignment.hpp"
#include "../text/CharSet.hpp"
#include "../text/u32/U32String.hpp"

namespace erbsland::cterm {

/// Options that control paragraph wrapping, indentation, tab handling, and fallback behavior.
///
/// These settings are used by `Terminal::printParagraph()` and by `BlockTextOptions` / `BlockText` when text is laid
/// out inside a rectangle.
///
/// @note Creating and copying paragraph options is expensive. Please keep and reuse created instances.
class ParagraphOptions {
public:
    /// Special value that makes `firstLineIndent()` or `wrappedLineIndent()` reuse `lineIndent()`.
    static constexpr auto cUseLineIndent = ParagraphIndents::cUseLineIndent;
    /// Special tab-stop value that resolves to the configured `wrappedLineIndent()`.
    static constexpr auto cTabWrappedLineIndent = -1;

public:
    /// Create paragraph options with the given alignment.
    explicit ParagraphOptions(const geometry::Alignment alignment) noexcept : _alignment{alignment} {}

    // defaults
    ParagraphOptions() = default;
    ParagraphOptions(const ParagraphOptions &) = default;
    ParagraphOptions(ParagraphOptions &&) = default;
    auto operator=(const ParagraphOptions &) -> ParagraphOptions & = default;
    auto operator=(ParagraphOptions &&) -> ParagraphOptions & = default;

public:
    /// The alignment of the paragraph.
    /// For the `Terminal::printParagraph` calls, vertical alignment is ignored.
    /// For the `drawBlockText(BlockText)` calls, the vertical alignment is used to align the text in the given
    /// rectangle.
    [[nodiscard]] auto alignment() const noexcept -> geometry::Alignment;
    /// Set the alignment of the paragraph.
    void setAlignment(geometry::Alignment alignment) noexcept;
    /// Get the configured indents and margins.
    [[nodiscard]] auto indents() const noexcept -> const ParagraphIndents &;
    /// Replace the configured indents and margins.
    /// @param indents The new indent and margin settings.
    void setIndents(const ParagraphIndents &indents) noexcept;
    /// The line indent for all lines.
    /// Only valid if the alignment is set to `geometry::Alignment::Left`.
    /// This indent can be overridden by `firstLineIndent` and `wrappedLineIndent`.
    [[nodiscard]] auto lineIndent() const noexcept -> int;
    /// Set the line indent for all lines.
    /// @param indent The new indent value. `>=0`
    void setLineIndent(int indent) noexcept;
    /// Get the first line indent.
    /// This is the indent for the first line of the paragraph.
    /// Only valid if the alignment is set to `geometry::Alignment::Left`.
    /// @code
    /// 1:| <first indent> A long text that is broken   |
    /// 2:| into multiple lines.                        |
    /// @endcode
    [[nodiscard]] auto firstLineIndent() const noexcept -> int;
    /// Set the first line indent.
    /// @param indent The new indent value. `>=0` or `cUseLineIndent` to use `lineIndent`
    void setFirstLineIndent(int indent) noexcept;
    /// Get the indent for wrapped lines.
    /// This is the indent for all lines that are wrapped at the terminal width.
    /// Only valid if the alignment is set to `geometry::Alignment::Left`.
    /// @code
    /// 1:| <first indent> A long text that is broken   |
    /// 2:| into multiple lines.                        |
    /// @endcode
    [[nodiscard]] auto wrappedLineIndent() const noexcept -> int;
    /// Set the indent for wrapped lines.
    /// @param indent The new indent value. `>=0` or `cUseLineIndent` to use `lineIndent`
    void setWrappedLineIndent(int indent) noexcept;
    /// Get the margins around the paragraph.
    [[nodiscard]] auto margins() const noexcept -> const block::Margins &;
    /// Set the margins around the paragraph.
    /// @param margins The margins around the paragraph area.
    void setMargins(block::Margins margins) noexcept;
    /// Get the background mode.
    /// The background mode determines how the background of the paragraph is handled when lines are wrapped.
    /// It also controls how the background is extended for the last line in the paragraph.
    ///
    /// With `BlockText` / `drawBlockText(...)`, cells outside the wrapped text keep the existing buffer background
    /// unless the selected mode fills them from the wrapped text. With `CursorWriter::printParagraph()`, indentation
    /// and padding are written as spaces, so cells not covered by wrapped-text background use the writer's current
    /// background.
    [[nodiscard]] auto backgroundMode() const noexcept -> ParagraphBackgroundMode;
    /// Set the background mode.
    /// When using `CursorWriter::printParagraph()`, configure the writer background before printing if indentation or
    /// trailing padding should visually match a surrounding panel.
    void setBackgroundMode(ParagraphBackgroundMode backgroundMode) noexcept;
    /// Get the line break end mark.
    /// The line break end mark is appended to wrapped physical lines.
    /// The mark is aligned to the right edge of the available paragraph area.
    /// If the mark contains color information, it will override the background color.
    /// @code
    /// 1:| A long text that is broken  <end mark> |
    /// 2:| into multiple lines.                   |
    /// @endcode
    /// @return The current line break end mark. Empty for no line break end mark.
    [[nodiscard]] auto lineBreakEndMark() const noexcept -> const BlockString &;
    /// Set the line break end mark.
    /// @param mark The new line break end mark. Must not exceed two characters.
    void setLineBreakEndMark(BlockString mark);
    /// Get the line break start mark.
    /// The line break start mark prepends wrapped continuation lines in left-aligned paragraphs.
    /// The mark is inserted after the continuation indentation.
    /// If the mark contains color information, it will override the background color.
    /// @code
    /// 1:| A long text that is broken           |
    /// 2:| <start mark> into multiple lines.    |
    /// @endcode
    /// @return The current line break start mark. Empty for no line break start mark.
    [[nodiscard]] auto lineBreakStartMark() const noexcept -> const BlockString &;
    /// Set the line break start mark.
    /// Unlike the end mark, this decoration may contain an arbitrary number of characters so nested line prefixes and
    /// continuation indentation can be represented.
    /// @param mark The new line break start mark.
    void setLineBreakStartMark(BlockString mark);
    /// The spacing between paragraphs.
    /// The behavior of the paragraph spacing depends on the used interface.
    /// For `Terminal::printParagraph()` and `CursorBuffer::printParagraph()`, embedded newlines are hard line breaks
    /// inside one printed paragraph, and the configured spacing is appended once after the whole call.
    /// For `drawBlockText(BlockText)` calls, each newline starts a new paragraph and spacing is inserted between
    /// paragraphs.
    [[nodiscard]] auto paragraphSpacing() const noexcept -> ParagraphSpacing;
    /// Set the paragraph spacing.
    void setParagraphSpacing(ParagraphSpacing spacing) noexcept;
    /// Get the configured word separators as a canonicalized character string.
    /// Word separators split a source line into words.
    /// Consecutive separators are rendered as a single space between words.
    /// Tabs use special tab-stop handling in left-aligned paragraphs before separator handling is applied.
    /// The returned string is duplicate-free and normalized for comparison and display.
    [[nodiscard]] auto wordSeparators() const -> text::U32String;
    /// Access the shared separator lookup used internally for paragraph layout.
    /// The returned set can be reused by callers that need repeated separator membership checks.
    [[nodiscard]] auto wordSeparatorSet() const noexcept -> const text::CharSet &;
    /// Set the word separators.
    /// Tabs remain special tab stops in left-aligned paragraphs and act as regular word separators in
    /// other alignments when included here.
    /// The provided string is canonicalized automatically and mapped to shared defaults for common patterns.
    /// @param separators A string of Unicode characters that are used to split words.
    void setWordSeparators(const text::U32String &separators);
    /// Get the word break mark.
    /// The word break mark is used when a long word had to be split in a paragraph.
    [[nodiscard]] auto wordBreakMark() const noexcept -> const Block &;
    /// Set the word break mark.
    void setWordBreakMark(Block mark) noexcept;
    /// Get the maximum line wraps.
    /// A value >0 limits the automatic wraps for one source line.
    /// Once the limit is reached, the current source line is truncated and `paragraphEllipsisMark()` is appended if
    /// configured.
    /// Embedded line breaks start a new source line and therefore reset the wrap counter.
    /// @return The maximum number of line wraps, or zero for unlimited line wraps.
    [[nodiscard]] auto maximumLineWraps() const noexcept -> int;
    /// Set the maximum number of line wraps.
    /// @param lines The maximum number of line wraps, or zero for unlimited line wraps.
    void setMaximumLineWraps(int lines) noexcept;
    /// Get the paragraph ellipsis mark.
    /// This string is used to indicate that a paragraph would have to be wrapped over even more lines to be
    /// displayed completely.
    /// A single character can be used, but a short text like `(more…)` works as well.
    /// Please note that the width of this mark further reduces the available space for the paragraph text.
    /// @return The paragraph ellipsis mark or an empty string if no ellipsis mark shall be used.
    [[nodiscard]] auto paragraphEllipsisMark() const noexcept -> const BlockString &;
    /// Set the paragraph ellipsis mark.
    /// We recommend using a single character or a very short text. Longer texts quickly make paragraph
    /// rendering impossible.
    /// @param mark The paragraph ellipsis mark. If empty, no ellipsis mark will be used.
    void setParagraphEllipsisMark(BlockString mark) noexcept;
    /// Get the tab stops for the paragraph.
    /// Only valid if the alignment is set to `geometry::Alignment::Left`.
    /// If a line (text up to a newline character) contains TAB characters, each tab character will pick the
    /// next tab-stop columns value from this array. If the column is larger than the current column,
    /// spacing is inserted until the cursor reaches the tab-stop column.
    /// If the tab column is smaller, or there is no further tab stop in the sequence, `tabOverflowBehavior()` is
    /// used to decide whether the tab becomes a single space or starts a wrapped continuation line.
    /// In centered or right-aligned paragraphs, tabs use `wordSeparators` instead of these tab stops.
    /// The special value `cTabWrappedLineIndent` can be used to use the same indent as for wrapped lines.
    [[nodiscard]] auto tabStops() const noexcept -> const std::vector<int> &;
    /// Set the tab stops.
    void setTabStops(std::vector<int> tabStops) noexcept;
    /// Get the overflow handling for non-advancing tab stops.
    /// This mode is used if a TAB resolves to a tab-stop column that is not larger than the current column, or if
    /// there is no further configured tab stop.
    [[nodiscard]] auto tabOverflowBehavior() const noexcept -> TabOverflowBehavior;
    /// Set the overflow handling for non-advancing tab stops.
    /// @param behavior The behavior to use for tabs that do not advance the current line.
    void setTabOverflowBehavior(TabOverflowBehavior behavior) noexcept;
    /// Get the error resolution if a paragraph cannot be rendered with the given settings.
    /// If the screen layout and the given parameters do not allow the paragraph to be rendered properly,
    /// this error resolution is used.
    /// @return The error resolution to use when a paragraph cannot be rendered.
    [[nodiscard]] auto onError() const noexcept -> ParagraphOnError;
    /// Set the error resolution.
    /// @param onError The error resolution to use when a paragraph cannot be rendered.
    void setOnError(ParagraphOnError onError) noexcept;

public:
    /// Globally shared default options.
    /// @return A reusable default instance with the library defaults for all paragraph settings.
    [[nodiscard]] static auto defaultOptions() noexcept -> const ParagraphOptions &;

private:
    geometry::Alignment _alignment{geometry::Alignment::TopLeft}; ///< The text alignment.
    ParagraphIndents _indents;                                    ///< Indent and margin settings for the paragraph.
    ParagraphBackgroundMode _backgroundMode{ParagraphBackgroundMode::Default}; ///< The background mode.
    BlockString _lineBreakEndMark{};   ///< BlockString to mark wrapped lines on the right side. Max 2 characters.
    BlockString _lineBreakStartMark{}; ///< BlockString to mark wrapped lines on the left side.
    ParagraphSpacing _paragraphSpacing{ParagraphSpacing::SingleLine};     ///< Spacing between paragraphs.
    text::CharSet _wordSeparators{{text::Char{U'\t'}, text::Char{U' '}}}; ///< Shared word separators.
    Block _wordBreakMark{text::Char{U'-'}}; ///< The mark added if a long word needs to be broken.
    int _maximumLineWraps{0};               ///< The maximum number of lines in a paragraph.
    BlockString _paragraphEllipsisMark{text::U32String{text::U32StringLiteral{U"…"}}}; ///< The ellipsis mark.
    std::vector<int> _tabStops{cTabWrappedLineIndent};                                 ///< Tab stops for TAB character.
    TabOverflowBehavior _tabOverflowBehavior{TabOverflowBehavior::AddSpace}; ///< Fallback for non-advancing tabs.
    ParagraphOnError _onError{ParagraphOnError::PlainOutput};                ///< Error handling mode for paragraphs.
};

}
