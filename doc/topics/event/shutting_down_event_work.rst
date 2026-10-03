..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Events; Cancellation
    single: Events; Shutdown
    single: Events; Queue Draining

*********************************
Cancelling Work and Shutting Down
*********************************

Shutdown is complete when work no longer needs callbacks and no running thread can access the resources you release.
Requesting cancellation is often only the first step.
This page explains how to stop producing work, finish or abandon outstanding operations, keep required completion paths
running, and finally end and join their threads.

The ownership rules in :doc:`callback_lifetimes` are the foundation.
For the examples, follow either the application-part design in :doc:`using_event_loops_in_application_parts` or the
worker design in :doc:`processing_work_on_event_threads`; you do not need both architectures in one application.

Resource Completion Comes Before Loop Termination
=================================================

Consider a server that has stopped accepting new work but still has committed responses to send.
Its close operation needs the owner's loop to process network events and deliver a final notification.
Ending that loop first would remove the mechanism that finishes the close.

A useful shutdown sequence therefore follows the resource's dependencies:

#. Stop accepting application requests and stop producers that could create more operations.
#. Ask active operations to finish or cancel, keeping their required reply targets running.
#. Observe the required completions, then release sources, timers, and subscriptions on their owner loops.
#. End the loops whose work is complete, join their threads from another thread, and release the remaining owners.

An application-level stopping state is useful during the first steps.
It prevents new requests while leaving loop admission open for close commands and final callbacks.
Closing the loop's queue is a later decision.
With multiple resources, count the terminal operations explicitly; the first final callback does not prove that every
resource has finished.

The file-server application demonstrates this sequence through an application part.
Its ``_server`` member retains the HTTP source for the entire close operation.
During initialization, it installs the error and final handlers before starting that source.
When shutdown begins, the framework calls ``stopping()`` on the part's own loop.
The excerpts below show the retained member, the end of the handler chain, and the shutdown hooks.

.. erbsland-demo::
    :source: event/FileServer/ServerPart.hpp
    :show: c:Event source owned, c:Bind to loopback(+6,-1), f:stopping, f:cleanup
    :sha256: 2b5beb6e43e4b66bda3775efd64b624c557cfb54adad3c59d1b0526a4d12616d

.. code-block:: cpp

    el::HttpServerPtr _server; ///< Event source owned by this part's loop.

    .onError([](const el::NetworkErrorContext &error) -> void { throw el::NetworkError{error}; })
    .onFinal([this]() -> void {
        if (state() == el::ApplicationPartState::Stopping) {
            completeStopping();
        }
    });

    void stopping() override {
        // Allow committed responses to drain before ending this part's event loop.
        if (_server == nullptr || _server->state() == el::NetworkSourceState::Closed ||
            _server->state() == el::NetworkSourceState::Failed) {
            completeStopping();
        } else {
            _server->close();
        }
    }

    void cleanup() noexcept override {
        if (_server != nullptr) {
            _server->abort();
            _server.reset();
        }
    }

.. erbsland-demo-end::

:cpp:func:`HttpServer::close() <erbsland::network::HttpServer::close>` begins graceful closure.
Returning from ``stopping()`` lets the loop continue dispatching.
Only the final handler calls :cpp:func:`completeStopping() <erbsland::core::ApplicationPart::completeStopping>` after
asynchronous shutdown reaches its terminal notification.
When the source is already absent, closed, or failed, the hook completes immediately because no close work remains.

.. mermaid::

    sequenceDiagram
        participant Manager as Part manager
        participant Owner as Part loop
        participant Server as Retained server
        Manager->>Owner: stopping()
        Owner->>Server: close()
        Note over Owner: Return and keep dispatching
        Server->>Owner: onFinal()
        Owner->>Manager: completeStopping()
        Note over Owner: Dispatch ends, cleanup() releases source

:cpp:func:`cleanup() <erbsland::core::ApplicationPart::cleanup>` runs after dispatch ends.
It performs unconditional release and does not begin another operation that needs callbacks.
The fallback ``abort()`` also makes cleanup safe after partial initialization or failure.
For this source's close and abort guarantees, see :doc:`/topics/network/using-http-servers`.
For registration, readiness, and the complete lifecycle example, see :doc:`using_event_loops_in_application_parts`.

