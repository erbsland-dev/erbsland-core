// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PosixBackend.hpp"

#include "KeyDecoder.hpp"
#include "PosixSignalDispatcher.hpp"

#include "../../text/StringConverter.hpp"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <unistd.h>

#include <algorithm>
#include <iostream>
#include <thread>

namespace erbsland::cterm {

auto Backend::createPlatformDefault(const TerminalFlags terminalFlags) -> BackendPtr {
    return impl::PosixBackend::getOrCreate(terminalFlags);
}

}

namespace erbsland::cterm::impl {

std::mutex PosixBackend::_instanceMutex;
PosixBackend *PosixBackend::_instance = nullptr;

PosixBackend::PosixBackend(TerminalFlags terminalFlags) : _terminalFlags{terminalFlags} {
    // called once per application.
    _instance = this;
    if (!_terminalFlags.has(TerminalFlag::NoSignalHandling)) {
        _signalHandler = std::make_unique<PosixSignalDispatcher>(
            [this](const int signalNumber) -> void { handleProcessSignal(signalNumber); });
    }
}

PosixBackend::~PosixBackend() {
    _signalHandler.reset();
    std::scoped_lock lock{_instanceMutex};
    if (_instance != nullptr) {
        _instance->restorePlatform();
        _instance = nullptr;
    }
}

void PosixBackend::initializePlatform() {
    _firstScreenSizeDetection = true;
    _hasNoTerminalAttached = false;
    _lastScreenSize = std::nullopt;
    _noSizeFailureCount = 0;
    _isInitialized = true;
}

void PosixBackend::restorePlatform() {
    closeTty();
    if (_keyInputSessionActive) {
        tcsetattr(STDIN_FILENO, TCSANOW, &_originalState);
        _keyInputSessionActive = false;
    }
    if (!_isInitialized) {
        return;
    }
    _isInitialized = false;
    if (!_hasNoTerminalAttached) {
        // make the cursor visible and disable the alternative screen buffer.
        if (_isAlternateScreenActive) {
            std::cout << "\x1b[?1049l"; // disable alternative screen buffer
        }
        std::cout << "\x1b[0m";         // restore to default color
        std::cout << "\x1b[?25h";       // make the cursor visible.
        std::cout << "\n";              // add a newline for compatibility.
        std::cout.flush();
    }
}

auto PosixBackend::supportsColorCodes() const noexcept -> bool {
    return true;
}

auto PosixBackend::supportsCursorCodes() const noexcept -> bool {
    return true;
}

auto PosixBackend::isInteractive() const noexcept -> bool {
    return _isInitialized && !_hasNoTerminalAttached;
}

auto PosixBackend::detectScreenSize() -> std::optional<bgeo::BlockSize> {
    if (!_firstScreenSizeDetection) {
        if (_hasNoTerminalAttached) {
            return std::nullopt;
        }
        const auto now = clock::now();
        if ((now - _lastScreenSizeDetection) < cMinimumDelayBetweenScreenSizeDetection) {
            return _lastScreenSize;
        }
        auto [result, size] = getScreenSize();
        _lastScreenSizeDetection = now;
        if (result == SizeDetectionResult::Success) {
            _noSizeFailureCount = 0;
            _lastScreenSize = size;
            return size;
        }
        _noSizeFailureCount += 1;
        if (_noSizeFailureCount > 100) {
            _hasNoTerminalAttached = true;
        }
        return std::nullopt;
    }
    _firstScreenSizeDetection = false;
    _lastScreenSizeDetection = clock::now();
    for (int i = 0; i < 10; ++i) {
        auto [result, size] = getScreenSize();
        if (result == SizeDetectionResult::NoTerminalAttached) {
            _hasNoTerminalAttached = true;
            return std::nullopt;
        }
        if (result == SizeDetectionResult::Success) {
            _noSizeFailureCount = 0;
            _lastScreenSize = size;
            return size;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    _noSizeFailureCount += 1;
    return std::nullopt;
}

void PosixBackend::emitText(const text::StringView &str) {
    const auto text = text::StringConverter{str}.toStdString();
    std::cout.write(text.data(), static_cast<std::streamsize>(text.size()));
}

void PosixBackend::emitFlush() {
    std::cout.flush();
}

void PosixBackend::setAlternateScreenBuffer(bool enabled) {
    _isAlternateScreenActive = enabled;
}

auto PosixBackend::inputMode() const noexcept -> Input::Mode {
    return _inputMode;
}

void PosixBackend::setInputMode(const Input::Mode mode) {
    if (_inputMode != mode) {
        if (mode == Input::Mode::Key) {
            initializeKeyInputSession();
        } else {
            restoreKeyInputSession();
        }
        _inputMode = mode;
    }
}

auto PosixBackend::readKey(const std::chrono::milliseconds timeout) -> Key {
    auto normalizedTimeout = timeout;
    if (normalizedTimeout < std::chrono::milliseconds::zero()) {
        normalizedTimeout = std::chrono::milliseconds::zero();
    }
    if (_inputMode == Input::Mode::ReadLine) {
        return Key::fromConsoleInput(readLine());
    }
    return readDecodedKey(normalizedTimeout);
}

auto PosixBackend::waitForKey() -> Key {
    if (_inputMode == Input::Mode::ReadLine) {
        return Key::fromConsoleInput(readLine());
    }
    return readDecodedKey(std::nullopt);
}

auto PosixBackend::readDecodedKey(const OptionalTimeout timeout) -> Key {
    while (true) {
        if (_pendingKeyInput.empty()) {
            _pendingEscapeStarted.reset();
            appendInputChunks(timeout);
            if (_pendingKeyInput.empty()) {
                return {};
            }
        }
        if (_pendingKeyInput[0] == '\x1b') {
            markPendingEscapeSequence();
            appendInputChunks(escapeSequenceWaitTimeout(timeout));
        }
        const auto parseResult = KeyDecoder{_pendingKeyInput}.parseConsoleInputPrefix();
        if (parseResult.status() == KeyParseStatus::Parsed) {
            erasePendingKeyInput(parseResult.consumedByteCount().toSizeT());
            return parseResult.key();
        }
        if (parseResult.status() == KeyParseStatus::NeedMoreData) {
            if (escapeSequenceExpired()) {
                if (_pendingKeyInput == "\x1b") {
                    erasePendingKeyInput(1);
                    return Key{Key::Escape};
                }
                _pendingKeyInput.clear();
                _pendingEscapeStarted.reset();
                return {};
            }
            appendInputChunks(escapeSequenceWaitTimeout(timeout));
            const auto retriedResult = KeyDecoder{_pendingKeyInput}.parseConsoleInputPrefix();
            if (retriedResult.status() == KeyParseStatus::Parsed) {
                erasePendingKeyInput(retriedResult.consumedByteCount().toSizeT());
                return retriedResult.key();
            }
            if (retriedResult.status() == KeyParseStatus::Invalid && retriedResult.consumedByteCount().toSizeT() > 0) {
                erasePendingKeyInput(retriedResult.consumedByteCount().toSizeT());
                continue;
            }
            if (escapeSequenceExpired()) {
                if (_pendingKeyInput == "\x1b") {
                    erasePendingKeyInput(1);
                    return Key{Key::Escape};
                }
                _pendingKeyInput.clear();
                _pendingEscapeStarted.reset();
            }
            return {};
        }
        if (parseResult.consumedByteCount().toSizeT() == 0) {
            _pendingKeyInput.clear();
            _pendingEscapeStarted.reset();
            return {};
        }
        erasePendingKeyInput(parseResult.consumedByteCount().toSizeT());
    }
    return {};
}

void PosixBackend::markPendingEscapeSequence() noexcept {
    if (!_pendingEscapeStarted.has_value()) {
        _pendingEscapeStarted = clock::now();
    }
}

auto PosixBackend::escapeSequenceExpired() const noexcept -> bool {
    return _pendingEscapeStarted.has_value() && clock::now() - *_pendingEscapeStarted >= cEscapeSequenceTimeout;
}

auto PosixBackend::escapeSequenceWaitTimeout(const OptionalTimeout timeout) const noexcept
    -> std::chrono::milliseconds {
    auto remaining = cEscapeSequenceTimeout;
    if (_pendingEscapeStarted.has_value()) {
        remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            cEscapeSequenceTimeout - (clock::now() - *_pendingEscapeStarted));
        if (remaining < std::chrono::milliseconds::zero()) {
            remaining = std::chrono::milliseconds::zero();
        }
    }
    if (!timeout.has_value()) {
        return remaining;
    }
    return std::min(*timeout, remaining);
}

void PosixBackend::erasePendingKeyInput(const std::size_t byteCount) {
    _pendingKeyInput.erase(0, byteCount);
    if (_pendingKeyInput.empty() || _pendingKeyInput[0] != '\x1b') {
        _pendingEscapeStarted.reset();
    } else {
        _pendingEscapeStarted = clock::now();
    }
}

auto PosixBackend::readLine() -> text::String {
    std::string input;
    std::getline(std::cin, input);
    return text::String{input};
}

auto PosixBackend::getOrCreate(const TerminalFlags terminalFlags) noexcept -> BackendPtr {
    std::scoped_lock lock{_instanceMutex};
    if (_instance == nullptr) {
        return std::make_shared<PosixBackend>(terminalFlags);
    }
    return _instance->shared_from_this();
}

auto PosixBackend::instance() noexcept -> PosixBackend * {
    std::scoped_lock lock(_instanceMutex);
    return _instance;
}

void PosixBackend::restoreGlobalPlatform() noexcept {
    std::scoped_lock lock(_instanceMutex);
    if (_instance == nullptr) {
        return;
    }
    _instance->restorePlatform();
}

auto PosixBackend::getScreenSize() -> std::pair<SizeDetectionResult, bgeo::BlockSize> {
    // first try the tty we had success with last time.
    if (_ttyFdForDetection >= 0) {
        const auto [result, size] = getScreenSizeForFd(_ttyFdForDetection);
        if (result == SizeDetectionResult::Success) {
            return {SizeDetectionResult::Success, size};
        }
        // if we fail, close it and do the complete detection.
        closeTty();
    }

    int noTerminalCount = 0;
    for (int fd : {STDOUT_FILENO, STDERR_FILENO, STDIN_FILENO}) {
        const auto [result, size] = getScreenSizeForFd(fd);
        if (result == SizeDetectionResult::Success) {
            _ttyFdForDetection = fd;
            return {SizeDetectionResult::Success, size};
        }
        if (result == SizeDetectionResult::NoTerminalAttached) {
            noTerminalCount += 1;
        }
    }

    if (const int tty = ::open("/dev/tty", O_RDONLY); tty >= 0) {
        const auto [result, size] = getScreenSizeForFd(tty);
        if (result == SizeDetectionResult::Success) {
            _ttyFdForDetection = tty;
            return {SizeDetectionResult::Success, size};
        }
        if (result == SizeDetectionResult::NoTerminalAttached) {
            noTerminalCount += 1;
        }
        ::close(tty);
    } else {
        noTerminalCount += 1;
    }

    if (noTerminalCount >= 4) {
        return {SizeDetectionResult::NoTerminalAttached, bgeo::BlockSize{0, 0}};
    }

    return {SizeDetectionResult::NoTerminalSize, bgeo::BlockSize{0, 0}};
}

auto PosixBackend::getScreenSizeForFd(const int fd) -> std::pair<SizeDetectionResult, bgeo::BlockSize> {
    if (fd < 0 || ::isatty(fd) == 0) {
        return {SizeDetectionResult::NoTerminalAttached, bgeo::BlockSize{0, 0}};
    }

    ::winsize ws{};
    if (::ioctl(fd, TIOCGWINSZ, &ws) != 0) {
        return {SizeDetectionResult::NoTerminalSize, bgeo::BlockSize{0, 0}};
    }

    if (ws.ws_col <= 0 || ws.ws_row <= 0) {
        return {SizeDetectionResult::NoTerminalSize, bgeo::BlockSize{0, 0}};
    }

    return {SizeDetectionResult::Success, bgeo::BlockSize{ws.ws_col, ws.ws_row}};
}

void PosixBackend::closeTty() {
    if (_ttyFdForDetection >= 0 && _ttyFdForDetection != STDOUT_FILENO && _ttyFdForDetection != STDERR_FILENO &&
        _ttyFdForDetection != STDIN_FILENO) {
        ::close(_ttyFdForDetection);
    }
    _ttyFdForDetection = -1;
}

void PosixBackend::initializeKeyInputSession() {
    if (tcgetattr(STDIN_FILENO, &_originalState) != 0) {
        return;
    }
    auto rawState = _originalState;
    rawState.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));
    rawState.c_cc[VMIN] = 0;
    rawState.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &rawState) != 0) {
        return;
    }
    _keyInputSessionActive = true;
}

