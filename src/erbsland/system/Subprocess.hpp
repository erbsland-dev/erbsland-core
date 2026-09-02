// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ProcessId.hpp"
#include "Subprocess_fwd.hpp"
#include "SubprocessExitStatus.hpp"
#include "SubprocessOptions.hpp"

#include "impl/SubprocessBackend_fwd.hpp"

#include "../path/Path.hpp"
#include "../text/String.hpp"
#include "../text/StringList.hpp"
#include "../time/TimeDelta.hpp"

#include <optional>

namespace erbsland::system {

/// Owns and controls one directly launched operating-system child process.
/// No method invokes a command shell. Destruction of a running owned process requests termination, waits briefly, then
/// forcefully terminates and reaps it. Use `startDetached()` for deliberate launch-and-forget behavior.
/// @seedoc{/reference/system/system_services}
/// @tested{SubprocessInteropTest}
class Subprocess final {
public:
    /// Destroy the subprocess owner and ensure a running child is terminated and reaped.
    ~Subprocess();
    /// Move ownership of a subprocess.
    Subprocess(Subprocess &&other) noexcept;
    /// Replace this subprocess owner by moving another owner into it.
    auto operator=(Subprocess &&other) noexcept -> Subprocess &;

    // defaults/deletions
    Subprocess() = delete;
    Subprocess(const Subprocess &) = delete;
    auto operator=(const Subprocess &) -> Subprocess & = delete;

public: // factories
    /// Launch an owned subprocess.
    /// @param executable A non-empty valid executable path. No `PATH` lookup or shell interpretation is performed.
    /// @param arguments Arguments following `argv[0]` in the child command line.
    /// @param options Child environment and standard-stream options.
    /// @return The owning subprocess object.
    /// @throws err::ParameterError If a path, argument, environment entry, or option is invalid.
    /// @throws PlatformError If native process creation fails.
    [[nodiscard]] static auto start(
        const path::Path &executable, const text::StringList &arguments = {}, const SubprocessOptions &options = {})
        -> Subprocess;
    /// Launch a subprocess without retaining ownership or process control.
    /// Output capture is not permitted for detached children. An internal waiter prevents POSIX zombies.
    /// @param executable A non-empty valid executable path. No `PATH` lookup or shell interpretation is performed.
    /// @param arguments Arguments following `argv[0]` in the child command line.
    /// @param options Child environment and standard-stream options.
    /// @throws err::ParameterError If a path, argument, environment entry, or option is invalid.
    /// @throws PlatformError If native process creation fails.
    static void startDetached(
        const path::Path &executable, const text::StringList &arguments = {}, const SubprocessOptions &options = {});

public: // lifecycle
    /// Get the identifier assigned to the child process.
    /// The identifier remains valid after exit; a moved-from subprocess returns an invalid identifier.
    [[nodiscard]] auto processId() const noexcept -> ProcessId;
    /// Test if the child is still running and update cached exit state.
    [[nodiscard]] auto isRunning() -> bool;
    /// Get the cached child exit status, if it has been observed.
    [[nodiscard]] auto exitStatus() const noexcept -> const std::optional<SubprocessExitStatus> &;
    /// Wait until the child exits and return its status.
    /// @throws PlatformError If the native wait operation fails.
    [[nodiscard]] auto wait() -> SubprocessExitStatus;
    /// Wait up to the given timeout for the child to exit.
    /// @param timeout A non-negative timeout.
    /// @return The exit status, or no value if the timeout expired.
    /// @throws err::ParameterError If `timeout` is negative.
    /// @throws PlatformError If the native wait operation fails.
    [[nodiscard]] auto wait(time::TimeDelta timeout) -> std::optional<SubprocessExitStatus>;
    /// Request child termination using the platform's regular termination mechanism.
    /// @throws PlatformError If the native operation fails.
    void terminate();
    /// Force the child to terminate.
    /// @throws PlatformError If the native operation fails.
    void kill();

public: // captured output
    /// Get the currently captured standard output prefix.
    [[nodiscard]] auto standardOutput() const -> text::String;
    /// Get the currently captured standard error prefix.
    /// This is empty when standard error was merged into standard output.
    [[nodiscard]] auto standardError() const -> text::String;
    /// Test if captured standard output exceeded its configured limit.
    [[nodiscard]] auto wasStandardOutputTruncated() const noexcept -> bool;
    /// Test if captured standard error exceeded its configured limit.
    [[nodiscard]] auto wasStandardErrorTruncated() const noexcept -> bool;

private:
    /// Create an owner around a newly launched native backend.
    explicit Subprocess(impl::SubprocessBackendPtr backend) noexcept;
    /// Terminate and reap the owned child without throwing.
    void cleanup() noexcept;
    /// Validate portable launch inputs before entering a native backend.
    static void validate(
        const path::Path &executable,
        const text::StringList &arguments,
        const SubprocessOptions &options,
        bool detached);

private:
    impl::SubprocessBackendPtr _backend; ///< Owned native process backend.
};

}
