// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#ifndef _WIN32
#error "WindowsSystemInfoBackend.hpp is only available on Windows."
#endif

#include "SystemInfoBackend.hpp"

#include "../../core/impl/WindowsApi.hpp"

#include <tlhelp32.h>

#include <cstddef>
#include <cstdint>
#include <optional>

namespace erbsland::system::impl {

/// Windows backend for process and machine information.
/// @tested{ProcessInfoTest SystemInfoTest SubprocessInteropTest}
class WindowsSystemInfoBackend final : public SystemInfoBackend {
public:
    // defaults
    WindowsSystemInfoBackend() = default;

public: // implement SystemInfoBackend
    [[nodiscard]] auto currentProcessId() const noexcept -> ProcessId override;
    [[nodiscard]] auto loadProcessInfo(ProcessId processId) const -> ProcessInfoData override;
    [[nodiscard]] auto operatingSystem() const noexcept -> OperatingSystem override;
    [[nodiscard]] auto cpuArchitecture() const noexcept -> CpuArchitecture override;
    [[nodiscard]] auto logicalCpuCount() const noexcept -> std::uint32_t override;

private:
    /// Create a Windows process-information error.
    [[nodiscard]] static auto error(text::String reason, DWORD errorCode) -> ProcessInfoError;
    /// Mark every attribute unavailable with the given error.
    static void setAttributeErrors(ProcessInfoData &data, const ProcessInfoError &queryError);
    /// Find the process snapshot entry for an identifier.
    [[nodiscard]] static auto processEntry(DWORD processId, DWORD &errorCode) -> std::optional<PROCESSENTRY32W>;
    /// Convert a Windows processor-machine value into a portable architecture.
    [[nodiscard]] static auto architectureFromMachine(USHORT machine) noexcept -> CpuArchitecture;
    /// Load process information from an opened process handle.
    [[nodiscard]] static auto loadOpenProcessInfo(DWORD processId, HANDLE process) -> ProcessInfoData;
};

}
