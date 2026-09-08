// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ServiceLifecycle.hpp"

#include "../../../core/impl/WindowsApi.hpp"

#include <atomic>
#include <exception>
#include <mutex>
#include <thread>

namespace erbsland::system::impl {

/// Windows own-process service and interactive-console lifecycle.
/// @tested{WindowsApplicationServiceLifecycleTest}
class WindowsServiceLifecycle final : public ServiceLifecycle {
private:
    static constexpr DWORD cDefaultWaitHintMilliseconds = 30'000U;

public:
    /// Create a Windows service and interactive lifecycle.
    /// @param stopFn Callback that requests graceful application shutdown.
    explicit WindowsServiceLifecycle(StopFn stopFn);
    /// Stop worker activity and release native resources.
    ~WindowsServiceLifecycle() override;

public: // implement ServiceLifecycle
    [[nodiscard]] auto run(RunFn runFn) -> int override;
    void reportStartupPending(time::TimeDelta expectedRemainingTime) override;
    void reportStartupComplete() override;
    void reportStopping() noexcept override;

private:
    /// Native ServiceMain entry point.
    static void WINAPI serviceMain(DWORD argumentCount, LPWSTR *arguments) noexcept;
    /// Native service-control callback.
    static auto WINAPI controlHandler(DWORD control, DWORD eventType, void *eventData, void *context) noexcept -> DWORD;
    /// Register the service status handle and execute the application.
    void runService(DWORD argumentCount, LPWSTR *arguments) noexcept;
    /// Coalesce and enqueue a graceful stop request.
    void requestStop() noexcept;
    /// Wait for native stop controls and invoke the application callback.
    void runStopWorker() noexcept;
    /// Stop and join the graceful-stop worker.
    void stopWorker() noexcept;
    /// Publish one SCM service status transition.
    void publishStatus(DWORD state, DWORD waitHint, DWORD win32ExitCode, DWORD serviceExitCode, bool throwOnError);
    /// Convert a duration into a bounded Windows wait hint.
    [[nodiscard]] static auto waitHintMilliseconds(time::TimeDelta expectedRemainingTime) noexcept -> DWORD;

private:
    static std::atomic<WindowsServiceLifecycle *> _instance; ///< Lifecycle used by native static callbacks.

    StopFn _stopFn;                                          ///< Graceful application-stop callback.
    RunFn _runFn;                                 ///< Application callback active during dispatcher execution.
    std::mutex _statusMutex;                      ///< Serializes SCM status changes.
    SERVICE_STATUS_HANDLE _statusHandle{nullptr}; ///< Registered SCM status handle.
    DWORD _checkpoint{0U};                        ///< Current pending-state progress checkpoint.
    std::atomic<DWORD> _startupWaitHint{cDefaultWaitHintMilliseconds}; ///< Latest startup wait hint.
    HANDLE _stopEvent{nullptr};                                        ///< Wakes the graceful-stop worker.
    std::atomic<bool> _stopWorkerDone{false};                          ///< Requests stop-worker shutdown.
    std::atomic<bool> _stopRequested{false};                           ///< Coalesces native stop controls.
    std::atomic<bool> _stoppingReported{false};                        ///< Prevents repeated stopping reports.
    std::atomic<DWORD> _currentState{SERVICE_STOPPED};                 ///< Last successfully reported SCM state.
    std::thread _stopWorker;                            ///< Calls application shutdown outside the control handler.
    std::exception_ptr _exception;                      ///< Foreign exception transferred to the run thread.
    int _result{};                                      ///< Application result transferred from ServiceMain.
    std::atomic<bool> _runningAsService{false};         ///< Whether the SCM dispatcher connected.
    std::atomic<bool> _startupCompleteRequested{false}; ///< Readiness requested before or during ServiceMain.
    bool _stoppedReported{false};                       ///< Prevents duplicate final status calls.
};

}
