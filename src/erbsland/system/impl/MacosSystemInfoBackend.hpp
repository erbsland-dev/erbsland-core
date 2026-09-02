// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#ifndef __APPLE__
#error "MacosSystemInfoBackend.hpp is only available on macOS."
#endif

#include "SystemInfoBackend.hpp"

#include <cstdint>
#include <string_view>

namespace erbsland::system::impl {

/// macOS backend for process and machine information.
/// @tested{ProcessInfoTest SystemInfoTest SubprocessInteropTest}
class MacosSystemInfoBackend final : public SystemInfoBackend {
public:
    // defaults
    MacosSystemInfoBackend() = default;

public: // implement SystemInfoBackend
    [[nodiscard]] auto currentProcessId() const noexcept -> ProcessId override;
    [[nodiscard]] auto loadProcessInfo(ProcessId processId) const -> ProcessInfoData override;
    [[nodiscard]] auto operatingSystem() const noexcept -> OperatingSystem override;
    [[nodiscard]] auto cpuArchitecture() const noexcept -> CpuArchitecture override;
    [[nodiscard]] auto logicalCpuCount() const noexcept -> std::uint32_t override;

private:
    /// Convert a native machine name into a portable architecture.
    [[nodiscard]] static auto architectureFromName(std::string_view name) noexcept -> CpuArchitecture;
    /// Create a POSIX process-information error.
    [[nodiscard]] static auto error(text::String reason, int errorCode) -> ProcessInfoError;
    /// Load process information once, retrying when the native identity changes.
    [[nodiscard]] static auto loadNativeProcessInfo(std::uint64_t processId) -> ProcessInfoData;
};

}
