// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <cstddef>

namespace erbsland::util {

/// Worker concurrency and operation admission limits, copied when a pool is created.
/// @tested{WorkerPoolTest}
class WorkerPoolOptions final {
public: // accessors
    /// Get the worker count; zero selects the hardware concurrency, with a minimum of one.
    [[nodiscard]] auto workerCount() const noexcept -> unsigned int { return _workerCount; }
    /// Set the worker count; zero selects automatic concurrency.
    auto setWorkerCount(unsigned int value) noexcept -> WorkerPoolOptions & {
        _workerCount = value;
        return *this;
    }
    /// Get the pending-operation limit; zero means unlimited.
    [[nodiscard]] auto maximumPendingTasks() const noexcept -> std::size_t { return _maximumPendingTasks; }
    /// Set the limit including results awaiting event delivery; a bulk operation counts once.
    auto setMaximumPendingTasks(std::size_t value) noexcept -> WorkerPoolOptions & {
        _maximumPendingTasks = value;
        return *this;
    }

private:
    unsigned int _workerCount{0};        ///< Zero selects automatic concurrency.
    std::size_t _maximumPendingTasks{0}; ///< Zero allows unlimited pending operations.
};

}
