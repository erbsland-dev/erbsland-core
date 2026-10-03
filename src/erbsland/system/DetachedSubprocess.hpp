// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "DetachedSubprocess_fwd.hpp"
#include "ProcessId.hpp"
#include "SubprocessOptions.hpp"

#include "../text/StringList.hpp"

#include <memory>

namespace erbsland::system {

/// Launch-only child handle; destruction does not stop the child.
/// @tested{SubprocessInteropTest}
class DetachedSubprocess final {
public:
    // defaults/deletions
    ~DetachedSubprocess() = default;
    DetachedSubprocess(const DetachedSubprocess &) = delete;
    auto operator=(const DetachedSubprocess &) -> DetachedSubprocess & = delete;

public: // factories
    /// Launch a child independently; an internal waiter prevents POSIX zombies.
    /// @param executable Direct executable path.
    /// @param arguments Arguments following argv[0].
    /// @param options Environment and stream settings; capture is prohibited.
    /// @return A shared launch identifier, without process control.
    /// @throws PlatformError If native launch fails.
    [[nodiscard]] static auto start(
        const path::Path &executable, const text::StringList &arguments = {}, const SubprocessOptions &options = {})
        -> DetachedSubprocessPtr;

public: // accessors
    /// Get the identifier assigned at launch.
    [[nodiscard]] auto processId() const noexcept -> ProcessId { return _processId; }

private:
    /// Retain an already assigned native identifier.
    /// @param processId The successful launch identifier.
    explicit DetachedSubprocess(ProcessId processId) : _processId{processId} {}

private:
    ProcessId _processId; ///< Identifier retained independently from native process cleanup.
};

}
