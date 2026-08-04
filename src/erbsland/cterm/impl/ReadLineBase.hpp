// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ReadLineLayout_fwd.hpp"

#include "../Block_fwd.hpp"
#include "../BlockString_fwd.hpp"
#include "../BlockStyle_fwd.hpp"
#include "../Buffer_fwd.hpp"
#include "../FrameBorder_fwd.hpp"
#include "../FrameBorderElement.hpp"
#include "../Input.hpp"
#include "../ReadableBuffer.hpp"
#include "../ReadLineOptions.hpp"
#include "../ReadLineStatus.hpp"
#include "../Terminal_fwd.hpp"

#include "../../text/u32/U32StringEditor.hpp"
#include "../../time/TimePoint.hpp"
#include "../../unit/CpIndex.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace erbsland::cterm::impl {

/// Shared terminal lifecycle, input dispatch, layout, and rendering for line editors.
/// Concrete subclasses exclusively own and mutate their ordinary or sensitive text storage.
/// @tested{ReadLineTest ReadSecretTest}
class ReadLineBase {
protected:
    using NowFn = std::function<time::TimePoint()>;

protected:
    /// Construct a line-editor base with the system monotonic clock.
    ReadLineBase(TerminalPtr terminal, ReadLineOptions options);
    /// Construct a line-editor base with an injected monotonic clock.
    ReadLineBase(TerminalPtr terminal, ReadLineOptions options, NowFn nowFn);

public:
    // defaults/deletions
    virtual ~ReadLineBase() = default;
    ReadLineBase(const ReadLineBase &) = delete;
    ReadLineBase(ReadLineBase &&) = delete;
    auto operator=(const ReadLineBase &) -> ReadLineBase & = delete;
    auto operator=(ReadLineBase &&) -> ReadLineBase & = delete;

protected: // frontend support
    /// Start an interactive line-editor operation.
    void startBase();
    /// Process available input and return the current operation status.
    [[nodiscard]] auto updateBase() -> ReadLineStatus;
    /// Run an interactive operation until it completes.
    [[nodiscard]] auto waitForInputBase() -> ReadLineStatus;
    /// End the active operation and restore the terminal state.
    void stopBase() noexcept;
    /// Test if a line-editor operation is active.
    [[nodiscard]] auto isActiveBase() const noexcept -> bool { return _active; }
    /// Get the retained terminal.
    [[nodiscard]] auto terminal() const noexcept -> const TerminalPtr & { return _terminal; }
    /// Get the normalized editor options.
    [[nodiscard]] auto options() const noexcept -> const ReadLineOptions & { return _options; }
    /// Get the current code-point insertion index.
    [[nodiscard]] auto cursorIndex() const noexcept -> unit::CpIndex { return _cursorIndex; }
    /// Set the current code-point insertion index.
    void setCursorIndex(unit::CpIndex index) noexcept { _cursorIndex = index; }
    /// Clear the column preserved for vertical cursor movement.
    void resetPreferredColumn() noexcept;

private: // concrete storage interface
    /// Reset the concrete text storage for a new operation.
    virtual void resetText() = 0;
    /// Discard text stored by the active operation.
    virtual void discardText() noexcept = 0;
    /// Commit text stored by the active operation.
    virtual void commitText() = 0;
    /// Get the text displayed by the editor.
    [[nodiscard]] virtual auto displayText() const noexcept -> const text::U32StringEditor & = 0;
    /// Insert a key's text at the specified index and return its length.
    [[nodiscard]] virtual auto insertKeyText(const Key &key, unit::CpIndex index) -> unit::CpLength = 0;
    /// Insert a new line at the specified index.
    [[nodiscard]] virtual auto insertNewLine(unit::CpIndex index) -> bool = 0;
    /// Erase text at the specified index and length.
    virtual void eraseText(unit::CpIndex index, unit::CpLength length) noexcept = 0;
    /// Find the preceding editing-unit boundary.
    [[nodiscard]] virtual auto previousUnitStart(unit::CpIndex index) const noexcept -> unit::CpIndex = 0;
    /// Find the following editing-unit boundary.
    [[nodiscard]] virtual auto nextUnitEnd(unit::CpIndex index) const noexcept -> unit::CpIndex = 0;
    /// Select the preceding history entry when available.
    [[nodiscard]] virtual auto selectPreviousHistory() -> bool { return false; }
    /// Select the following history entry when available.
    [[nodiscard]] virtual auto selectNextHistory() -> bool { return false; }

private: // setup and processing
    /// Validate the normalized line-editor options.
    void validateOptions() const;
    /// Reset state for a newly started operation.
    void resetOperation();
    /// Process terminal input after waiting for at most the specified duration.
    [[nodiscard]] auto process(time::Milliseconds wait) -> ReadLineStatus;
    /// Return the latched terminal status or idle status.
    [[nodiscard]] auto currentStatus() const noexcept -> ReadLineStatus;
    /// Handle one input key and report whether it was recognized.
    [[nodiscard]] auto handleKey(const Key &key) -> bool;
    /// Handle a cursor-navigation key.
    [[nodiscard]] auto handleNavigationKey(const Key &key) -> bool;
    /// Commit the current editor text.
    void commit();
    /// Cancel the current editor operation.
    void cancel();
    /// Finish the current editor operation because it timed out.
    void timeOut();
    /// Update the activity and timeout countdown state.
    void resetActivity(time::TimePoint now) noexcept;
    /// Make the cursor visible and restart its blink interval.
    void resetBlink(time::TimePoint now) noexcept;
    /// Test whether the configured inactivity timeout has elapsed.
    [[nodiscard]] auto timeoutExpired(time::TimePoint now) const noexcept -> bool;
    /// Calculate the maximum wait time before the next editor update.
    [[nodiscard]] auto waitDuration(time::TimePoint now) const noexcept -> time::Milliseconds;
    /// Calculate the remaining timeout in rounded-up seconds.
    [[nodiscard]] auto countdownSeconds(time::TimePoint now) const noexcept -> std::int64_t;
    /// Test whether a timeout value should be shown to the user.
    [[nodiscard]] auto countdownDisplayed(std::int64_t countdown) const noexcept -> bool;

private: // editing
    /// Move the cursor to the preceding editing-unit boundary.
    auto moveLeft() noexcept -> bool;
    /// Move the cursor to the following editing-unit boundary.
    auto moveRight() noexcept -> bool;
    /// Move the cursor to the start of its visual row.
    auto moveHome(const ReadLineLayout &layout) noexcept -> bool;
    /// Move the cursor to the end of its visual row.
    auto moveEnd(const ReadLineLayout &layout) noexcept -> bool;
    /// Move the cursor to the preceding visual row.
    auto moveUp(const ReadLineLayout &layout) -> bool;
    /// Move the cursor to the following visual row.
    auto moveDown(const ReadLineLayout &layout) -> bool;
    /// Erase the editing unit preceding the cursor.
    auto eraseBeforeCursor() -> bool;
    /// Erase the editing unit at the cursor.
    auto eraseAtCursor() -> bool;

private: // layout and rendering
    /// Create a horizontal frame line block.
    [[nodiscard]] static auto horizontalBlock(const FrameBorder &border, FrameBorderElement element) noexcept -> Block;
    /// Create a vertical frame line block.
    [[nodiscard]] static auto verticalBlock(const FrameBorder &border, FrameBorderElement element) noexcept -> Block;
    /// Set a block when its position is inside the buffer.
    static void setClipped(Buffer &buffer, int x, int y, const Block &block) noexcept;
    /// Draw styled blocks within the specified maximum width.
    static void drawBlockString(
        Buffer &buffer, int x, int y, int maximumWidth, const BlockString &text, BlockStyle style);
    /// Create the title text, including a visible timeout countdown.
    [[nodiscard]] static auto titleWithCountdown(const ReadLineOptions &options, std::int64_t countdown) -> BlockString;
    /// Build the visual layout for the displayed text.
    [[nodiscard]] auto createLayout() const -> ReadLineLayout;
    /// Find the closest editing boundary to a visual row column.
    [[nodiscard]] static auto closestBoundary(const ReadLineLayoutRow &row, int column) noexcept -> unit::CpIndex;
    /// Find the visual row containing the cursor.
    [[nodiscard]] auto cursorRow(const ReadLineLayout &layout) const noexcept -> std::size_t;
    /// Find the visual column containing the cursor.
    [[nodiscard]] auto cursorColumn(const ReadLineLayout &layout) const noexcept -> int;
    /// Render the editor with the requested cursor visibility.
    void render(bool showCursor);
    /// Render the completed editor state without an active cursor.
    void renderFinal() noexcept;
    /// Clear the terminal area occupied by the editor.
    void clearRenderedArea() noexcept;
    /// Write the rendered buffer to the terminal.
    void writeRenderedBuffer(const ReadableBuffer &buffer, int actualHeight);

private:
    TerminalPtr _terminal;                                 ///< Retained terminal.
    ReadLineOptions _options;                              ///< Normalized operation options.
    NowFn _nowFn;                                          ///< Monotonic clock source.
    unit::CpIndex _cursorIndex;                            ///< Code-point insertion index.
    std::optional<int> _preferredColumn;                   ///< Column preserved by vertical movement.
    Input::Mode _previousInputMode{Input::Mode::ReadLine}; ///< Input mode restored by `stopBase()`.
    time::TimePoint _lastActivity;                         ///< Last recognized user activity.
    time::TimePoint _lastBlink;                            ///< Last cursor blink transition.
    std::int64_t _lastCountdown{-1};                       ///< Last rendered timeout value.
    bool _cursorVisible{true};                             ///< Rendered cursor blink phase.
    bool _active{false};                                   ///< If the terminal is currently owned.
    std::optional<ReadLineStatus> _terminalStatus;         ///< Latched terminal status.
    int _renderedHeight{0};                                ///< Height of the currently occupied area.
    int _renderedWidth{0};                                 ///< Width used for the last rendered area.
};

}
