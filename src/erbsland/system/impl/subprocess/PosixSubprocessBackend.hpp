// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SubprocessBackend.hpp"

#include <mutex>
#include <string>
#include <thread>

namespace erbsland::system::impl {

/// POSIX implementation of an owned subprocess.
/// @tested{SubprocessInteropTest}
class PosixSubprocessBackend final : public SubprocessBackend {
private:
    /// Native delivery state shared with a pipe-reader thread.
    struct OutputState final {
        SubprocessOutputSink sink; ///< Internal workflow byte/error delivery.
        bool standardError{};      ///< Whether this pipe carries stderr.
    };

public:
    /// Launch one POSIX child with the requested environment and stream setup.
    PosixSubprocessBackend(
        const path::Path &executable,
        const text::StringList &arguments,
        const SubprocessOptions &options,
        const SubprocessOutputSink &sink);
    ~PosixSubprocessBackend() override;

public: // implement SubprocessBackend
    [[nodiscard]] auto processId() const noexcept -> ProcessId override;
    [[nodiscard]] auto isRunning() -> bool override;
    [[nodiscard]] auto wait() -> SubprocessExitStatus override;
    [[nodiscard]] auto wait(time::TimeDelta timeout) -> std::optional<SubprocessExitStatus> override;
    void terminate() override;
    void kill() override;

private:
    /// Transfer a pipe to its draining thread after successful thread creation.
    void startReader(int &descriptor, bool error, const SubprocessOutputSink &sink);
    /// Poll or wait for native process termination using ``waitpid`` options.
    auto pollStatus(int options) -> bool;
    /// Convert and cache one native ``waitpid`` status.
    void storeStatus(int status);
    /// Join all active output drain threads.
    void joinReaders();
    /// Drain a pipe, delivering original bytes to its workflow.
    static void readCaptured(int descriptor, const std::shared_ptr<OutputState> &state) noexcept;

private:
    int _processId{-1};                              ///< Native child process identifier.
    std::optional<SubprocessExitStatus> _exitStatus; ///< Cached child exit status.
    std::shared_ptr<OutputState> _standardOutput;    ///< Optional standard output capture.
    std::shared_ptr<OutputState> _standardError;     ///< Optional standard error capture.
    std::thread _standardOutputReader;               ///< Standard output draining thread.
    std::thread _standardErrorReader;                ///< Standard error draining thread.
    std::mutex _waitMutex;                           ///< Serializes native wait calls.
};

}
