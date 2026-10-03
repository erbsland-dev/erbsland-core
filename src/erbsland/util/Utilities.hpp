// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "WorkerPool.hpp"

#include "../event/EventBackendId.hpp"
#include "../event/EventRegistry.hpp"

namespace erbsland::util {

/// Event-loop frontend for utility event sources.
/// @seedoc{/reference/util/worker_pool}
/// @tested{UtilitiesTest}
class Utilities {
public:
    // defaults
    virtual ~Utilities() = default;

public: // factories
    /// Get the utilities backend identifier.
    /// @return The built-in utilities backend identifier.
    [[nodiscard]] static constexpr auto backendId() noexcept -> event::EventBackendId {
        return event::id::UtilitiesBackend;
    }
    /// Create an open pool bound to this loop; call on its owner loop.
    /// @param options Concurrency and operation limits, copied into the pool.
    /// @return An independently owned pool with event delivery bound to this loop.
    /// @throws LogicFailure If called outside this frontend's owner loop.
    /// @throws EventQueueFullError If the owner cannot reserve completion delivery capacity.
    [[nodiscard]] virtual auto createWorkerPool(const WorkerPoolOptions &options = {}) -> WorkerPoolPtr = 0;
};

}
