// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#ifndef __linux__
#error "LinuxSystemInfoBackend.hpp is only available on Linux."
#endif

#include "SystemInfoBackend.hpp"

#include "../../path/Path_fwd.hpp"
#include "../../text/String_fwd.hpp"

#include <cstdint>
#include <optional>

namespace erbsland::system::impl {

/// Linux backend for process and machine information.
/// @tested{ProcessInfoTest SystemInfoTest SubprocessInteropTest}
class LinuxSystemInfoBackend final : public SystemInfoBackend {
private:
    /// Native process identity and core attributes from `/proc/<pid>/stat`.
    struct NativeProcessData final {
        std::uint64_t parentProcessId{}; ///< Native parent process identifier.
        std::uint64_t startTicks{};      ///< Start time in clock ticks after boot.
    };

public:
    // defaults
    LinuxSystemInfoBackend() = default;

public: // implement SystemInfoBackend
    [[nodiscard]] auto currentProcessId() const noexcept -> ProcessId override;
    [[nodiscard]] auto loadProcessInfo(ProcessId processId) const -> ProcessInfoData override;
    [[nodiscard]] auto operatingSystem() const noexcept -> OperatingSystem override;
    [[nodiscard]] auto cpuArchitecture() const noexcept -> CpuArchitecture override;
    [[nodiscard]] auto logicalCpuCount() const noexcept -> std::uint32_t override;

private:
    /// Create a POSIX process-information error.
    [[nodiscard]] static auto error(text::String reason, int errorCode) -> ProcessInfoError;
    /// Assemble one path in the Linux process filesystem.
    [[nodiscard]] static auto processFilePath(std::uint64_t processId, const text::String &name) -> path::Path;
    /// Read a bounded process file as text and retain any path diagnostic.
    [[nodiscard]] static auto readText(const path::Path &path, ProcessInfoError &readError)
        -> std::optional<text::String>;
    /// Resolve a process path and retain any path diagnostic.
    [[nodiscard]] static auto resolvePath(const path::Path &path, ProcessInfoError &resolveError) -> path::Path;
    /// Test if a path operation reports a missing process file.
    [[nodiscard]] static auto isNotFound(const ProcessInfoError &operationError) noexcept -> bool;
    /// Parse parent and start-time fields from a process stat record.
    [[nodiscard]] static auto parseProcessStat(const text::String &text) -> std::optional<NativeProcessData>;
    /// Parse the effective user identifier from a process status record.
    [[nodiscard]] static auto parseEffectiveUserId(const text::String &text) -> std::optional<std::uint64_t>;
    /// Read the system boot time in POSIX seconds and retain any failure diagnostic.
    [[nodiscard]] static auto bootTimeSeconds(ProcessInfoError &operationError) -> std::optional<std::uint64_t>;
    /// Convert native process start ticks to UTC.
    [[nodiscard]] static auto processStartTime(std::uint64_t startTicks, ProcessInfoError &operationError)
        -> time::DateTime;
    /// Load process information once, retrying when the native identity changes.
    [[nodiscard]] static auto loadNativeProcessInfo(std::uint64_t processId) -> ProcessInfoData;
};

}
