..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Events; Communication Between Loops
    single: Events; Requests and Results
    single: Events; Signals and Notifications
    single: Bindings; Parameters and Replies
    single: Events; Obsolete Replies

************************************************
Sending Requests and Results Between Event Loops
************************************************

When two components run on different event loops, how can one ask the other to do work and receive its result? The
familiar function-call model needs one change: the receiving loop decides when to run the requested code.
The sender submits a message and continues; a result arrives through another message later.

This page explains that exchange, from a notification with no data to a request carrying parameters and a bound reply.
You will learn where each part executes, how values survive queued delivery, and why a reply is different from a
synchronous return value.
It builds on the state ownership described in :doc:`event_workflows`.

From Synchronous Calls to Queued Messages
=========================================

In a synchronous call, the caller enters the function on its current thread and resumes after that function returns.
Arguments pass into the active call, and a return value passes back through the same call sequence.
Calling an ordinary callback follows that same rule.
Neither a method name nor the fact that its object belongs to a worker changes the executing thread.

To execute code on another loop, submit work to that loop's
:cpp:class:`EventTarget <erbsland::event::EventTarget>`.
The target queues a callback, and the receiving loop calls it when it can dispatch the work.
The submission does not wait for that callback's result.
The sender can continue its current callback and then return to its own loop while the receiving loop works
independently.
If the loops run on different threads, their callbacks can run concurrently.

.. list-table::
    :header-rows: 1
    :widths: 25 37 38

    * - Boundary
      - Ordinary function call
      - Queued delivery
    * - Execution
      - The function runs on the calling thread.
      - The handler runs when the receiving loop dispatches it.
    * - Parameters
      - They are available during the active call.
      - They must survive until the handler runs.
    * - Result
      - A return value resumes the waiting caller.
      - A separate reply delivers the result to a receiving handler.
    * - Failure in the work
      - An exception can propagate through the active call.
      - The operation reports failure later, or the receiving loop handles it.

Queueing is a scheduling boundary, not a promise about which thread runs its next instruction first.
A worker may begin processing while the sender is still finishing its callback.
Likewise, independent loops do not establish one global order for all messages.
Express required dependencies through completion messages and operation state.

Sending a Notification or Request
=================================

You can think of an event as a signal telling a component that it should react.
Here, a signal means an application notification delivered through the event loop; it does not mean an operating-system
signal.
A notification reports something that happened, a request asks the receiver to perform work, and a reply reports an
outcome.
They use the same queued-delivery mechanism; their meaning comes from your application.

:cpp:func:`invoke() <erbsland::event::EventTarget::invoke>` queues a callback whose work is already specified.
The expression ``target->invoke(callback)`` executes on the sending thread; the callback's body executes on the
receiving loop later.
Capture the request's input in that callback so it remains available after the sender returns.
A notification that needs no data can simply queue the receiving action.

Queueing selects where the action runs; it does not create a thread or transfer an object's ownership automatically.
The receiver still needs an owner that runs its loop, and any object used by the callback must remain alive.
A directly called method that touches another loop's mutable state bypasses this boundary.
Keeping that access inside the queued handler preserves the state ownership rule.

Even when sender and receiver use the same loop, ``invoke()`` queues the callback rather than calling it inline.
It therefore cannot give you the handler's result as its own return value.
Submission can itself fail, and configured queue limits can affect admission; those policies are covered in
:doc:`controlling_event_load`.
The essential distinction is that submitting work does not wait for its completion.

Passing Parameters with Bindings
================================

Sometimes the receiving action is known now, but its arguments will only be available later.
:cpp:func:`bindEvent() <erbsland::event::bindEvent>` packages a receiving target and a handler into a callable.
Creating the binding prepares this route without submitting work.
Calling the binding supplies the arguments, stores them for delivery, and queues the handler on the captured target.
When the receiving loop dispatches it, the handler gets those arguments as ordinary C++ parameters.

A binding can therefore represent either a request or a result notification.
Its producer calls it like a callback and does not need to manage the receiving loop directly.
The handler returns ``void``: calling the binding provides no synchronous return value from that handler.
You can call a binding repeatedly, with each call submitting a separate delivery.
Delivery remains queued even if you call it from its own receiving loop.

