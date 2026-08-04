// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SubprocessBackend_fwd.hpp"

#include "../SubprocessExitStatus.hpp"
#include "../SubprocessOptions_fwd.hpp"

#include "../../path/Path.hpp"
#include "../../text/String.hpp"
#include "../../text/StringList.hpp"
#include "../../time/TimeDelta.hpp"

#include <optional>

namespace erbsland::system::impl {

/// Native backend for one owned subprocess.
/// @tested{SubprocessInteropTest}
class SubprocessBackend {
public:
    // defaults/deletions
    virtual ~SubprocessBackend() = default;
    SubprocessBackend(const SubprocessBackend &) = delete;
    SubprocessBackend(SubprocessBackend &&) = delete;
    auto operator=(const SubprocessBackend &) -> SubprocessBackend & = delete;
    auto operator=(SubprocessBackend &&) -> SubprocessBackend & = delete;

public:
    /// Test if the child is still running and update cached exit state.
    [[nodiscard]] virtual auto isRunning() -> bool = 0;
    /// Get the cached child exit status.
    [[nodiscard]] virtual auto exitStatus() const noexcept -> const std::optional<SubprocessExitStatus> & = 0;
    /// Wait until the child exits.
    virtual auto wait() -> SubprocessExitStatus = 0;
    /// Wait up to the given timeout for the child to exit.
    [[nodiscard]] virtual auto wait(time::TimeDelta timeout) -> std::optional<SubprocessExitStatus> = 0;
    /// Request termination using the platform's regular process-termination mechanism.
    virtual void terminate() = 0;
    /// Force the child to terminate.
    virtual void kill() = 0;
    /// Get the captured standard output prefix.
    [[nodiscard]] virtual auto standardOutput() const -> text::String = 0;
    /// Get the captured standard error prefix.
    [[nodiscard]] virtual auto standardError() const -> text::String = 0;
    /// Test if captured standard output was truncated.
    [[nodiscard]] virtual auto wasStandardOutputTruncated() const noexcept -> bool = 0;
    /// Test if captured standard error was truncated.
    [[nodiscard]] virtual auto wasStandardErrorTruncated() const noexcept -> bool = 0;

protected:
    // defaults
    SubprocessBackend() = default;
};

/// Launch a child using the backend for the current platform.
/// @tested{SubprocessInteropTest}
[[nodiscard]] auto createSubprocessBackend(
    const path::Path &executable, const text::StringList &arguments, const SubprocessOptions &options)
    -> SubprocessBackendPtr;

}
