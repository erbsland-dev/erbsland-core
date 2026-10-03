..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Events; Scheduling Work
    single: Timers; Repetition and Cancellation
    single: Callbacks; Deferred Execution
    single: Debouncing

**************************
Scheduling Work and Timers
**************************

A responsive event loop spends little time inside each callback.
When work should happen later, you can return to the loop and let it dispatch other callbacks while you wait.
This page explains how to defer a step, schedule a delay, repeat an action, and replace or cancel a pending deadline.

Read :doc:`using_event_apis` and :doc:`callback_lifetimes` first for targets and callback ownership.
The examples use small local loops so each scheduling decision is visible.
The same scheduling methods are available through an application's or event thread's target.

Deferred Execution with invoke()
================================

Sometimes you need another step soon, but it should begin only after the current callback returns.
For example, a component can finish updating its state before notifying another component.
:cpp:func:`Events::invoke() <erbsland::event::Events::invoke>` queues a callback on the target loop, including when you
call it from that loop itself.
It does not call the callback inline and does not create a background thread.

The following demo queues an observation from inside its preparation callback.
The line following ``invoke()`` therefore prints before the queued observation.
It also demonstrates a delayed callback whose recipient disappears before dispatch.

.. erbsland-demo::
    :source: event/EventScheduler/QueuedWork.cpp
    :exec: event/event_scheduler --demo QueuedWork
    :sha256: 9a2a72bbc6d3ca2d389b9fabb26bb8ecfd3840a95a386ee776180080e889d155

