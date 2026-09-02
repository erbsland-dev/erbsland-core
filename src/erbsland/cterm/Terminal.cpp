// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Terminal.hpp"

#include "Buffer.hpp"
#include "BufferView.hpp"

#include "impl/AnsiSequence.hpp"
#include "impl/BlockPrintContextToTerminal.hpp"
#include "impl/paragraph/Layout.hpp"
#include "impl/paragraph/Printer.hpp"

#include "../err/RuntimeError.hpp"
#include "../text/Literals.hpp"
#include "../text/StringEditor.hpp"

#include <algorithm>
#include <cassert>
#include <vector>

namespace erbsland::cterm {

using namespace text::literals;

auto Terminal::synchronizeOutput() const -> TerminalOutputGuard {
    return TerminalOutputGuard{_outputMutex};
}

Terminal::Terminal() : Terminal(block::Size{80, 25}, TerminalFlags{}) {
}

Terminal::Terminal(const TerminalFlags flags) : Terminal(block::Size{80, 25}, flags) {
}

Terminal::Terminal(const block::Size size, const TerminalFlags flags) :
    _flags{flags}, _size{size.limitedWith(cMaximumSize).expandedWith(cMinimumSize)} {
    _backend = Backend::createPlatformDefault(flags);
    _input.setBackend(_backend);
    _lineBuffer.setBackend(_backend);
}

Terminal::Terminal(BackendPtr backend, const block::Size size) :
    _backend{std::move(backend)}, _size{size.limitedWith(cMaximumSize).expandedWith(cMinimumSize)} {
    if (_backend == nullptr) {
        _backend = Backend::createPlatformDefault(TerminalFlags{});
    }
    _input.setBackend(_backend);
    _lineBuffer.setBackend(_backend);
}

void Terminal::setSize(const block::Size size) noexcept {
    if (_size != size) {
        _size = size.limitedWith(cMaximumSize).expandedWith(cMinimumSize);
        _afterResize = true;
    }
}

void Terminal::setOutputMode(const OutputMode outputMode) noexcept {
    if (_outputMode != outputMode) {
        _outputMode = outputMode;
        if (_outputMode == OutputMode::BlockText) {
            setSizeDetectionEnabled(false);
            setRefreshMode(RefreshMode::Keep);
            setBackBufferEnabled(false);
        }
    }
}

auto Terminal::sizeDetectionEnabled() const noexcept -> bool {
    return _sizeDetectionEnabled;
}

void Terminal::setSizeDetectionEnabled(const bool enabled) noexcept {
    if (_outputMode == OutputMode::BlockText && enabled) {
        return;
    }
    _sizeDetectionEnabled = enabled;
}

auto Terminal::lineBufferEnabled() const noexcept -> bool {
    return _lineBuffer.cachingEnabled();
}

void Terminal::setLineBufferEnabled(const bool enabled) noexcept {
    if ((_outputMode == OutputMode::BlockText || !canUseLineBuffer()) && enabled) {
        return;
    }
    _lineBuffer.setCachingEnabled(enabled);
}

auto Terminal::safeMarginEnabled() const noexcept -> bool {
    return _safeMarginEnabled;
}

void Terminal::setSafeMarginEnabled(const bool enabled) noexcept {
    _safeMarginEnabled = enabled;
    if (_terminalSize == block::Size{}) {
        return; // ignore if we have no detected terminal size.
    }
    setSize(applySafeMargin(_terminalSize));
}

auto Terminal::backBufferEnabled() const noexcept -> bool {
    return _backBufferEnabled;
}

void Terminal::setBackBufferEnabled(const bool enabled) noexcept {
    if (_outputMode == OutputMode::BlockText && enabled) {
        return;
    }
    if (_backBufferEnabled != enabled) {
        _backBufferEnabled = enabled;
        if (enabled) {
            // enforce a full rebuild on the next update.
            _backBuffer = {};
            _afterResize = true;
        } else {
            _backBuffer.reset();
        }
    }
}

void Terminal::setBackend(BackendPtr backend) noexcept {
    if (_backend != backend) {
        if (backend == nullptr) {
            _backend = Backend::createPlatformDefault(_flags);
        } else {
            _backend = std::move(backend);
        }
        _input.setBackend(_backend);
        _lineBuffer.setBackend(_backend);
        if (!canUseLineBuffer()) {
            _lineBuffer.setCachingEnabled(false);
        }
    }
}

auto Terminal::input() noexcept -> Input & {
    return _input;
}

}
