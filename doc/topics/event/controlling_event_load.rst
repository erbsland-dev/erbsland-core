..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Events; Keeping Event Loops Responsive Under Load
    single: Events; Queue Capacity

*****************************************
Keeping Event Loops Responsive Under Load
*****************************************

A loop becomes unresponsive when callbacks occupy it for too long or producers submit work faster than it can finish.
Timers run late, messages accumulate, and even shutdown requests wait behind application work.
This page explains how to bound outstanding requests, keep individual callbacks short, and choose what happens when a
queue reaches capacity.
It builds on :doc:`scheduling_work`; worker threads and larger workflows are optional ways to apply these patterns.

Bounding Outstanding Requests
=============================

Start by limiting the work your application launches.
A request can remain outstanding while it waits in a queue, runs on a worker, or waits for an external source.
Counting those requests gives you a useful control point across the whole operation, beyond the number of callbacks
currently waiting on one loop.

The ``CollectReadings`` demo launches at most two requests at once.
Each result releases one slot before the batch starts another request.
All accounting belongs to one loop, so the counter changes and the decision to launch are serialized.
The retained state is small and explicit:

.. erbsland-demo::
    :source: event/EventLoad/ReadingBatch.hpp
    :show: c:Loop that owns-c:Aggregated observation
    :sha256: c109e33aef4ac4bd7c6c404549048415453f22a6e2d0943b4e6cd77b5fe3bbdd

.. code-block:: cpp

    el::EventLoopPtr _loop;     ///< Loop that owns all batch state.
    std::size_t _next = 0;      ///< Next request to launch.
    std::size_t _inFlight = 0;  ///< Requests without a result yet.
    std::size_t _completed = 0; ///< Results received so far.
    int _sum = 0;               ///< Aggregated observation count.

.. erbsland-demo-end::

``_next`` identifies work not yet launched, ``_inFlight`` counts requests awaiting results, and ``_completed`` counts
received results.
The local provider queues a reply for each request rather than computing on a separate thread.
It makes the completion boundary visible without requiring a worker framework:

.. erbsland-demo::
    :source: event/EventLoad/ReadingBatch.cpp
    :show: f:ReadingBatch::launchAvailable, f:ReadingBatch::receive
    :exec: event/event_load --demo CollectReadings
    :sha256: f2807597a2b5867f56083b9343e4a8b60150019360861b468204bc57a2a5e616

