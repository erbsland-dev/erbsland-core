// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "WorkerDelivery.hpp"
#include "WorkerPoolState_fwd.hpp"
#include "WorkerTask.hpp"

#include "../../../event/EventLoop_fwd.hpp"
#include "../../../event/Events_fwd.hpp"
#include "../../../event/EventSource_fwd.hpp"
#include "../../../event/impl/EventContinuation_fwd.hpp"
#include "../../StopSource.hpp"
#include "../../WorkerPoolOptions.hpp"

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <vector>

namespace erbsland::util::impl {

/// Joinable worker scheduling and independently drained owner-loop delivery.
/// @tested{WorkerPoolTest WorkerPoolBulkTest WorkerPoolEventTest WorkerPoolFatalTest}
class WorkerPoolState final : public std::enable_shared_from_this<WorkerPoolState> {
public:
    /// Configure an independently owned worker domain.
    /// @param options Immutable concurrency and admission settings.
    explicit WorkerPoolState(const WorkerPoolOptions &options);
    /// Drain and join any remaining workers.
    ~WorkerPoolState();

public: // lifecycle
    /// Create workers; unwind and join any partially created set on failure.
    void start();
    /// Close admission and finish accepted operations.
    void quit() noexcept;
    /// Close admission and select cancellation for unfinished operations.
    void stop() noexcept;
    /// Wait for all work captures and future publications, excluding event delivery.
    void wait();
    /// Wait for worker exits without closing admission.
    void join();
    /// Suppress event delivery, drain work, and join workers.
    void shutdown() noexcept;
    /// Get the effective fixed concurrency.
    [[nodiscard]] auto workerCount() const noexcept -> unsigned int { return _workerCount; }
    /// Test whether this pool can admit more operations.
    [[nodiscard]] auto isAccepting() const noexcept -> bool;
    /// Get the shared cooperative token.
    [[nodiscard]] auto stopToken() const noexcept -> StopToken { return _stopSource.stopToken(); }

public: // work
    /// Admit an operation without waiting for capacity.
    /// @param task The prepared operation with its result recipient configured.
    void enqueue(WorkerTaskPtr task);

public: // events
    /// Reserve delivery capacity and attach one event owner.
    /// @param owner The current owner loop.
    /// @param source The weak pool facade used to guard dispatch lifetime.
    void bind(const event::EventsPtr &owner, event::EventSourceWeakPtr source);
    /// Schedule a reserved wake when delivery or lifecycle notifications are pending.
    void kick();
    /// Dispatch one outcome or lifecycle notification, rearming before invoking user code.
    void dispatch();
    /// Discard event outcomes and release admission when delivery is no longer possible.
    void discardEvents() noexcept;
    /// Replace the pool fallback error handler on the owner loop.
    /// @param callback The replacement, or empty to rethrow on the owner loop.
    void setError(std::function<void(std::exception_ptr)> callback);
    /// Replace the cancellation notification on the owner loop.
    /// @param callback The replacement callback.
    void setCancelled(event::EventCallback callback);
    /// Replace the idle notification on the owner loop.
    /// @param callback The replacement callback.
    void setIdle(event::EventCallback callback);
    /// Replace the final notification on the owner loop.
    /// @param callback The replacement callback.
    void setFinal(event::EventCallback callback);

private:
    /// Resolve automatic concurrency to a positive worker count.
    static auto resolveWorkerCount(unsigned int count) noexcept -> unsigned int;
    /// Execute work until closed and fully drained.
    void run();
    /// Release worker captures, publish a committed result, then release working admission.
    void finish(WorkerTaskPtr task);
    /// Reject barriers on one of this pool's own workers.
    void verifyExternalThread() const;

private:
    static thread_local WorkerPoolState *_currentPool; ///< Detect self-wait and self-destruction.
    const unsigned int _workerCount;                   ///< Effective fixed concurrency.
    const std::size_t _maximumPending;                 ///< Admission ceiling; zero means unlimited.
    mutable std::mutex _mutex;                         ///< Scheduling and mailbox state.
    std::mutex _joinMutex;                             ///< Serialize native joins without holding scheduling locks.
    std::condition_variable _condition;                ///< Work availability, draining, and shutdown.
    std::vector<std::thread> _workers;                 ///< Joinable workers owned by this pool.
    std::deque<WorkerTaskPtr> _queue;                  ///< At most one runnable entry per operation.
    std::unordered_set<WorkerTaskPtr> _tasks;          ///< Uncommitted operations, including running items.
    StopSource _stopSource;                            ///< Pool-wide stop request.
    std::size_t _working{0};                           ///< Operations whose captures/publication are not finished.
    std::size_t _pending{0};                           ///< Working operations plus outcomes awaiting event delivery.
    unsigned int _liveWorkers{0};                      ///< Workers that have not left the scheduler.
    bool _closed{false};                               ///< Admission is permanently closed.
    bool _hadWork{false};                              ///< Whether idle notification is meaningful.
    bool _idleNotified{true};                          ///< Current idle transition was observed.
    bool _finalNotified{false};                        ///< Terminal notification was observed.
    bool _eventsEnabled{false};                        ///< Owner binding completed.
    bool _eventsDiscarded{false};                      ///< Delivery has permanently ended.
    bool _wakePending{false};                          ///< One reserved wake is queued or being posted.
    event::impl::EventContinuationPtr _continuation;   ///< One reserved owner-loop slot.
    std::weak_ptr<event::EventLoop> _owner;            ///< Owner used for shutdown admission checks.
    event::EventSourceWeakPtr _source;                 ///< No ownership cycle with the public pool.
    std::deque<WorkerDelivery> _deliveries;            ///< Committed operation outcomes.
    std::function<void(std::exception_ptr)> _onError;  ///< Owner-loop failure fallback.
    event::EventCallback _onCancelled;                 ///< Owner-loop cancellation fallback.
    event::EventCallback _onIdle;                      ///< Owner-loop idle transition.
    event::EventCallback _onFinal;                     ///< Owner-loop terminal notification.
};

}
