// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ProcessInfoData.hpp"
#include "SystemInfoBackend_fwd.hpp"

#include "../CpuArchitecture.hpp"
#include "../OperatingSystem.hpp"
#include "../ProcessId.hpp"

#include <cstdint>

namespace erbsland::system::impl {

/// Platform backend for process and machine information.
/// @tested{ProcessInfoTest SystemInfoTest SubprocessInteropTest}
class SystemInfoBackend {
public:
    // defaults/deletions
    virtual ~SystemInfoBackend() = default;
    SystemInfoBackend(const SystemInfoBackend &) = delete;
    SystemInfoBackend(SystemInfoBackend &&) = delete;
    auto operator=(const SystemInfoBackend &) -> SystemInfoBackend & = delete;
    auto operator=(SystemInfoBackend &&) -> SystemInfoBackend & = delete;

public:
    /// Get the current process identifier.
    [[nodiscard]] virtual auto currentProcessId() const noexcept -> ProcessId = 0;
    /// Load one coherent process snapshot.
    [[nodiscard]] virtual auto loadProcessInfo(ProcessId processId) const -> ProcessInfoData = 0;
    /// Get the current operating-system family.
    [[nodiscard]] virtual auto operatingSystem() const noexcept -> OperatingSystem = 0;
    /// Get the native host CPU architecture.
    [[nodiscard]] virtual auto cpuArchitecture() const noexcept -> CpuArchitecture = 0;
    /// Get the usable logical CPU count.
    [[nodiscard]] virtual auto logicalCpuCount() const noexcept -> std::uint32_t = 0;

protected:
    // defaults
    SystemInfoBackend() = default;
};

/// Access the process-wide system-information backend.
/// @tested{ProcessInfoTest SystemInfoTest}
[[nodiscard]] auto systemInfoBackend() noexcept -> SystemInfoBackend &;

/// Replace the system-information backend, or restore the platform default with null.
/// @tested{ProcessInfoTest SystemInfoTest}
void setSystemInfoBackend(SystemInfoBackendPtr backend) noexcept;

/// Create the default system-information backend for the current platform.
/// @tested{ProcessInfoTest SystemInfoTest}
[[nodiscard]] auto createSystemInfoBackend() -> SystemInfoBackendPtr;

}
