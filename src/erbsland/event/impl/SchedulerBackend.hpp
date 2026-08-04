// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EventTimer_fwd.hpp"
#include "SchedulerBackend_fwd.hpp"

#include "../EventBackend.hpp"
#include "../EventBackendTarget_fwd.hpp"
#include "../EventScheduler.hpp"

#include <memory>
#include <mutex>
#include <vector>

namespace erbsland::event::impl {

/// Backend for scheduled timer callbacks.
/// @tested{EventTimerTest EventBackendTest}
class SchedulerBackend final : public EventBackend, public EventScheduler {
    /// Stores a callback scheduled for a future time.
    struct DelayedInvocation {
        time::TimePoint dueTime; ///< Time when this callback shall be queued.
        EventCallback callback;  ///< Callback to execute on the target event loop.
    };

    /// Stores scheduler state shared with timer objects.
    struct State {
        mutable std::mutex mutex;                          ///< Protects backend state.
        EventBackendTargetWeakPtr target;                  ///< Target for scheduler events.
        std::vector<DelayedInvocation> delayedInvocations; ///< Strongly held delayed invocations.
        std::vector<impl::EventTimerWeakPtr> timers;       ///< Timers held weakly by this backend.
    };

public:
    /// Create a scheduler backend with empty shared state.
    SchedulerBackend();

    // defaults
    ~SchedulerBackend() override = default;

public: // implement EventBackend
    [[nodiscard]] auto backendId() const noexcept -> EventBackendId override;
    /// @param target The target for scheduler events.
    /// @param driver The unused native driver.
    void attach(EventBackendTargetWeakPtr target, EventLoopDriverWeakPtr driver) override;
    void poll(time::TimePoint now) override;
    [[nodiscard]] auto handleEvent(const Event &event) -> bool override;
    [[nodiscard]] auto nextWakeTime() const -> std::optional<time::TimePoint> override;

public: // implement EventScheduler
    void invokeAfter(time::TimeDelta delay, EventCallback callback) override;
    [[nodiscard]] auto createTimer(EventCallback callback) -> event::EventTimerPtr override;

public:
    /// Test if this scheduler has an attached target.
    /// @return `true` if a target is attached.
    [[nodiscard]] auto hasTarget() const noexcept -> bool;

private:
    /// Schedule a timer against shared backend state.
    static void schedule(const std::shared_ptr<State> &state, const impl::EventTimerPtr &timer);
    /// Wake the attached target after scheduler state changes.
    static void wakeAttachedTarget(const std::shared_ptr<State> &state) noexcept;

private:
    std::shared_ptr<State> _state; ///< Shared state weakly referenced by timers.
};

}
