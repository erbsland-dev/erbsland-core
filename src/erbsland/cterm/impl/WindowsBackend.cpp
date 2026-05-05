// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WindowsBackend.hpp"

#include "WindowsSignalDispatcher.hpp"

#include "../../text/impl/UnicodeData.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringConverter.hpp"

#include <conio.h>
#include <fcntl.h>
#include <windows.h>

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

struct WindowsBackend::WindowsPrivate {
    HANDLE outputHandle{INVALID_HANDLE_VALUE};
};

WindowsBackend::WindowsBackend(const TerminalFlags terminalFlags) :
    _terminalFlags{terminalFlags}, _windows{std::make_unique<WindowsPrivate>()} {

    // called once per application.
    _instance = this;
    if (!_terminalFlags.has(TerminalFlag::NoSignalHandling)) {
        _signalHandler = std::make_unique<WindowsSignalDispatcher>(
            [this](const int signalNumber) -> void { handleProcessSignal(signalNumber); });
    }
}

WindowsBackend::~WindowsBackend() {
    _signalHandler.reset();
    std::scoped_lock lock{_instanceMutex};
    if (_instance != nullptr) {
        _instance->restorePlatform();
        _instance = nullptr;
    }
}

void WindowsBackend::initializePlatform() {
    _windows->outputHandle = ::GetStdHandle(STD_OUTPUT_HANDLE);
    const auto screenSize = detectScreenSize();
    (void)screenSize;
    if (_isInteractive) {
        enableUtf8Mode();
        enableAnsiMode();
        _cursorVisible = changeCursorVisibility(false);
        _cursorStateSaved = true;
    }
    _initialized = true;
}

void WindowsBackend::restorePlatform() {
    if (!_initialized) {
        return;
    }
    if (_cursorStateSaved) {
        _cursorStateSaved = false;
        changeCursorVisibility(_cursorVisible);
    }
    if (_isAlternateScreenActive) {
        emitText("\x1b[?1049l"_el); // disable alternative screen buffer
    }
    if (_isInteractive) {
        emitText("\x1b[0m\x1b[?25h\n"_el); // restore default color and make the cursor visible.
    }
    emitFlush();
    _initialized = false;
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
    return _isInteractive;
}

auto WindowsBackend::detectScreenSize() -> std::optional<bgeo::BlockSize> {
    if (_windows->outputHandle == nullptr || _windows->outputHandle == INVALID_HANDLE_VALUE) {
        _isInteractive = false;
        return std::nullopt;
    }
    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (::GetConsoleScreenBufferInfo(_windows->outputHandle, &info) == 0) {
        _isInteractive = false;
        return std::nullopt;
    }
    const auto width = static_cast<int>(info.srWindow.Right - info.srWindow.Left + 1);
    const auto height = static_cast<int>(info.srWindow.Bottom - info.srWindow.Top + 1);
    if (width <= 0 || height <= 0) {
        _isInteractive = false;
        return std::nullopt;
    }
    _isInteractive = true;
    return bgeo::BlockSize{width, height};
}

void WindowsBackend::setCursorVisible(const bool visible) {
    changeCursorVisibility(visible);
}

