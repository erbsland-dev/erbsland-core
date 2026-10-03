// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "WorkerPool_fwd.hpp"

#include "impl/worker_pool/WorkerPoolState_fwd.hpp"

#include "../event/EventCallback.hpp"
#include "../event/impl/CommonEventEditor.hpp"

#include <exception>
#include <functional>

namespace erbsland::util {

/// Source-owned single-handler configuration for a worker pool's owner-loop notifications.
/// @seedoc{/reference/util/worker_pool}
/// @tested{WorkerPoolEventTest}
class WorkerPoolEventEditor final : public event::impl::CommonEventEditor {
    friend class WorkerPool;

public:
    using ErrorFn = std::function<void(std::exception_ptr)>; ///< An operational failure recipient.

public:                                                      // handlers
    /// Replace the fallback operational-error handler; empty rethrows on the owner loop.
    /// @param callback The replacement handler.
    auto onError(ErrorFn callback) -> WorkerPoolEventEditor &;
    /// Replace the fallback per-operation cancellation notification; empty ignores cancellation.
    /// @param callback The replacement handler.
    auto onCancelled(event::EventCallback callback) -> WorkerPoolEventEditor &;
    /// Replace the notification after work and pending outcomes drain.
    /// @param callback The replacement handler.
    auto onIdle(event::EventCallback callback) -> WorkerPoolEventEditor &;
    /// Replace the final notification after shutdown and outcome delivery.
    /// @param callback The replacement handler.
    auto onFinal(event::EventCallback callback) -> WorkerPoolEventEditor &;

private:
    /// Retain the common editor target and worker state.
    /// @param source The pool facade.
    /// @param owner The owner event loop.
    /// @param state The synchronized worker state.
    WorkerPoolEventEditor(event::EventSourcePtr source, event::EventsPtr owner, impl::WorkerPoolStatePtr state);
    /// Verify that handler editing runs on the owner loop.
    void verifyOwner() const;

private:
    impl::WorkerPoolStatePtr _state; ///< Shared scheduler state.
};

}
