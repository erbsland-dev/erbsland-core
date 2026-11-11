..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Events; Application State
    single: Events; Procedural and Event-Driven Design
    single: Callbacks; State Transitions
    single: Events; Workflow Completion

************************************************
Designing Event-Driven Applications Around State
************************************************

In procedural code, you can often follow an operation from beginning to end by reading one function.
In an event-driven application, that operation may span several callbacks, with other work happening between them.
How do you keep its progress understandable when no single function stays in control?

Organize the operation around persistent state and let events drive changes to that state.
This page explains how that design differs from a procedural sequence, where to keep the state, and how callbacks use it
to decide what happens next.
The same principle applies to a whole application component or to one task within it.

From Sequential Execution to Event Handling
===========================================

A synchronous function can start an operation, wait for its result, and use that result in its next statement.
Its local variables retain the data, and its position in the call sequence tells you how far it has progressed.
The caller resumes when the function returns a result or throws an exception.
While it waits, that thread cannot return to an event loop to handle other work.

An event-driven operation separates starting work from reacting to its outcome.
A callback starts an asynchronous step and returns to the loop.
Later, a result notification makes another callback ready, and the loop dispatches it.
Between those callbacks, the loop can handle input, timers, and other operations.
There is no suspended call to the first callback that automatically resumes at its next statement.
The later callback must find the data and progress it needs in state that survived the first callback's return.

.. list-table::
    :header-rows: 1
    :widths: 25 37 38

    * - Design question
      - Synchronous sequence
      - Event-driven operation
    * - What determines the next action?
      - The next statement after a call finishes.
      - An incoming event interpreted against current state.
    * - Where does progress survive?
      - Local variables and active calls can hold it.
      - An owner retains it between callbacks.
    * - What happens while work is pending?
      - The calling thread waits for the call to finish.
      - The initiating callback returns and the loop dispatches other work.
    * - How does the caller receive the outcome?
      - Through a return value or an exception from the call.
      - Through a later completion notification.

The code inside an individual callback is still ordinary sequential C++.
What changes is how you connect steps whose results are not available yet.
Waiting for such a result inside a callback prevents its loop from dispatching other callbacks, including a result
handler that needs that same loop.

Keeping State Between Events
============================

Start with the information that must remain meaningful after a callback returns.
An operation may need its input, partial results, the step currently in progress, and the destination for its final
outcome.
A longer-lived component may also retain a current selection or a collection of active operations.
Keep related information together in an object whose lifetime covers the events that use it.

This object gives the operation a central place for decisions.
Callbacks receive new information and consult that shared context instead of each maintaining a separate view of
progress.
A phase such as pending, processing, or finished can make valid transitions explicit, but state includes the data as
well as the phase.
A small operation may need only a pending flag and a result; a larger one may benefit from named states.
Neither requires a general state-machine framework.

Centralizing related state does not mean putting all application data in one global object.
Each component or independent operation can have its own owner.
Several operations can be pending on the same loop, with each callback updating the state of the operation it belongs
to.
The event loop schedules those callbacks; your application defines what their events mean.

.. erbsland-in-practice-begin:: Keeping One Archive Operation Together

The :doc:`checksum packer </demo_apps/event/checksum_packer>` uses one ``Workflow`` object per archive.
Its persistent members hold the completion callback, progress, and the components that perform its two steps:

.. erbsland-demo::
    :source: event/apps/ChecksumPacker/src/Workflow.hpp
    :show: c:readonly setup data(+0,+7)
    :sha256: 94d0d144d623a8fe4edc360ce3af647ed7e17242cad742afdd2359529f4afa05

.. code-block:: cpp

    // readonly setup data:
    const el::LogStreamPtr _logStream; ///< The log stream to write output to during the operation.
    const OnDone _onDone;              ///< Callback for when the workflow is done.
    // owned by this working set and its events:
    State _state; ///< The state of the working set.
    // the workflow components
    Scanner _scanner; ///< The scanner logic.
    Packer _packer;   ///< The packer logic.

.. erbsland-demo-end::

The application retains each workflow through a shared pointer until cleanup, and the workflow derives from
``std::enable_shared_from_this<Workflow>``.
Its bound result callbacks also retain it while they need it.
``State`` records the phase: ``Initialized``, ``Scanning``, ``Packing``, ``Done``, or ``Failed``.
The scanner and packer perform individual steps; the workflow decides how those steps contribute to one completed
archive.

.. erbsland-in-practice-end::

Making Decisions from Events and Current State
==============================================

An event tells you that something happened.
The current state tells you what that event means for the operation now.
A completion can make the next step possible while an operation is pending, yet be irrelevant after that operation has
been cancelled or replaced.
The event's data alone cannot make that decision.

A handler therefore starts by establishing which operation the event belongs to and whether that operation expects it.
It incorporates the new information, records the resulting state, and initiates any work that follows.
Then it returns to dispatch.
Recording the new state before starting the next asynchronous step makes the operation ready to interpret that step's
notifications.

.. mermaid::

    flowchart TD
        event["An event becomes ready"] --> dispatch["Loop dispatches its handler"]
        state["Persistent operation state"] --> decision["Interpret event using current state"]
        dispatch --> decision
        decision --> update["Update data and progress"]
        update --> action["Start next work or report an outcome, if needed"]
        action --> return["Return to the event loop"]
        update -.->|"Retained for later events"| state

