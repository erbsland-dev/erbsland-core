// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CpuArchitecture.hpp"
#include "OperatingSystem.hpp"

#include <cstdint>

namespace erbsland::system::info {

/// Get the current operating-system family.
/// @tested{SystemInfoTest}
[[nodiscard]] auto operatingSystem() noexcept -> OperatingSystem;

/// Get the native host CPU architecture.
/// This is the host architecture, not the current executable's emulated architecture.
/// @tested{SystemInfoTest}
[[nodiscard]] auto cpuArchitecture() noexcept -> CpuArchitecture;

/// Get the usable logical CPU count as a thread-count hint.
/// The returned count is always at least one.
/// @tested{SystemInfoTest}
[[nodiscard]] auto logicalCpuCount() noexcept -> std::uint32_t;

}
