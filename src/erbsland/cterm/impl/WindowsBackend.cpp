// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WindowsBackend.hpp"

#include "WindowsBackendPrivate.hpp"

#include "../../text/Literals.hpp"
#include "../../text/StringConverter.hpp"
#include "../../unit/CpLength.hpp"

#include <conio.h>
#include <fcntl.h>

#include <chrono>
#include <iostream>

namespace erbsland::cterm {

auto Backend::createPlatformDefault(const TerminalFlags terminalFlags) -> BackendPtr {
    return impl::WindowsBackend::getOrCreate(terminalFlags);
}

}

namespace erbsland::cterm::impl {

using namespace text::literals;

std::mutex WindowsBackend::_instanceMutex;
WindowsBackend *WindowsBackend::_instance = nullptr;

WindowsBackend::WindowsBackend(const TerminalFlags terminalFlags) :
    _p{std::make_unique<WindowsBackendPrivate>(terminalFlags)} {

    // called once per application.
    _instance = this;
    if (!_p->_terminalFlags.has(TerminalFlag::NoSignalHandling)) {
        _p->_signalHandler = std::make_unique<WindowsSignalDispatcher>(
            [this](const int signalNumber) -> void { handleProcessSignal(signalNumber); });
    }
}

WindowsBackend::~WindowsBackend() {
    _p->_signalHandler.reset();
    std::scoped_lock lock{_instanceMutex};
    if (_instance != nullptr) {
        _instance->restorePlatform();
        _instance = nullptr;
    }
}

void WindowsBackend::initializePlatform() {
    _p->outputHandle = ::GetStdHandle(STD_OUTPUT_HANDLE);
    const auto screenSize = detectScreenSize();
    (void)screenSize;
    if (_p->_isInteractive) {
        enableUtf8Mode();
        enableAnsiMode();
        _p->_cursorVisible = changeCursorVisibility(false);
        _p->_cursorStateSaved = true;
    }
    _p->_initialized = true;
}

void WindowsBackend::restorePlatform() {
    if (!_p->_initialized) {
        return;
    }
    if (_p->_cursorStateSaved) {
        _p->_cursorStateSaved = false;
        changeCursorVisibility(_p->_cursorVisible);
    }
    if (_p->_isAlternateScreenActive) {
        emitText("\x1b[?1049l"_el); // disable alternative screen buffer
    }
    if (_p->_isInteractive) {
        emitText("\x1b[0m\x1b[?25h\n"_el); // restore default color and make the cursor visible.
    }
    emitFlush();
    _p->_initialized = false;
}

auto WindowsBackend::supportsColorCodes() const noexcept -> bool {
    return true;
}

auto WindowsBackend::supportsCursorCodes() const noexcept -> bool {
    return true;
}

bool WindowsBackend::supportsCursorVisibilityCodes() const noexcept {
    return false;
}

bool WindowsBackend::supportsAlternateScreenBufferCodes() const noexcept {
    return true;
}

auto WindowsBackend::isInteractive() const noexcept -> bool {
    return _p->_isInteractive;
}

auto WindowsBackend::detectScreenSize() -> std::optional<bgeo::BlockSize> {
    if (_p->outputHandle == nullptr || _p->outputHandle == INVALID_HANDLE_VALUE) {
        _p->_isInteractive = false;
        return std::nullopt;
    }
    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (::GetConsoleScreenBufferInfo(_p->outputHandle, &info) == 0) {
        _p->_isInteractive = false;
        return std::nullopt;
    }
    const auto width = static_cast<int>(info.srWindow.Right - info.srWindow.Left + 1);
    const auto height = static_cast<int>(info.srWindow.Bottom - info.srWindow.Top + 1);
    if (width <= 0 || height <= 0) {
        _p->_isInteractive = false;
        return std::nullopt;
    }
    _p->_isInteractive = true;
    return bgeo::BlockSize{width, height};
}

void WindowsBackend::setCursorVisible(const bool visible) {
    changeCursorVisibility(visible);
}

void WindowsBackend::emitText(const text::String &str) {
    const auto text = text::StringConverter{str}.toStdString();
    if (text.empty()) {
        return;
    }
    if (_p->outputHandle == nullptr || _p->outputHandle == INVALID_HANDLE_VALUE) {
        std::cout.write(text.data(), static_cast<std::streamsize>(text.size()));
        return;
    }
    DWORD written{};
    std::size_t index = 0;
    if (_p->_isInteractive) {
        const auto wideLength =
            ::MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
        if (wideLength <= 0) {
            return;
        }
        std::wstring wide(static_cast<size_t>(wideLength), L'\0');
        ::MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), wide.data(), wideLength);
        while (index < wide.size()) {
            if (::WriteConsoleW(
                    _p->outputHandle,
                    wide.data() + index,
                    static_cast<DWORD>(wide.size() - index),
                    &written,
                    nullptr) == 0 ||
                written == 0) {
                break; // prevent write lock if we lose the output handle.
            }
            index += written;
            if (index < wide.size()) {
                std::this_thread::yield();
            }
        }
    } else {
        while (index < text.size()) {
            if (::WriteFile(
                    _p->outputHandle,
                    text.data() + index,
                    static_cast<DWORD>(text.size() - index),
                    &written,
                    nullptr) == 0 ||
                written == 0) {
                break; // prevent write lock if we lose the output handle.
            }
            index += written;
            if (index < text.size()) {
                std::this_thread::yield();
            }
        }
    }
}

void WindowsBackend::emitFlush() {
    if (_p->outputHandle == nullptr || _p->outputHandle == INVALID_HANDLE_VALUE) {
        std::cout.flush();
        return;
    }
    ::FlushFileBuffers(_p->outputHandle);
}

void WindowsBackend::setAlternateScreenBuffer(const bool enabled) {
    _p->_isAlternateScreenActive = enabled;
}

auto WindowsBackend::inputMode() const noexcept -> Input::Mode {
    return _p->_inputMode;
}

void WindowsBackend::setInputMode(Input::Mode mode) {
    _p->_inputMode = mode;
}

auto WindowsBackend::readKey(std::chrono::milliseconds timeout) -> Key {
    if (timeout < std::chrono::milliseconds::zero()) {
        timeout = std::chrono::milliseconds::zero();
    }
    if (_p->_inputMode == Input::Mode::ReadLine) {
        return Key::fromConsoleInput(readLine());
    }
    return readKeyFromConsole(timeout);
}

auto WindowsBackend::waitForKey() -> Key {
    if (_p->_inputMode == Input::Mode::ReadLine) {
        return Key::fromConsoleInput(readLine());
    }
    return readKeyFromConsole(std::nullopt);
}

}