This is more than splitting a long function into smaller functions.
The connections are expressed by state and events rather than by one uninterrupted sequence of calls.
Some events advance the operation, some only update its data, and others leave it unchanged.
An impossible transition can indicate a programming error; an obsolete notification can be an expected consequence of
cancellation.
Choose that reaction according to the operation's rules.

.. erbsland-in-practice-begin:: Letting a Scan Result Decide the Next Step

The checksum workflow's ``start()`` runs on the main loop.
It records that scanning is pending before launching the scan:

.. erbsland-demo::
    :source: event/apps/ChecksumPacker/src/Workflow.hpp
    :show: f:start(-2,-1)
    :sha256: 57e8cc5422c44f60d344cbe08fb25c33c7ca2e3bfa1a53dcb9eaba4a1cf2012f

.. code-block:: cpp

    _state.requireAndSet(State::Initialized, State::Scanning);
    _scanner.scan(el::bindEvent(shared_from_this(), &Workflow::onScanned));

.. erbsland-demo-end::

``scan()`` starts background work and returns.
The binding arranges for the eventual result to reach ``onScanned()`` on this same loop, retaining the workflow through
``shared_from_this()``.
The later handler makes the next decision:

.. erbsland-demo::
    :source: event/apps/ChecksumPacker/src/Workflow.hpp
    :show: f:onScanned
    :sha256: 1dd72c0c118cdb89d63ba18308eb080d3c214696fa0c941afcda5a671f799755

.. code-block:: cpp

    void onScanned(FileWithDigestList fileList, std::exception_ptr error) {
        _scanner.shutdown();
        if (error != nullptr) {
            _state.requireAndSet(State::Scanning, State::Failed);
            _onDone(std::move(error));
            return;
        }
        _state.requireAndSet(State::Scanning, State::Packing);
        _packer.pack(std::move(fileList), el::bindEvent(shared_from_this(), &Workflow::onPacked));
    }

.. erbsland-demo-end::

``requireAndSet()`` checks that the workflow is still ``Scanning`` before changing its phase.
An error leads to ``Failed`` and completion notification; a file list leads to ``Packing`` and another asynchronous
step.
The method then returns, leaving the new phase in the persistent workflow.
Here ``shutdown()`` joins the scanner after it has reported its result, before the workflow advances.
The worker and archive implementation are covered in the application's walkthrough.

.. erbsland-in-practice-end::

Keeping State on Its Owner Loop
===============================

Give each mutable component or operation state one owner loop.
Perform its reads, decisions, and updates inside callbacks dispatched by that loop.
Because the loop executes one callback at a time, those callbacks do not access the state concurrently.
You can reason about each handler using the state left by earlier handlers without placing a mutex around every member.
This guarantee depends on all access following the ownership rule.
A shared pointer keeps an object alive but does not make access from another thread safe.

Background work can use independent input and produce a result without touching the owner's mutable state.
Send that result back to the owner loop, where a callback decides what it means for the operation.
An ordinary call to an object's method does not move execution to its owner loop.
Queue it through :cpp:func:`invoke() <erbsland::event::EventTarget::invoke>` or supply a callback bound to that loop
with :cpp:func:`bindEvent() <erbsland::event::bindEvent>`.
:doc:`communicating_between_loops` explains how these messages carry parameters and results.

Keep the owner's handlers short enough to let the loop process other events.
Moving lengthy work to another thread lets the owner remain responsive, but the worker must still have a safe way to
receive input and report completion.
:doc:`processing_work_on_event_threads` explains how to create and manage such workers.

Serialized access prevents simultaneous updates; it does not make every later event relevant.
While background work runs, another callback may replace the input or cancel the operation.
A result handler must therefore use the state at delivery time, not assume that the state at submission time still
applies.
An operation identifier and a pending state can distinguish a current result from one that should be discarded.

Representing Completion and Failure
===================================

Returning from a callback means that this reaction has finished.
It says nothing by itself about whether the whole operation has finished.
The operation's state must express that distinction: a step can finish while the operation still needs another step.
Only the owner has enough context to decide that the requested task has succeeded or can no longer continue.

Give the operation explicit terminal outcomes and notify its caller when it reaches one.
Record the outcome before sending the notification so that later observers see a completed operation.
A worker error needs a transition just as a successful result does; otherwise the state can remain pending forever.
An exception from a later callback does not unwind the earlier call that started the work.
Choose how the operation reports failure and how that interacts with application-wide error handling, as described in
:doc:`handling_event_errors`.

Once an operation has finished, later progress notifications must not reopen it or report a second completion.
For replaceable or cancellable work, check both the operation's identity and whether it is still pending before
accepting a result.
Stopping the producer can save work, but a notification already queued may still arrive.

.. erbsland-in-practice-begin:: Completing the Archive Rather Than Just One Step

A successful scan leaves the checksum workflow in ``Packing``.
Only a successful pack changes it to ``Done``; an error from either step changes it to ``Failed``.
Both terminal paths invoke ``_onDone`` after recording the state.
The application can then decide whether its other archive operations are complete or whether a failure should end the
application.
That application-wide decision is separate from the state of one workflow.

.. erbsland-in-practice-end::

Completion also needs an ownership plan: keep the state alive while required callbacks use it, and release worker
resources only after the workers have stopped accessing them.
:doc:`callback_lifetimes` covers callback recipients, and :doc:`shutting_down_event_work` explains how to finish or
cancel outstanding work before ending dispatch.
