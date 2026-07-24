// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EventLoop_fwd.hpp"

#include "../EventBackend.hpp"
#include "../EventBackendId.hpp"
#include "../EventBackendTarget.hpp"
#include "../EventLoop.hpp"
#include "../EventLoopDriver.hpp"

#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <vector>

namespace erbsland::event::impl {

/// The implementation of the event loop.
class EventLoop final : public event::EventLoop,
                        public event::EventBackendTarget,
                        public std::enable_shared_from_this<EventLoop> {
public:
    EventLoop();
    explicit EventLoop(EventLoopDriverPtr driver);

public: // implement Events
    void post(Event event) override;
    void invoke(EventCallback callback) override;
    void invokeAfter(time::TimeDelta delay, EventCallback callback) override;

public: // implement EventBackendTarget
    void postFromBackend(Event event) override;
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
    [[nodiscard]] auto hasError() const noexcept -> bool override;
    [[nodiscard]] auto takeError() noexcept -> std::exception_ptr override;
    void setErrorHandler(EventLoopErrorHandler handler) override;
    void registerBackend(EventBackendPtr backend) override;

private:
    [[nodiscard]] auto getBackend(EventBackendId backendId) -> EventBackend & override;

private:
    [[nodiscard]] auto registerBackendInternal(EventBackendPtr backend) -> EventBackend &;
    [[nodiscard]] auto createFundamentalBackend(EventBackendId backendId) -> EventBackendPtr;
    void ensureBackendsAttached();
    void postInternal(Event event, bool allowAfterQuit);
    [[nodiscard]] auto runOnceImpl(std::optional<time::TimeDelta> maximumWait) -> bool;
    [[nodiscard]] auto takeNextEvent() -> std::optional<Event>;
    void clearQueuedEvents() noexcept;
    void processEvent(const Event &event);
    void processInvocationEvent(const Event &event);
    [[nodiscard]] auto dispatchBackendEvent(const Event &event) -> bool;
    void captureError(std::exception_ptr error) noexcept;
    void handleError(std::exception_ptr error) noexcept;
    [[nodiscard]] auto isStopRequested() const noexcept -> bool;
    [[nodiscard]] auto backendSnapshot() const -> std::vector<EventBackend *>;
    void pollBackends(time::TimePoint now);
    [[nodiscard]] auto nextBackendWakeTime() const -> std::optional<time::TimePoint>;
    void waitForWake();
    void waitForWake(time::TimeDelta waitTime);

private:
    mutable std::mutex _mutex;              ///< Protects loop state.
    EventLoopDriverPtr _driver;             ///< Native wait and wake driver.
    std::deque<Event> _queue;               ///< Immediate event queue.
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
