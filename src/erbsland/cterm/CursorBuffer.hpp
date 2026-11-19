// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CursorWriter.hpp"
#include "RemappedBuffer.hpp"

#include "impl/TerminalDefaults.hpp"

#include <functional>
#include <utility>

namespace erbsland::cterm {

/// A buffer that can be used to write text using a cursor.
/// Cursor movement emulates VT100 terminals:
/// - Cursor movements are bound to the buffer (they do not wrap around).
/// - Printing characters do wrap around if wrap mode is enabled (it's enabled by default.)
/// - Printing a character in the last column is special:
///   - A 1-width character is printed in the last column, but the cursor isn't moved yet. A `wrapOnNextChar` is
///     set instead of the cursor movement.
///   - As soon as the next character is printed, the cursor is moved to the next line first; then the character is
///     printed.
///   - When a 2-width character is printed in the last column, the cursor is moved to the next line first; then the
///     character is printed (not VT100, it didn't have 2-width characters).
///   - If the `wrapOnNextChar` is set, and the cursor is moved, the flag is cleared.
///   - If the `wrapOnNextChar` is set, and a line-break is printed, the flag is cleared first before moving the
///     cursor to the next line.
/// @tested{CursorBufferTest}
class CursorBuffer : public RemappedBuffer, public CursorWriter {
public:
    /// Receive a borrowed, complete row before it scrolls out of the top.
    /// The row is valid only during the call. Callbacks must not throw or modify this buffer.
    using ScrollOutFn = std::function<void(const ReadableBuffer &)>;

public:
    /// The overflow mode determines what happens when the cursor gets a line-break in the last line.
    enum class OverflowMode : uint8_t {
        Shift,           ///< Shift the buffer content one line up, exposing a new blank line at the bottom.
        Wrap,            ///< The cursor wraps back to the first line of the buffer.
        ExpandThenShift, ///< Expand the buffer by one line, keeping the content. Up to maximumSize(), then scroll.
        ExpandThenWrap,  ///< Expand the buffer by one line, keeping the content. Up to maximumSize(), then wrap.
    };

public:
    /// Create a new cursor buffer with the given startSize.
    /// @param startSize The start size of the buffer. Must not exceed the maximum height.
    /// @param overflowMode The overflow mode of the buffer.
    /// @param maximumSize The maximum size of the buffer. Only `height` is used.
    /// @param fillChar The character used to initialize and refill empty cells. Must have display width one.
    explicit CursorBuffer(
        const block::Size startSize,
        const OverflowMode overflowMode = OverflowMode::Shift,
        const block::Size maximumSize = cMaximumSize,
        const Block fillChar = Block::space()) :
        RemappedBuffer{startSize, geometry::Orientation::Vertical, fillChar},
        _overflowMode(overflowMode),
        _maximumSize(maximumSize),
        _fillChar(fillChar) {
        validateFillChar(_fillChar);
    }
    /// Create a new cursor buffer with a default size of 120x25 and overflow mode `Shift`.
    CursorBuffer() : CursorBuffer{impl::cDefaultTerminalSize, OverflowMode::Shift} {}

    /// Copy content and cursor state without copying the source observer.
    /// @param other Source buffer.
    CursorBuffer(const CursorBuffer &other);
    /// Move content and cursor state without transferring the source observer.
    /// @param other Source buffer.
    CursorBuffer(CursorBuffer &&other) noexcept;

