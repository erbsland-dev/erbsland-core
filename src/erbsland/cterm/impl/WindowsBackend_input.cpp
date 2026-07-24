// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WindowsBackend.hpp"

#include "InputRecordEraseGuard.hpp"
#include "StandardInput.hpp"
#include "WindowsBackendPrivate.hpp"
#include "WindowsSignalDispatcher.hpp"

#include "../../text/Literals.hpp"
#include "../../text/StringConverter.hpp"
#include "../../unit/CpLength.hpp"

#include <conio.h>
#include <fcntl.h>
#include <windows.h>

#include <chrono>
#include <iostream>

namespace erbsland::cterm::impl {

auto WindowsBackend::readKeyFromConsole(const OptionalTimeout timeout) -> Key {
    if (!_p->_pendingKeys.empty()) {
        auto key = _p->_pendingKeys.front();
        _p->_pendingKeys.front() = {};
        _p->_pendingKeys.pop_front();
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

        INPUT_RECORD inputRecord{};
        const auto inputRecordEraseGuard = InputRecordEraseGuard{inputRecord};
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
    if (_p->_pendingKeys.empty()) {
        return {};
    }
    auto key = _p->_pendingKeys.front();
    _p->_pendingKeys.front() = {};
    _p->_pendingKeys.pop_front();
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
    return readStandardInputLine();
}

void WindowsBackend::purgePendingInput() noexcept {
    for (auto &key : _p->_pendingKeys) {
        mem::impl::secureErase(std::as_writable_bytes(std::span{&key, 1U}));
    }
    _p->_pendingKeys.clear();
    if (_p->_pendingTextInput.has_value()) {
        mem::impl::secureErase(std::as_writable_bytes(std::span{&*_p->_pendingTextInput, 1U}));
        _p->_pendingTextInput.reset();
    }
    if (_p->_pendingHighSurrogate.has_value()) {
        mem::impl::secureErase(std::as_writable_bytes(std::span{&*_p->_pendingHighSurrogate, 1U}));
        _p->_pendingHighSurrogate.reset();
    }
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
    if (_p->outputHandle == nullptr || _p->outputHandle == INVALID_HANDLE_VALUE) {
        return;
    }
    DWORD mode = 0;
    GetConsoleMode(_p->outputHandle, &mode);
    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(_p->outputHandle, mode);
}

auto WindowsBackend::changeCursorVisibility(bool visible) -> bool {
    if (_p->outputHandle == nullptr || _p->outputHandle == INVALID_HANDLE_VALUE) {
        return false;
    }
    CONSOLE_CURSOR_INFO cursorInfo{};
    if (::GetConsoleCursorInfo(_p->outputHandle, &cursorInfo) == 0) {
        return true;
    }
    const bool previousState = cursorInfo.bVisible != FALSE;
    cursorInfo.bVisible = visible ? TRUE : FALSE;
    ::SetConsoleCursorInfo(_p->outputHandle, &cursorInfo);
    return previousState;
}

void WindowsBackend::enqueueKey(const Key &key, const std::size_t repeatCount) {
    for (std::size_t index = 0; index < repeatCount; ++index) {
        _p->_pendingKeys.push_back(key);
    }
}

void WindowsBackend::flushPendingTextInput() {
    if (!_p->_pendingTextInput.has_value()) {
        return;
    }
    if (_p->_pendingTextInput->characterCount() <= unit::CpLength::one()) {
        _p->_pendingKeys.emplace_back(Key::Character, _p->_pendingTextInput->first());
    } else {
        _p->_pendingKeys.emplace_back(Key::Combined, *_p->_pendingTextInput);
    }
    mem::impl::secureErase(std::as_writable_bytes(std::span{&*_p->_pendingTextInput, 1U}));
    _p->_pendingTextInput.reset();
}

void WindowsBackend::appendTextCodePoint(const char32_t codePoint, const std::size_t repeatCount) {
    for (std::size_t index = 0; index < repeatCount; ++index) {
        const auto character = text::Char{codePoint};
        if (character.isNull() || character.isControl()) {
            continue;
        }
        if (character.displayWidth() == 0) {
            if (_p->_pendingTextInput.has_value()) {
                _p->_pendingTextInput = _p->_pendingTextInput->withCombining(character);
            }
            continue;
        }
        flushPendingTextInput();
        _p->_pendingTextInput = text::CombinedChar{character};
    }
}

auto WindowsBackend::decodeUtf16CodeUnit(const char16_t codeUnit) -> std::optional<char32_t> {
    constexpr auto cHighSurrogateStart = char16_t{0xd800U};
    constexpr auto cHighSurrogateEnd = char16_t{0xdbffU};
    constexpr auto cLowSurrogateStart = char16_t{0xdc00U};
    constexpr auto cLowSurrogateEnd = char16_t{0xdfffU};

    if (codeUnit >= cHighSurrogateStart && codeUnit <= cHighSurrogateEnd) {
        if (_p->_pendingHighSurrogate.has_value()) {
            mem::impl::secureErase(std::as_writable_bytes(std::span{&*_p->_pendingHighSurrogate, 1U}));
        }
        _p->_pendingHighSurrogate = codeUnit;
        return std::nullopt;
    }
    if (codeUnit >= cLowSurrogateStart && codeUnit <= cLowSurrogateEnd) {
        if (!_p->_pendingHighSurrogate.has_value()) {
            return std::nullopt;
        }
        const auto high = static_cast<uint32_t>(*_p->_pendingHighSurrogate - cHighSurrogateStart);
        const auto low = static_cast<uint32_t>(codeUnit - cLowSurrogateStart);
        mem::impl::secureErase(std::as_writable_bytes(std::span{&*_p->_pendingHighSurrogate, 1U}));
        _p->_pendingHighSurrogate.reset();
        return static_cast<char32_t>(0x10000U + ((high << 10U) | low));
    }
    if (_p->_pendingHighSurrogate.has_value()) {
        mem::impl::secureErase(std::as_writable_bytes(std::span{&*_p->_pendingHighSurrogate, 1U}));
        _p->_pendingHighSurrogate.reset();
    }
    return static_cast<char32_t>(codeUnit);
}

void WindowsBackend::handleProcessSignal(const int exitCode) noexcept {
    restoreGlobalPlatform();
    std::_Exit(exitCode);
}

}
