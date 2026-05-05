// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <windows.h>

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>

namespace erbsland::cterm::impl {

/// Forward Windows termination events to a regular worker thread.
///
/// Console-control callbacks only enqueue a lightweight event.
/// A watcher thread invokes the configured callback in a normal thread context,
/// where terminal cleanup can safely use locks and iostreams.
class WindowsSignalDispatcher final {
public:
    static constexpr int cCtrlCExitCode = 130;
    static constexpr int cCtrlBreakExitCode = 131;
    static constexpr int cCloseExitCode = 1;

public:
    using SignalCallback = std::function<void(int)>;

public:
    /// Create and activate the signal-forwarding helper.
    /// @param onSignal Callback invoked on the watcher thread for a received event.
    explicit WindowsSignalDispatcher(SignalCallback onSignal);
    ~WindowsSignalDispatcher();

    WindowsSignalDispatcher(const WindowsSignalDispatcher &) = delete;
    auto operator=(const WindowsSignalDispatcher &) -> WindowsSignalDispatcher & = delete;
    WindowsSignalDispatcher(WindowsSignalDispatcher &&) = delete;
    auto operator=(WindowsSignalDispatcher &&) -> WindowsSignalDispatcher & = delete;

private:
    /// Register our control handler.
    void registerHandler();
    /// Start the watcher.
    void startWatcher();
    /// Stop the watcher.
    void stopWatcher();
    /// Restore the previously installed handler.
    void unregisterHandler();
    /// Wait for queued events on the watcher thread.
    void runWatcher();
    /// Add the exit code of a signal to the queue.
    void pushSignal(int exitCode) noexcept;

    /// Registered Windows console control callback.
    static auto onConsoleControl(DWORD controlType) noexcept -> BOOL;

private:
    static std::atomic<WindowsSignalDispatcher *> _instance; ///< The globally active Windows signal forwarder.

    SignalCallback _onSignal;                                ///< Callback executed on the watcher thread.
    std::mutex _queueMutex;                                  ///< Protects the queued forwarded events.
    std::condition_variable _queueCv;                        ///< Notifies the watcher about queued events.
    std::optional<int> _pendingExitCode;                     ///< The first exit code for the signal handler.
    std::thread _watcher;                                    ///< Dedicated watcher thread for forwarded events.
    bool _stopped{false};                                    ///< Stops the watcher thread.
    bool _consoleHandlerActive{false}; ///< If the console control handler was successfully registered.
};

}
