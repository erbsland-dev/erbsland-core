// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ProcessSignalBackend.hpp"

#include <signal.h>

#include <array>
#include <thread>

namespace erbsland::system::impl {

/// POSIX self-pipe backend for process signal forwarding.
/// @tested{PosixApplicationServiceLifecycleTest}
class PosixProcessSignalBackend final : public ProcessSignalBackend {
private:
    static constexpr std::array<int, 4> cSignals = {SIGINT, SIGTERM, SIGHUP, SIGQUIT};
    static constexpr int cShutdownToken = -1;

public:
    /// Create and install the POSIX process-signal backend.
    /// @param signalFn Callback invoked by the watcher thread.
    explicit PosixProcessSignalBackend(SignalFn signalFn);
    /// Restore native handlers and stop the watcher thread.
    ~PosixProcessSignalBackend() override;

public: // implement ProcessSignalBackend
    void terminateWithDefault(ProcessSignal signal) noexcept override;

private:
    /// Open the self-pipe used by the native signal handler.
    void openPipe();
    /// Install the process signal handlers.
    void registerHandlers();
    /// Restore all previous process signal handlers.
    void unregisterHandlers() noexcept;
    /// Forward native signal tokens from the self-pipe.
    void runWatcher() noexcept;
    /// Write one native signal number into the self-pipe.
    static void onSignal(int signalNumber) noexcept;
    /// Convert a native signal number into the shared representation.
    [[nodiscard]] static auto normalizedSignal(int signalNumber) noexcept -> ProcessSignal;
    /// Convert the shared representation into a native signal number.
    [[nodiscard]] static auto nativeSignal(ProcessSignal signal) noexcept -> int;

private:
    static std::array<struct sigaction, cSignals.size()> _previousActions; ///< Previously installed handlers.
    static_assert(sizeof(sig_atomic_t) >= sizeof(int));
    static volatile sig_atomic_t _writeFd;                                 ///< Async-handler pipe endpoint.

    SignalFn _signalFn;                                                    ///< Worker-thread callback.
    std::array<int, 2> _pipe{-1, -1};                                      ///< Signal forwarding pipe.
    std::thread _watcher;                                                  ///< Signal watcher thread.
    bool _handlersRegistered{false};                                       ///< Whether previous actions were saved.
};

}
