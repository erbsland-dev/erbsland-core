..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Events; Driving a Loop Directly
    single: Event Loops; Direct Dispatch

****************************
Driving Event Loops Directly
****************************

A procedural program can borrow the event system for part of its work.
For example, it may need to process a few queued callbacks before advancing to the next stage, or keep checking a
deadline while it waits for a result.
With :cpp:func:`EventLoop::create() <erbsland::event::EventLoop::create>`, the caller owns the loop and decides when to
dispatch it.
Callbacks run on the thread making the dispatch call.
:doc:`using_event_loops` covers the application lifecycle that can take over this responsibility for you.

Dispatching Work That Is Ready
==============================

You can queue a callback with :cpp:func:`invoke() <erbsland::event::EventTarget::invoke>` as soon as you have a loop.
It stays queued until you give the loop time to run.
Call :cpp:func:`runUntilIdle() <erbsland::event::EventLoop::runUntilIdle>` to process the work available now, including
ready work queued by those callbacks.
The call returns the number of events it processed once it finds no immediate work.
A callback that continually queues another ready callback can keep this call busy indefinitely.

During dispatch, :cpp:func:`currentLoop() <erbsland::event::currentLoop>` gives callbacks the target of the loop running
on this thread.
Queue setup that needs this context, such as creating a loop-bound event source and registering its handlers.
When the dispatch call returns, it restores the thread's previous context.
A procedural caller outside any dispatch can keep using its stored loop pointer to queue the next operation.

.. erbsland-in-practice-begin:: Reading Queued Journal Entries

The host queues two journal entries and prints a message before giving the loop control.
Each callback keeps its own entry by value.

.. erbsland-demo::
    :source: event/snippets/DirectEventLoops/ReadyEntries.cpp
    :show: f:readyEntries
    :exec: event/direct_event_loops --demo ReadyEntries
    :sha256: 71bfa2a7804e7e680265ed73dc54ff138bc673263658a4396504b7f96bc42e5c

.. code-block:: cpp

    void readyEntries() {
        const auto loop = el::EventLoop::create();
        const auto entries = el::StringList{"抵达山口"_el, "发现石桥"_el};
        for (const auto &entry : entries) {
            loop->invoke([entry]() -> void { el::io::printLine("Journal: "_el, entry); });
        }

        el::io::printLine("Entries queued."_el);
        const auto dispatched = loop->runUntilIdle();
        if (const auto error = loop->takeError()) {
            std::rethrow_exception(error);
        }
        el::io::printLine("Back in the host after "_el, dispatched, " callbacks."_el);
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mEntries queued.
        Journal: 抵达山口
        Journal: 发现石桥
        Back in the host after 2 callbacks.␛[0m

.. erbsland-demo-end::

The line before the entries comes from the host, the entries appear during dispatch, and the final line comes from the
host again.
Here each queued event corresponds to one entry, so the returned count is two.

.. erbsland-in-practice-end::

Waiting for a Result Between Host Turns
=======================================

An idle loop can still have a timer due later or a source waiting for input.
Give the operation an explicit completion condition, such as a flag set when its result has been delivered.
If the host has other duties, let it do a bounded piece of its own work and call
:cpp:func:`runOnce(maximumWait) <erbsland::event::EventLoop::runOnce>` for one dispatch cycle.
A true return means the loop processed an event.
That event might have been setup or unrelated input.
A false return means this cycle processed no event.
Check the operation's state to decide whether another turn is needed.

Choose a maximum wait based on how soon the host needs control again.
The limit applies to waiting for an event.
Once a callback starts, its code runs to completion on the calling thread, even if that takes longer than the wait
limit.
Use short callbacks and a separate deadline for an operation that might never produce a result.

.. erbsland-in-practice-begin:: Waiting for a Delayed Journal Entry

The first callback schedules a journal entry on the current loop.
The host allows up to five milliseconds of waiting per turn and checks its own deadline between turns.
The delivery flag lives in the host and both callbacks run on its thread.

.. erbsland-demo::
    :source: event/snippets/DirectEventLoops/DelayedEntry.cpp
    :show: f:delayedEntry
    :exec: event/direct_event_loops --demo DelayedEntry
    :sha256: 12c754c2982174c56a0c0723f105e25e7740c26b25ea617cb1ed17aa9160d448

.. code-block:: cpp

    void delayedEntry() {
        auto delivered = false;
        const auto loop = el::EventLoop::create();
        loop->invoke([&delivered]() -> void {
            // Dispatch has established the current loop for this callback.
            el::currentLoop()->invokeAfter(el::Milliseconds{30}, [&delivered]() -> void {
                el::io::printLine("Journal: "_el, "月下渡河"_el);
                delivered = true;
            });
        });

        const auto deadline = el::TimePoint::inFuture(el::Seconds{2});
        while (!delivered) {
            if (el::TimePoint::now() >= deadline) {
                throw el::ApplicationError{"Timed out waiting for the journal entry."_el};
            }
            (void)loop->runOnce(el::Milliseconds{5});
            if (const auto error = loop->takeError()) {
                std::rethrow_exception(error);
            }
        }
        el::io::printLine("The host has received the journal entry."_el);
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mJournal: 月下渡河
        The host has received the journal entry.␛[0m

.. erbsland-demo-end::

Only the callback that writes the entry can mark it delivered.
The host inspects captured errors after every turn so a failed callback reports its cause before the deadline can turn
it into a timeout.
The output stays the same however many turns the delivery takes.

.. erbsland-in-practice-end::

Giving the Loop Control Until Shutdown
======================================

If the host has nothing to do between events, ``runOnce()`` without an argument can wait for the next event without a
time limit.
When the callbacks themselves know when the work is complete, :cpp:func:`run() <erbsland::event::EventLoop::run>` is
simpler still.
Queue setup, enter the loop, and let the last required callback initiate shutdown.
After dispatch returns, check for captured operational errors with
:cpp:func:`takeError() <erbsland::event::EventLoop::takeError>`.
:doc:`handling_event_errors` explains how the loop's policy determines which failures it captures.

Use :cpp:func:`stop() <erbsland::event::EventLoop::stop>` when the host needs to regain control and may drive the same
loop again.
The next dispatch call can process work that was left queued.
Use :cpp:func:`quit() <erbsland::event::EventLoop::quit>` for final closure.
It closes admission and lets dispatch drain already accepted queued events.
A future timer or an outstanding operation may still need to produce its event, so wait for required completions before
requesting quit.
:doc:`shutting_down_event_work` covers that ordering for multiple sources.

Only one dispatch call can be active on the same loop.
If a callback needs more work, queue a continuation and return so dispatch can proceed.
If your host must remain free even while a callback is executing, give the loop its own thread.
The :ref:`unmanaged worker template <event-unmanaged-worker-template>` shows how its owner starts the thread, arranges
termination, joins it, and checks for errors.
