// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ProcessSignalBackend.hpp"

#include "../../../core/impl/WindowsApi.hpp"

#include <atomic>
#include <thread>

namespace erbsland::system::impl {

/// Windows event backend for console-control forwarding.
/// @tested{WindowsApplicationServiceLifecycleTest}
class WindowsProcessSignalBackend final : public ProcessSignalBackend {
public:
    /// Create and install the Windows console-control backend.
    /// @param signalFn Callback invoked by the watcher thread.
    explicit WindowsProcessSignalBackend(SignalFn signalFn);
    /// Restore native handling and stop the watcher thread.
    ~WindowsProcessSignalBackend() override;

public: // implement ProcessSignalBackend
    void terminateWithDefault(ProcessSignal signal) noexcept override;

private:
    /// Forward pending console controls from the native event.
    void runWatcher() noexcept;
    /// Record a console control and wake the watcher.
    static auto onConsoleControl(DWORD controlType) noexcept -> BOOL;
    /// Convert a native console control into the shared representation.
    [[nodiscard]] static auto normalizedSignal(DWORD controlType) noexcept -> ProcessSignal;

private:
    static std::atomic<WindowsProcessSignalBackend *> _instance; ///< Active process backend.

    SignalFn _signalFn;                                          ///< Worker-thread callback.
    HANDLE _event{nullptr};                                      ///< Wakes the watcher without native-handler locks.
    std::atomic<DWORD> _pendingControl{0U};                      ///< First pending console control plus one.
    std::atomic<bool> _stopped{false};                           ///< Watcher shutdown flag.
    std::thread _watcher;                                        ///< Console-control watcher thread.
    bool _registered{false};                                     ///< Whether the native handler is active.
};

}
