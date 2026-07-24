// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ReadLineBase.hpp"

#include "../Terminal.hpp"

#include "../../err/ParameterError.hpp"
#include "../../err/RuntimeError.hpp"
#include "../../text/Literals.hpp"
#include "../../time/Literals.hpp"
#include "../../time/TimeDelta.hpp"
#include "../../unit/CpRange.hpp"

#include <algorithm>
#include <chrono>
#include <utility>

namespace erbsland::cterm::impl {

using namespace std::chrono_literals;
using namespace text::literals;
using namespace unit;
using namespace time::literals;
using time::TimeDelta;
using time::TimePoint;

ReadLineBase::ReadLineBase(TerminalPtr terminal, ReadLineOptions options) :
    ReadLineBase{std::move(terminal), std::move(options), []() noexcept -> TimePoint { return TimePoint::now(); }} {
}

ReadLineBase::ReadLineBase(TerminalPtr terminal, ReadLineOptions options, NowFn nowFn) :
    _terminal{std::move(terminal)}, _options{std::move(options)}, _nowFn{std::move(nowFn)} {
    if (_terminal == nullptr) {
        throw err::ParameterError{"The line-editor terminal must not be null.", "terminal"};
    }
    if (!_nowFn) {
        throw err::ParameterError{"The line-editor clock function must not be empty.", "nowFn"};
    }
    validateOptions();
}

void ReadLineBase::validateOptions() const {
    switch (_options.displayStyle()) {
    case ReadLineDisplayStyle::Compact:
    case ReadLineDisplayStyle::HorizontalSpace:
    case ReadLineDisplayStyle::HorizontalFrame:
    case ReadLineDisplayStyle::Frame:
        break;
    default:
        throw err::ParameterError{"The read-line display style is invalid.", "displayStyle"};
    }
    if (!_options.commitKey().valid() || !_options.newLineKey().valid() || !_options.cancelKey().valid()) {
        throw err::ParameterError{"Read-line key bindings must contain valid keys.", "keyBindings"};
    }
    if (_options.commitKey() == _options.newLineKey() || _options.commitKey() == _options.cancelKey() ||
        _options.newLineKey() == _options.cancelKey()) {
        throw err::ParameterError{"Read-line key bindings must be distinct.", "keyBindings"};
    }
    if (_options.maximumLines().isZero() || _options.maximumDisplayLines().isZero()) {
        throw err::ParameterError{"Read-line line limits must be at least one.", "lineLimits"};
    }
    if (_options.timeout().isNegative() || _options.timeoutDisplayThreshold().isNegative() ||
        !_options.blinkInterval().isPositive()) {
        throw err::ParameterError{"Read-line timing values are invalid.", "timing"};
    }
}

void ReadLineBase::resetOperation() {
    resetText();
    _cursorIndex = CpIndex::end(displayText().length());
    _preferredColumn.reset();
    _terminalStatus.reset();
    _cursorVisible = true;
    _renderedHeight = 0;
    _renderedWidth = 0;
    const auto now = _nowFn();
    _lastActivity = now;
    _lastBlink = now;
    _lastCountdown = countdownSeconds(now);
}

void ReadLineBase::startBase() {
    if (_active) {
        throw err::RuntimeError{"The line-editor operation is already active."};
    }
    if (!_terminal->isInteractive() || _terminal->outputMode() != Terminal::OutputMode::FullControl) {
        throw err::RuntimeError{"Line-editor input requires an interactive full-control terminal."};
    }
    resetOperation();
    _previousInputMode = _terminal->input().mode();
    _terminal->input().setMode(Input::Mode::Key);
    _active = true;
    try {
        render(true);
    } catch (...) {
        _active = false;
        _terminal->input().setMode(_previousInputMode);
        discardText();
        throw;
    }
}

auto ReadLineBase::updateBase() -> ReadLineStatus {
    if (!_active) {
        throw err::RuntimeError{"The line-editor operation is not active."};
    }
    return process(0_ms);
}

auto ReadLineBase::waitForInputBase() -> ReadLineStatus {
    startBase();
    try {
        while (true) {
            const auto result = process(waitDuration(_nowFn()));
            if (!result.isIdle()) {
                stopBase();
                return result;
            }
        }
    } catch (...) {
        stopBase();
        throw;
    }
}

void ReadLineBase::stopBase() noexcept {
    if (!_active) {
        return;
    }
    try {
        if (_options.cleanupEnabled()) {
            clearRenderedArea();
        } else {
            renderFinal();
        }
        _terminal->input().setMode(_previousInputMode);
        _terminal->flush();
    } catch (...) {
        try {
            _terminal->input().setMode(_previousInputMode);
        } catch (...) {}
    }
    _active = false;
    discardText();
    _renderedHeight = 0;
    _renderedWidth = 0;
}

auto ReadLineBase::process(const time::Milliseconds wait) -> ReadLineStatus {
    if (_terminalStatus.has_value()) {
        return currentStatus();
    }
    auto now = _nowFn();
    if (timeoutExpired(now)) {
        timeOut();
        return currentStatus();
    }

    const auto key = _terminal->input().readKey(wait);
    now = _nowFn();
    auto needsRender = false;
    if (key.valid()) {
        const auto previousCursorIndex = _cursorIndex;
        if (handleKey(key)) {
            resetActivity(now);
            if (_cursorIndex != previousCursorIndex) {
                resetBlink(now);
            }
            needsRender = !_terminalStatus.has_value();
        }
    }
    if (!_terminalStatus.has_value() && timeoutExpired(now)) {
        timeOut();
    }
    if (_terminalStatus.has_value()) {
        return currentStatus();
    }

    const auto blinkInterval = TimeDelta{_options.blinkInterval()};
    const auto blinkElapsed = _lastBlink.timeDeltaTo(now);
    if (blinkElapsed >= blinkInterval) {
        const auto elapsedIntervals = blinkElapsed / blinkInterval;
        if (elapsedIntervals % 2 != 0) {
            _cursorVisible = !_cursorVisible;
        }
        _lastBlink += blinkInterval * elapsedIntervals;
        needsRender = true;
    }
    const auto countdown = countdownSeconds(now);
    if (countdown != _lastCountdown) {
        const auto displayChanged = countdownDisplayed(countdown) || countdownDisplayed(_lastCountdown);
        _lastCountdown = countdown;
        needsRender = needsRender || displayChanged;
    }
    if (_renderedWidth != _terminal->size().width().toRawValue()) {
        needsRender = true;
    }
    if (needsRender) {
        render(_cursorVisible);
    }
    return currentStatus();
}

auto ReadLineBase::currentStatus() const noexcept -> ReadLineStatus {
    return _terminalStatus.value_or(ReadLineStatus::Idle);
}

auto ReadLineBase::handleKey(const Key &key) -> bool {
    if (key == _options.cancelKey()) {
        cancel();
        return true;
    }
    if (key == _options.commitKey()) {
        commit();
        return true;
    }
    if (key == _options.newLineKey()) {
        if (insertNewLine(_cursorIndex)) {
            ++_cursorIndex;
            resetPreferredColumn();
        }
        return true;
    }
    switch (key.type()) {
    case Key::Left:
    case Key::Right:
    case Key::Home:
    case Key::End:
    case Key::Up:
    case Key::Down:
        return handleNavigationKey(key);
    case Key::Backspace:
        static_cast<void>(eraseBeforeCursor());
        return true;
    case Key::Delete:
        static_cast<void>(eraseAtCursor());
        return true;
    default:
        break;
    }
    const auto inserted = insertKeyText(key, _cursorIndex);
    if (!inserted.isZero()) {
        _cursorIndex += inserted;
        resetPreferredColumn();
        return true;
    }
    return !key.modifiers().empty()
        ? false
        : key.type() == Key::Space || key.type() == Key::Character || key.type() == Key::Combined;
}

void ReadLineBase::commit() {
    commitText();
    _terminalStatus = ReadLineStatus::Committed;
}

void ReadLineBase::cancel() {
    discardText();
    _terminalStatus = ReadLineStatus::Cancelled;
}

void ReadLineBase::timeOut() {
    discardText();
    _terminalStatus = ReadLineStatus::Timeout;
}

void ReadLineBase::resetActivity(const TimePoint now) noexcept {
    _lastActivity = now;
    _lastCountdown = countdownSeconds(now);
}

void ReadLineBase::resetBlink(const TimePoint now) noexcept {
    _cursorVisible = true;
    _lastBlink = now;
}

auto ReadLineBase::timeoutExpired(const TimePoint now) const noexcept -> bool {
    return !_options.timeout().isZero() && now - _lastActivity >= TimeDelta{_options.timeout()};
}

auto ReadLineBase::waitDuration(const TimePoint now) const noexcept -> time::Milliseconds {
    const auto blinkInterval = TimeDelta{_options.blinkInterval()}.minimum(1_ms);
    auto wait = now.timeDeltaTo(_lastBlink + blinkInterval);
    if (!_options.timeout().isZero()) {
        wait = std::min(wait, now.timeDeltaTo(_lastActivity + TimeDelta{_options.timeout()}));
        if (!_options.timeoutDisplayThreshold().isZero()) {
            wait = std::min(wait, TimeDelta{time::Seconds{1}});
        }
    }
    return wait.minimum(1_ms).toMilliseconds();
}

auto ReadLineBase::countdownDisplayed(const std::int64_t countdown) const noexcept -> bool {
    return !_options.timeoutDisplayThreshold().isZero() && countdown >= 0 &&
        countdown <= _options.timeoutDisplayThreshold().toRawValue();
}

auto ReadLineBase::countdownSeconds(const TimePoint now) const noexcept -> std::int64_t {
    if (_options.timeout().isZero()) {
        return -1;
    }
    const auto remaining = std::max(TimeDelta::zero(), TimeDelta{_options.timeout()} - (now - _lastActivity));
    const auto milliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(remaining.toStdNanoseconds()).count();
    return (milliseconds + 999) / 1000;
}

auto ReadLineBase::moveLeft() noexcept -> bool {
    const auto next = previousUnitStart(_cursorIndex);
    if (next == _cursorIndex) {
        return false;
    }
    _cursorIndex = next;
    resetPreferredColumn();
    return true;
}

auto ReadLineBase::moveRight() noexcept -> bool {
    const auto next = nextUnitEnd(_cursorIndex);
    if (next == _cursorIndex) {
        return false;
    }
    _cursorIndex = next;
    resetPreferredColumn();
    return true;
}

auto ReadLineBase::eraseBeforeCursor() -> bool {
    const auto start = previousUnitStart(_cursorIndex);
    if (start == _cursorIndex) {
        return false;
    }
    eraseText(start, start.absoluteDistanceTo(_cursorIndex));
    _cursorIndex = start;
    resetPreferredColumn();
    return true;
}

auto ReadLineBase::eraseAtCursor() -> bool {
    const auto end = nextUnitEnd(_cursorIndex);
    if (end == _cursorIndex) {
        return false;
    }
    eraseText(_cursorIndex, _cursorIndex.absoluteDistanceTo(end));
    resetPreferredColumn();
    return true;
}

void ReadLineBase::resetPreferredColumn() noexcept {
    _preferredColumn.reset();
}

}
