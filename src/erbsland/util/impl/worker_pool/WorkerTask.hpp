// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "WorkerTask_fwd.hpp"

#include "../../../event/EventCallback.hpp"
#include "../../StopToken.hpp"

#include <cstddef>
#include <exception>
#include <functional>

namespace erbsland::util::impl {

/// One admitted operation, with scheduling fields protected by its pool mutex.
/// @tested{WorkerPoolTest WorkerPoolBulkTest WorkerPoolEventTest}
class WorkerTask {
public:
    // defaults
    virtual ~WorkerTask() = default;

public: // interface
    /// Get the number of independently runnable items.
    [[nodiscard]] virtual auto itemCount() const noexcept -> std::size_t = 0;
    /// Execute one exclusively claimed item.
    /// @param index The item index.
    /// @param token The pool's cooperative stop token.
    virtual void execute(std::size_t index, StopToken token) = 0;
    /// Release callable/input captures and assemble a successful bulk result before publication.
    virtual void releaseWork() = 0;
    /// Publish a future result or produce the successful owner-loop invocation.
    /// @param error The committed operational error, or null for success.
    /// @return The successful event callback, or empty for future/error delivery.
    virtual auto publish(std::exception_ptr error) -> event::EventCallback = 0;

public:
    std::size_t next{0};       ///< Next unclaimed item.
    std::size_t running{0};    ///< Currently executing items.
    std::exception_ptr error;  ///< First failure or cancellation decision.
    bool finished{false};      ///< Result decision committed and operation removed from scheduling.
    bool eventDelivery{false}; ///< Deliver through the owner loop instead of a future.
    bool cancelled{false};     ///< The first terminal decision was cancellation.
    std::function<void(std::exception_ptr)> failed; ///< Optional per-operation error callback.
};

}
