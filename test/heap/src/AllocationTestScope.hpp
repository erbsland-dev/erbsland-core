// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstddef>
#include <new>

namespace erbsland::test {

/// Count global C++ heap allocations on the current test thread within one explicit scope.
/// @tested{AllocationTestScopeTest UnicodeNormalizationAllocationTest}
class AllocationTestScope final {
public:
    /// Start counting allocations on the current thread.
    AllocationTestScope() noexcept;
    /// Stop counting if the result was not taken explicitly.
    ~AllocationTestScope();

    // defaults/deletions
    AllocationTestScope(const AllocationTestScope &) = delete;
    AllocationTestScope(AllocationTestScope &&) = delete;
    auto operator=(const AllocationTestScope &) -> AllocationTestScope & = delete;
    auto operator=(AllocationTestScope &&) -> AllocationTestScope & = delete;

public:
    /// Stop counting and return the number of observed allocations.
    [[nodiscard]] auto finish() noexcept -> std::size_t;

public: // global allocation hooks
    /// Allocate unaligned storage and record the successful allocation.
    [[nodiscard]] static auto allocate(std::size_t size) -> void *;
    /// Allocate aligned storage and record the successful allocation.
    [[nodiscard]] static auto allocate(std::size_t size, std::align_val_t alignment) -> void *;
    /// Deallocate aligned storage using the platform-matching operation.
    static void deallocate(void *pointer, std::align_val_t alignment) noexcept;

private:
    /// Record one successful allocation from a global allocation hook.
    static void recordAllocation() noexcept;

private:
    static thread_local bool _isActive;           ///< Whether allocation counting is active on this thread.
    static thread_local std::size_t _allocations; ///< Number of allocations in the current scope.
    bool _isFinished{};                           ///< Whether `finish()` was already called.
};

}