Without an explicit target, ``bindEvent()`` captures
:cpp:func:`currentLoop() <erbsland::event::currentLoop>` when you create it.
Create that binding inside a callback dispatched by the intended receiving loop.
If you already have the target, pass its :cpp:type:`EventTargetPtr <erbsland::event::EventTargetPtr>` explicitly as the
first argument, followed by the handler.
That form also works when creating the binding outside the receiving loop.
In both cases, later calls use the captured destination regardless of which thread makes them.

For a request, bind its handler to the loop that performs the work.
For a reply, bind its handler to the loop that owns the requesting operation's state.
Preparing the reply inside the worker without an explicit target would capture the worker's loop instead.
:doc:`event_bindings` develops the callable and member-function forms and their ownership choices.

Sending a Result Back
=====================

A request that needs a result has two delivery paths.
First, the requesting component supplies the work's input and a reply callback to the worker.
Then the worker supplies its output to that reply callback.
When the reply is bound to the requesting loop, calling it queues the output there for a later handler to accept.
The original request call has already returned; the reply starts a new callback rather than resuming that call.

Prepare the reply route before submitting the request.
This lets the requesting component choose where the result belongs and which operation will receive it.
The worker only needs to call the supplied callback with its output.
The same arrangement can carry several progress updates followed by one final outcome, provided the receiver can
distinguish progress from completion.

.. mermaid::

    sequenceDiagram
        participant A as Requesting loop / operation state
        participant B as Worker loop
        A->>A: Record pending state and bind reply to A
        A-->>B: Queue request with input and reply callback
        Note over A: Return to dispatch and handle other events
        Note over B: Dispatch the request when ready
        B->>B: Perform work using received input
        B-->>A: Call bound reply to queue result delivery
        Note over B: Continue and return to dispatch
        Note over A: Dispatch the reply when ready
        A->>A: Check current state and accept result

Both loops must continue dispatching for this round trip to complete.
Blocking the requesting loop until a reply runs on that same loop prevents it from receiving the reply.
The requesting component instead records that work is pending and returns, as described in :doc:`event_workflows`.
A procedural caller waiting for a separate worker to finish is a different arrangement, covered in
:doc:`processing_work_on_event_threads`.

.. erbsland-in-practice-begin:: Sending a Search and Receiving Its Matches

The :doc:`file-search application </demo_apps/event/file_search>` keeps its result state on main and scans on a running
managed worker.
These members retain the result state and worker and record whether replies are still wanted:

.. erbsland-demo::
    :source: event/apps/FileSearch/src/FileSearchApp.hpp
    :show: c:Main-loop result state, c:Retained until Application, c:Suppresses late
    :sha256: d64f80a0b80b30a206ab0676f62fc36a1034ed26e2d8f09ed5b794af76195af3

.. code-block:: cpp

    SearchResults _results;                                      ///< Main-loop result state.

    el::ManagedEventThreadPtr _worker;                           ///< Retained until Application joins it.

    bool _exiting{false};                                        ///< Suppresses late rendering and result deliveries.

.. erbsland-demo-end::

``submitSearch()`` runs on main.
After compiling the current input into an immutable pattern, it creates a reply on main, retaining the current request's
identity in ``generation``:

.. erbsland-demo::
    :source: event/apps/FileSearch/src/FileSearchApp.cpp
    :show: c:Bind on main(+1,+6)
    :sha256: 868f84ddf2a0bc29f7a24c3acaf5dbd3738a80f1cbf91a7deb5299d88f48aa0c

.. code-block:: cpp

    const auto generation = _results.generation();
    // Bind on main before crossing to the worker. The reply retains this request's identity.
    auto reply = el::bindEvent([this, generation](SearchUpdate update) -> void {
        if (!_exiting) {
            _results.receive(generation, std::move(update));
            _dirty = true;
        }
    });

.. erbsland-demo-end::

The reply accepts a ``SearchUpdate`` containing owned result data.
Its handler checks whether the application is exiting, hands the update to the result state, and marks the display as
changed.
The capture of ``this`` borrows the application, whose lifetime covers main-loop dispatch and managed-worker shutdown.

The following statement submits the request to the worker, transferring the input and the prepared reply together:

.. erbsland-demo::
    :source: event/apps/FileSearch/src/FileSearchApp.cpp
    :show: f:FileSearchApp::submitSearch(-23,-1)
    :sha256: 508ca286a0165464261d89d760935edf99817cabb86029406c53160f03f88b21

