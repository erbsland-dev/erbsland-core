// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CursorWriter.hpp"
#include "RemappedBuffer.hpp"

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
class CursorBuffer : public RemappedBuffer, public CursorWriter {
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
    /// @param startSize The start size of the buffer.
    /// @param overflowMode The overflow mode of the buffer.
    /// @param maximumSize The maximum size of the buffer. Only `height` is used.
    /// @param fillChar The character used to initialize and refill empty cells.
    /// @throws err::ParameterError if startSize exceeds the maximum or fillChar is not a single-width character.
    explicit CursorBuffer(
        const bgeo::BlockSize startSize,
        const OverflowMode overflowMode = OverflowMode::Shift,
        const bgeo::BlockSize maximumSize = cMaximumSize,
        const Block fillChar = Block::space()) :
        RemappedBuffer{startSize, bgeo::Orientation::Vertical, fillChar},
        _overflowMode(overflowMode),
        _maximumSize(maximumSize),
        _fillChar(fillChar) {
        validateFillChar(_fillChar);
    }
    /// Create a new cursor buffer with a default size of 80x25 and overflow mode `Shift`.
    CursorBuffer() : CursorBuffer{bgeo::BlockSize{80, 25}, OverflowMode::Shift} {}

    // default
    ~CursorBuffer() override = default;
    CursorBuffer(const CursorBuffer &) = default;
    CursorBuffer(CursorBuffer &&) = default;
    auto operator=(const CursorBuffer &) -> CursorBuffer & = default;
    auto operator=(CursorBuffer &&) -> CursorBuffer & = default;

public: // setup
    /// Get the maximum size.
    [[nodiscard]] auto maximumSize() const noexcept -> bgeo::BlockSize;
    /// Change the maximum size.
    /// Changing the maximum size will not affect the current content.
    /// @param maximumSize The new maximum size.
    void setMaximumSize(bgeo::BlockSize maximumSize) noexcept;
    /// Get the overflow mode for this buffer.
    [[nodiscard]] auto overflowMode() const noexcept -> OverflowMode;
    /// Set the overflow mode for this buffer.
    /// @param mode The new overflow mode.
    void setOverflowMode(OverflowMode mode) noexcept;
    /// Get the character used for newly exposed or empty cells.
    /// @return The current fill character.
    [[nodiscard]] auto fillChar() const noexcept -> const Block &;
    /// Set the character used for newly exposed or empty cells.
    /// The character should have a display width of one cell.
    /// @param fillChar The new fill character.
    /// @throws err::ParameterError if fillChar is not a single-width character.
    void setFillChar(Block fillChar);

public: // implement CursorWriter
    using CursorWriter::setColor;
    using CursorWriter::write;
    using CursorWriter::writeLineBreak;
    [[nodiscard]] auto size() const noexcept -> bgeo::BlockSize override { return _size; }
    [[nodiscard]] auto color() const noexcept -> Color override;
    [[nodiscard]] auto blockAttributes() const noexcept -> BlockAttributes override;
    void setColor(Color color) noexcept override;
    void setBlockAttributes(BlockAttributes attributes) noexcept override;
    void setForeground(Foreground color) noexcept override;
    void setBackground(Background color) noexcept override;
    [[nodiscard]] auto supportedBlockAttributes() const noexcept -> BlockAttributes override;
    void moveCursor(bgeo::BlockPosition posOrDelta, MoveMode mode) noexcept override;
    void setAutoWrap(bool enabled) noexcept override;
    void clearScreen() noexcept override;
    void write(const Block &character) noexcept override;
    void write(const BlockString &str) noexcept override;
    void writeResolved(const Block &character) noexcept override;
    void writeResolved(const BlockString &str) noexcept override;
    void write(const ReadableBuffer &buffer) noexcept override;
    void writeLineBreak() noexcept override;

protected:
    /// Print a paragraph using the cursor-aware buffer output.
    auto printParagraphImpl(const BlockString &paragraph, const ParagraphOptions &options) noexcept -> int override;

private:
    static void validateFillChar(const Block &fillChar);
    void writeResolvedBlock(const Block &character) noexcept;

private:
    OverflowMode _overflowMode{OverflowMode::Shift};     ///< The overflow mode of the buffer.
    bgeo::BlockSize _maximumSize{bgeo::BlockSize{0, 0}}; ///< The maximum size of the buffer.
    Block _fillChar{Block::space()};                     ///< The character used for empty cells.
    bgeo::BlockPosition _cursorPosition{0, 0};           ///< The current position of the cursor.
    BlockStyle _currentStyle{BlockStyle::reset()};       ///< The current character style.
    bool _autoWrap{true};                                ///< If auto-wrap is enabled.
    bool _wrapOnNextChar{false};                         ///< If the next character should be wrapped to the next line.
};

}
