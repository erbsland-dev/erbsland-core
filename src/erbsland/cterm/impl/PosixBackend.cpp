// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PosixBackend.hpp"

#include "KeyDecoder.hpp"
#include "StandardInput.hpp"

#include "../../err/ParameterFailure.hpp"
#include "../../mem/impl/SecureErase.hpp"
#include "../../system/impl/signal/ProcessSignalDispatcher.hpp"
#include "../../text/impl/UnsafeU8StringAccess.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringEditor.hpp"
#include "../../unit/ByteIndex.hpp"

#include <sys/ioctl.h>
#include <sys/select.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <thread>

namespace erbsland::cterm::impl {

using namespace mem;
using mem::impl::secureErase;
using namespace text;
using namespace text::literals;

std::mutex PosixBackend::_instanceMutex;
PosixBackend *PosixBackend::_instance = nullptr;

PosixBackend::PosixBackend(const TerminalFlags terminalFlags) : PosixBackend{STDIN_FILENO} {
    _terminalFlags = terminalFlags;
    // called once per application.
    _instance = this;
    if (!_terminalFlags.has(TerminalFlag::NoSignalHandling)) {
        _signalSubscription = system::impl::ProcessSignalDispatcher::instance().addSignal(
            [](system::impl::ProcessSignal, bool &) -> void { restoreGlobalPlatform(); });
    }
}

PosixBackend::PosixBackend(const int inputFd) : _terminalFlags{TerminalFlag::NoSignalHandling}, _inputFd{inputFd} {
    if (inputFd < 0 || inputFd >= FD_SETSIZE) {
        throw err::ParameterFailure{"Invalid POSIX key-input descriptor.", "inputFd"};
    }
    _pendingKeyInput.reserve(cMaximumPendingKeyInputSize + cMaximumInputReadSize);
}

PosixBackend::~PosixBackend() {
    purgePendingInput();
    _signalSubscription.cancel();
    std::scoped_lock lock{_instanceMutex};
    restorePlatform();
    if (_instance == this) {
        _instance = nullptr;
    }
}

void PosixBackend::initializePlatform() {
    _firstScreenSizeDetection = true;
    _hasNoTerminalAttached = !isInteractiveOutput(STDOUT_FILENO);
    _lastScreenSize = std::nullopt;
    _isInitialized = true;
}

void PosixBackend::restorePlatform() {
    if (_keyInputSessionActive) {
        tcsetattr(_inputFd, TCSANOW, &_originalState);
        _keyInputSessionActive = false;
    }
    if (!_isInitialized) {
        return;
    }
    _isInitialized = false;
    if (!_hasNoTerminalAttached) {
        // make the cursor visible and disable the alternative screen buffer.
        if (_isAlternateScreenActive) {
            emitText("\x1b[?1049l"_el); // disable alternative screen buffer
        }
        emitText("\x1b[0m"_el);         // restore to default color
        emitText("\x1b[?25h"_el);       // make the cursor visible.
        emitText("\n"_el);              // add a newline for compatibility.
        emitFlush();
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

auto PosixBackend::isInteractiveOutput(const int outputFd) noexcept -> bool {
    return outputFd >= 0 && ::isatty(outputFd) != 0;
}

auto PosixBackend::detectScreenSize() -> std::optional<block::Size> {
    if (_hasNoTerminalAttached) {
        return std::nullopt;
    }
    if (!_firstScreenSizeDetection) {
        const auto now = time::TimePoint::now();
        if ((now - _lastScreenSizeDetection) < time::TimeDelta{cMinimumDelayBetweenScreenSizeDetection}) {
            return _lastScreenSize;
        }
        auto [result, size] = getScreenSize();
        _lastScreenSizeDetection = now;
        if (result == SizeDetectionResult::Success) {
            _lastScreenSize = size;
            return size;
        }
        return std::nullopt;
    }
    _firstScreenSizeDetection = false;
    _lastScreenSizeDetection = time::TimePoint::now();
    for (int i = 0; i < 10; ++i) {
        auto [result, size] = getScreenSize();
        if (result == SizeDetectionResult::NoTerminalAttached) {
            _hasNoTerminalAttached = true;
            return std::nullopt;
        }
        if (result == SizeDetectionResult::Success) {
            _lastScreenSize = size;
            return size;
        }
        std::this_thread::sleep_for(time::TimeDelta{time::Milliseconds{100}}.toStdNanoseconds());
    }
    return std::nullopt;
}

void PosixBackend::emitText(const String &str) {
    _output->writeBytes(text::impl::UnsafeU8StringAccess{str}.dataView().dataSpan());
}

void PosixBackend::emitFlush() {
    _output->flush();
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

auto PosixBackend::readKey(const time::Milliseconds timeout) -> Key {
    auto normalizedTimeout = timeout;
    if (normalizedTimeout < time::Milliseconds{}) {
        normalizedTimeout = time::Milliseconds{};
    }
    if (_inputMode == Input::Mode::ReadLine) {
        return Key::fromConsoleInput(readLine());
    }
    return readDecodedKey(InputTimeout{normalizedTimeout});
}

auto PosixBackend::waitForKey() -> Key {
    if (_inputMode == Input::Mode::ReadLine) {
        return Key::fromConsoleInput(readLine());
    }
    return readDecodedKey(InputTimeout{});
}

auto PosixBackend::readDecodedKey(const InputTimeout &timeout) -> Key {
    const auto polling = timeout.expired();
    auto attempted = false;
    while (true) {
        if (attempted && ((polling && _pendingKeyInput.empty()) || (!polling && timeout.expired()))) {
            return {};
        }
        attempted = true;
        if (_pendingKeyInput.empty()) {
            _pendingEscapeStarted.reset();
            appendInputChunks(timeout.remaining());
            if (_pendingKeyInput.empty()) {
                return {};
            }
        }
        if (_pendingKeyInput[0] == '\x1b') {
            markPendingEscapeSequence();
            appendInputChunks(escapeSequenceWaitTimeout(timeout.remaining()));
        }
        const auto parseResult = KeyDecoder{String{_pendingKeyInput}, true}.parseConsoleInputPrefix();
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
                purgePendingInput();
                return {};
            }
            appendInputChunks(escapeSequenceWaitTimeout(timeout.remaining()));
            const auto retriedResult = KeyDecoder{String{_pendingKeyInput}, true}.parseConsoleInputPrefix();
            if (retriedResult.status() == KeyParseStatus::Parsed) {
                erasePendingKeyInput(retriedResult.consumedByteCount().toSizeT());
                return retriedResult.key();
            }
            if (retriedResult.status() == KeyParseStatus::Invalid &&
                retriedResult.consumedByteCount() > unit::ByteIndex{0}) {
                erasePendingKeyInput(retriedResult.consumedByteCount().toSizeT());
                continue;
            }
            if (escapeSequenceExpired()) {
                if (_pendingKeyInput == "\x1b") {
                    erasePendingKeyInput(1);
                    return Key{Key::Escape};
                }
                purgePendingInput();
            }
            return {};
        }
        if (parseResult.consumedByteCount() == unit::ByteIndex{0}) {
            purgePendingInput();
            return {};
        }
        erasePendingKeyInput(parseResult.consumedByteCount().toSizeT());
    }
    return {};
}

void PosixBackend::markPendingEscapeSequence() noexcept {
    if (!_pendingEscapeStarted.has_value()) {
        _pendingEscapeStarted = time::TimePoint::now();
    }
}

auto PosixBackend::escapeSequenceExpired() const noexcept -> bool {
    return _pendingEscapeStarted.has_value() &&
        time::TimePoint::now() - *_pendingEscapeStarted >= time::TimeDelta{cEscapeSequenceTimeout};
}

auto PosixBackend::escapeSequenceWaitTimeout(const OptionalTimeout timeout) const noexcept -> time::Milliseconds {
    auto remaining = cEscapeSequenceTimeout;
    if (_pendingEscapeStarted.has_value()) {
        remaining = InputTimeout::roundedMilliseconds(
            time::TimeDelta{cEscapeSequenceTimeout} - (time::TimePoint::now() - *_pendingEscapeStarted));
        if (remaining < time::Milliseconds{}) {
            remaining = time::Milliseconds{};
        }
    }
    if (!timeout.has_value()) {
        return remaining;
    }
    return std::min(*timeout, remaining);
}

void PosixBackend::erasePendingKeyInput(const std::size_t byteCount) {
    const auto count = std::min(byteCount, _pendingKeyInput.size());
    const auto remaining = _pendingKeyInput.size() - count;
    std::memmove(_pendingKeyInput.data(), _pendingKeyInput.data() + count, remaining);
    secureErase(std::as_writable_bytes(std::span{_pendingKeyInput.data() + remaining, count}));
    _pendingKeyInput.resize(remaining);
    if (_pendingKeyInput.empty() || _pendingKeyInput[0] != '\x1b') {
        _pendingEscapeStarted.reset();
    } else {
        _pendingEscapeStarted = time::TimePoint::now();
    }
}

auto PosixBackend::readLine() -> String {
    return readStandardInputLine();
}

void PosixBackend::purgePendingInput() noexcept {
    secureErase(std::as_writable_bytes(std::span{_pendingKeyInput.data(), _pendingKeyInput.size()}));
    _pendingKeyInput.clear();
    _pendingEscapeStarted.reset();
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

auto PosixBackend::getScreenSize() -> std::pair<SizeDetectionResult, block::Size> {
    return getScreenSizeForFd(STDOUT_FILENO);
}

auto PosixBackend::getScreenSizeForFd(const int fd) -> std::pair<SizeDetectionResult, block::Size> {
    if (fd < 0 || ::isatty(fd) == 0) {
        return {SizeDetectionResult::NoTerminalAttached, block::Size{0, 0}};
    }

    ::winsize ws{};
    if (::ioctl(fd, TIOCGWINSZ, &ws) != 0) {
        return {SizeDetectionResult::NoTerminalSize, block::Size{0, 0}};
    }

    if (ws.ws_col <= 0 || ws.ws_row <= 0) {
        return {SizeDetectionResult::NoTerminalSize, block::Size{0, 0}};
    }

    return {SizeDetectionResult::Success, block::Size{ws.ws_col, ws.ws_row}};
}

void PosixBackend::initializeKeyInputSession() {
    if (tcgetattr(_inputFd, &_originalState) != 0) {
        return;
    }
    auto rawState = _originalState;
    rawState.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));
    rawState.c_cc[VMIN] = 0;
    rawState.c_cc[VTIME] = 0;
    if (tcsetattr(_inputFd, TCSANOW, &rawState) != 0) {
        return;
    }
    _keyInputSessionActive = true;
}

void PosixBackend::restoreKeyInputSession() {
    tcsetattr(_inputFd, TCSANOW, &_originalState);
    _keyInputSessionActive = false;
    purgePendingInput();
}

auto PosixBackend::waitForInput(const OptionalTimeout timeout) const -> bool {
    fd_set set;
    FD_ZERO(&set);
    FD_SET(_inputFd, &set);
    auto tv = timeval{};
    auto *ptv = static_cast<timeval *>(nullptr);
    if (timeout.has_value()) {
        auto normalizedTimeout = *timeout;
        if (normalizedTimeout < time::Milliseconds{}) {
            normalizedTimeout = time::Milliseconds{};
        }
        const auto [seconds, microseconds] = InputTimeout::posixParts(normalizedTimeout);
        tv.tv_sec = static_cast<decltype(tv.tv_sec)>(
            std::min<int64_t>(seconds, std::numeric_limits<decltype(tv.tv_sec)>::max()));
        tv.tv_usec = static_cast<decltype(tv.tv_usec)>(microseconds);
        ptv = &tv;
    }
    return select(_inputFd + 1, &set, nullptr, nullptr, ptv) > 0;
}

auto PosixBackend::pollForInput() const -> bool {
    fd_set set;
    FD_ZERO(&set);
    FD_SET(_inputFd, &set);
    auto tv = timeval{};
    return select(_inputFd + 1, &set, nullptr, nullptr, &tv) > 0;
}

void PosixBackend::appendInputChunks(const OptionalTimeout timeout) {
    if (!waitForInput(timeout)) {
        return;
    }
    while (_pendingKeyInput.size() < cMaximumPendingKeyInputSize) {
        char buffer[cMaximumInputReadSize];
        const auto length = ::read(_inputFd, buffer, sizeof(buffer));
        if (length <= 0) {
            secureErase(std::as_writable_bytes(std::span{buffer}));
            return;
        }
        _pendingKeyInput.append(buffer, static_cast<std::size_t>(length));
        secureErase(std::as_writable_bytes(std::span{buffer}));
        if (!pollForInput()) {
            return;
        }
    }
}

}
