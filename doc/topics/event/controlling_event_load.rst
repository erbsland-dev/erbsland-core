..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Events; Responsiveness
    single: Events; Overload Control
    single: Events; Queue Capacity
    single: Events; Coalescing

**********************************************
Event Loop Responsiveness and Overload Control
**********************************************

An event loop can fall behind for two different reasons.
One callback can occupy it for too long, preventing other callbacks from running.
Or producers can submit more work than the loop can finish, so even short callbacks accumulate in a growing queue.

This page explains how to address both problems: bound the work in each callback, limit outstanding operations, combine
replaceable updates, and choose what happens when a queue reaches capacity.
You will also learn what queue counts can tell you, and why neither a short queue nor a queue limit guarantees timely
completion.
It builds on :doc:`scheduling_work`; :doc:`processing_work_on_event_threads` explains moving work to another thread.

Callback Duration and Time to Respond
=====================================

A loop dispatches one callback at a time.
Until that callback returns, the loop cannot deliver another result, process a queued cancellation command, or dispatch
a timer whose deadline has passed.
A timer is therefore a request for dispatch at or after a time, not an interrupt that can stop the current callback.

For incremental work, process a bounded chunk and then use
:cpp:func:`EventTarget::invoke() <erbsland::event::EventTarget::invoke>` to queue a continuation.
Retain the progress between calls and schedule only the next chunk.
The next callback continues from that progress rather than restarting the operation.
Returning to dispatch gives other waiting callbacks an opportunity to run:

.. mermaid::

    flowchart LR
        subgraph Long[One long callback]
            A[Process all work] --> B[Handle waiting input]
        end
        subgraph Chunked[Bounded callbacks]
            C[Process one chunk] --> D[Return to dispatch]
            D --> E[Handle waiting input]
            E --> F[Process next chunk]
        end

A chunk limit only helps if each unit inside the chunk is bounded too.
Processing ten records is not a useful time bound if one record can trigger an indefinite blocking read.
Choose a chunk size based on the cost of the work and the response time your application needs; queue capacity cannot
compensate for an excessively long callback.

.. erbsland-in-practice-begin:: Processing a Survey in Chunks

This survey processes two readings per callback.
The retained state identifies the next reading and the accumulated total:

.. erbsland-demo::
    :source: event/snippets/EventLoad/ChunkedSurvey.hpp
    :show: c:Loop that owns-c:Accumulated observations
    :sha256: 168b233f925327771f4d65abcd11fd8f0384838d889a3e0138c073d96913cb87

.. code-block:: cpp

    el::EventLoopPtr _loop; ///< Loop that owns and dispatches this survey.
    std::size_t _next = 0;  ///< Next reading to process, retained between callbacks.
    int _total = 0;         ///< Accumulated observations.

.. erbsland-demo-end::

.. erbsland-demo::
    :source: event/snippets/EventLoad/ChunkedSurvey.cpp
    :show: f:ChunkedSurvey::advance
    :sha256: 05259c0f0c71ca2c40b53ffd7660e9840fe979f4264b263e0fc7f9de9e1b172a

.. code-block:: cpp

    void ChunkedSurvey::advance() {
        constexpr auto readings = std::array{2, 4, 1, 3, 5, 2};
        constexpr auto chunkSize = std::size_t{2};
        auto processed = std::size_t{0};
        while (_next < readings.size() && processed < chunkSize) {
            _total += readings[_next];
            ++_next;
            ++processed;
        }
        el::io::printLine("Readings processed: "_el, _next);
        if (_next < readings.size()) {
            // Schedule only the next chunk, leaving other callbacks an opportunity to run.
            _loop->invoke([this]() -> void { advance(); });
        } else {
            el::io::printLine("Survey complete; observations: "_el, _total);
        }
    }

.. erbsland-demo-end::

The runner queues the first chunk and a separate observer before dispatch starts.
The first chunk queues its continuation behind that observer:

.. erbsland-demo::
    :source: event/snippets/EventLoad/ChunkedWork.cpp
    :show: a:loop-a:dispatched
    :exec: event/event_load --demo ChunkedWork
    :sha256: 773827492e1d8d254995f06bdc5354830fba25dba75214d6be78169ba5d3108e

