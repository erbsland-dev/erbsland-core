// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SubprocessBackend.hpp"

#include "../../../core/impl/WindowsApi.hpp"

#include <mutex>
#include <string>
#include <thread>

namespace erbsland::system::impl {

/// Windows implementation of an owned subprocess.
/// @tested{SubprocessInteropTest}
class WindowsSubprocessBackend final : public SubprocessBackend {
private:
    /// Native delivery state shared with a pipe-reader thread.
    struct OutputState final {
        SubprocessOutputSink sink; ///< Internal workflow byte/error delivery.
        bool standardError{};      ///< Whether this pipe carries stderr.
    };

public:
    /// Launch one Windows child with the requested environment and stream setup.
    WindowsSubprocessBackend(
        const path::Path &executable,
        const text::StringList &arguments,
        const SubprocessOptions &options,
        const SubprocessOutputSink &sink);
    ~WindowsSubprocessBackend() override;

public: // implement SubprocessBackend
    [[nodiscard]] auto processId() const noexcept -> ProcessId override;
    [[nodiscard]] auto isRunning() -> bool override;
    [[nodiscard]] auto wait() -> SubprocessExitStatus override;
    [[nodiscard]] auto wait(time::TimeDelta timeout) -> std::optional<SubprocessExitStatus> override;
    void terminate() override;
    void kill() override;

private:
    /// Transfer a pipe to its draining thread after successful thread creation.
    void startReader(HANDLE &handle, bool error, const SubprocessOutputSink &sink);
    /// Read and cache the native process exit code.
    void storeStatus();
    /// Join all active output drain threads.
    void joinReaders();
    /// Drain a pipe, delivering original bytes to its workflow.
    static void readCaptured(HANDLE handle, const std::shared_ptr<OutputState> &state) noexcept;

private:
    HANDLE _process{nullptr};                        ///< Native process handle.
    DWORD _processId{};                              ///< Native child process identifier.
    std::optional<SubprocessExitStatus> _exitStatus; ///< Cached child exit status.
    std::shared_ptr<OutputState> _standardOutput;    ///< Optional standard output capture.
    std::shared_ptr<OutputState> _standardError;     ///< Optional standard error capture.
    std::thread _standardOutputReader;               ///< Standard output draining thread.
    std::thread _standardErrorReader;                ///< Standard error draining thread.
    std::mutex _waitMutex;                           ///< Serializes native wait calls.
};

}
