// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Terminal.hpp"

#include "Buffer.hpp"
#include "BufferConstRefView.hpp"
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
using block::Coordinate;
using block::Position;
using block::Size;
using geometry::Alignment;

void Terminal::clearScreen() noexcept {
    if (_outputMode == OutputMode::BlockText) {
        return;
    }
    if (!_backend->supportsCursorCodes()) {
        _backend->clearScreen();
        return;
    }
    _lineBuffer.write("\x1b[2J\x1b[1;1H"_el);
    _lineBuffer.handleEmit();
}

auto Terminal::isAlternateScreenActive() const noexcept -> bool {
    return _isAlternateScreenActive;
}

void Terminal::setAlternateScreen(const bool enabled) noexcept {
    if (_isAlternateScreenActive == enabled) {
        return;
    }
    if (_backend->supportsAlternateScreenBufferCodes()) {
        if (enabled) {
            _lineBuffer.write("\x1b[?1049h"_el);
        } else {
            _lineBuffer.write("\x1b[?1049l"_el);
        }
        _lineBuffer.handleEmit();
        flush(); // make sure this is done before any rendering is started.
    }
    // always notify the backend about the alternate screen buffer switch.
    _backend->setAlternateScreenBuffer(enabled);
    _isAlternateScreenActive = enabled;
}

void Terminal::updateScreen(const ReadableBuffer &buffer, const UpdateSettings &settings) noexcept {
    if (_outputMode == OutputMode::BlockText) {
        write(buffer);
        flush();
        return;
    }
    if (settings.switchToAlternateBuffer() && !isAlternateScreenActive()) {
        setAlternateScreen(true);
    }
    {
        impl::LineBufferEmitLockGuard guard(_lineBuffer);
        updateSizeTooSmallBuffer(settings);
        refreshScreen();
        setAutoWrap(false); // disable auto wrapping.
        if (_sizeTooSmallBuffer) {
            if (backBufferEnabled()) {
                updateScreenWithBackBuffer(*_sizeTooSmallBuffer, UpdateSettings::defaultSettings());
            } else {
                updateScreenWithoutBackBuffer(*_sizeTooSmallBuffer, UpdateSettings::defaultSettings());
            }
        } else {
            if (backBufferEnabled()) {
                updateScreenWithBackBuffer(buffer, settings);
            } else {
                updateScreenWithoutBackBuffer(buffer, settings);
            }
        }
        moveCursor(Position{Coordinate{0}, _size.height() - 1}, MoveMode::Absolute);
        setAutoWrap(true); // activate auto wrapping again.
    }
    flush();
}

void Terminal::moveHome() noexcept {
    if (_outputMode == OutputMode::BlockText) {
        return;
    }
    if (!_backend->supportsCursorCodes()) {
        _backend->moveCursor(Position{0, 0}, MoveMode::Absolute);
        return;
    }
    _lineBuffer.write("\x1b[H"_el);
    _lineBuffer.handleEmit();
}

void Terminal::moveCursor(const Position posOrDelta, const MoveMode mode) noexcept {
    if (!_backend->supportsCursorCodes()) {
        _backend->moveCursor(posOrDelta, mode);
        return;
    }
    if (mode == MoveMode::Absolute) {
        moveTo(posOrDelta);
    } else {
        if (posOrDelta.x() < 0) {
            moveLeft(-posOrDelta.x());
        } else if (posOrDelta.x() > 0) {
            moveRight(posOrDelta.x());
        }
        if (posOrDelta.y() < 0) {
            moveUp(-posOrDelta.y());
        } else if (posOrDelta.y() > 0) {
            moveDown(posOrDelta.y());
        }
    }
}

void Terminal::setAutoWrap(const bool enabled) noexcept {
    if (_outputMode == OutputMode::BlockText || !_backend->supportsCursorCodes()) {
        return;
    }
    if (enabled) {
        _lineBuffer.write("\x1b[?7h"_el);
    } else {
        _lineBuffer.write("\x1b[?7l"_el);
    }
    _lineBuffer.handleEmit();
}

void Terminal::setCursorVisible(const bool visible) noexcept {
    if (_outputMode == OutputMode::BlockText) {
        return;
    }
    if (_backend->supportsCursorVisibilityCodes()) {
        if (visible) {
            _lineBuffer.write("\x1b[?25h"_el);
        } else {
            _lineBuffer.write("\x1b[?25l"_el);
        }
    } else {
        _backend->setCursorVisible(visible);
    }
}

void Terminal::setColorEnabled(const bool enabled) noexcept {
    setOutputMode(enabled ? OutputMode::FullControl : OutputMode::BlockText);
}

void Terminal::refreshScreen() noexcept {
    if (_afterResize) {
        _afterResize = false;
        clearScreen();
        return;
    }
    switch (refreshMode()) {
    case RefreshMode::Clear:
        clearScreen();
        break;
    case RefreshMode::Overwrite:
        moveHome();
        break;
    default:
        break;
    }
}

auto Terminal::updateScreenWithBackBuffer(const ReadableBuffer &buffer, const UpdateSettings &settings) -> void {
    setAutoWrap(false); // disable auto wrapping.
    if (_backBuffer == nullptr) {
        updateScreenAndCreateNewBackBuffer(buffer, settings);
    } else if (_backBuffer->size() != _size) {
        updateScreenAndResizeBackBuffer(buffer, settings);
    } else {
        BufferConstRefView view(buffer, _size);
        settings.applyTo(view);
        const auto differences = _backBuffer->countDifferencesTo(view);
        const auto percent = differences * 100U / buffer.size().area().toSizeT();
        if (percent <= 20) {
            updateScreenPartialWithBackBuffer(view);
        } else {
            writeImpl(view, true);
            _backBuffer->setAndResizeFrom(view);
        }
    }
}

