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
    /// Thread-safe retained output state for one native pipe.
    struct CaptureState final {
        mutable std::mutex mutex; ///< Protects captured bytes and the truncation flag.
        std::string bytes;        ///< Retained output prefix.
        std::size_t limit{};      ///< Maximum retained byte count.
        bool truncated{};         ///< Whether additional bytes were discarded.
    };

public:
    /// Launch one POSIX child with the requested environment and stream setup.
    PosixSubprocessBackend(
        const path::Path &executable, const text::StringList &arguments, const SubprocessOptions &options);
    ~PosixSubprocessBackend() override;

public: // implement SubprocessBackend
    [[nodiscard]] auto processId() const noexcept -> ProcessId override;
    [[nodiscard]] auto isRunning() -> bool override;
    [[nodiscard]] auto exitStatus() const noexcept -> const std::optional<SubprocessExitStatus> & override;
    [[nodiscard]] auto wait() -> SubprocessExitStatus override;
    [[nodiscard]] auto wait(time::TimeDelta timeout) -> std::optional<SubprocessExitStatus> override;
    void terminate() override;
    void kill() override;
    [[nodiscard]] auto standardOutput() const -> text::String override;
    [[nodiscard]] auto standardError() const -> text::String override;
    [[nodiscard]] auto wasStandardOutputTruncated() const noexcept -> bool override;
    [[nodiscard]] auto wasStandardErrorTruncated() const noexcept -> bool override;

private:
    /// Poll or wait for native process termination using ``waitpid`` options.
    auto pollStatus(int options) -> bool;
    /// Convert and cache one native ``waitpid`` status.
    void storeStatus(int status);
    /// Join all active output drain threads.
    void joinReaders();
    /// Drain a pipe, retaining only its configured prefix.
    static void readCaptured(int descriptor, const std::shared_ptr<CaptureState> &state) noexcept;
    /// Convert a thread-safe capture prefix to Core text.
    [[nodiscard]] static auto capturedText(const std::shared_ptr<CaptureState> &state) -> text::String;
    /// Read the thread-safe truncation flag.
    [[nodiscard]] static auto wasTruncated(const std::shared_ptr<CaptureState> &state) noexcept -> bool;

private:
    int _processId{-1};                              ///< Native child process identifier.
    std::optional<SubprocessExitStatus> _exitStatus; ///< Cached child exit status.
    std::shared_ptr<CaptureState> _standardOutput;   ///< Optional standard output capture.
    std::shared_ptr<CaptureState> _standardError;    ///< Optional standard error capture.
    std::thread _standardOutputReader;               ///< Standard output draining thread.
    std::thread _standardErrorReader;                ///< Standard error draining thread.
    std::mutex _waitMutex;                           ///< Serializes native wait calls.
};

}
