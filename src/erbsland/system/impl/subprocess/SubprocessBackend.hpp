// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SubprocessBackend_fwd.hpp"
#include "SubprocessOutputSink.hpp"

#include "../../../path/Path.hpp"
#include "../../../text/String.hpp"
#include "../../../text/StringList.hpp"
#include "../../../time/TimeDelta.hpp"
#include "../../ProcessId.hpp"
#include "../../SubprocessExitStatus.hpp"
#include "../../SubprocessOptions_fwd.hpp"

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
    /// Get the child process identifier.
    [[nodiscard]] virtual auto processId() const noexcept -> ProcessId = 0;
    /// Test if the child is still running and update cached exit state.
    [[nodiscard]] virtual auto isRunning() -> bool = 0;
    /// Wait until the child exits.
    virtual auto wait() -> SubprocessExitStatus = 0;
    /// Wait up to the given timeout for the child to exit.
    /// @param timeout Non-negative native process wait budget. Zero polls once. After native exit,
    /// joining output readers may exceed this budget while inherited pipe writers remain open.
    /// @return Cached status after exit and pipe drainage, or no value if the process wait expires.
    /// @throws PlatformError If the native wait fails.
    [[nodiscard]] virtual auto wait(time::TimeDelta timeout) -> std::optional<SubprocessExitStatus> = 0;
    /// Request termination using the platform's regular process-termination mechanism.
    virtual void terminate() = 0;
    /// Force the child to terminate.
    virtual void kill() = 0;

public: // factories
    /// Launch a child using the backend for the current platform.
    /// @param executable Direct executable path.
    /// @param arguments Child argument values.
    /// @param options Validated native launch settings.
    /// @param sink Internal raw-byte and read-error delivery, or empty for detached launches.
    /// @return The owning native process backend.
    /// @throws PlatformError If native process creation fails.
    [[nodiscard]] static auto create(
        const path::Path &executable,
        const text::StringList &arguments,
        const SubprocessOptions &options,
        const SubprocessOutputSink &sink = {}) -> SubprocessBackendPtr;

protected:
    // defaults
    SubprocessBackend() = default;
};

}
