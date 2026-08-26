// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AllocationMeasurement.hpp"

#include <cstddef>

namespace erbsland::profiling {

/// Scoped allocation measurement for the current worker thread.
/// @notest{Validated by profiler allocation smoke workloads.}
class AllocationScope final {
public:
    /// Start optional allocation tracking for the current thread.
    explicit AllocationScope(bool isEnabled = true) noexcept;
    /// Stop tracking if the result was not taken explicitly.
    ~AllocationScope();

    // defaults/deletions
    AllocationScope(const AllocationScope &) = delete;
    AllocationScope(AllocationScope &&) = delete;
    auto operator=(const AllocationScope &) -> AllocationScope & = delete;
    auto operator=(AllocationScope &&) -> AllocationScope & = delete;

public:
    /// Stop tracking and return the collected values.
    [[nodiscard]] auto finish() noexcept -> AllocationMeasurement;

public: // global allocation hooks
    /// Record one successful allocation from the global allocation hooks.
    static void recordAllocation(std::size_t size) noexcept;
    /// Record one deallocation from the global allocation hooks.
    static void recordDeallocation() noexcept;

private:
    /// Start tracking after resetting current-thread counters.
    static void start() noexcept;
    /// Stop tracking on the current thread.
    static void stop() noexcept;

private:
    static thread_local bool _isActive;                ///< Whether current-thread tracking is active.
    static thread_local AllocationMeasurement _result; ///< Current-thread allocation counters.
    bool _isEnabled{};                                 ///< Whether this scope requested tracking.
    bool _isFinished{};                                ///< Whether tracking was explicitly finished.
};

}
