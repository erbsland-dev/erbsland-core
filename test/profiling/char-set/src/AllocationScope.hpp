// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AllocationMeasurement.hpp"

#include <cstddef>

namespace app::charset {

/// Scoped allocation measurement for the current worker thread.
/// @notest{Validated by the character-set profiling smoke workload.}
class AllocationScope final {
public:
    /// Optionally start measuring allocations on the current thread.
    explicit AllocationScope(bool isEnabled = true) noexcept;
    /// Stop measuring when the result was not taken explicitly.
    ~AllocationScope();

    // defaults/deletions
    AllocationScope(const AllocationScope &) = delete;
    AllocationScope(AllocationScope &&) = delete;
    /// Copy assignment is disabled.
    auto operator=(const AllocationScope &) -> AllocationScope & = delete;
    /// Move assignment is disabled.
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
    /// Start allocation measurement for the current scope.
    static void start() noexcept;
    /// Stop allocation measurement for the current scope.
    static void stop() noexcept;

private:
    static thread_local bool _isActive;                ///< Whether the current thread is tracking allocations.
    static thread_local AllocationMeasurement _result; ///< Counters for the current thread.
    bool _isEnabled{};                                 ///< Whether allocation tracking was requested.
    bool _isFinished{};                                ///< Whether the result was already taken.
};

}
