// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WindowsProcessSignalBackend.hpp"

#include <cstdlib>
#include <utility>

namespace erbsland::system::impl {

std::atomic<WindowsProcessSignalBackend *> WindowsProcessSignalBackend::_instance{};

auto ProcessSignalBackend::create(SignalFn signalFn) -> std::unique_ptr<ProcessSignalBackend> {
    return std::make_unique<WindowsProcessSignalBackend>(std::move(signalFn));
}

WindowsProcessSignalBackend::WindowsProcessSignalBackend(SignalFn signalFn) :
    _signalFn{std::move(signalFn)}, _event{::CreateEventW(nullptr, FALSE, FALSE, nullptr)} {
    _instance.store(this, std::memory_order_release);
    if (_event != nullptr) {
        _watcher = std::thread{[this]() noexcept -> void { runWatcher(); }};
        _registered = (::SetConsoleCtrlHandler(&WindowsProcessSignalBackend::onConsoleControl, TRUE) != FALSE);
    }
}

WindowsProcessSignalBackend::~WindowsProcessSignalBackend() {
    if (_registered) {
        ::SetConsoleCtrlHandler(&WindowsProcessSignalBackend::onConsoleControl, FALSE);
    }
    _instance.store(nullptr, std::memory_order_release);
    _stopped.store(true, std::memory_order_release);
    if (_event != nullptr) {
        ::SetEvent(_event);
    }
    if (_watcher.joinable()) {
        _watcher.join();
    }
    if (_event != nullptr) {
        ::CloseHandle(_event);
    }
}

void WindowsProcessSignalBackend::runWatcher() noexcept {
    while (!_stopped.load(std::memory_order_acquire)) {
        if (::WaitForSingleObject(_event, INFINITE) != WAIT_OBJECT_0 || _stopped.load(std::memory_order_acquire)) {
            return;
        }
        const auto control = _pendingControl.exchange(0U, std::memory_order_acq_rel);
        if (control != 0U) {
            _signalFn(normalizedSignal(control - 1U));
        }
    }
}

auto WindowsProcessSignalBackend::onConsoleControl(const DWORD controlType) noexcept -> BOOL {
    auto *instance = _instance.load(std::memory_order_acquire);
    if (instance == nullptr || instance->_event == nullptr) {
        return FALSE;
    }
    switch (controlType) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        break;
    default:
        return FALSE;
    }
    auto expected = DWORD{0U};
    instance->_pendingControl.compare_exchange_strong(expected, controlType + 1U, std::memory_order_acq_rel);
    ::SetEvent(instance->_event);
    return TRUE;
}

auto WindowsProcessSignalBackend::normalizedSignal(const DWORD controlType) noexcept -> ProcessSignal {
    switch (controlType) {
    case CTRL_C_EVENT:
        return ProcessSignal::Interrupt;
    case CTRL_BREAK_EVENT:
        return ProcessSignal::ConsoleBreak;
    default:
        return ProcessSignal::ConsoleClose;
    }
}

void WindowsProcessSignalBackend::terminateWithDefault(const ProcessSignal signal) noexcept {
    switch (signal) {
    case ProcessSignal::Interrupt:
        std::_Exit(130);
    case ProcessSignal::ConsoleBreak:
        std::_Exit(131);
    default:
        std::_Exit(1);
    }
}

}
