// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstddef>
#include <cstdint>

namespace app::charset {

/// Allocation counters collected for one worker thread.
/// @notest{Validated by the character-set profiling smoke workload.}
struct AllocationMeasurement {
    std::uint64_t allocations{};    ///< Successful ordinary and aligned allocations.
    std::uint64_t allocatedBytes{}; ///< Requested bytes for successful allocations.
    std::uint64_t deallocations{};  ///< Deallocation calls while tracking was active.
};

/// Thread-local allocation tracking for the measured operation loop.
/// @notest{Validated by the character-set profiling smoke workload.}
class AllocationTracker final {
public:
    /// Start a fresh measurement on the current thread.
    static void start() noexcept;
    /// Stop and return the measurement on the current thread.
    [[nodiscard]] static auto finish() noexcept -> AllocationMeasurement;
    /// Record one successful allocation from the global allocation hooks.
    static void recordAllocation(std::size_t size) noexcept;
    /// Record one deallocation from the global allocation hooks.
    static void recordDeallocation() noexcept;

private:
    static thread_local bool _isActive;
    static thread_local AllocationMeasurement _measurement;
};

/// Scoped allocation measurement that safely disables tracking on exceptions.
/// @notest{Validated by the character-set profiling smoke workload.}
class AllocationScope final {
public:
    /// Optionally start measuring allocations on the current thread.
    explicit AllocationScope(const bool isEnabled = true) noexcept : _isEnabled{isEnabled} {
        if (_isEnabled) {
            AllocationTracker::start();
        }
    }
    /// Stop measuring when the result was not taken explicitly.
    ~AllocationScope() {
        if (_isEnabled && !_isFinished) {
            static_cast<void>(AllocationTracker::finish());
        }
    }

    // deletions
    AllocationScope(const AllocationScope &) = delete;
    AllocationScope(AllocationScope &&) = delete;
    auto operator=(const AllocationScope &) -> AllocationScope & = delete;
    auto operator=(AllocationScope &&) -> AllocationScope & = delete;

public:
    /// Stop tracking and return the collected values.
    [[nodiscard]] auto finish() noexcept -> AllocationMeasurement {
        _isFinished = true;
        return _isEnabled ? AllocationTracker::finish() : AllocationMeasurement{};
    }

private:
    bool _isEnabled{};  ///< Whether allocation tracking was requested.
    bool _isFinished{}; ///< Whether the result was already taken.
};

}