.. code-block:: cpp

    _worker->loop()->invoke(
        [root = _root,
            pattern = std::move(pattern),
            generation,
            token = _generation,
            stopToken = _worker->stopToken(),
            reply = std::move(reply)]() -> void {
            const auto cancelled = [token, generation, stopToken]() -> bool {
                return token->load() != generation || stopToken.isStopRequested();
            };
            SearchScanner{root, pattern}.scan(cancelled, reply);
        });

.. erbsland-demo-end::

The owned root path and immutable pattern are the search parameters.
``generation`` identifies this request; ``token`` shares an atomic generation counter for cooperative cancellation,
while ``stopToken`` reports worker shutdown.
These tokens let the worker stop without reading main's mutable result state.

The queued body calls ``scan()`` on the worker.
When the scanner calls the supplied ``reply`` with a batch, the binding queues it on main; the scanner does not call
``_results.receive()`` on the worker.
``submitSearch()`` returns after submission, allowing main to keep handling input and delivered results.
The complete walkthrough covers input handling and cancellation.

.. erbsland-in-practice-end::

Owning Data and Accepting Replies
=================================

Keeping Values Alive Until Delivery
-----------------------------------

The gap between submission and dispatch changes the lifetime requirements for parameters.
With ``invoke()``, the queued callback must retain the values it will use through its captures.
With a binding, each call stores its arguments: lvalues are copied, rvalues are moved, and the stored arguments are
passed to the handler as rvalues.
This lets a producer finish using a result while the receiving loop has yet to process it.

Owning the argument storage does not necessarily mean owning everything it refers to.
A copied pointer or view still refers to the original storage; an explicit reference wrapper still borrows its object.
A shared pointer retains its pointee but does not synchronize access to it.
Transfer independent values, share immutable data, or provide synchronization for genuinely shared mutable data.
Keep each component's changing state on its owner loop.

The receiving object also needs to survive delivery.
A strong member binding retains its recipient; a weak member binding can skip delivery if the recipient disappears.
A lambda capturing ``this`` only borrows it.
Retaining an event target keeps the target object alive, but does not keep its thread running or guarantee continued
dispatch.
See :doc:`event_bindings` and :doc:`callback_lifetimes` for those separate ownership choices.

Accepting Only Results That Still Belong
----------------------------------------

The requesting loop can change its state while the worker computes.
A result may be correct for the input that produced it and still be obsolete for the input currently selected.
Give each replaceable request an identifier and retain that identity in its reply.
On delivery, compare it with the current request before changing state.
For an operation with a terminal outcome, also check whether it is still pending.

.. erbsland-in-practice-begin:: Rejecting a Reply from an Older Search

In FileSearch, editing the query advances the current generation immediately.
The reply shown above still holds the generation of the request that produced it.
The result handler begins with this check:

.. erbsland-demo::
    :source: event/apps/FileSearch/src/SearchResults.cpp
    :show: f:SearchResults::receive(-1,-11)
    :sha256: 1d46365543424c8bb4a48b16e17b747d1535cfcdaf11ca89253293c620482651

.. code-block:: cpp

    if (generation != _generation) {
        return;
    }

.. erbsland-demo-end::

A match allows the handler to append paths and process the batch's completion status.
A mismatch discards the entire update, including an obsolete final status.
The worker's cancellation check reduces unnecessary scanning, while this receiving-side check prevents an already queued
result from updating the new search.

.. erbsland-in-practice-end::

Cancellation and result acceptance serve different purposes.
Asking a producer to stop cannot retract a result already sent, and a cancellation request queued on a busy worker
cannot run until that worker returns to dispatch.
:doc:`shutting_down_event_work` explains cooperative stopping and shutdown ordering.

Completing Requests When Work Fails
-----------------------------------

A required result needs a failure path as well as a success path.
Report an error through the reply or arrange for the operation's owner to end the pending state through its failure
policy.
If a recipient may disappear or a reply may never arrive, cancellation or a deadline must also be able to end the wait.
Optional progress notifications can be skipped; required completion needs an owner that remains available.

An exception in a worker callback cannot unwind the sender's call stack through the request boundary.
The worker's error policy and the operation's completion policy must therefore agree on how failure ends the request.
:doc:`handling_event_errors` explains those choices.
The receiving loop must also remain responsive for a deadline or failure notification to run; neither can interrupt work
that is blocking another thread.