void PosixBackend::restoreKeyInputSession() {
    tcsetattr(STDIN_FILENO, TCSANOW, &_originalState);
    _keyInputSessionActive = false;
    _pendingKeyInput.clear();
}

auto PosixBackend::waitForInput(const OptionalTimeout timeout) -> bool {
    fd_set set;
    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);
    auto tv = timeval{};
    auto *ptv = static_cast<timeval *>(nullptr);
    if (timeout.has_value()) {
        auto normalizedTimeout = *timeout;
        if (normalizedTimeout < std::chrono::milliseconds::zero()) {
            normalizedTimeout = std::chrono::milliseconds::zero();
        }
        tv.tv_sec = normalizedTimeout.count() / 1000;
        tv.tv_usec = (static_cast<int>(normalizedTimeout.count()) % 1000) * 1000;
        ptv = &tv;
    }
    return select(STDIN_FILENO + 1, &set, nullptr, nullptr, ptv) > 0;
}

auto PosixBackend::pollForInput() -> bool {
    fd_set set;
    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);
    auto tv = timeval{};
    return select(STDIN_FILENO + 1, &set, nullptr, nullptr, &tv) > 0;
}

auto PosixBackend::readInputChunk() -> std::string {
    char buffer[64];
    const auto length = ::read(STDIN_FILENO, buffer, sizeof(buffer));
    if (length <= 0) {
        return {};
    }
    return {buffer, static_cast<std::size_t>(length)};
}

void PosixBackend::appendInputChunks(const OptionalTimeout timeout) {
    if (!waitForInput(timeout)) {
        return;
    }
    while (_pendingKeyInput.size() < cMaximumPendingKeyInputSize) {
        const auto chunk = readInputChunk();
        if (chunk.empty()) {
            return;
        }
        _pendingKeyInput += chunk;
        if (!pollForInput()) {
            return;
        }
    }
}

void PosixBackend::handleProcessSignal(const int signalNumber) noexcept {
    restoreGlobalPlatform();
    struct sigaction action{};
    action.sa_handler = SIG_DFL;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(signalNumber, &action, nullptr);
    kill(getpid(), signalNumber);
    std::_Exit(128 + signalNumber);
}

}
