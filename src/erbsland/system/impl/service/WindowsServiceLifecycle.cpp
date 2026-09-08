// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WindowsServiceLifecycle.hpp"

#include "../signal/ProcessSignalDispatcher.hpp"
#include "../WindowsErrorContext.hpp"

#include "../../../text/Literals.hpp"
#include "../../PlatformError.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <utility>

namespace erbsland::system::impl {

using namespace text::literals;

std::atomic<WindowsServiceLifecycle *> WindowsServiceLifecycle::_instance{};

auto ServiceLifecycle::create(StopFn stopFn) -> ServiceLifecyclePtr {
    return std::make_unique<WindowsServiceLifecycle>(std::move(stopFn));
}

WindowsServiceLifecycle::WindowsServiceLifecycle(StopFn stopFn) :
    _stopFn{std::move(stopFn)}, _stopEvent{::CreateEventW(nullptr, FALSE, FALSE, nullptr)} {
    if (_stopEvent == nullptr) {
        throw system::PlatformError{
            "Failed to create the Windows service stop event."_el, WindowsErrorContext::fromLastError()};
    }
}

WindowsServiceLifecycle::~WindowsServiceLifecycle() {
    stopWorker();
    if (_stopEvent != nullptr) {
        ::CloseHandle(_stopEvent);
    }
}

auto WindowsServiceLifecycle::run(RunFn runFn) -> int {
    const auto signalSubscription =
        ProcessSignalDispatcher::instance().addSignal([this](const ProcessSignal signal, bool &claimed) -> void {
            if (signal != ProcessSignal::Interrupt && signal != ProcessSignal::ConsoleBreak) {
                return;
            }
            claimed = true;
            _stopFn();
        });
    _runFn = std::move(runFn);
    _instance.store(this, std::memory_order_release);
    wchar_t serviceName[] = L"erbsland-core-service";
    SERVICE_TABLE_ENTRYW table[] = {{serviceName, &WindowsServiceLifecycle::serviceMain}, {nullptr, nullptr}};
    if (::StartServiceCtrlDispatcherW(table) == FALSE) {
        const auto errorCode = ::GetLastError();
        _instance.store(nullptr, std::memory_order_release);
        if (errorCode == ERROR_FAILED_SERVICE_CONTROLLER_CONNECT) {
            return _runFn();
        }
        throw system::PlatformError{
            "Failed to connect the application to the Windows Service Control Manager."_el,
            WindowsErrorContext::fromErrorCode(errorCode)};
    }
    _instance.store(nullptr, std::memory_order_release);
    if (_exception != nullptr) {
        std::rethrow_exception(_exception);
    }
    return _result;
}

void WINAPI WindowsServiceLifecycle::serviceMain(const DWORD argumentCount, LPWSTR *arguments) noexcept {
    if (auto *instance = _instance.load(std::memory_order_acquire); instance != nullptr) {
        instance->runService(argumentCount, arguments);
    }
}

auto WINAPI WindowsServiceLifecycle::controlHandler(
    const DWORD control,
    [[maybe_unused]] const DWORD eventType,
    [[maybe_unused]] void *eventData,
    void *context) noexcept -> DWORD {
    auto *instance = static_cast<WindowsServiceLifecycle *>(context);
    if (instance == nullptr) {
        return ERROR_CALL_NOT_IMPLEMENTED;
    }
    switch (control) {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
        if (instance->_currentState.load(std::memory_order_acquire) != SERVICE_RUNNING) {
            return ERROR_CALL_NOT_IMPLEMENTED;
        }
        instance->requestStop();
        return NO_ERROR;
    case SERVICE_CONTROL_INTERROGATE:
        return NO_ERROR;
    default:
        return ERROR_CALL_NOT_IMPLEMENTED;
    }
}

void WindowsServiceLifecycle::runService(const DWORD argumentCount, LPWSTR *arguments) noexcept {
    try {
        if (argumentCount == 0U || arguments == nullptr || arguments[0] == nullptr) {
            throw system::PlatformError{"The Service Control Manager supplied no service name."_el};
        }
        _statusHandle = ::RegisterServiceCtrlHandlerExW(arguments[0], &WindowsServiceLifecycle::controlHandler, this);
        if (_statusHandle == nullptr) {
            throw system::PlatformError{
                "Failed to register the Windows service control handler."_el, WindowsErrorContext::fromLastError()};
        }
        _runningAsService.store(true, std::memory_order_release);
        publishStatus(SERVICE_START_PENDING, _startupWaitHint.load(std::memory_order_acquire), NO_ERROR, 0U, true);
        if (_startupCompleteRequested.load(std::memory_order_acquire)) {
            publishStatus(SERVICE_RUNNING, 0U, NO_ERROR, 0U, true);
        }
        if (_stopEvent != nullptr) {
            _stopWorker = std::thread{[this]() noexcept -> void { runStopWorker(); }};
        }
        _result = _runFn();
        stopWorker();
        const auto failed = _result != 0;
        publishStatus(
            SERVICE_STOPPED,
            0U,
            failed ? ERROR_SERVICE_SPECIFIC_ERROR : NO_ERROR,
            failed ? static_cast<DWORD>(_result) : 0U,
            true);
    } catch (...) {
        _exception = std::current_exception();
        stopWorker();
        try {
            publishStatus(SERVICE_STOPPED, 0U, ERROR_EXCEPTION_IN_SERVICE, 0U, false);
        } catch (...) {}
    }
}

void WindowsServiceLifecycle::reportStartupPending(const time::TimeDelta expectedRemainingTime) {
    const auto waitHint = waitHintMilliseconds(expectedRemainingTime);
    _startupWaitHint.store(waitHint, std::memory_order_release);
    if (_runningAsService.load(std::memory_order_acquire)) {
        publishStatus(SERVICE_START_PENDING, waitHint, NO_ERROR, 0U, true);
    }
}

void WindowsServiceLifecycle::reportStartupComplete() {
    _startupCompleteRequested.store(true, std::memory_order_release);
    if (_runningAsService.load(std::memory_order_acquire)) {
        publishStatus(SERVICE_RUNNING, 0U, NO_ERROR, 0U, true);
    }
}

void WindowsServiceLifecycle::reportStopping() noexcept {
    if (!_runningAsService.load(std::memory_order_acquire) ||
        _currentState.load(std::memory_order_acquire) != SERVICE_RUNNING ||
        _stoppingReported.exchange(true, std::memory_order_acq_rel)) {
        return;
    }
    try {
        publishStatus(SERVICE_STOP_PENDING, cDefaultWaitHintMilliseconds, NO_ERROR, 0U, false);
    } catch (...) {}
}

void WindowsServiceLifecycle::requestStop() noexcept {
    if (_stopRequested.exchange(true, std::memory_order_acq_rel)) {
        return;
    }
    ::SetEvent(_stopEvent);
}

void WindowsServiceLifecycle::runStopWorker() noexcept {
    if (::WaitForSingleObject(_stopEvent, INFINITE) == WAIT_OBJECT_0 &&
        !_stopWorkerDone.load(std::memory_order_acquire)) {
        reportStopping();
        _stopFn();
    }
}

void WindowsServiceLifecycle::stopWorker() noexcept {
    _stopWorkerDone.store(true, std::memory_order_release);
    if (_stopEvent != nullptr) {
        ::SetEvent(_stopEvent);
    }
    if (_stopWorker.joinable()) {
        _stopWorker.join();
    }
}

void WindowsServiceLifecycle::publishStatus(
    const DWORD state,
    const DWORD waitHint,
    const DWORD win32ExitCode,
    const DWORD serviceExitCode,
    const bool throwOnError) {
    const auto lock = std::scoped_lock{_statusMutex};
    if (_statusHandle == nullptr || _stoppedReported) {
        return;
    }
    const auto pending = state == SERVICE_START_PENDING || state == SERVICE_STOP_PENDING;
    if (pending) {
        ++_checkpoint;
    } else {
        _checkpoint = 0U;
    }
    auto status = SERVICE_STATUS{
        .dwServiceType = SERVICE_WIN32_OWN_PROCESS,
        .dwCurrentState = state,
        .dwControlsAccepted = state == SERVICE_RUNNING ? SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN : 0U,
        .dwWin32ExitCode = win32ExitCode,
        .dwServiceSpecificExitCode = serviceExitCode,
        .dwCheckPoint = pending ? _checkpoint : 0U,
        .dwWaitHint = pending ? waitHint : 0U,
    };
    if (::SetServiceStatus(_statusHandle, &status) == FALSE) {
        if (throwOnError) {
            throw system::PlatformError{
                "Failed to report the Windows service status."_el, WindowsErrorContext::fromLastError()};
        }
        return;
    }
    _currentState.store(state, std::memory_order_release);
    if (state == SERVICE_STOPPED) {
        _stoppedReported = true;
    }
}

auto WindowsServiceLifecycle::waitHintMilliseconds(const time::TimeDelta expectedRemainingTime) noexcept -> DWORD {
    const auto milliseconds = expectedRemainingTime.toMilliseconds().toRawValue();
    return static_cast<DWORD>(std::clamp<std::int64_t>(milliseconds, 1, std::numeric_limits<DWORD>::max()));
}

}
