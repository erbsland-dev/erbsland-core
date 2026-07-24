// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Block.hpp"
#include "BlockString.hpp"
#include "FrameBorder.hpp"
#include "Key.hpp"
#include "ReadLineDisplayStyle.hpp"

#include "../bgeo/BlockMargins.hpp"
#include "../text/String.hpp"
#include "../text/StringList.hpp"
#include "../time/TimeAmounts.hpp"
#include "../unit/CpLength.hpp"
#include "../unit/LineCount.hpp"

#include <optional>

namespace erbsland::cterm {

/// Options for an interactive read-line operation.
/// Text limits are applied to private copies when a `ReadLine` is constructed, making setter order irrelevant.
/// @tested{ReadLineOptionsTest}
class ReadLineOptions final {
public:
    /// Create options with the standard interactive read-line appearance.
    ReadLineOptions();

    // defaults
    ~ReadLineOptions() = default;
    ReadLineOptions(const ReadLineOptions &) = default;
    ReadLineOptions(ReadLineOptions &&) noexcept = default;
    auto operator=(const ReadLineOptions &) -> ReadLineOptions & = default;
    auto operator=(ReadLineOptions &&) noexcept -> ReadLineOptions & = default;

public: // styles
    /// Get the input-area background style.
    [[nodiscard]] auto backgroundStyle() const noexcept -> const BlockStyle & { return _backgroundStyle; }
    /// Set the input-area background style.
    auto setBackgroundStyle(BlockStyle style) noexcept -> ReadLineOptions &;
    /// Get the title style.
    [[nodiscard]] auto titleStyle() const noexcept -> const BlockStyle & { return _titleStyle; }
    /// Set the title style.
    auto setTitleStyle(BlockStyle style) noexcept -> ReadLineOptions &;
    /// Get the prompt style.
    [[nodiscard]] auto promptStyle() const noexcept -> const BlockStyle & { return _promptStyle; }
    /// Set the prompt style.
    auto setPromptStyle(BlockStyle style) noexcept -> ReadLineOptions &;
    /// Get the placeholder style.
    [[nodiscard]] auto placeholderStyle() const noexcept -> const BlockStyle & { return _placeholderStyle; }
    /// Set the placeholder style.
    auto setPlaceholderStyle(BlockStyle style) noexcept -> ReadLineOptions &;
    /// Get the entered-text style.
    [[nodiscard]] auto textStyle() const noexcept -> const BlockStyle & { return _textStyle; }
    /// Set the entered-text style.
    auto setTextStyle(BlockStyle style) noexcept -> ReadLineOptions &;
    /// Get the cursor style used over visible text.
    [[nodiscard]] auto cursorStyle() const noexcept -> const BlockStyle & { return _cursorStyle; }
    /// Set the cursor style used over visible text.
    auto setCursorStyle(BlockStyle style) noexcept -> ReadLineOptions &;

public: // layout
    /// Get the display layout.
    [[nodiscard]] auto displayStyle() const noexcept -> ReadLineDisplayStyle { return _displayStyle; }
    /// Set the display layout.
    auto setDisplayStyle(ReadLineDisplayStyle style) noexcept -> ReadLineOptions &;
    /// Get the frame border.
    [[nodiscard]] auto frameBorder() const noexcept -> const FrameBorder & { return _frameBorder; }
    /// Set the frame border.
    auto setFrameBorder(FrameBorder border) noexcept -> ReadLineOptions &;
    /// Get the horizontal input padding.
    /// Vertical values are always zero.
    [[nodiscard]] auto padding() const noexcept -> const bgeo::BlockMargins & { return _padding; }
    /// Set the input padding.
    /// Negative horizontal values are clamped to zero and vertical values are discarded.
    auto setPadding(bgeo::BlockMargins padding) noexcept -> ReadLineOptions &;

public: // displayed text
    /// Get the title.
    [[nodiscard]] auto title() const noexcept -> const BlockString & { return _title; }
    /// Set the title.
    auto setTitle(BlockString title) noexcept -> ReadLineOptions &;
    /// Set the title from plain text.
    auto setTitle(const text::String &title) -> ReadLineOptions &;
    /// Get the prompt.
    [[nodiscard]] auto prompt() const noexcept -> const BlockString & { return _prompt; }
    /// Set the prompt.
    auto setPrompt(BlockString prompt) noexcept -> ReadLineOptions &;
    /// Set the prompt from plain text.
    auto setPrompt(const text::String &prompt) -> ReadLineOptions &;
    /// Get the placeholder.
    [[nodiscard]] auto placeholder() const noexcept -> const BlockString & { return _placeholder; }
    /// Set the placeholder.
    auto setPlaceholder(BlockString placeholder) noexcept -> ReadLineOptions &;
    /// Set the placeholder from plain text.
    auto setPlaceholder(const text::String &placeholder) -> ReadLineOptions &;

public: // input
    /// Get the maximum entered-text length in code points.
    [[nodiscard]] auto maximumLength() const noexcept -> unit::CpLength { return _maximumLength; }
    /// Set the maximum entered-text length in code points.
    auto setMaximumLength(unit::CpLength maximumLength) noexcept -> ReadLineOptions &;
    /// Get the maximum logical line count.
    [[nodiscard]] auto maximumLines() const noexcept -> unit::LineCount { return _maximumLines; }
    /// Set the maximum logical line count.
    /// @throws err::ParameterError if the count is zero.
    auto setMaximumLines(unit::LineCount maximumLines) -> ReadLineOptions &;
    /// Get the maximum displayed edit-row count.
    [[nodiscard]] auto maximumDisplayLines() const noexcept -> unit::LineCount { return _maximumDisplayLines; }
    /// Set the maximum displayed edit-row count.
    /// @throws err::ParameterError if the count is zero.
    auto setMaximumDisplayLines(unit::LineCount maximumDisplayLines) -> ReadLineOptions &;
    /// Get the history entries.
    [[nodiscard]] auto history() const noexcept -> const text::StringList & { return _history; }
    /// Replace the history entries.
    auto setHistory(text::StringList history) noexcept -> ReadLineOptions &;
    /// Get the initial text.
    [[nodiscard]] auto currentText() const noexcept -> const text::String & { return _currentText; }
    /// Set the initial text.
    auto setCurrentText(text::String currentText) noexcept -> ReadLineOptions &;

public: // timing
    /// Get the inactivity timeout.
    /// Zero disables the timeout.
    [[nodiscard]] auto timeout() const noexcept -> time::Seconds { return _timeout; }
    /// Set the inactivity timeout.
    /// @throws err::ParameterError if the timeout is negative.
    auto setTimeout(time::Seconds timeout) -> ReadLineOptions &;
    /// Get the remaining-time threshold for displaying the timeout countdown.
    /// Zero disables the countdown display.
    [[nodiscard]] auto timeoutDisplayThreshold() const noexcept -> time::Seconds { return _timeoutDisplayThreshold; }
    /// Set the remaining-time threshold for displaying the timeout countdown.
    /// @param timeoutDisplayThreshold The non-negative display threshold.
    /// @return This options object.
    /// @throws err::ParameterError if the threshold is negative.
    auto setTimeoutDisplayThreshold(time::Seconds timeoutDisplayThreshold) -> ReadLineOptions &;
    /// Get the cursor blink interval.
    [[nodiscard]] auto blinkInterval() const noexcept -> time::Milliseconds { return _blinkInterval; }
    /// Set the cursor blink interval.
    /// @throws err::ParameterError if the interval is not positive.
    auto setBlinkInterval(time::Milliseconds blinkInterval) -> ReadLineOptions &;

public: // cursor and keys
    /// Get the cursor block used over empty space.
    [[nodiscard]] auto cursorBlock() const noexcept -> const Block & { return _cursorBlock; }
    /// Set the cursor block used over empty space.
    /// @throws err::ParameterError if the block does not occupy exactly one terminal cell.
    auto setCursorBlock(Block cursorBlock) -> ReadLineOptions &;
    /// Get the key that commits entered text.
    [[nodiscard]] auto commitKey() const noexcept -> const Key & { return _commitKey; }
    /// Set the key that commits entered text.
    auto setCommitKey(Key commitKey) noexcept -> ReadLineOptions &;
    /// Get the key that inserts a logical line break.
    [[nodiscard]] auto newLineKey() const noexcept -> const Key & { return _newLineKey; }
    /// Set the key that inserts a logical line break.
    auto setNewLineKey(Key newLineKey) noexcept -> ReadLineOptions &;
    /// Get the key that cancels input.
    [[nodiscard]] auto cancelKey() const noexcept -> const Key & { return _cancelKey; }
    /// Set the key that cancels input.
    auto setCancelKey(Key cancelKey) noexcept -> ReadLineOptions &;

public: // cleanup
    /// Test if the input area is removed when the operation stops.
    [[nodiscard]] auto cleanupEnabled() const noexcept -> bool { return _cleanupEnabled; }
    /// Enable or disable removal of the input area when the operation stops.
    auto setCleanupEnabled(bool cleanupEnabled) noexcept -> ReadLineOptions &;

private:
    BlockStyle _backgroundStyle;            ///< Style for the complete input area.
    BlockStyle _titleStyle;                 ///< Style for the title.
    BlockStyle _promptStyle;                ///< Style for the prompt.
    BlockStyle _placeholderStyle;           ///< Style for placeholder text.
    BlockStyle _textStyle;                  ///< Style for entered text.
    BlockStyle _cursorStyle;                ///< Style overlay for the cursor.
    ReadLineDisplayStyle _displayStyle;     ///< Layout of the input area.
    FrameBorder _frameBorder;               ///< Border around the input area.
    bgeo::BlockMargins _padding;            ///< Horizontal input padding.
    BlockString _title;                     ///< Optional title.
    BlockString _prompt;                    ///< Optional prompt.
    BlockString _placeholder;               ///< Optional placeholder.
    time::Seconds _timeout;                 ///< Inactivity timeout.
    time::Seconds _timeoutDisplayThreshold; ///< Remaining-time threshold for the timeout display.
    unit::CpLength _maximumLength;          ///< Maximum code-point count.
    unit::LineCount _maximumLines;          ///< Maximum logical line count.
    unit::LineCount _maximumDisplayLines;   ///< Maximum visible edit rows.
    time::Milliseconds _blinkInterval;      ///< Cursor blink interval.
    text::StringList _history;              ///< Initial history entries.
    text::String _currentText;              ///< Initial edit text.
    Block _cursorBlock;                     ///< Cursor over empty space.
    Key _commitKey;                         ///< Commit binding.
    Key _newLineKey;                        ///< New-line binding.
    Key _cancelKey;                         ///< Cancel binding.
    bool _cleanupEnabled;                   ///< If the rendered area is removed.
};

}
