// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::profiling {

/// Allocation counters collected for one measured worker interval.
struct AllocationMeasurement {
    std::uint64_t allocations{};    ///< Successful ordinary and aligned allocations.
    std::uint64_t allocatedBytes{}; ///< Requested bytes for successful allocations.
    std::uint64_t deallocations{};  ///< Deallocation calls while tracking was active.
};

}
