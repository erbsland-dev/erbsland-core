// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CurrentEventsScope.hpp"
#include "EventContinuation_fwd.hpp"
#include "EventLoop_fwd.hpp"

#include "../EventBackend.hpp"
#include "../EventBackendId.hpp"
#include "../EventBackendTarget.hpp"
#include "../EventLoop.hpp"
#include "../EventLoopDriver.hpp"

#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <vector>

namespace erbsland::event::impl {

/// The implementation of the event loop.
/// @tested{EventLoopTest EventLoopOptionsTest ApplicationEventTest}
class EventLoop final : public event::EventLoop,
                        public event::EventBackendTarget,
                        public std::enable_shared_from_this<EventLoop> {
    friend class EventLoopRunScope;
    friend class EventContinuation;

public:
    /// Internal application-failure delivery callback.
    using FailureHandler = std::function<void(std::exception_ptr)>;

public:
    /// Create an event loop with the default driver.
    /// @param options Copied capacity and error policies. Policies must be valid.
    /// @param failureHandler Optional managed-application failure delivery. Must be set if a policy requires
    /// application delivery.
    explicit EventLoop(const EventLoopOptions &options = {}, FailureHandler failureHandler = {});
    /// Create an event loop with a supplied driver.
    /// @param driver The non-null native wait and wake driver.
    /// @param options Copied capacity and error policies. Policies must be valid and supported by the failure handler.
    /// @param failureHandler Optional managed-application failure delivery.
    explicit EventLoop(
        EventLoopDriverPtr driver, const EventLoopOptions &options = {}, FailureHandler failureHandler = {});
    /// Stop the current or next run for application failure without preventing later shutdown dispatch.
    void stopForFailure() noexcept;

public: // implement Events
    void post(Event event) override;
    void invoke(EventCallback callback) override;
    void invokeAfter(time::TimeDelta delay, EventCallback callback) override;

public: // implement EventBackendTarget
    auto postFromBackend(Event event) -> bool override;
    void wakeFromBackend() noexcept override;

public: // implement EventLoop
    void run() override;
    [[nodiscard]] auto runOnce() -> bool override;
    [[nodiscard]] auto runOnce(time::TimeDelta maximumWait) -> bool override;
    [[nodiscard]] auto runUntilIdle() -> std::size_t override;
    void stop() noexcept override;
    void quit() noexcept override;
    [[nodiscard]] auto isRunning() const noexcept -> bool override;
    [[nodiscard]] auto isQuitRequested() const noexcept -> bool override;
    [[nodiscard]] auto queuedEventCount() const noexcept -> std::size_t override;
    [[nodiscard]] auto hasError() const noexcept -> bool override;
    [[nodiscard]] auto takeError() noexcept -> std::exception_ptr override;
    void setErrorHandler(EventLoopErrorHandler handler) override;
    void registerBackend(EventBackendPtr backend) override;

private:
    [[nodiscard]] auto getBackend(EventBackendId backendId) -> EventBackend & override;

private:
    /// Register `backend` and return its stable reference.
    auto registerBackendInternal(EventBackendPtr backend) -> EventBackend &;
    /// Create the fundamental backend identified by `backendId`.
    [[nodiscard]] auto createFundamentalBackend(EventBackendId backendId) -> EventBackendPtr;
    /// Attach all registered backends to this event loop.
    void ensureBackendsAttached();
    /// Admit an ordinary event or reserved quit control according to the configured policy.
    auto postInternal(Event event) -> bool;
    /// Store and deliver an application failure, then quit this worker.
    void failApplication(std::exception_ptr error) noexcept;
    /// Enter dispatch and bind this loop to the calling thread.
    [[nodiscard]] auto beginDispatch() -> CurrentEventsScope;
    /// Finish dispatch and release the consumer identity.
    void finishDispatch() noexcept;
    /// Process events and backends once with an optional wait limit.
    [[nodiscard]] auto runOnceImpl(std::optional<time::TimeDelta> maximumWait) -> bool;
    /// Take the next queued event, if any.
    [[nodiscard]] auto takeNextEvent() -> std::optional<Event>;
    /// Remove all queued events.
    void clearQueuedEvents() noexcept;
    /// Process one queued event.
    void processEvent(const Event &event);
    /// Process one invocation event.
    void processInvocationEvent(const Event &event);
    /// Dispatch an event to its target backend.
    auto dispatchBackendEvent(const Event &event) -> bool;
    /// Store one callback error.
    void captureError(std::exception_ptr error) noexcept;
    /// Send one callback error to the configured handler.
    void handleError(std::exception_ptr error) noexcept;
    /// Test whether a stop request is pending.
    [[nodiscard]] auto isStopRequested() const noexcept -> bool;
    /// Snapshot raw backend pointers while holding the mutex.
    [[nodiscard]] auto backendSnapshot() const -> std::vector<EventBackend *>;
    /// Poll all backends at `now`.
    void pollBackends(time::TimePoint now);
    /// Get the next time a backend requests to wake.
    [[nodiscard]] auto nextBackendWakeTime() const -> std::optional<time::TimePoint>;
    /// Wait until an event-loop wake notification.
    void waitForWake();
    /// Wait for a wake notification or `waitTime`.
    void waitForWake(time::TimeDelta waitTime);

private:
    EventLoopOptions _options;              ///< Installed options.
    FailureHandler _failureHandler;         ///< Weak application failure delivery.
    std::condition_variable _queueSpace;    ///< Producer wait notification.
    std::thread::id _consumerThread;        ///< Active consumer identity.
    std::size_t _stopGeneration{0};         ///< Detect stop even across a subsequent restart.
    bool _failureStopRequested{false};      ///< Sticky stop for current or next run.
    mutable std::mutex _mutex;              ///< Protects loop state.
    EventLoopDriverPtr _driver;             ///< Native wait and wake driver.
    std::deque<Event> _queue;               ///< Immediate event queue.
    std::deque<Event> _continuations;       ///< Separately admitted completions, bounded by reservation count.
    std::size_t _continuationCount{};       ///< Live reserved slots, including native work not yet complete.
    bool _preferContinuation{true};         ///< Fair alternation with ordinary queued work.
    std::queue<std::exception_ptr> _errors; ///< Captured callback errors.
    std::vector<EventBackendPtr> _backends; ///< Registered backends.
    EventLoopErrorHandler _errorHandler;    ///< Optional event-loop error handler.
    bool _backendsAttached{false};          ///< True if backends are attached to this loop backend target.
    bool _running{false};                   ///< True while `run()` is active.
    bool _stopRequested{false};             ///< Stop flag for `run()`.
    bool _quitRequested{false};             ///< Quit flag for terminal loop shutdown.
    bool _quitEventQueued{false};           ///< True if the quit event was queued.
};

}