.. code-block:: cpp

    /// Invocations defer work on one loop, even when posted from that loop's own callback.
    /// Delayed invocations retain their callback without a cancellation handle.
    void queuedWork() {
        const auto loop = el::EventLoop::create(el::EventLoopOptions{}.setErrorPolicy(el::EventLoopErrorPolicy::Quit));
        auto recipient = std::make_shared<el::String>("Sulak alan"_el);
        const auto weakRecipient = std::weak_ptr<el::String>{recipient};
        loop->invoke([loop]() -> void {
            el::io::printLine("Preparing observation."_el);
            loop->invoke([]() -> void { el::io::printLine("Queued observation."_el); });
            el::io::printLine("Preparation returned before observation."_el);
        });
        loop->invokeAfter(el::Milliseconds{-1}, [weakRecipient]() -> void {
            if (const auto owner = weakRecipient.lock()) {
                el::io::printLine("Observation: "_el, *owner);
            } else {
                el::io::printLine("Obsolete observation ignored."_el);
            }
        });
        recipient.reset();
        (void)loop->runUntilIdle();
        loop->invokeAfter(el::Milliseconds{5}, [&loop, caption = "Sulak alan"_el]() -> void {
            el::io::printLine("Delayed reminder: "_el, caption);
            loop->quit();
        });
        loop->run();
        if (const auto error = loop->takeError()) {
            std::rethrow_exception(error);
        }
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mPreparing observation.
        Preparation returned before observation.
        Obsolete observation ignored.
        Queued observation.
        Delayed reminder: Sulak alan␛[0m

.. erbsland-demo-end::

The nested invocation gives the loop another short step to dispatch.
It does not make that step run concurrently with preparation.
Long computations still occupy the dispatching thread; moving a computation into an invocation on the same loop only
changes when it blocks that loop.
See :doc:`processing_work_on_event_threads` when work needs a separate worker.

.. mermaid::

    sequenceDiagram
        participant L as Event loop
        participant C as Application callbacks
        L->>+C: Preparation callback
        C->>L: invoke(observation)
        L-->>C: Return after queueing
        C->>C: Finish preparation
        C-->>-L: Preparation returns
        Note over L: Other ready work may run
        L->>+C: Observation callback
        C-->>-L: Observation returns

Both participants represent roles on the same thread.
The preparation callback queues the observation and continues; the loop can dispatch the observation only after
preparation returns.

Other queued work may also be ready when the callback returns.
Treat this as a return to dispatch, rather than a precise promise about which unrelated callback runs next.

Delayed Invocations and Timer Handles
=====================================

:cpp:func:`Events::invokeAfter() <erbsland::event::Events::invokeAfter>` retains a callback for delayed dispatch without
returning a cancellation handle.
It is useful for an independent reminder or a step whose recipient can check whether it is still relevant.
A zero or negative delay queues the callback immediately, just like ``invoke()``; it still does not run inline.
The negative-delay callback in the demo uses a weak recipient, so releasing the recipient makes the eventual callback
harmless.
For a still-living recipient whose request has changed, a request identifier provides the additional check shown in
:doc:`communicating_between_loops`.

The delayed reminder at the end of the demo captures independent input by value and runs after a positive delay.
Its reference to the local loop remains valid until ``run()`` returns.
A positive delay makes work eligible after that delay; the target loop must still be running and ready to dispatch.
Delayed work remains subject to the loop's admission and queue policy when it becomes due.
Neither a delay nor retaining a target guarantees delivery during shutdown.

When you need to change or cancel the deadline, :cpp:func:`Events::createTimer() <erbsland::event::Events::createTimer>`
returns an inactive :cpp:class:`EventTimer <erbsland::event::EventTimer>`.
Retain that pointer in the object or scope that owns the scheduled activity.
Creating a timer alone does not start it.

.. erbsland-demo::
    :source: event/EventScheduler/TimerLifetime.cpp
    :exec: event/event_scheduler --demo TimerLifetime
    :sha256: 9c0575b4dc1c4bdaf13aa7185e0f17cc4731e6ab79d79e542c08153bf8091f1a

.. code-block:: cpp

    /// Retaining a timer permits restart and cancellation before dispatch begins.
    /// An inactive timer owns its callback without scheduling it.
    void timerLifetime() {
        const auto loop = el::EventLoop::create(el::EventLoopOptions{}.setErrorPolicy(el::EventLoopErrorPolicy::Quit));
        auto calls = 0;
        auto timer = loop->createTimer([&calls]() -> void { ++calls; });
        el::io::printLine("Initially active: "_el, timer->isActive());
        timer->startOnce(el::Seconds{10});
        timer->startOnce(el::Milliseconds{0}); // Replace the previous deadline.
        el::io::printLine("One-shot mode: "_el, timer->mode() == el::EventTimerMode::Once);
        el::io::printLine("Immediate interval: "_el, timer->interval() == el::Milliseconds{0});
        (void)loop->runUntilIdle();
        el::io::printLine("Calls after restart: "_el, calls);
        el::io::printLine("Active after dispatch: "_el, timer->isActive());

        timer->startOnce(el::Milliseconds{0});
        timer->stop();
        (void)loop->runUntilIdle();
        el::io::printLine("Calls after stop: "_el, calls);
        timer->startOnce(el::Milliseconds{0});
        timer.reset(); // Release the last handle before the occurrence is queued.
        (void)loop->runUntilIdle();
        el::io::printLine("Calls after destruction: "_el, calls);
        if (const auto error = loop->takeError()) {
            std::rethrow_exception(error);
        }
        if (calls != 1) {
            throw el::LogicFailure{"Unexpected timer callback count."_el};
        }
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mInitially active: false
        One-shot mode: true
        Immediate interval: true
        Calls after restart: 1
        Active after dispatch: false
        Calls after stop: 1
        Calls after destruction: 1␛[0m

.. erbsland-demo-end::

:cpp:func:`startOnce() <erbsland::event::EventTimer::startOnce>` schedules one occurrence.
Calling it again replaces the previous schedule, so the demo's ten-second deadline becomes an immediate occurrence.
A zero or negative one-shot delay schedules immediate execution through the loop.
After that callback finishes, the timer becomes inactive and remains available for reuse.

:cpp:func:`stop() <erbsland::event::EventTimer::stop>` cancels future occurrences without destroying the timer or its
callback.
The demo stops one occurrence before pumping the loop and then releases another timer before it becomes due.
Neither adds a callback to the counter.
Releasing your pointer cancels through destruction only when no other owner retains the timer.
During dispatch the scheduler can retain it temporarily, so explicit ``stop()`` is the clearer cancellation request when
an occurrence may already be queued.

:cpp:func:`isActive() <erbsland::event::EventTimer::isActive>` includes an active schedule or an occurrence queued for
execution.
:cpp:func:`mode() <erbsland::event::EventTimer::mode>` identifies the current
:cpp:enum:`EventTimerMode <erbsland::event::EventTimerMode>`, while
:cpp:func:`interval() <erbsland::event::EventTimer::interval>` reports the configured delay or interval.
These accessors give snapshots, not synchronization barriers.
Stopping a timer sets its mode to inactive; the interval is still the last configured value.

Fixed-Delay and Fixed-Rate Timers
=================================

A periodic refresh needs a rule for what happens when refreshing takes time.
With :cpp:func:`startFixedDelay() <erbsland::event::EventTimer::startFixedDelay>`, the next interval begins after the
callback finishes.
This leaves a full pause between executions and is useful when each refresh should settle before the next begins.
With :cpp:func:`startFixedRate() <erbsland::event::EventTimer::startFixedRate>`, deadlines follow the cadence
established when the timer starts.
This is useful when you want observations near a regular schedule despite varying callback durations.

Both repeating methods require a strictly positive interval and throw
:cpp:class:`ParameterFailure <erbsland::err::ParameterFailure>` for zero or negative values.
A slow callback does not create overlapping executions on the same loop.
Fixed-rate ticks missed while it runs are skipped, rather than accumulated into a burst of catch-up callbacks.

This illustrative timeline starts a timer at time zero with a 20 ms interval and a callback taking 35 ms:

.. code-block:: text

    fixed delay: first starts 20 -> finishes 55 -> next starts 75 -> finishes 110
    fixed rate:  first starts 20 -> finishes 55 -> next starts 60 -> finishes 95
    cadence:     due at 20, 40, 60, 80, 100 ...; missed ticks at 40 and 80 are skipped

The demo deliberately sleeps inside its callback to make the difference visible.
This models a blocked loop; ordinary refresh callbacks should use asynchronous sources or hand substantial work to a
worker.
It stops after three occurrences of each mode.

.. erbsland-demo::
    :source: event/EventScheduler/RepeatingTimers.cpp
    :sha256: 67d4060f98630641d27f95809ba988e09fd514e2a8c1c47c5db2fc134a391a40

.. code-block:: cpp

    /// Compare fixed-delay and fixed-rate timers with deliberately slow callbacks.
    /// This sleep illustrates a blocked loop; production callbacks should stay short.
    void repeatingTimers() {
        for (const auto mode : {el::EventTimerMode::FixedDelay, el::EventTimerMode::FixedRate}) {
            const auto loop = el::EventLoop::create(el::EventLoopOptions{}.setErrorPolicy(el::EventLoopErrorPolicy::Quit));
            auto timer = el::EventTimerPtr{};
            auto count = 0;
            const auto start = std::chrono::steady_clock::now();
            timer = loop->createTimer([&]() -> void {
                const auto elapsed =
                    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
                el::io::printLine("Tick "_el, ++count, " at about "_el, elapsed, " ms"_el);
                std::this_thread::sleep_for(std::chrono::milliseconds{35});
                if (count == 3) {
                    timer->stop(); // A running callback may stop its next occurrence.
                    loop->quit();
                }
            });
            if (mode == el::EventTimerMode::FixedDelay) {
                el::io::printLine("Fixed delay:"_el);
                timer->startFixedDelay(el::Milliseconds{20});
            } else {
                el::io::printLine("Fixed rate:"_el);
                timer->startFixedRate(el::Milliseconds{20});
            }
            loop->run();
            if (const auto error = loop->takeError()) {
                std::rethrow_exception(error);
            }
        }
    }

.. erbsland-demo-end::

Run the compiled example with ``cmake-build-debug/demo-apps/event/event_scheduler --demo RepeatingTimers``.
The printed start times are observations, not exact timing assertions.
OS scheduling and other loop work can make either mode late; neither is a real-time scheduling guarantee.

Debouncing with Timer Restart
=============================

A debounce collects a burst of changes and processes the latest value once the input has been quiet for a chosen
interval.
Instead of creating a delayed invocation for every change, restart the same one-shot timer.
Each restart replaces its deadline, and the callback reads the latest state when the final deadline arrives.

.. erbsland-demo::
    :source: event/EventScheduler/Debouncing.cpp
    :show: f:debouncing
    :exec: event/event_scheduler --demo Debouncing
    :sha256: 83674e4fa362c7de3b2f4c806026621aeca09c46e55b93a473343683ec681f7c

.. code-block:: cpp

    void debouncing() {
        const auto loop = el::EventLoop::create(el::EventLoopOptions{}.setErrorPolicy(el::EventLoopErrorPolicy::Quit));
        auto latestObservation = el::String{};
        auto savedCount = 0;
        const auto timer = loop->createTimer([&]() -> void {
            ++savedCount;
            el::io::printLine("Saved observation: "_el, latestObservation);
            loop->quit();
        });
        loop->invoke([&]() -> void {
            for (const auto observation : {"Sazlık"_el, "Gölet"_el, "Orman"_el}) {
                latestObservation = observation;
                timer->startOnce(el::Milliseconds{20});
            }
        });
        loop->run();
        if (const auto error = loop->takeError()) {
            std::rethrow_exception(error);
        }
        if (savedCount != 1) {
            throw el::LogicFailure{"Debounce must save exactly one observation."_el};
        }
        inactivityTimeout();
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mSaved observation: Orman
        Inactive observation stream closed.␛[0m

.. erbsland-demo-end::

The input values are observations in Turkish, selected for this demo's nature-reserve theme.
Three updates happen during one callback; only the latest value is saved after the quiet period.
A real input source can apply exactly the same restart after each received change.
Keep the timer and latest input in the receiving component, as the local scope does here.

An inactivity timeout uses the same scheduling pattern with a different completion action.
Restart a one-shot timer after accepted activity; when no activity replaces the deadline, its callback ends the wait.
The second part of the same demo makes the timeout close an observation stream:

.. erbsland-demo::
    :source: event/EventScheduler/Debouncing.cpp
    :show: f:inactivityTimeout
    :sha256: 3d86a54beeb58c1806caac008f9f591b042ce60b93596d3ff4ef2f43258702d6

.. code-block:: cpp

    void inactivityTimeout() {
        const auto loop = el::EventLoop::create(el::EventLoopOptions{}.setErrorPolicy(el::EventLoopErrorPolicy::Quit));
        const auto timeout = loop->createTimer([loop]() -> void {
            el::io::printLine("Inactive observation stream closed."_el);
            loop->quit();
        });
        loop->invoke([timeout]() -> void {
            timeout->startOnce(el::Milliseconds{20});
            // Every accepted observation restarts the waiting budget.
            timeout->startOnce(el::Milliseconds{20});
        });
        loop->run();
        if (const auto error = loop->takeError()) {
            std::rethrow_exception(error);
        }
    }

.. erbsland-demo-end::

For an overall operation deadline, start once when the operation begins and leave it running across retries.
That bounds the whole operation instead of measuring inactivity since its latest update.

Timer Cancellation and Captured State
=====================================

Timer control and its accessors are thread-safe.
That lets another thread request a stop or restart without directly manipulating the scheduler's state.
It does not make the data captured by the timer callback safe to access from several threads.
In the debounce demo, the latest value is changed and read only on its target loop; another thread should send an input
update to that loop with ``invoke()``.

Stopping or restarting cannot interrupt a callback already executing.
That callback may finish using its old inputs even after another thread's control call has returned.
If it can publish an obsolete result, guard publication with operation state or identity as well as controlling the
timer.
:doc:`callback_lifetimes` explains captures and recipient destruction in more detail.

A blocked loop delays timers, input notifications, and cancellation commands sent through that loop alike.
Keep each callback short enough for the responsiveness your application needs.
Callback exceptions are handled by the loop's configured error policy; they are not successful completion of your
operation.
The demos configure callback errors to request loop termination and check
:cpp:func:`EventLoop::takeError() <erbsland::event::EventLoop::takeError>` after dispatch.
The :ref:`event-loop-options` reference describes the available error and queue policies.

For reusable callbacks that return results to a chosen loop, continue with :doc:`event_bindings`.
It builds on the queued execution and ownership rules introduced here.
