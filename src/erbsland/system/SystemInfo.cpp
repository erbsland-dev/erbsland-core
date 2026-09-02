// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SystemInfo.hpp"

#include "impl/SystemInfoBackend.hpp"

namespace erbsland::system::info {

auto operatingSystem() noexcept -> OperatingSystem {
    return impl::systemInfoBackend().operatingSystem();
}

auto cpuArchitecture() noexcept -> CpuArchitecture {
    return impl::systemInfoBackend().cpuArchitecture();
}

auto logicalCpuCount() noexcept -> std::uint32_t {
    const auto result = impl::systemInfoBackend().logicalCpuCount();
    return result > 0U ? result : 1U;
}

}
