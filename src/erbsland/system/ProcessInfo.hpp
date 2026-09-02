// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ProcessId.hpp"
#include "ProcessInfo_fwd.hpp"
#include "UserId.hpp"

#include "impl/ProcessInfoData.hpp"

#include "../path/Path.hpp"
#include "../time/DateTime.hpp"

namespace erbsland::system {

/// A cached snapshot of portable information about one process identifier.
/// Construction eagerly loads all attributes. The snapshot remains unchanged until `reload()` is called. Reloading
/// follows the numeric identifier, so compare `startTime()` values when process-identifier reuse matters.
/// @seedoc{/reference/system/system_services}
/// @tested{ProcessInfoTest SubprocessInteropTest}
class ProcessInfo final {
public:
    /// Create and load process information for the current process.
    ProcessInfo() noexcept;
    /// Create and load process information for the given identifier.
    /// An invalid or currently unused identifier produces a non-existing snapshot.
    /// @param processId The process identifier to inspect.
    explicit ProcessInfo(ProcessId processId) noexcept;

    // defaults
    ~ProcessInfo() = default;
    ProcessInfo(const ProcessInfo &) = default;
    ProcessInfo(ProcessInfo &&) noexcept = default;
    auto operator=(const ProcessInfo &) -> ProcessInfo & = default;
    auto operator=(ProcessInfo &&) noexcept -> ProcessInfo & = default;

public: // tests and attributes
    /// Get the process identifier represented by this snapshot.
    [[nodiscard]] auto processId() const noexcept -> ProcessId { return _processId; }
    /// Test if the platform reported a process for this identifier when the snapshot was loaded.
    [[nodiscard]] auto exists() const noexcept -> bool { return _data.exists; }
    /// Get the absolute executable image path, or an empty path when unavailable.
    [[nodiscard]] auto executablePath() const noexcept -> const path::Path & { return _data.executablePath; }
    /// Get the absolute executable image path.
    /// @throws PlatformError If the path was unavailable in this snapshot.
    [[nodiscard]] auto executablePathOrThrow() const -> path::Path;
    /// Get the parent process identifier, or an invalid identifier when unavailable.
    [[nodiscard]] auto parentProcessId() const noexcept -> ProcessId { return _data.parentProcessId; }
    /// Get the parent process identifier.
    /// @throws PlatformError If the identifier was unavailable in this snapshot.
    [[nodiscard]] auto parentProcessIdOrThrow() const -> ProcessId;
    /// Get the UTC process start time, or an invalid date/time when unavailable.
    [[nodiscard]] auto startTime() const noexcept -> time::DateTime { return _data.startTime; }
    /// Get the UTC process start time.
    /// @throws PlatformError If the start time was unavailable in this snapshot.
    [[nodiscard]] auto startTimeOrThrow() const -> time::DateTime;
    /// Get the process owner identifier, or an empty identifier when unavailable.
    [[nodiscard]] auto ownerId() const noexcept -> UserId { return _data.ownerId; }
    /// Get the process owner identifier.
    /// @throws PlatformError If the owner was unavailable in this snapshot.
    [[nodiscard]] auto ownerIdOrThrow() const -> UserId;

public: // reload
    /// Replace this snapshot with newly loaded information without throwing.
    void reload() noexcept;
    /// Replace this snapshot with newly loaded information.
    /// An unused identifier is a normal non-existing result.
    /// @throws PlatformError If the platform cannot determine the process snapshot.
    void reloadOrThrow();

private:
    /// Throw the best cached error for an unavailable attribute.
    [[noreturn]] void throwUnavailable(
        const impl::ProcessInfoError &attributeError, const text::String &fallbackReason) const;

private:
    ProcessId _processId;        ///< Process identifier followed by reloads.
    impl::ProcessInfoData _data; ///< Cached process snapshot.
};

}