auto Terminal::updateScreenAndCreateNewBackBuffer(const ReadableBuffer &buffer, const UpdateSettings &settings)
    -> void {
    BufferConstRefView view(buffer, _size);
    settings.applyTo(view);
    writeImpl(view, true);
    _backBuffer = view.clone();
}

auto Terminal::updateScreenAndResizeBackBuffer(const ReadableBuffer &buffer, const UpdateSettings &settings) -> void {
    if (_backBuffer == nullptr) {
        throw err::RuntimeError{"Back buffer is not initialized."};
    }
    BufferConstRefView view(buffer, _size);
    settings.applyTo(view);
    writeImpl(view, true);
    _backBuffer->setAndResizeFrom(view);
}

auto Terminal::updateScreenPartialWithBackBuffer(const ReadableBuffer &view) -> void {
    if (_backBuffer == nullptr) {
        throw err::RuntimeError{"Back buffer is not initialized."};
    }
    impl::LineBufferEmitLockGuard guard(_lineBuffer);
    auto lastWriteCursor = Position{0, 0};
    view.size().forEach([&](const Position pos) -> void {
        const auto &newCharacter = view.get(pos);
        const auto &oldCharacter = _backBuffer->get(pos);
        if (newCharacter == oldCharacter) {
            return;
        }
        if (lastWriteCursor != pos) {
            moveTo(pos);
            lastWriteCursor = pos;
        }
        write(newCharacter);
        _backBuffer->set(pos, newCharacter);
        lastWriteCursor += Position{newCharacter.displayWidth(), 0};
        if (newCharacter.displayWidth() == 2) {
            // if we have a 2-width character, copy the second position as well (it should be empty).
            const auto secondPosition = pos + Position{1, 0};
            _backBuffer->set(secondPosition, view.get(secondPosition));
        }
    });
}

void Terminal::moveTo(const Position pos) noexcept {
    if (_outputMode == OutputMode::BlockText) {
        return;
    }
    if (!_backend->supportsCursorCodes()) {
        _backend->moveCursor(pos, MoveMode::Absolute);
        return;
    }
    _lineBuffer.write(impl::ansi_sequence::cursorPosition((pos.y() + 1).toRawValue(), (pos.x() + 1).toRawValue()));
    _lineBuffer.handleEmit();
}

auto Terminal::updateScreenWithoutBackBuffer(const ReadableBuffer &buffer, const UpdateSettings &settings) -> void {
    BufferConstRefView view(buffer, _size);
    settings.applyTo(view);
    writeImpl(view, true); // Output the view, line by line.
}

void Terminal::updateSizeTooSmallBuffer(const UpdateSettings &settings) noexcept {
    if (!settings.minimumSize().fitsInto(_size)) {
        if (_sizeTooSmallBuffer == nullptr) {
            _sizeTooSmallBuffer = std::make_unique<Buffer>(_size);
        } else {
            _sizeTooSmallBuffer->resize(_size);
        }
        _sizeTooSmallBuffer->fill(settings.minimumSizeBackground());
        if (!settings.minimumSizeMessage().isEmpty()) {
            _sizeTooSmallBuffer->drawBlockText(
                settings.minimumSizeMessage(), _sizeTooSmallBuffer->rect(), Alignment::Center);
        }
    } else {
        _sizeTooSmallBuffer = {};
    }
}

void Terminal::writeImpl(const ReadableBuffer &buffer, const bool withRowMove) noexcept {
    for (auto y = Coordinate{0}; y < buffer.size().height(); ++y) {
        if (withRowMove && y != 0) {
            moveTo(Position{Coordinate{0}, y});
        }
        for (auto x = Coordinate{0}; x < buffer.size().width(); ++x) {
            auto &character = buffer.get(Position{x, y});
            setStyle(character.style());
            _lineBuffer.write(character);
        }
        if (!withRowMove) {
            setDefaultColor();
            setBlockAttributes(BlockAttributes::reset());
            _lineBuffer.write("\n"_el);
        }
    }
}

auto Terminal::applySafeMargin(const Size terminalSize) const noexcept -> Size {
    if (!_safeMarginEnabled) {
        return terminalSize;
    }
    return terminalSize - Size{1, 1};
}

void Terminal::initializeScreen() noexcept {
    _backend->initializePlatform();
    _style = BlockStyle::reset();
    testScreenSize();
    if (isInteractive()) {
        setCursorVisible(false);
    }
    flush();
}

auto Terminal::isInteractive() const noexcept -> bool {
    return _backend->isInteractive();
}

void Terminal::testScreenSize() noexcept {
    if (!_sizeDetectionEnabled) {
        return;
    }
    if (const auto detectedSize = _backend->detectScreenSize(); detectedSize.has_value()) {
        _terminalSize = *detectedSize;
        setSize(applySafeMargin(_terminalSize));
    }
}

void Terminal::restoreScreen() noexcept {
    if (isInteractive()) {
        setDefaultColor();
        setBlockAttributes(BlockAttributes::reset());
        setCursorVisible(true);
        if (isAlternateScreenActive()) {
            setAlternateScreen(false);
        }
        flush();
    }
    _isAlternateScreenActive = false;
    _lineBuffer.shutdown();
    _backBuffer = {}; // free the back buffer resources
    _backend->restorePlatform();
}

}