void WindowsBackend::emitText(const text::StringView &str) {
    const auto text = text::StringConverter{str}.toStdString();
    if (text.empty()) {
        return;
    }
    if (_windows->outputHandle == nullptr || _windows->outputHandle == INVALID_HANDLE_VALUE) {
        std::cout.write(text.data(), static_cast<std::streamsize>(text.size()));
        return;
    }
    DWORD written{};
    std::size_t index = 0;
    if (_isInteractive) {
        const auto wideLength =
            ::MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
        if (wideLength <= 0) {
            return;
        }
        std::wstring wide(static_cast<size_t>(wideLength), L'\0');
        ::MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), wide.data(), wideLength);
        while (index < wide.size()) {
            if (::WriteConsoleW(
                    _windows->outputHandle,
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
                    _windows->outputHandle,
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
    if (_windows->outputHandle == nullptr || _windows->outputHandle == INVALID_HANDLE_VALUE) {
        std::cout.flush();
        return;
    }
    ::FlushFileBuffers(_windows->outputHandle);
}

void WindowsBackend::setAlternateScreenBuffer(const bool enabled) {
    _isAlternateScreenActive = enabled;
}

auto WindowsBackend::inputMode() const noexcept -> Input::Mode {
    return _inputMode;
}

void WindowsBackend::setInputMode(Input::Mode mode) {
    _inputMode = mode;
}

auto WindowsBackend::readKey(std::chrono::milliseconds timeout) -> Key {
    if (timeout < std::chrono::milliseconds::zero()) {
        timeout = std::chrono::milliseconds::zero();
    }
    if (_inputMode == Input::Mode::ReadLine) {
        return Key::fromConsoleInput(readLine());
    }
    return readKeyFromConsole(timeout);
}

auto WindowsBackend::waitForKey() -> Key {
    if (_inputMode == Input::Mode::ReadLine) {
        return Key::fromConsoleInput(readLine());
    }
    return readKeyFromConsole(std::nullopt);
}

auto WindowsBackend::readKeyFromConsole(const OptionalTimeout timeout) -> Key {
    if (!_pendingKeys.empty()) {
        const auto key = _pendingKeys.front();
        _pendingKeys.pop_front();
        return key;
    }
    using namespace std::chrono;
    const auto inputHandle = GetStdHandle(STD_INPUT_HANDLE);
    auto timeoutMilliseconds = DWORD{INFINITE};
    if (timeout.has_value()) {
        auto normalizedTimeout = *timeout;
        if (normalizedTimeout < milliseconds::zero()) {
            normalizedTimeout = milliseconds::zero();
        }
        timeoutMilliseconds = static_cast<DWORD>(normalizedTimeout.count());
    }
    if (WaitForSingleObject(inputHandle, timeoutMilliseconds) != WAIT_OBJECT_0) {
        return {};
    }
    for (;;) {
        DWORD available = 0;
        if ((GetNumberOfConsoleInputEvents(inputHandle, &available) == 0) || available == 0) {
            break;
        }

        INPUT_RECORD inputRecord;
        DWORD read = 0;
        if ((ReadConsoleInputW(inputHandle, &inputRecord, 1, &read) == 0) || read != 1) {
            break;
        }

        if (inputRecord.EventType != KEY_EVENT) {
            continue;
        }

        const auto &keyEvent = inputRecord.Event.KeyEvent;
        if (keyEvent.bKeyDown == 0) {
            continue; // consume, ignore
        }
        const auto keyModifiers = keyModifiersFromControlState(keyEvent.dwControlKeyState);

        switch (keyEvent.wVirtualKeyCode) {
        case VK_UP:
            flushPendingTextInput();
            enqueueKey(Key{Key::Up, keyModifiers}, keyEvent.wRepeatCount);
            break;
        case VK_DOWN:
            flushPendingTextInput();
            enqueueKey(Key{Key::Down, keyModifiers}, keyEvent.wRepeatCount);
            break;
        case VK_LEFT:
            flushPendingTextInput();
            enqueueKey(Key{Key::Left, keyModifiers}, keyEvent.wRepeatCount);
            break;
        case VK_RIGHT:
            flushPendingTextInput();
            enqueueKey(Key{Key::Right, keyModifiers}, keyEvent.wRepeatCount);
            break;
        case VK_RETURN:
            flushPendingTextInput();
            enqueueKey(Key{Key::Enter, keyModifiers}, keyEvent.wRepeatCount);
            break;
        case VK_TAB:
            flushPendingTextInput();
            if ((keyEvent.dwControlKeyState & SHIFT_PRESSED) != 0) {
                enqueueKey(Key{Key::BackTab}, keyEvent.wRepeatCount);
            } else {
                enqueueKey(Key{Key::Tab, keyModifiers}, keyEvent.wRepeatCount);
            }
            break;
        case VK_SPACE:
            flushPendingTextInput();
            enqueueKey(Key{Key::Space, keyModifiers}, keyEvent.wRepeatCount);
            break;
        case VK_ESCAPE:
            flushPendingTextInput();
            enqueueKey(Key{Key::Escape, keyModifiers}, keyEvent.wRepeatCount);
            break;
        case VK_BACK:
            flushPendingTextInput();
            enqueueKey(Key{Key::Backspace, keyModifiers}, keyEvent.wRepeatCount);
            break;
        case VK_INSERT:
            flushPendingTextInput();
            enqueueKey(Key{Key::Insert, keyModifiers}, keyEvent.wRepeatCount);
            break;
        case VK_DELETE:
            flushPendingTextInput();
            enqueueKey(Key{Key::Delete, keyModifiers}, keyEvent.wRepeatCount);
            break;
        case VK_HOME:
            flushPendingTextInput();
            enqueueKey(Key{Key::Home, keyModifiers}, keyEvent.wRepeatCount);
            break;
        case VK_END:
            flushPendingTextInput();
            enqueueKey(Key{Key::End, keyModifiers}, keyEvent.wRepeatCount);
            break;
        case VK_PRIOR:
            flushPendingTextInput();
            enqueueKey(Key{Key::PageUp, keyModifiers}, keyEvent.wRepeatCount);
            break;
        case VK_NEXT:
            flushPendingTextInput();
            enqueueKey(Key{Key::PageDown, keyModifiers}, keyEvent.wRepeatCount);
            break;
        case VK_F1:
            flushPendingTextInput();
            enqueueKey(Key{Key::F1, keyModifiers}, keyEvent.wRepeatCount);
            break;
        case VK_F2:
            flushPendingTextInput();
            enqueueKey(Key{Key::F2, keyModifiers}, keyEvent.wRepeatCount);
            break;
        case VK_F3:
            flushPendingTextInput();
            enqueueKey(Key{Key::F3, keyModifiers}, keyEvent.wRepeatCount);
            break;
        case VK_F4:
            flushPendingTextInput();
            enqueueKey(Key{Key::F4, keyModifiers}, keyEvent.wRepeatCount);
            break;
        case VK_F5:
            flushPendingTextInput();
            enqueueKey(Key{Key::F5, keyModifiers}, keyEvent.wRepeatCount);
            break;
        case VK_F6:
            flushPendingTextInput();
            enqueueKey(Key{Key::F6, keyModifiers}, keyEvent.wRepeatCount);
            break;
        case VK_F7:
            flushPendingTextInput();
            enqueueKey(Key{Key::F7, keyModifiers}, keyEvent.wRepeatCount);
            break;
        case VK_F8:
            flushPendingTextInput();
            enqueueKey(Key{Key::F8, keyModifiers}, keyEvent.wRepeatCount);
            break;
        case VK_F9:
            flushPendingTextInput();
            enqueueKey(Key{Key::F9, keyModifiers}, keyEvent.wRepeatCount);
            break;
        case VK_F10:
            flushPendingTextInput();
            enqueueKey(Key{Key::F10, keyModifiers}, keyEvent.wRepeatCount);
            break;
        case VK_F11:
            flushPendingTextInput();
            enqueueKey(Key{Key::F11, keyModifiers}, keyEvent.wRepeatCount);
            break;
        case VK_F12:
            flushPendingTextInput();
            enqueueKey(Key{Key::F12, keyModifiers}, keyEvent.wRepeatCount);
            break;
        default:
            if (const auto codePoint = decodeUtf16CodeUnit(static_cast<char16_t>(keyEvent.uChar.UnicodeChar));
                codePoint.has_value()) {
                appendTextCodePoint(*codePoint, keyEvent.wRepeatCount);
            }
            break;
        }
    }
    flushPendingTextInput();
    if (_pendingKeys.empty()) {
        return {};
    }
    const auto key = _pendingKeys.front();
    _pendingKeys.pop_front();
    return key;
}

auto WindowsBackend::keyModifiersFromControlState(const uint32_t controlKeyState) noexcept -> KeyModifiers {
    auto modifiers = KeyModifiers{};
    if ((controlKeyState & SHIFT_PRESSED) != 0) {
        modifiers.set(KeyModifier::Shift);
    }
    if ((controlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) != 0) {
        modifiers.set(KeyModifier::Control);
    }
    if ((controlKeyState & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) != 0) {
        modifiers.set(KeyModifier::Alt);
    }
    return modifiers;
}

auto WindowsBackend::readLine() -> text::String {
    std::string input;
    std::getline(std::cin, input);
    return text::String{input};
}

auto WindowsBackend::getOrCreate(const TerminalFlags terminalFlags) noexcept -> BackendPtr {
    std::scoped_lock lock{_instanceMutex};
    if (_instance == nullptr) {
        return std::make_shared<WindowsBackend>(terminalFlags);
    }
    return _instance->shared_from_this();
}

auto WindowsBackend::instance() noexcept -> WindowsBackend * {
    std::scoped_lock lock(_instanceMutex);
    return _instance;
}

void WindowsBackend::restoreGlobalPlatform() noexcept {
    std::scoped_lock lock(_instanceMutex);
    if (_instance == nullptr) {
        return;
    }
    _instance->restorePlatform();
}

void WindowsBackend::enableUtf8Mode() {
    constexpr UINT cUtf8CodePage = 65001;
    ::SetConsoleCP(cUtf8CodePage);
    ::SetConsoleOutputCP(cUtf8CodePage);
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);
}

void WindowsBackend::enableAnsiMode() {
    if (_windows->outputHandle == nullptr || _windows->outputHandle == INVALID_HANDLE_VALUE) {
        return;
    }
    DWORD mode = 0;
    GetConsoleMode(_windows->outputHandle, &mode);
    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(_windows->outputHandle, mode);
}

auto WindowsBackend::changeCursorVisibility(bool visible) -> bool {
    if (_windows->outputHandle == nullptr || _windows->outputHandle == INVALID_HANDLE_VALUE) {
        return false;
    }
    CONSOLE_CURSOR_INFO cursorInfo{};
    if (::GetConsoleCursorInfo(_windows->outputHandle, &cursorInfo) == 0) {
        return true;
    }
    const bool previousState = cursorInfo.bVisible != FALSE;
    cursorInfo.bVisible = visible ? TRUE : FALSE;
    ::SetConsoleCursorInfo(_windows->outputHandle, &cursorInfo);
    return previousState;
}

void WindowsBackend::enqueueKey(const Key &key, const std::size_t repeatCount) {
    for (std::size_t index = 0; index < repeatCount; ++index) {
        _pendingKeys.push_back(key);
    }
}

void WindowsBackend::flushPendingTextInput() {
    if (!_pendingTextInput.has_value()) {
        return;
    }
    if (_pendingTextInput->codePointCount() <= 1) {
        _pendingKeys.emplace_back(Key::Character, _pendingTextInput->mainCodePoint());
    } else {
        _pendingKeys.emplace_back(Key::Combined, _pendingTextInput->utf32());
    }
    _pendingTextInput.reset();
}

void WindowsBackend::appendTextCodePoint(const char32_t codePoint, const std::size_t repeatCount) {
    for (std::size_t index = 0; index < repeatCount; ++index) {
        if (codePoint == 0 || CombinedBlock::isControlCode(codePoint)) {
            continue;
        }
        if (text::impl::unicodeDisplayWidthFor(codePoint) == 0) {
            if (_pendingTextInput.has_value()) {
                _pendingTextInput = _pendingTextInput->withCombining(codePoint);
            }
            continue;
        }
        flushPendingTextInput();
        _pendingTextInput = CombinedBlock{codePoint};
    }
}

auto WindowsBackend::decodeUtf16CodeUnit(const char16_t codeUnit) -> std::optional<char32_t> {
    constexpr auto cHighSurrogateStart = char16_t{0xd800U};
    constexpr auto cHighSurrogateEnd = char16_t{0xdbffU};
    constexpr auto cLowSurrogateStart = char16_t{0xdc00U};
    constexpr auto cLowSurrogateEnd = char16_t{0xdfffU};

    if (codeUnit >= cHighSurrogateStart && codeUnit <= cHighSurrogateEnd) {
        _pendingHighSurrogate = codeUnit;
        return std::nullopt;
    }
    if (codeUnit >= cLowSurrogateStart && codeUnit <= cLowSurrogateEnd) {
        if (!_pendingHighSurrogate.has_value()) {
            return std::nullopt;
        }
        const auto high = static_cast<uint32_t>(*_pendingHighSurrogate - cHighSurrogateStart);
        const auto low = static_cast<uint32_t>(codeUnit - cLowSurrogateStart);
        _pendingHighSurrogate.reset();
        return static_cast<char32_t>(0x10000U + ((high << 10U) | low));
    }
    _pendingHighSurrogate.reset();
    return static_cast<char32_t>(codeUnit);
}

void WindowsBackend::handleProcessSignal(const int exitCode) noexcept {
    restoreGlobalPlatform();
    std::_Exit(exitCode);
}

}
