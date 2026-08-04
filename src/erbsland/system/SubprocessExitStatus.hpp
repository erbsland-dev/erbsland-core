// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>
#include <optional>

namespace erbsland::system {

/// Describes how an owned subprocess exited.
/// POSIX signal termination has no direct Windows equivalent. On Windows, native termination is represented by the
/// process exit code and `terminationSignal()` remains empty.
/// @seedoc{/reference/system/subprocess}
/// @tested{SubprocessInteropTest}
class SubprocessExitStatus final {
public:
    /// Create a status for a process that returned an exit code.
    /// @param exitCode The process exit code.
    [[nodiscard]] static constexpr auto exited(const std::int32_t exitCode) noexcept -> SubprocessExitStatus {
        return SubprocessExitStatus{exitCode, std::nullopt};
    }
    /// Create a status for a process terminated by a POSIX signal.
    /// @param signal The positive native signal number.
    [[nodiscard]] static constexpr auto signaled(const std::int32_t signal) noexcept -> SubprocessExitStatus {
        return SubprocessExitStatus{std::nullopt, signal};
    }

public: // tests
    /// Test if the process returned an exit code.
    [[nodiscard]] constexpr auto hasExited() const noexcept -> bool { return _exitCode.has_value(); }
    /// Test if the process was terminated by a POSIX signal.
    [[nodiscard]] constexpr auto wasSignaled() const noexcept -> bool { return _terminationSignal.has_value(); }
    /// Test if the process returned exit code zero.
    [[nodiscard]] constexpr auto isSuccess() const noexcept -> bool { return _exitCode == 0; }

public: // accessors
    /// Get the process exit code, if the platform reported one.
    [[nodiscard]] constexpr auto exitCode() const noexcept -> const std::optional<std::int32_t> & { return _exitCode; }
    /// Get the POSIX termination signal, if the platform reported one.
    [[nodiscard]] constexpr auto terminationSignal() const noexcept -> const std::optional<std::int32_t> & {
        return _terminationSignal;
    }

private:
    /// Create an exit status from its mutually exclusive native representations.
    constexpr SubprocessExitStatus(
        std::optional<std::int32_t> exitCode, std::optional<std::int32_t> terminationSignal) noexcept :
        _exitCode{exitCode}, _terminationSignal{terminationSignal} {}

private:
    std::optional<std::int32_t> _exitCode;          ///< Native exit code when the process exited normally.
    std::optional<std::int32_t> _terminationSignal; ///< POSIX signal when the process was terminated by a signal.
};

}