Cancellation Belongs to the Operation
=====================================

There is no universal meaning of "cancel all event work".
A timer, observer registration, network source, loop, and application each have a different scope:

.. list-table::
    :header-rows: 1
    :widths: 25 75

    * - Action
      - Purpose
    * - Operation cancellation
      - Abandon one operation according to its source's contract; completion notifications may differ by API.
    * - Graceful source closure
      - Finish the source's accepted work and observe its required terminal notification while its loop still runs.
    * - Loop ``quit()``
      - Close queue admission and drain already accepted queued work.
    * - Loop ``stop()``
      - End the current dispatch run without draining its waiting queue.
    * - Application ``quit()``
      - Request application-wide shutdown, including the managed part and thread lifecycle.

Stopping a retained timer prevents further scheduled activity according to the timer's contract.
Cancelling a subscription prevents future observer invocations, but a callback already executing may still finish.
Neither action proves that another thread has stopped accessing a borrowed recipient.
See :doc:`callback_lifetimes` for that distinction and for the lookup example whose cancellation suppresses its result
and error callbacks while still allowing a final notification.

A workflow can also become logically cancelled before its underlying work ends.
Mark it terminal, stop its retry and deadline timers, and reject obsolete replies by request identity.
That prevents an abandoned result from changing current state; it does not interrupt a running computation.
:doc:`communicating_between_loops` applies identity checks across thread boundaries.

For lengthy computation, cancellation must be cooperative.
Divide the work into bounded steps so the worker can process a queued cancellation command between steps, or use an
explicitly synchronized cancellation signal checked by the computation.
A cancellation command on the same loop cannot run while the current callback is still calculating or blocked in I/O.
The I/O operation also needs its own timeout or cancellation facility if it can block indefinitely.

What Graceful Loop Quit Drains
==============================

:cpp:func:`EventLoop::quit() <erbsland::event::EventLoop::quit>` closes admission and lets already accepted queued
callbacks finish.
It stops backend polling; it does not wait for every timer deadline or native completion that might eventually produce
an event.
A callback that runs during draining cannot rely on posting another accepted callback to that loop.

This small demo queues two satellite observations, requests quit, and then tries to queue a third.
It runs the loop only after all three submissions so the admission boundary is unambiguous:

.. erbsland-demo::
    :source: event/EventShutdown/DrainAcceptedWork.cpp
    :exec: event/event_shutdown --demo DrainAcceptedWork
    :show-cmd-line:
    :sha256: 682ef16d9b7a631412e4dea3c50908cad35fff0bae99ac145ffb35eae9bf648b

.. code-block:: cpp

    /// Finish accepted satellite observations before ending dispatch; reject later work.
    void drainAcceptedWork() {
        const auto loop = el::EventLoop::create(el::EventLoopOptions{}.setErrorPolicy(el::EventLoopErrorPolicy::Quit));
        auto observations = el::StringList{};
        loop->invoke([&observations]() -> void { observations.append("Molntäcke"_el); });
        loop->invoke([&observations]() -> void { observations.append("Havsis"_el); });
        loop->quit();
        loop->invoke([&observations]() -> void { observations.append("Snödjup"_el); });
        loop->run();

        if (const auto error = loop->takeError()) {
            std::rethrow_exception(error);
        }
        if (observations != el::StringList{"Molntäcke"_el, "Havsis"_el}) {
            throw el::LogicFailure{"Quit did not preserve exactly the accepted observations."_el};
        }
        for (const auto &observation : observations) {
            el::io::printLine("Recorded: "_el, observation);
        }
    }