    // defaults
    ~CursorBuffer() override = default;

public:
    /// Copy content and cursor state, retaining this instance's observer.
    /// @param other Source buffer.
    /// @return This buffer.
    auto operator=(const CursorBuffer &other) -> CursorBuffer &;
    /// Move content and cursor state, retaining this instance's observer.
    /// @param other Source buffer.
    /// @return This buffer.
    auto operator=(CursorBuffer &&other) noexcept -> CursorBuffer &;

public: // cursor state
    /// Get the tracked cursor position without querying a device.
    [[nodiscard]] auto position() const noexcept -> block::Position { return _cursorPosition; }
    /// Test whether the next printable character wraps before writing.
    [[nodiscard]] auto wrapPending() const noexcept -> bool { return _wrapOnNextChar; }
    /// Restore a previously saved cursor position and delayed wrap state.
    /// @param position Position clamped to the current buffer bounds.
    /// @param wrapPending Whether printing should first perform a delayed wrap.
    void restorePosition(block::Position position, bool wrapPending) noexcept;

public: // scrolling notifications
    /// Observe each row leaving the top during a shift, in top-to-bottom order.
    /// Automatic overflow and explicit northward shifts are observed; other operations are not.
    /// Observers belong to their buffer instance and are not copied or moved with its contents.
    /// @param callback Borrowed-row callback; an empty callback disables notification.
    void setScrollOutCallback(ScrollOutFn callback) { _scrollOutCallback = std::move(callback); }

public: // setup
    /// Get the maximum size.
    [[nodiscard]] auto maximumSize() const noexcept -> block::Size;
    /// Change the maximum size.
    /// Changing the maximum size will not affect the current content.
    /// @param maximumSize The new maximum size.
    void setMaximumSize(block::Size maximumSize) noexcept;
    /// Get the overflow mode for this buffer.
    [[nodiscard]] auto overflowMode() const noexcept -> OverflowMode;
    /// Set the overflow mode for this buffer.
    /// @param mode The new overflow mode.
    void setOverflowMode(OverflowMode mode) noexcept;
    /// Get the character used for newly exposed or empty cells.
    /// @return The current fill character.
    [[nodiscard]] auto fillChar() const noexcept -> const Block &;
    /// Set the character used for newly exposed or empty cells.
    /// @param fillChar The new fill character. Must have display width one.
    void setFillChar(Block fillChar);

public: // implement CursorWriter
    using CursorWriter::setColor;
    using CursorWriter::write;
    using CursorWriter::writeLineBreak;
    [[nodiscard]] auto size() const noexcept -> block::Size override { return _size; }
    [[nodiscard]] auto color() const noexcept -> Color override;
    [[nodiscard]] auto blockAttributes() const noexcept -> BlockAttributes override;
    void setColor(Color color) noexcept override;
    void setBlockAttributes(BlockAttributes attributes) noexcept override;
    void setForeground(Foreground color) noexcept override;
    void setBackground(Background color) noexcept override;
    [[nodiscard]] auto supportedBlockAttributes() const noexcept -> BlockAttributes override;
    auto cursorPosition() noexcept -> std::optional<block::Position> override { return _cursorPosition; }
    void moveCursor(block::Position posOrDelta, MoveMode mode) noexcept override;
    void setAutoWrap(bool enabled) noexcept override;
    void clearScreen() noexcept override;
    void write(const Block &character) noexcept override;
    void write(const BlockString &str) noexcept override;
    void writeResolved(const Block &character) noexcept override;
    void writeResolved(const BlockString &str) noexcept override;
    void write(const ReadableBuffer &buffer) noexcept override;
    void writeLineBreak() noexcept override;

protected: // implement RemappedBuffer
    void onScroll(block::Direction direction, int count) noexcept override;

protected:
    /// Print a paragraph using the cursor-aware buffer output.
    auto printParagraphImpl(const BlockString &paragraph, const ParagraphOptions &options) noexcept -> int override;

private:
    /// Copy cursor policies and presentation after copying or moving the base buffer.
    void copyCursorState(const CursorBuffer &other) noexcept;
    /// Validate a block used for fill operations.
    static void validateFillChar(const Block &fillChar);
    /// Write a block after resolving cursor state.
    void writeResolvedBlock(const Block &character) noexcept;

private:
    ScrollOutFn _scrollOutCallback;                  ///< Observer of complete rows leaving the top.
    OverflowMode _overflowMode{OverflowMode::Shift}; ///< The overflow mode of the buffer.
    block::Size _maximumSize{block::Size{0, 0}};     ///< The maximum size of the buffer.
    Block _fillChar{Block::space()};                 ///< The character used for empty cells.
    block::Position _cursorPosition{0, 0};           ///< The current position of the cursor.
    BlockStyle _currentStyle{BlockStyle::reset()};   ///< The current character style.
    bool _autoWrap{true};                            ///< If auto-wrap is enabled.
    bool _wrapOnNextChar{false};                     ///< If the next character should be wrapped to the next line.
};

}