.. code-block:: cpp

    void ReadingBatch::launchAvailable() {
        const auto receiveReading =
            el::bindEvent(_loop, [this](std::size_t index, int value) -> void { receive(index, value); });
        constexpr auto readings = std::array{4, 7, 2};
        while (_next < readings.size() && _inFlight < 2) {
            const auto index = _next++;
            const auto value = readings[index];
            ++_inFlight;
            // Stand-in for independent asynchronous sources; this callback still runs on the same loop.
            receiveReading(index, value);
        }
    }

    void ReadingBatch::receive(const std::size_t index, const int value) {
        --_inFlight;
        ++_completed;
        _sum += value;
        el::io::printLine("Reading "_el, index, " received: "_el, value);
        if (_completed == 3) {
            if (_sum != 13 || _inFlight != 0) {
                throw el::LogicFailure{"Unexpected batch result."_el};
            }
            el::io::printLine("Batch complete; total observations: "_el, _sum);
            _loop->quit();
            return;
        }
        launchAvailable();
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mReading 0 received: 4
        Reading 1 received: 7
        Reading 2 received: 2
        Batch complete; total observations: 13␛[0m

.. erbsland-demo-end::

The reusable bound handler captures ``this``.
The runner retains the stack-owned batch until dispatch ends, so that borrowed recipient remains valid:

.. erbsland-demo::
    :source: event/EventLoad/CollectReadings.cpp
    :show: f:collectReadings
    :sha256: d26fb9ed26a1cf7c1b244189493ae1c783df71423a488e5ae83b4fe16b62ae19

.. code-block:: cpp

    void collectReadings() {
        const auto loop = el::EventLoop::create(el::EventLoopOptions{}.setErrorPolicy(el::EventLoopErrorPolicy::Quit));
        auto batch = ReadingBatch{loop};
        loop->invoke([&batch]() -> void { batch.start(); });
        loop->run();
        if (const auto error = loop->takeError()) {
            std::rethrow_exception(error);
        }
    }

.. erbsland-demo-end::

Two is an application choice, not an event API limit.
For a worker service, the same pattern sends at most two commands and releases slots when replies return to the
controller's loop.
For a source-driven operation, it retains at most two active source requests.
See :doc:`communicating_between_loops` for routing those replies and :doc:`event_workflows` for per-request state.

The example provider produces exactly one result for every launched request.
A real controller must also finish or cancel requests that fail, time out, or are replaced.
Count each terminal outcome once; duplicate notifications must not release a slot twice.
A callback exception needs a failure path such as the runner's ``Quit`` policy and error check.
A source that never replies needs a deadline.
:doc:`handling_event_errors` explains why storing an exception alone may leave a waiting caller unfinished.

This window bounds active requests, not an unlimited input list or all completed results retained in memory.
Read input incrementally and consume completed output when the total batch can be large.
For variable-size jobs, also account for payload sizes when choosing the window.

Callback Duration and Shared Loop Time
======================================

A short queue can still be unresponsive if its current callback performs a long computation or blocking read.
No queued timer, cancellation command, or result callback on that loop can run until the current callback returns.
A timer deadline therefore detects overdue work only once its loop gets an opportunity to dispatch it.

Split incremental computation into bounded chunks and queue one continuation after each chunk.
Retain the operation's progress between chunks, check cancellation before starting the next one, and avoid submitting
all chunks at once.
A single pending continuation bounds the queued work and gives the loop opportunities to process other callbacks.
The chunk's worst-case duration determines how long those callbacks can still be delayed.

Move genuinely blocking calls or work that cannot be divided to a worker, as shown in
:doc:`processing_work_on_event_threads`.
Keep the request window there too: moving work to another loop does not change how quickly that work can be processed.
A worker's blocked callback still delays its own timers and termination.
For native asynchronous I/O, let the source wait for readiness instead of blocking a loop callback.

Queue Capacity as an Overload Policy
====================================

A request window controls your own producers.
A queue limit provides a separate response when producers nevertheless fill the waiting queue.
Configure :cpp:class:`EventLoopOptions <erbsland::event::EventLoopOptions>` before creating the loop or thread:
:cpp:func:`setMaximumQueuedEvents() <erbsland::event::EventLoopOptions::setMaximumQueuedEvents>` sets the capacity and
:cpp:func:`setQueueOverflowAction() <erbsland::event::EventLoopOptions::setQueueOverflowAction>` chooses the response.
The options are copied; later changes to the original value do not affect the existing loop.
:cpp:func:`maximumQueuedEvents() <erbsland::event::EventLoopOptions::maximumQueuedEvents>` is zero by default, meaning unlimited.
The default overflow action is
:cpp:enum:`EventQueueOverflowAction::Terminate <erbsland::event::EventQueueOverflowAction>`, so selecting a finite
capacity also requires a deliberate decision about overflow.

Do not use a lossy queue policy as a substitute for request accounting.
If a command or reply is discarded, a controller waiting for exactly one reply per request may never finish.
Calls through event bindings use the same queue admission policy.
Frontend :cpp:func:`post() <erbsland::event::Events::post>` and :cpp:func:`invoke() <erbsland::event::Events::invoke>`
return no acceptance result, so returning normally does not prove the callback was queued or will run.
Choose a workload protocol with explicit completion and failure outcomes, and keep required commands and replies away
from queues that intentionally discard updates.

A Deliberately Lossy Update Stream
==================================

Progress displays can often tolerate missing intermediate samples.
The following example uses ``Ignore`` for such samples and queues four updates before dispatching anything.
Capacity is two, so the accepted samples are predictable without timing assumptions or concurrent producers:

.. erbsland-demo::
    :source: event/EventLoad/ReplaceableUpdates.cpp
    :show: f:replaceableUpdates
    :exec: event/event_load --demo ReplaceableUpdates
    :sha256: 29c2e0096b6188fb936d573101bbbd015c2a87ddec3407cf23196a7199845aa6

.. code-block:: cpp

    void replaceableUpdates() {
        auto options = el::EventLoopOptions{};
        options.setMaximumQueuedEvents(2);
        options.setQueueOverflowAction(el::EventQueueOverflowAction::Ignore);
        const auto loop = el::EventLoop::create(options);
        auto displayed = std::vector<int>{};
        const auto route = el::String{"地球 → 火星"_el};

        const auto displayProgress = el::bindEvent(loop, [&displayed, route](int progress) -> void {
            displayed.push_back(progress);
            el::io::printLine(route, ": "_el, progress, "%"_el);
        });
        // Fill the queue before dispatch, making this burst deterministic.
        for (const auto progress : {10, 20, 30, 40}) {
            displayProgress(progress);
        }
        const auto processed = loop->runUntilIdle();
        if (const auto error = loop->takeError()) {
            std::rethrow_exception(error);
        }
        if (processed != 2 || displayed != std::vector<int>{10, 20}) {
            throw el::LogicFailure{"Expected only the two previously accepted updates."_el};
        }
        el::io::printLine("Two samples displayed; two incoming samples discarded."_el);
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0m地球 → 火星: 10%
        地球 → 火星: 20%
        Two samples displayed; two incoming samples discarded.␛[0m

.. erbsland-demo-end::

``Ignore`` discards the incoming update and preserves the accepted queue.
Here the display sees 10 and 20 percent; it does not automatically receive the latest value, 40 percent.
The local ``displayed`` collection survives the dispatch and lets the demo verify exactly what ran.
The reusable binding captures the route by value; each call stores its own progress value for delivery.
These samples carry no completion obligation.
A real task's final outcome needs a reliable path independent of these disposable progress updates.

If the display must eventually see the newest value, use application-level coalescing instead.
Keep one latest-value slot and at most one pending display callback.
A producer replaces the slot; it schedules a callback only when none is pending.
The consumer takes the latest value and updates the pending flag under the same synchronization used by producers.
The handover must ensure that an update arriving during consumption either joins that consumption or schedules another
callback.
If producers run on other threads, both the value and pending flag need synchronization.
The callback's recipient must also survive dispatch, as explained in :doc:`callback_lifetimes`.

Coalescing bounds queued update callbacks only when the pending flag actually prevents additional submissions.
Overwriting a shared value while posting a new callback for every update still permits an unbounded queue.
Use a submission path that remains available for that single notification; a silent rejection must not leave the pending
flag permanently set.
Similarly, :doc:`scheduling_work` shows debounce for sources whose notifications already reach the owner loop, but a
rescheduled timer alone does not bound a backlog of cross-thread input callbacks.

Queue Overflow Failure and Waiting Policies
===========================================

If silently losing work is unacceptable, choose a response that makes the task fail or stops new admission.
``Quit`` discards the overflowing event, closes admission, and drains already accepted callbacks.
It does not manufacture the missing completion; the task controller must recognize that shutdown can leave requests
unfinished.
``FailApplication`` reports :cpp:class:`EventQueueFullError <erbsland::event::EventQueueFullError>` to the owning
application and initiates its failure handling.
That policy is supported only for application-managed workers.
Standalone loops and unmanaged threads reject it during creation.
``Terminate`` ends the process and is appropriate only when overflow represents a condition the program cannot recover
from.
The :ref:`event-loop-options` reference provides the complete policy contracts.

``Block`` waits for a slot rather than discarding immediately.
Another thread must be able to drain the loop, including when posting before a worker starts.
If the active consumer posts into its own full queue, blocking would prevent the only consumer from freeing space; the
policy calls ``std::terminate()`` in that case.
Two loops can also deadlock if each blocks while submitting work to the other's full queue.
For loop-to-loop requests, bounded asynchronous submission usually makes the completion path easier to reason about.

Stop and quit wake producers waiting inside blocking admission, which discard their pending events.
They cannot release a callback waiting in arbitrary application code or interrupt a blocking external operation.
See :doc:`shutting_down_event_work` for termination and joining after resources finish.
Queue overflow never invokes :cpp:func:`setErrorHandler() <erbsland::event::EventLoop::setErrorHandler>`: admission
decisions and callback exceptions are separate policies.
For backend authors,
:cpp:func:`EventBackendTarget::postFromBackend() <erbsland::event::EventBackendTarget::postFromBackend>` returns a
boolean acceptance result; that facility does not change the frontend's void-returning submission contract.

Interpreting Queue Counts and Delayed Work
==========================================

:cpp:func:`queuedEventCount() <erbsland::event::EventLoop::queuedEventCount>` is a synchronized snapshot of ordinary waiting events.
It is useful for observing trends, but checking it before posting does not reserve a slot.
Another producer can submit work between the check and your own call; admission performs the actual capacity check.
Nor is the count a measure of payload bytes or latency.
One callback can retain a large buffer or run much longer than many small callbacks.

The limit excludes the executing callback, pending delayed scheduler work, timers not yet queued, events retained by
blocked producers, and captured exceptions.
Due timer and delayed-invocation callbacks become subject to capacity when the scheduler posts them.
With a discarding policy, a timer occurrence can therefore be lost just like another event.
Rejected repeating occurrences are skipped and rescheduled while admission remains open; they do not provide a
completion guarantee for the missed occurrence.
Reserved graceful-quit control bypasses capacity and is excluded from the count.

Use request accounting to understand outstanding work, callback duration to understand responsiveness, and queue
snapshots to observe waiting work.
Together they explain whether you need fewer producers, smaller chunks, coalesced updates, or a different execution
thread.
Transport-specific flow control belongs to the relevant network source; a loop queue limit alone does not bound a peer's
buffers or network traffic.
