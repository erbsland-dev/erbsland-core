// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WindowsSignalDispatcher.hpp"

#include <utility>

namespace erbsland::cterm::impl {

std::atomic<WindowsSignalDispatcher *> WindowsSignalDispatcher::_instance{};

WindowsSignalDispatcher::WindowsSignalDispatcher(SignalCallback onSignal) : _onSignal{std::move(onSignal)} {
    _instance.store(this, std::memory_order_release);
    startWatcher();
    registerHandler();
}

WindowsSignalDispatcher::~WindowsSignalDispatcher() {
    unregisterHandler();
    stopWatcher();
    _instance.store(nullptr, std::memory_order_release);
}

void WindowsSignalDispatcher::registerHandler() {
    _consoleHandlerActive = (::SetConsoleCtrlHandler(&WindowsSignalDispatcher::onConsoleControl, TRUE) != FALSE);
}

void WindowsSignalDispatcher::startWatcher() {
    _watcher = std::thread([this]() -> void { runWatcher(); });
}

void WindowsSignalDispatcher::stopWatcher() {
    {
        std::scoped_lock lock{_queueMutex};
        _stopped = true;
    }
    _queueCv.notify_one();
    if (_watcher.joinable()) {
        _watcher.join();
    }
}

void WindowsSignalDispatcher::unregisterHandler() {
    if (_consoleHandlerActive) {
        ::SetConsoleCtrlHandler(&WindowsSignalDispatcher::onConsoleControl, FALSE);
        _consoleHandlerActive = false;
    }
}

void WindowsSignalDispatcher::runWatcher() {
    std::unique_lock lock{_queueMutex};
    while (true) {
        _queueCv.wait(lock, [this]() -> bool { return _stopped || _pendingExitCode.has_value(); });
        if (_stopped || !_pendingExitCode.has_value()) {
            return;
        }
        const auto exitCode = *_pendingExitCode;
        _pendingExitCode.reset();
        lock.unlock();
        _onSignal(exitCode);
        return;
    }
}

void WindowsSignalDispatcher::pushSignal(const int exitCode) noexcept {
    std::scoped_lock lock{_queueMutex};
    if (_stopped || _pendingExitCode.has_value()) {
        return;
    }
    _pendingExitCode = exitCode;
    _queueCv.notify_one();
}

auto WindowsSignalDispatcher::onConsoleControl(const DWORD controlType) noexcept -> BOOL {
    auto *instance = _instance.load(std::memory_order_acquire);
    if (instance == nullptr) {
        return FALSE;
    }
    switch (controlType) {
    case CTRL_C_EVENT:
        instance->pushSignal(cCtrlCExitCode);
        return TRUE;
    case CTRL_BREAK_EVENT:
        instance->pushSignal(cCtrlBreakExitCode);
        return TRUE;
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        instance->pushSignal(cCloseExitCode);
        return TRUE;
    default:
        break;
    }
    return FALSE;
}

}
