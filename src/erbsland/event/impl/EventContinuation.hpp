#pragma once

#include "EventContinuation_fwd.hpp"
#include "EventLoop_fwd.hpp"
#include "SchedulerBackend_fwd.hpp"

#include "../EventCallback.hpp"
#include "../EventTarget_fwd.hpp"

#include "../../time/TimeDelta.hpp"

#include <atomic>
#include <memory>

namespace erbsland::event::impl {

/// Reserved owner-loop capacity for one outstanding completion independent of the ordinary event queue.
/// @tested{TlsBackgroundOperationTest}
class EventContinuation final : public std::enable_shared_from_this<EventContinuation> {
    friend class SchedulerBackend;
    /// Atomic delivery state shared by scheduling, cancellation, and owner-loop dispatch.
    enum class State { Idle, Scheduled, Queued, Cancelled };

public:
    /// Release admission after the operation and any queued callback release shared ownership.
    ~EventContinuation();
    /// Reserve one of the loop's 1024 continuation slots before accepting external work.
    [[nodiscard]] static auto create(const EventTargetPtr &events) -> EventContinuationPtr;
    /// Queue one completion; the reservation may be reused once that callback starts.
    void invoke(EventCallback callback);
    /// Schedule a completion through the existing timer backend while retaining reserved queue capacity.
    void invokeAfter(time::TimeDelta delay, EventCallback callback);
    /// Cancel queued delivery and release any scheduled timer's retained admission promptly.
    void cancel() noexcept;

private:
    /// Bind a previously reserved loop slot without retaining the loop.
    explicit EventContinuation(std::weak_ptr<EventLoop> loop);
    /// Transfer an idle or scheduled reservation into its one queued callback.
    void queue(EventCallback callback, State expected);

private:
    std::weak_ptr<EventLoop> _loop;         ///< Weak target; shutdown never waits for native work.
    std::atomic<State> _state{State::Idle}; ///< At most one scheduled or queued callback per admission.
};

}