.. code-block:: cpp

    const auto loop = el::EventLoop::create(el::EventLoopOptions{}.setErrorPolicy(el::EventLoopErrorPolicy::Quit));
    auto survey = ChunkedSurvey{loop};
    auto progressBetweenChunks = std::size_t{0};
    el::io::printLine("Habitat: 青龙林"_el);

    loop->invoke([&survey]() -> void { survey.advance(); });
    loop->invoke([&survey, &progressBetweenChunks]() -> void {
        progressBetweenChunks = survey.processedCount();
        el::io::printLine("Another callback sees "_el, progressBetweenChunks, " of 6 readings processed."_el);
    });
    // The stack-owned survey survives all three chunks and the observing callback.
    const auto dispatched = loop->runUntilIdle();

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mHabitat: 青龙林
        Readings processed: 2
        Another callback sees 2 of 6 readings processed.
        Readings processed: 4
        Readings processed: 6
        Survey complete; observations: 17␛[0m

.. erbsland-demo-end::

The observer sees two of six readings processed, proving that it ran between chunks.
The final checks verify all six readings and the total, so returning early from a callback has not abandoned the task.
The stack-owned survey survives every callback borrowing it.
Two readings per chunk is an illustrative choice for this cheap calculation, not a universal scheduling limit.

.. erbsland-in-practice-end::

A chain of continuations needs its operation state to survive and its submission path to stay open.
Do not quit the loop while a required continuation still needs to be submitted, or send it through a queue that
intentionally discards work.
For cancellation, check an operation token before the next chunk and finish with an explicit cancelled outcome instead
of silently ending the chain; see :doc:`shutting_down_event_work`.

Work that cannot be divided, or a genuinely blocking call, belongs on a worker if it would otherwise delay a responsive
loop.
That worker still has a serial loop: its blocked callback delays its own timers and shutdown.
Keep a limit on work submitted to it, and prefer native asynchronous I/O when a source can wait for readiness without
blocking a callback.

Limiting Outstanding Operations
===============================

An operation can be outstanding while its request waits in a queue, while it runs, or while its result travels back.
A queue count sees only one of those stages.
An application-level request window counts the whole interval from launch to terminal outcome.

Keep that accounting on the controller's owner loop.
Launching an operation consumes a slot; handling its terminal outcome releases the slot and permits another launch.
Because both decisions happen on one loop, the counter needs no cross-thread lock.
The actual work can happen elsewhere and return results to that loop, as described in
:doc:`communicating_between_loops`.

.. mermaid::

    sequenceDiagram
        participant Controller as Controller loop (two slots)
        participant Work as Asynchronous work
        Controller->>Work: Launch A, one slot occupied
        Controller->>Work: Launch B, two slots occupied
        Note over Controller: C waits without being launched
        Work->>Controller: A completes
        Controller->>Controller: Release A's slot
        Controller->>Work: Launch C, two slots occupied
        Work->>Controller: B completes
        Work->>Controller: C completes
        Note over Controller: No operations outstanding

Completion includes failure, cancellation, timeout, and replacement as well as success.
Choose one terminal transition per request; a late or duplicate result must not release the same slot twice.
A callback exception is not automatically a reply, so it needs a failure path.
Likewise, a source that can remain silent needs a deadline on a loop able to dispatch it.
:doc:`event_workflows` explains per-request state, and :doc:`handling_event_errors` explains failure completion.

.. erbsland-in-practice-begin:: Maintaining a Window of Two Requests

The readings demo retains a batch on the runner's stack until its local loop returns.
The batch's ``_next`` counter identifies unlaunched requests, ``_inFlight`` counts requests awaiting a result, and
``_completed`` counts received results.
The provider queues replies on that same loop so the request/completion boundary can be studied without a worker:

.. erbsland-demo::
    :source: event/snippets/EventLoad/ReadingBatch.cpp
    :show: f:ReadingBatch::launchAvailable, f:ReadingBatch::receive
    :exec: event/event_load --demo CollectReadings
    :sha256: 10dec32ef8c9b9ef6b46aba29c6eb5edf5954a1fc34f7d138c6573625c19c625

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

Incrementing ``_inFlight`` before submission reserves a slot.
``receive()`` releases it before calling ``launchAvailable()`` again.
The bound handler borrows the batch through ``this``; the runner keeps it alive throughout dispatch and checks stored
errors afterward.
This provider returns exactly one successful reply per request.
A real provider needs the failure and cancellation accounting described above as well.

.. erbsland-in-practice-end::

A request window bounds active operations, not an unlimited waiting input list or all completed output retained in
memory.
For large workloads, read input incrementally and consume results as they become available.
If jobs have very different payload sizes, account for retained bytes as well as the number of operations.

Choosing a Queue Capacity and Overflow Action
=============================================

A request window controls how much work your application launches.
A queue limit provides a separate policy for a burst that nevertheless fills a loop's waiting queue.
Configure :cpp:class:`EventLoopOptions <erbsland::event::EventLoopOptions>` before creating the loop or thread:
:cpp:func:`setMaximumQueuedEvents() <erbsland::event::EventLoopOptions::setMaximumQueuedEvents>` sets the ordinary
waiting-event limit, and
:cpp:func:`setQueueOverflowAction() <erbsland::event::EventLoopOptions::setQueueOverflowAction>` selects the response at
that limit.
Creation copies the options; changing the original value later does not reconfigure the loop.

The default capacity is zero, meaning unlimited.
The default overflow action is
:cpp:enum:`EventQueueOverflowAction::Terminate <erbsland::event::EventQueueOverflowAction>`.
When selecting a finite capacity, choose the overflow behavior deliberately rather than assuming submissions will wait.

.. list-table::
    :header-rows: 1
    :widths: 20 40 40

    * - Action
      - At capacity
      - Effect on the task
    * - ``Ignore``
      - Discard it; preserve the accepted queue.
      - Suitable only when losing this delivery is acceptable.
    * - ``Quit``
      - Discard it, close admission, and drain accepted work.
      - Outstanding requests may remain without completion.
    * - ``FailApplication``
      - Reject it and report :cpp:class:`EventQueueFullError <erbsland::event::EventQueueFullError>` to the owning application.
      - Enters application failure handling; supported only for application-managed workers.
    * - ``Block``
      - Wait for space, provided the caller is not the active consumer of the full queue.
      - Introduces a blocking dependency on another thread's ability to drain the queue.
    * - ``Terminate``
      - Call ``std::terminate()``.
      - Treat overflow as an unrecoverable process failure.

Standalone loops and unmanaged threads reject ``FailApplication`` during creation.
The :ref:`event-loop-options` reference contains the complete policy contracts.

Ordinary :cpp:func:`EventTarget::post() <erbsland::event::EventTarget::post>` and
:cpp:func:`invoke() <erbsland::event::EventTarget::invoke>` return no acceptance result, and event bindings use that
same admission path.
A normally returning submission does not prove that a callback was accepted or will run.
Discarding a required command or reply can leave a controller waiting forever; neither ``Ignore`` nor ``Quit``
manufactures the missing completion.
Keep required delivery away from intentionally lossy queues and give the operation an explicit failure or abandonment
path when its loop ends.

Queue overflow is an admission decision, separate from an exception thrown while executing a callback.
It does not invoke :cpp:func:`EventLoop::setErrorHandler() <erbsland::event::EventLoop::setErrorHandler>`.
Backend authors can obtain an acceptance result through
:cpp:func:`EventBackendTarget::postFromBackend() <erbsland::event::EventBackendTarget::postFromBackend>`; that backend
facility does not change the frontend contract.

Dropping Samples or Keeping the Latest Value
============================================

Progress displays often need less information than the operation producing them.
Losing some intermediate samples can be acceptable, while losing the final operation outcome is not.
Treat progress notification and required completion as separate responsibilities.

``Ignore`` drops the incoming event when the queue is full.
It preserves older accepted samples, so it does not ensure that a display eventually receives the newest value.
This difference matters when choosing between dropping samples and coalescing updates.

.. erbsland-in-practice-begin:: Dropping Incoming Progress Samples

This demo sets capacity to two, then submits four progress values before dispatch starts:

.. erbsland-demo::
    :source: event/snippets/EventLoad/ReplaceableUpdates.cpp
    :show: a:options-a:processed
    :exec: event/event_load --demo ReplaceableUpdates
    :sha256: 972a4718e6f8558d0edcb146e915686ae40c277c2fc52fffacacbbcecc7874a3

.. code-block:: cpp

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

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0m地球 → 火星: 10%
        地球 → 火星: 20%
        Two samples displayed; two incoming samples discarded.␛[0m

.. erbsland-demo-end::

The display sees 10 and 20 percent; 30 and 40 percent were discarded.
The runner owns the captured ``displayed`` collection and ``route`` value and verifies the exact result after dispatch.
These samples carry no completion obligation.
The newest submitted value, 40 percent, is not automatically delivered later.

.. erbsland-in-practice-end::

Coalescing Updates
------------------

Coalescing uses an application-owned latest-value slot instead of queuing every update.
A producer replaces that slot and queues a notification only if none is pending.
The notification reads the newest value, clears the pending flag, and updates the recipient.
A burst can therefore produce one display callback without discarding its newest value.

When producers run on other threads, protect both the value and pending flag with the same synchronization.
Reading the value and clearing the flag must be one protected handover.
An update before that handover belongs to the current delivery; an update afterward schedules another notification.
Call application display code outside the lock so it does not prevent producers from publishing.

.. mermaid::

    flowchart TD
        A[Producer locks shared state] --> B[Replace latest value]
        B --> C{Notification pending?}
        C -->|Yes| D[Unlock; no additional callback]
        C -->|No| E[Queue notification and mark pending]
        E --> F[Unlock]
        G[Consumer locks shared state] --> H[Copy latest value and clear pending]
        H --> I[Unlock, then display copied value]
        I --> J[An update after the handover queues another notification]

.. erbsland-in-practice-begin:: Coalescing a Burst into Its Newest Progress Value

``LatestProgress`` retains a target and a display callback.
The runner owns the object until its producer has stopped and all queued deliveries have finished.
The mutex protects the latest value and pending flag together:

.. erbsland-demo::
    :source: event/snippets/EventLoad/LatestProgress.hpp
    :show: c:Open target retained-c:Whether a notification
    :sha256: 340b7b8854839f4dab18774e5c292d0a1b826e2447f69c97e6799760454474b9

.. code-block:: cpp

    el::EventTargetPtr _target;        ///< Open target retained through all publications.
    std::function<void(int)> _display; ///< Display handler, invoked only on the receiving loop.
    std::mutex _mutex;                 ///< Protects the latest value and pending flag together.
    int _latest = 0;                   ///< Newest value supplied by a producer.
    bool _pending = false;             ///< Whether a notification is waiting to consume the value.

.. erbsland-demo-end::

.. erbsland-demo::
    :source: event/snippets/EventLoad/LatestProgress.cpp
    :show: f:LatestProgress::publish, f:LatestProgress::deliver
    :sha256: 5242f20afb3491e610d7788a5540747aacb8abf8f4153352828d16187b78e391

.. code-block:: cpp

    void LatestProgress::publish(const int progress) {
        auto lock = std::scoped_lock{_mutex};
        _latest = progress;
        if (_pending) {
            return;
        }
        // invoke() queues delivery; the handler never runs inline under this lock.
        _target->invoke([this]() -> void { deliver(); });
        _pending = true;
    }

    void LatestProgress::deliver() {
        auto progress = int{};
        {
            auto lock = std::scoped_lock{_mutex};
            progress = _latest;
            _pending = false;
        }
        // A producer can now schedule another notification, even while display is running.
        _display(progress);
    }

.. erbsland-demo-end::

:cpp:func:`invoke() <erbsland::event::EventTarget::invoke>` queues its handler rather than calling it inline.
The producer holds the state lock through submission and setting ``_pending``, so a consumer cannot take the value
between those two steps.
If submission throws, the pending flag has not been set.
The target uses unlimited ordinary admission and remains open throughout publication and consumption.

The runner sends one finite burst from another thread and joins that producer before dispatch to make the output
deterministic.
This join is safe because publishing does not wait for the receiving loop.
It then sends another burst after the first notification has been consumed:

.. erbsland-demo::
    :source: event/snippets/EventLoad/CoalescedUpdates.cpp
    :show: a:loop-a:secondDispatch
    :exec: event/event_load --demo CoalescedUpdates
    :sha256: 77d2b4a584d3b82eb2741a17fc5e720848ee99f9b03d81c0d763cc45b6244474

.. code-block:: cpp

    const auto loop = el::EventLoop::create(el::EventLoopOptions{}.setErrorPolicy(el::EventLoopErrorPolicy::Quit));
    auto displayed = el::List<int>{};
    auto updates = LatestProgress{loop, [&displayed](const int progress) -> void {
                                      displayed.append(progress);
                                      el::io::printLine("青龙林 survey: "_el, progress, "%"_el);
                                  }};

    // Join this finite producer before dispatch to make the captured burst deterministic.
    auto producer = std::jthread{[&updates]() -> void {
        for (const auto progress : {10, 20, 30, 40}) {
            updates.publish(progress);
        }
    }};
    producer.join();
    if (loop->queuedEventCount() != 1) {
        throw el::LogicFailure{"Expected one queued notification for the whole burst."_el};
    }
    const auto firstDispatch = loop->runUntilIdle();

    // Consumption clears the pending flag, so a later burst schedules a fresh notification.
    updates.publish(60);
    updates.publish(80);
    const auto secondDispatch = loop->runUntilIdle();

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0m青龙林 survey: 40%
        青龙林 survey: 80%␛[0m

.. erbsland-demo-end::

The first burst displays 40 percent and the later burst displays 80 percent.
The demo verifies one queued notification for the first burst and one delivery for each burst.
Clearing the pending flag permits the later delivery; retaining the latest value alone would not be enough.

.. erbsland-in-practice-end::

Coalescing only bounds notification backlog if the pending flag prevents additional submissions.
Replacing a shared value while posting a callback for every update still creates a growing queue.
At most one notification should be waiting; a producer can queue the next one while the current callback is displaying
its copied value.

The notification path must remain available.
A silent rejection on a full or quitting loop can leave a pending flag set with no callback to clear it.
Stop producers and finish pending deliveries before closing that path, and retain the recipient until delivery ends.
Coalescing is an application protocol, not an acceptance guarantee supplied by
:cpp:func:`invoke() <erbsland::event::EventTarget::invoke>`.

Debouncing, covered in :doc:`scheduling_work`, delays an action until notifications become quiet.
It is useful once those notifications reach their owner loop, but rescheduling a timer does not bound a backlog of
cross-thread input callbacks already waiting to reach it.
Neither debounce nor coalesced progress replaces the operation's required completion signal.

Blocking Admission and Circular Waits
=====================================

``Block`` trades growing or discarded queue entries for a waiting producer.
It can be useful when a separate producer can wait and an independent consumer is guaranteed to keep draining.
Posting before that consumer starts can block too, so starting the consumer must not depend on the blocked call
returning.

If the active consumer posts into its own full queue, waiting would prevent the only consumer from freeing a slot.
The policy calls ``std::terminate()`` in that case.
Two loops can also deadlock when each fills the other's queue and then blocks on another submission:

.. mermaid::

    flowchart LR
        A[Loop A callback] -->|Waits for space in B's full queue| B[Loop B callback]
        B -->|Waits for space in A's full queue| A

Neither callback can return to drain its own queue.
Bounded asynchronous requests avoid adding this blocking dependency to a loop-to-loop exchange.
They still need correct completion and failure accounting.

Stop and quit wake producers waiting inside blocking admission; those producers discard their pending submissions.
They cannot release a callback waiting in arbitrary application code or interrupt an external blocking operation.
:doc:`shutting_down_event_work` explains cancellation and joining when such work must end.

What Queue Counts Measure
=========================

:cpp:func:`EventLoop::queuedEventCount() <erbsland::event::EventLoop::queuedEventCount>` is a synchronized snapshot of waiting callbacks, not a reservation for the next
submission.
Another producer can post between your count check and your own call.
Admission performs the actual ordinary-capacity check.

The count includes ordinary waiting events and reserved completion callbacks.
Some background operations and deadlines reserve their completion capacity separately; those callbacks do not consume
:cpp:func:`maximumQueuedEvents() <erbsland::event::EventLoopOptions::maximumQueuedEvents>`, so the observed count can exceed that ordinary limit.
See :ref:`event-loop-options` for the reservation contract.

The executing callback, delayed scheduler work not yet queued, timers not yet queued, and reserved quit control are
excluded from the count.
The ordinary queue limit also does not bound events retained by blocked producers, stored errors, or payload bytes.
A single queued callback can retain a large buffer, and a single executing callback can delay the loop despite a queue
count of zero.

Ordinary timer and delayed-invocation callbacks become subject to capacity when the scheduler posts them.
A discarding policy can lose such a delivery.
Rejected repeating occurrences are skipped and rescheduled while admission remains open, without guaranteeing delivery
of the missed occurrence.
A loop queue limit likewise does not control a network peer's buffers or transport flow; those limits belong to the
relevant network source.

Observe queue counts to identify accumulating waiting work, but use operation accounting for total outstanding demand
and callback duration for responsiveness.
These measurements answer different questions and help you choose which part of the workload to change.
