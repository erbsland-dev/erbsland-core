// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <signal.h>

#include <array>
#include <functional>
#include <thread>

namespace erbsland::cterm::impl {

/// Forward POSIX termination signals to a regular worker thread.
///
/// The async signal handler only writes the received signal number into a pipe.
/// The watcher thread reads that pipe and invokes the configured callback in a
/// normal thread context, where terminal cleanup can safely use locks and iostreams.
class PosixSignalDispatcher final {
public:
    using SignalCallback = std::function<void(int)>;

private:
    /// The signals handled by this dispatcher.
    static constexpr std::array<int, 4> cSignals = {SIGINT, SIGTERM, SIGHUP, SIGQUIT};
    /// A token that indicates the shutdown signal.
    static constexpr int cShutdownToken = -1;

public:
    /// Create and activate the signal-forwarding helper.
    /// @param onSignal Callback invoked on the watcher thread for a received signal.
    explicit PosixSignalDispatcher(SignalCallback onSignal);
    ~PosixSignalDispatcher();

    PosixSignalDispatcher(const PosixSignalDispatcher &) = delete;
    auto operator=(const PosixSignalDispatcher &) -> PosixSignalDispatcher & = delete;
    PosixSignalDispatcher(PosixSignalDispatcher &&) = delete;
    auto operator=(PosixSignalDispatcher &&) -> PosixSignalDispatcher & = delete;

private:
    /// Create the pipe used to forward signals out of the signal handler.
    void openSignalPipe();
    /// Start the watcher
    void startWatcher();
    /// Send the shutdown signal
    void sendShutdownSignal() noexcept;
    /// Wait for shutdown.
    void waitForShutdown() noexcept;
    /// Close the signal forwarding pipe.
    void closeSignalPipe() noexcept;
    /// Register all handled signals.
    void registerHandlers();
    /// Restore previously installed signal handlers.
    void unregisterHandlers();
    /// Wait for forwarded signals on the watcher thread.
    void runWatcher();

    /// Registered signal handler.
    static void onSignal(int signalNumber) noexcept;

private:
    static std::array<struct sigaction, cSignals.size()> _previousSignalActions; ///< Previous signal actions.
    static_assert(sizeof(sig_atomic_t) >= sizeof(int), "sig_atomic_t must be able to hold a file descriptor");
    static volatile sig_atomic_t _signalWriteFd; ///< The descriptor for sending the signals.

    SignalCallback _onSignal;                    ///< Callback executed on the watcher thread.
    std::array<int, 2> _signalPipe{-1, -1};      ///< Pipe to forward signals into the watcher thread.
    std::thread _watcher;                        ///< Dedicated watcher thread for forwarded signals.
};

}
