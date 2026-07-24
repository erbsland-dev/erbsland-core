// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

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

private:
    struct Cell;
    struct Boundary;
    struct LayoutRow;
    struct Layout;

protected:
    ReadLineBase(TerminalPtr terminal, ReadLineOptions options);
    ReadLineBase(TerminalPtr terminal, ReadLineOptions options, NowFn nowFn);
    virtual ~ReadLineBase() = default;

    ReadLineBase(const ReadLineBase &) = delete;
    ReadLineBase(ReadLineBase &&) = delete;
    auto operator=(const ReadLineBase &) -> ReadLineBase & = delete;
    auto operator=(ReadLineBase &&) -> ReadLineBase & = delete;

protected: // frontend support
    void startBase();
    [[nodiscard]] auto updateBase() -> ReadLineStatus;
    [[nodiscard]] auto waitForInputBase() -> ReadLineStatus;
    void stopBase() noexcept;
    [[nodiscard]] auto isActiveBase() const noexcept -> bool { return _active; }
    [[nodiscard]] auto terminal() const noexcept -> const TerminalPtr & { return _terminal; }
    [[nodiscard]] auto options() const noexcept -> const ReadLineOptions & { return _options; }
    [[nodiscard]] auto cursorIndex() const noexcept -> unit::CpIndex { return _cursorIndex; }
    void setCursorIndex(unit::CpIndex index) noexcept { _cursorIndex = index; }
    void resetPreferredColumn() noexcept;

private: // concrete storage interface
    virtual void resetText() = 0;
    virtual void discardText() noexcept = 0;
    virtual void commitText() = 0;
    [[nodiscard]] virtual auto displayText() const noexcept -> const text::U32StringEditor & = 0;
    [[nodiscard]] virtual auto insertKeyText(const Key &key, unit::CpIndex index) -> unit::CpLength = 0;
    [[nodiscard]] virtual auto insertNewLine(unit::CpIndex index) -> bool = 0;
    virtual void eraseText(unit::CpIndex index, unit::CpLength length) noexcept = 0;
    [[nodiscard]] virtual auto previousUnitStart(unit::CpIndex index) const noexcept -> unit::CpIndex = 0;
    [[nodiscard]] virtual auto nextUnitEnd(unit::CpIndex index) const noexcept -> unit::CpIndex = 0;
    [[nodiscard]] virtual auto selectPreviousHistory() -> bool { return false; }
    [[nodiscard]] virtual auto selectNextHistory() -> bool { return false; }

private: // setup and processing
    void validateOptions() const;
    void resetOperation();
    [[nodiscard]] auto process(time::Milliseconds wait) -> ReadLineStatus;
    [[nodiscard]] auto currentStatus() const noexcept -> ReadLineStatus;
    [[nodiscard]] auto handleKey(const Key &key) -> bool;
    [[nodiscard]] auto handleNavigationKey(const Key &key) -> bool;
    void commit();
    void cancel();
    void timeOut();
    void resetActivity(time::TimePoint now) noexcept;
    void resetBlink(time::TimePoint now) noexcept;
    [[nodiscard]] auto timeoutExpired(time::TimePoint now) const noexcept -> bool;
    [[nodiscard]] auto waitDuration(time::TimePoint now) const noexcept -> time::Milliseconds;
    [[nodiscard]] auto countdownSeconds(time::TimePoint now) const noexcept -> std::int64_t;
    [[nodiscard]] auto countdownDisplayed(std::int64_t countdown) const noexcept -> bool;

private: // editing
    [[nodiscard]] auto moveLeft() noexcept -> bool;
    [[nodiscard]] auto moveRight() noexcept -> bool;
    [[nodiscard]] auto moveHome(const Layout &layout) noexcept -> bool;
    [[nodiscard]] auto moveEnd(const Layout &layout) noexcept -> bool;
    [[nodiscard]] auto moveUp(const Layout &layout) -> bool;
    [[nodiscard]] auto moveDown(const Layout &layout) -> bool;
    [[nodiscard]] auto eraseBeforeCursor() -> bool;
    [[nodiscard]] auto eraseAtCursor() -> bool;

private: // layout and rendering
    [[nodiscard]] auto createLayout() const -> Layout;
    [[nodiscard]] auto closestBoundary(const LayoutRow &row, int column) const noexcept -> unit::CpIndex;
    [[nodiscard]] auto cursorRow(const Layout &layout) const noexcept -> std::size_t;
    [[nodiscard]] auto cursorColumn(const Layout &layout) const noexcept -> int;
    void render(bool showCursor);
    void renderFinal() noexcept;
    void clearRenderedArea() noexcept;
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
