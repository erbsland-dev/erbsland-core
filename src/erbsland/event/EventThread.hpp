// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EventDriver.hpp"
#include "EventScheduler.hpp"
#include "StopToken.hpp"

#include "../core/Application_fwd.hpp"

#include <condition_variable>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>

namespace erbsland::event {

class EventThread;
using EventThreadPtr = std::shared_ptr<EventThread>;

/// A thread that executes queued invocations via the UI event system.
/// All public methods are thread-safe.
class EventThread final : public std::enable_shared_from_this<EventThread> {
    friend class core::Application;

public:
    /// Duration type used by the event thread API.
    using milliseconds = std::chrono::milliseconds;
    /// Callback type for invocations without stop support.
    using InvocationFn = std::function<void()>;
    /// Callback type for invocations that can observe cooperative stop requests.
    using StoppableInvocationFn = std::function<void(StopToken)>;

private:
    /// Protected tag for controlled construction via `create()`.
    struct ProtectedTag {};

public:
    /// Create and start a new event thread.
    /// @return The started event thread.
    [[nodiscard]] static auto create() -> EventThreadPtr;

public:
    /// Create an event thread instance.
    /// @param protectedTag Internal construction tag.
    explicit EventThread(ProtectedTag protectedTag) noexcept;
    ~EventThread() = default;

    EventThread(const EventThread &) = delete;
    EventThread(EventThread &&) = delete;
    auto operator=(const EventThread &) -> EventThread & = delete;
    auto operator=(EventThread &&) -> EventThread & = delete;

public:
    /// Queue one invocation on the event thread.
    /// If the thread is already stopping or finished, this call is ignored.
    /// @param fn The callback to execute.
    void invoke(InvocationFn fn);
    /// Queue one stoppable invocation on the event thread.
    /// If the thread is already stopping or finished, this call is ignored.
    /// @param fn The callback to execute.
    void invoke(StoppableInvocationFn fn);
    /// Queue one invocation on the event thread after a delay.
    /// If the thread is already stopping or finished, this call is ignored.
    /// @param fn The callback to execute.
    /// @param delay The delay before execution.
    void invokeDelayed(InvocationFn fn, milliseconds delay);
    /// Queue one stoppable invocation on the event thread after a delay.
    /// If the thread is already stopping or finished, this call is ignored.
    /// @param fn The callback to execute.
    /// @param delay The delay before execution.
    void invokeDelayed(StoppableInvocationFn fn, milliseconds delay);
    /// Request a graceful shutdown.
    /// Delayed work that is not yet due is cancelled immediately.
    void quit();
    /// Request an immediate shutdown.
    /// Queued work that has not started yet is discarded.
    void abort();
    /// Wait for the thread to finish.
    /// @param timeout The maximum wait time.
    /// @return `true` if the thread finished before the timeout expired.
    /// @throw std::logic_error If called from the event thread itself before it finished.
    /// @throw std::exception Any stored callback exception after the thread finished.
    [[nodiscard]] auto waitForQuit(milliseconds timeout) -> bool;
    /// Request graceful shutdown and wait for completion.
    /// @param timeout The maximum wait time.
    /// @return `true` if the thread finished before the timeout expired.
    /// @throw std::logic_error If called from the event thread itself before it finished.
    /// @throw std::exception Any stored callback exception after the thread finished.
    [[nodiscard]] auto quitAndWait(milliseconds timeout) -> bool;
    /// Test if the worker thread is still running.
    /// @return `true` while the event loop is active.
    [[nodiscard]] auto isRunning() const -> bool;
    /// Test if graceful shutdown was requested.
    /// @return `true` if `quit()` was called.
    [[nodiscard]] auto isQuitRequested() const -> bool;
    /// Test if immediate shutdown was requested.
    /// @return `true` if `abort()` was called.
    [[nodiscard]] auto isAbortRequested() const -> bool;
    /// Test if the worker thread finished.
    /// @return `true` if the worker thread ended.
    [[nodiscard]] auto isFinished() const -> bool;
    /// Test if the worker thread stored a failure.
    /// @return `true` if one callback threw an exception.
    [[nodiscard]] auto hasFailed() const -> bool;

private:
    using FinishedCallback = std::function<void()>;

private:
    /// Start the detached worker thread.
    void start();
    /// Run the worker loop.
    void runWorker();
    /// Process one event loop iteration.
    /// @return `true` if the worker thread should continue running.
    [[nodiscard]] auto processLoopIteration() -> bool;
    /// Handle one invocation event.
    /// @param event The event to process.
    void handleEvent(const Event &event);
    /// Queue one stoppable invocation on the event driver.
    /// @param fn The callback to execute.
    void invokeImpl(StoppableInvocationFn fn);
    /// Queue one delayed stoppable invocation on the event scheduler.
    /// @param fn The callback to execute.
    /// @param delay The delay before execution.
    void invokeDelayedImpl(StoppableInvocationFn fn, milliseconds delay);
    /// Create one invocation event payload.
    /// @param fn The callback to wrap.
    /// @return The event payload.
    [[nodiscard]] static auto createInvocationEvent(StoppableInvocationFn fn) -> EventDataUniquePtr;
    /// Wake the worker loop.
    void wakeWorker();
    /// Get the next time slice to wait for.
    /// @return `std::nullopt` for an indefinite wait, or the maximum wait duration.
    [[nodiscard]] auto nextWaitDuration() const -> std::optional<milliseconds>;
    /// Request stop on the currently running callback, if any.
    void requestActiveStop();
    /// Mark the worker thread as finished and notify waiters.
    void finishWorker() noexcept;
    /// Rethrow the stored callback exception, if any.
    void rethrowFailureIfNeeded() const;
    /// Install a callback that runs after the worker thread finished.
    /// @param callback The callback to invoke after shutdown.
    void setFinishedCallback(FinishedCallback callback);

private:
    static constexpr auto cMaximumWaitSlice = milliseconds{5};

private:
    mutable std::mutex _mutex;                   ///< Protects the worker state.
    std::condition_variable _wakeupCv;           ///< Wakes the worker loop.
    std::condition_variable _finishedCv;         ///< Wakes waiting threads after shutdown.
    EventDriver _eventDriver;                    ///< The immediate event queue.
    EventScheduler _eventScheduler;              ///< The delayed event queue.
    std::thread::id _workerThreadId;             ///< The worker thread id.
    bool _quitRequested{false};                  ///< A graceful shutdown was requested.
    bool _abortRequested{false};                 ///< An immediate shutdown was requested.
    bool _finished{false};                       ///< The worker thread finished.
    bool _wakeupRequested{false};                ///< A wakeup was requested while waiting.
    std::optional<StopSource> _activeStopSource; ///< The stop source for the active callback.
    std::exception_ptr _failure;                 ///< The first callback failure.
    FinishedCallback _finishedCallback;          ///< Optional callback after shutdown.
};

}