.. erbsland-demo-run::

    .. code-block:: console

        $ event/event_shutdown --demo DrainAcceptedWork

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mRecorded: Molntäcke
        Recorded: Havsis␛[0m

.. erbsland-demo-end::

The verified result contains exactly the first two observations, in order.
The later invocation is discarded; returning from ``invoke()`` is not an acknowledgement that a callback will execute.
The captured vector is safe here because all dispatch happens on the calling thread and ``run()`` returns before the
vector leaves scope.
This local loop isolates queue admission from application and resource shutdown.

In a request/reply exchange, quitting the receiver as soon as the last command is sent is too early.
The workers have not necessarily posted their replies yet.
Keep the receiver open until required replies are accounted for, then finish the workers and receiver in an order that
preserves any remaining dependencies.
The preview demo waits for all of its replies before asking the application to quit.
Likewise, the server above calls ``completeStopping()`` after source closure, not immediately after requesting it.

An immediate :cpp:func:`stop() <erbsland::event::EventLoop::stop>` ends dispatch without draining queued callbacks.
It does not interrupt the callback already running, and it does not mark waiting application tasks as cancelled.
If those callbacks would have reported completion, your shutdown logic must account for their abandonment separately.
For a directly controlled loop, stopping a run is also distinct from permanently closing admission with ``quit()``; it
is not a resource-cleanup protocol.

Failure and Shutdown Deadlines
==============================

A graceful close can fail or take too long.
Decide how the owner transitions to failure and releases resources even when the expected successful completion never
arrives.
Keep any deadline on a loop that remains able to dispatch until that decision is made.
Once a deadline wins, mark the shutdown failed, use the source's abort or release path, and do not continue waiting for
callbacks that path suppresses.
The application may then end the affected loops and join their threads.

The file server's error handler throws so a source failure reaches the part manager.
The default manager policy stops the part graph; ``cleanup()`` still releases the server safely.
The manager also bounds graceful stopping, with a default timeout of 30 seconds per part.
A timeout records a failure and requests loop termination.
These lifecycle policies and their configuration are covered in :doc:`/topics/core/application_parts`.

A deadline does not forcibly interrupt C++ code.
A timer on a blocked loop cannot dispatch at all, and a timeout detected on another loop still cannot make an unbounded
callback return.
Therefore callbacks, lifecycle hooks, and blocking operations need their own bounded or cooperative behavior.
A worker join can otherwise remain blocked even after shutdown has been requested.

Ending and Joining Worker Threads
=================================

A worker's :cpp:func:`quit() <erbsland::event::EventThread::quit>` is an asynchronous, idempotent request to end its
loop gracefully.
:cpp:func:`join() <erbsland::event::EventThread::join>` waits for thread completion; it does not ask the thread to stop.
Request termination only after the worker's required asynchronous operations are complete, then join from an owner
thread whose dispatch is no longer needed by that worker.

For a worker whose only remaining work is already queued and needs no further callbacks, the final owner-side sequence
can be as short as:

.. code-block:: cpp

    worker->quit();
    worker->join();
    worker.reset();

The demo in :doc:`processing_work_on_event_threads` retains its application-managed workers until application shutdown
requests their termination and joins them.
Their registration does not replace that retained owner.
An :cpp:class:`UnmanagedEventThread <erbsland::event::UnmanagedEventThread>` has no application registration: its owner
must arrange the whole shutdown sequence explicitly.
The same quit-before-join and callback-lifetime rules apply when you own execution directly.

Joining a worker requires another thread: a callback on the worker cannot wait for its own thread to finish.
Destroying the final owning thread pointer also requests termination and joins a started worker, so release that final
pointer on another thread.
A worker callback that captures the only remaining owning pointer to its own thread can otherwise cause destruction on
that worker as the callback is released.
Keeping an owner-side handle through the join avoids that trap.

Event threads are one-shot objects.
After :cpp:func:`start() <erbsland::event::EventThread::start>` succeeds, they cannot be restarted, even after joining.
``isStarted()`` stays true after completion; ``isRunning()`` is only a snapshot of current execution.
Neither accessor is a startup or completion synchronization barrier.
Use a ready message for application readiness and a join for completed thread execution.
Quit may be requested before startup; if started afterward, the loop drains previously accepted work and ends.
Joining before startup or after a previous join is a no-op.
The precise lifecycle contract is in :doc:`/reference/event/event_system`.

A join does not transport worker exceptions.
Choose an error policy and a failure completion path before starting work; the managed-worker tutorial explains
``FailApplication`` and its application diagnostic boundary.
An exception escaping an event thread terminates the process rather than becoming a result of ``join()``.

Application-Wide Shutdown
=========================

Use :cpp:func:`Application::quit() <erbsland::core::Application::quit>` when the application should end, after the
resources you manage directly no longer need its loops.
For application parts, the framework provides the earlier coordination point through ``stopping()`` and
``completeStopping()`` so sources can close before those loops end.
Keep dependency ordering and service shutdown in that framework instead of manually quitting each part's loop.

:doc:`/topics/core/application_parts` explains dependency-aware stopping and cleanup.
:doc:`/topics/core/service_lifecycle` connects the same lifecycle to platform stop requests.
These are ways to enter the shutdown sequence; they do not change which callbacks your resources need to finish.

Required Completion After an Optional View Closes
=================================================

The pi calculator in :doc:`event_bindings` keeps the computation session alive independently of its display.
**S** cancels computation and leaves the stopped display open for browsing.
**Q**, **Escape**, and a positive timeout release the view and request cancellation, but main keeps dispatching until
required completion arrives:

.. erbsland-demo::
    :source: event/PiCalculator/src/PiCalculatorApp.cpp
    :show: f:PiCalculatorApp::requestExit
    :sha256: f71e6ad5eb62e08b7ba9c6674bb299b247efe3b44fc0a03a6e6fa8dc870bbd56

.. code-block:: cpp

    void PiCalculatorApp::requestExit() {
        _exitRequested = true;
        // Release optional UI immediately, but keep main open for strongly bound required completion.
        _view.reset();
        if (_session->isFinished()) {
            quit();
        } else {
            _session->requestStop();
        }
    }

.. erbsland-demo-end::

Releasing the weak notification recipient does not cancel computation or satisfy required completion.
The worker observes a separate cancellation flag between arithmetic advancements, and a waiting worker is woken without
requiring a queued command on its thread.
It flushes a nonempty partial batch before its final callback.
That callback is strongly bound to the session on main, where completion joins the worker before notifying the
application:

.. erbsland-demo::
    :source: event/PiCalculator/src/PiSession.cpp
    :show: f:PiSession::onFinished
    :sha256: 64a3dfecf53abc64afcfb0431ea296feb5e5593d22be08772b666ba247cb0338

.. code-block:: cpp

    void PiSession::onFinished(std::exception_ptr error) {
        _worker.join();
        _finished = true;
        if (_onUpdated) {
            _onUpdated();
        }
        // Retire target-retaining callbacks once the operation is terminal, including on exceptional shutdown.
        _onUpdated = {};
        _events.reset();
        if (auto completion = std::move(_onFinished)) {
            completion(std::move(error));
        }
    }

.. erbsland-demo-end::

The application still owns the session here.
After joining, the session retires its retained target and callbacks; the application can then quit main safely.
When **S** initiated the stop, completion instead leaves the history and view available.
The independent timeout timer remains active while the user browses.

An exception can end main dispatch before this normal sequence completes.
The inherited application loop propagates callback failures.
Application cleanup cancels and joins the calculation unconditionally, without depending on another delivery:

.. erbsland-demo::
    :source: event/PiCalculator/src/PiSession.cpp
    :show: f:PiSession::shutdown
    :sha256: 638d78ec4554c98a996b54fb99f983d67c7baf2875228a7769ae49ac106de7a1

.. code-block:: cpp

    void PiSession::shutdown() {
        // Exception cleanup can run after dispatch has ended; joining must not depend on a pending main-loop callback.
        _stopping = true;
        _worker.requestStop();
        _worker.join();
        // A stopped loop may still own callbacks retaining this session. Break the return ownership path.
        _onUpdated = {};
        _onFinished = {};
        _events.reset();
    }

.. erbsland-demo-end::

Cancellation can stop between advancements even while the next digit is being determined.
A single growing large-integer operation cannot be interrupted, so this design promises eventual cooperative cleanup,
not a fixed bound on joining time.
The retained history and arithmetic values grow with the computation.
