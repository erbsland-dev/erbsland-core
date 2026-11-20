..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Events; Cancellation
    single: Events; Shutdown
    single: Events; Queue Draining
    single: Events; Cooperative Cancellation

***************************************
Cancelling Event Work and Shutting Down
***************************************

How do you stop an operation without losing its final result or destroying something it still uses? In event-driven
code, asking work to stop and knowing that it has stopped are separate steps.
The callbacks connecting those steps need somewhere to run.

This page explains cancellation, completion, loop termination, and thread joining as distinct operations.
You will learn how to keep required callbacks running during shutdown, when it is safe to end a loop, and how to release
resources when normal completion fails.
It builds on the ownership rules in :doc:`callback_lifetimes`.

Cancellation, Completion, and Termination
=========================================

An operation is a piece of application work, such as a request or calculation.
A loop is the mechanism that dispatches its callbacks, and a thread may run that loop.
Stopping one operation need not stop the loop or thread: other operations may still need them.

**Cancellation is a request to abandon work.** Its meaning depends on the operation: a computation can check a flag, a
timer can stop scheduling occurrences, and a network source can begin closing.
**Completion is the point at which the operation has reached a terminal outcome**, whether success, failure, or
cancellation.
The owner must know how that outcome is reported before deciding when to release the operation's state.

These APIs act at different levels:

.. list-table::
    :header-rows: 1
    :widths: 32 68

    * - Operation
      - What it establishes
    * - :cpp:func:`StopSource::requestStop() <erbsland::util::StopSource::requestStop>`
      - Signals a cooperative stop request; the computation must observe it and return.
    * - :cpp:func:`EventTimer::stop() <erbsland::event::EventTimer::stop>` / :cpp:func:`EventSubscription::cancel() <erbsland::event::EventSubscription::cancel>`
      - Cancels the timer's scheduled activity or future observer dispatch according to that API's contract.
    * - A source's ``close()`` / ``abort()``
      - Begins graceful closure or abandons source work; the source defines which final notifications remain possible.
    * - :cpp:func:`EventLoop::stop() <erbsland::event::EventLoop::stop>` / :cpp:func:`quit() <erbsland::event::EventLoop::quit>`
      - Ends a dispatch run or permanently closes loop admission and drains accepted queued work.
    * - :cpp:func:`EventThread::join() <erbsland::event::EventThread::join>`
      - Waits for the thread to finish; it does not request termination.
    * - :cpp:func:`Application::quit() <erbsland::core::Application::quit>`
      - Requests application shutdown, including the managed part and worker lifecycle.

None of these cancellation requests forcibly interrupts a callback already executing.
In particular, cancelling an observer does not wait for an invocation already in progress, or retract separate work that
an earlier invocation posted elsewhere.
Keep borrowed recipients alive until such access has ended; cancellation alone is not a lifetime guarantee.

Cooperative Cancellation of Computation
=======================================

A long computation needs places where it can notice that its result is no longer wanted.
Divide it into bounded units of work and check a :cpp:class:`StopToken <erbsland::util::StopToken>` before beginning
each unit.
The largest unchecked unit determines how long cancellation can take to be observed.
An unbounded blocking call needs its own timeout or cancellation facility.

Create a :cpp:class:`StopSource <erbsland::util::StopSource>` for an independently cancellable operation and pass its
:cpp:func:`stopToken() <erbsland::util::StopSource::stopToken>` by value to the work.
Calling :cpp:func:`requestStop() <erbsland::util::StopSource::requestStop>` signals all copies of that token without
waiting for the work to finish.
The request is irreversible; create a new source for a new operation.
Destroying the source does not request stopping, and token copies remain valid independently of the source's lifetime.

There are two useful places to check the token: inside an executing computation, between bounded steps, or at the start
of each continuation when the computation is split across callbacks.
The second arrangement also lets the loop dispatch other work between those steps.
:doc:`controlling_event_load` develops that scheduling pattern.

.. erbsland-in-practice-begin:: Stopping a shipment check at the first mismatch

The :doc:`manifest verifier </demo_apps/event/manifest_verifier>` checks the SHA-256 of each file in a shipment against
its manifest.
A full audit checks every entry.
An importer using ``--fail-fast`` wants to abandon the batch when a checksum differs, while still recording the file
that failed and how much work remains.

.. erbsland-demo::
    :source: event/apps/ManifestVerifier/src/VerificationSession.cpp
    :show: f:VerificationSession::record, f:VerificationSession::check
    :sha256: d8f1a3b274e1ff75e2f16edaf3d515fa98236fd9b5845734366ee38b56276d51

.. code-block:: cpp

    void VerificationSession::record(const el::ItemIndex index, const bool matches) {
        ++_checked;
        const auto &name = _entries.getRefOrThrow(index).name();
        if (matches) {
            el::io::printLine("OK        "_el, name);
        } else {
            ++_mismatches;
            el::io::printLine("MISMATCH  "_el, name);
            if (_failFast) {
                _cancellation.requestStop();
            }
        }
        schedule(index.incremented());
    }

    void VerificationSession::check(const el::ItemIndex index, const el::StopToken &stop) {
        if (stop.isStopRequested() || !index.isWithin(_entries.count())) {
            _owner->invoke([this]() -> void { finish({}); });
            return;
        }
        const auto &entry = _entries.getRefOrThrow(index);
        const auto matches = entry.matchesFile(_directory);
        _owner->invoke([this, index, matches]() -> void { record(index, matches); });
    }

.. erbsland-demo-end::

Consider a shipment containing a catalog, a price list, and a stock list.
The worker hashes the catalog and returns its result to the owner.
Only after recording it does the owner authorize the next check.
If the price list has changed, the owner records the mismatch before signalling the stop source.
The next command observes the token and sends a terminal reply without opening the stock file.
Without the flag, that same command verifies the stock file.

One file is the cancellation unit here.
The owner makes its decision between files, when no hash is in progress.
The 64 KiB reads bound the hash buffer size.
A different utility that accepts stop requests while hashing a large file would need to check the token inside that read
loop as well.

.. erbsland-in-practice-end::

Cancelling an Operation or Ending a Worker
------------------------------------------

:cpp:func:`EventThread::stopToken() <erbsland::event::EventThread::stopToken>` observes the permanent quit request of that thread's loop.
Capture it before submitting work, then poll :cpp:func:`isStopRequested() <erbsland::util::StopToken::isStopRequested>`
inside the computation.
It is signalled by explicit quit and application shutdown of managed workers, without needing another callback to run or
retaining the thread object.
Resumable loop :cpp:func:`stop() <erbsland::event::EventLoop::stop>` does not signal it.

An operation's own token has a narrower purpose: cancelling that operation while the worker remains available.
If either reason should end the computation, check both tokens between work units.
A request generation can also identify obsolete work, as explained in :doc:`communicating_between_loops`.
Rejecting an obsolete reply protects current state but does not itself stop the old computation.

A cancellation command posted to the busy worker's own loop cannot execute until its current callback returns.
A token signalled from another thread can be observed while that callback runs, provided the computation checks it.
Neither mechanism can interrupt a single unbounded calculation or operating-system call.

Keeping Required Completion Paths Alive
=======================================

Cancellation often starts an asynchronous exchange: the owner requests stopping, the producer finishes or abandons its
current step, and a final callback reports the outcome.
Keep both the operation state and its receiving loop alive until that exchange is complete.
A strong callback binding can retain the recipient, but retaining a loop target does not keep its dispatch running.

.. mermaid::

    sequenceDiagram
        participant Owner as Operation owner loop
        participant Worker as Worker thread
        Owner->>Worker: Signal operation cancellation
        Note over Owner: Keep dispatching required replies
        Worker->>Worker: Observe token and leave computation
        Worker->>Owner: Report terminal outcome
        Note over Worker: Worker loop remains available
        Owner->>Worker: quit(), after required work is complete
        Owner->>Worker: join(), when no owner callback is needed
        Note over Owner: Release worker and operation state

The terminal callback is evidence about the operation; joining is evidence that the thread has finished executing.
They are different boundaries.
If several operations share a worker, account for all required terminal outcomes before quitting that worker.
Do not join on a receiving loop while the worker still needs that loop to process a reply or acknowledgement.

An optional display has a different lifetime from required operation state.
A weak display notification may be skipped after its recipient disappears, but it must not be the only notification that
releases a request slot or permits shutdown to finish.

.. erbsland-in-practice-begin:: Closing a View While Its Session Finishes

The :doc:`/demo_apps/event/pi_calculator` keeps its calculation session independently of its display.
Its exit handler can release the view immediately while keeping main dispatch available for the session's required
completion:

.. erbsland-demo::
    :source: event/apps/PiCalculator/src/PiCalculatorApp.cpp
    :show: f:PiCalculatorApp::requestExit
    :sha256: 46f7283fdbafb7c06429ff6eb54574015659de3a9e94fa029d6ac81449e9911e

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

The application retains ``_session``.
The worker's final callback is strongly bound to that session on main; the session joins the worker before notifying the
application that it can quit.
Releasing ``_view`` only removes optional display delivery.
The full walkthrough explains the worker's cancellation and exceptional-cleanup paths.

.. erbsland-in-practice-end::

Stopping a Run and Quitting a Loop
==================================

:cpp:func:`EventLoop::stop() <erbsland::event::EventLoop::stop>` ends the current dispatch run without draining its waiting queue.
The executing callback still has to return.
For a directly controlled loop, a later run can resume dispatch; :cpp:func:`stop() <erbsland::event::EventLoop::stop>`
is not permanent cancellation of waiting application work.
If you abandon those callbacks instead, account for any completion obligations they would have fulfilled.

:cpp:func:`EventLoop::quit() <erbsland::event::EventLoop::quit>` permanently closes admission and lets already accepted queued work drain.
Later calls to :cpp:func:`invoke() <erbsland::event::EventTarget::invoke>` or
:cpp:func:`post() <erbsland::event::EventTarget::post>` cannot add ordinary work.
These frontend methods return no acceptance result, so returning normally never proves that a callback will execute.
A draining callback must not depend on submitting a further continuation to the same loop.

.. erbsland-in-practice-begin:: Keeping the final verification report

The manifest owner must receive the worker's terminal reply before closing admission.
That reply records any I/O error and queues one final report while the loop is still open:

.. erbsland-demo::
    :source: event/apps/ManifestVerifier/src/VerificationSession.cpp
    :show: f:VerificationSession::finish, f:VerificationSession::printSummary
    :files: ../../../demos/event/apps/ManifestVerifier/example-files/shipment.sha256 ../../../demos/event/apps/ManifestVerifier/example-files/catalog.txt ../../../demos/event/apps/ManifestVerifier/example-files/prices.txt ../../../demos/event/apps/ManifestVerifier/example-files/stock.txt
    :exec: event/manifest-verifier --fail-fast shipment.sha256
    :exec-exit-code: 1
    :show-cmd-line:
    :sha256: 3b122ca51e60524bff79d405fb9b80dfc99148568f50435b0881b9486eccd632

.. code-block:: cpp

    void VerificationSession::finish(std::exception_ptr error) {
        _error = std::move(error);
        _owner->invoke([this]() -> void { printSummary(); });
        _owner->quit();
    }

    void VerificationSession::printSummary() const {
        el::io::printLine(
            "Checked: "_el, _checked, ", mismatches: "_el, _mismatches, ", unchecked: "_el, _entries.count() - _checked);
    }

.. erbsland-demo-run::

    .. code-block:: console

        $ event/manifest-verifier --fail-fast shipment.sha256

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mOK        catalog.txt
        MISMATCH  prices.txt
        Checked: 2, mismatches: 1, unchecked: 1␛[0m

.. erbsland-demo-end::

The summary runs during drain and needs no follow-up callback.
If the owner called ``quit()`` in the mismatch handler, the terminal reply could arrive after admission closed.
Queuing the report after ``quit()`` would also lose it.
The caller joins the worker after owner dispatch returns.
The stack-owned session outlives both the report and the worker, including the path that delivers a file error.

.. erbsland-in-practice-end::

Draining queued callbacks is not the same as finishing every asynchronous operation.
Quit stops backend polling; it does not wait for a future timer deadline, network event, or background completion to
produce another callback.
Close or cancel resources while their required loops still dispatch, and request loop quit only after those completion
paths are no longer needed.
This includes a receiver waiting for worker replies: sending the last command does not mean the last reply is queued.

Coordinating Application Shutdown
=================================

Order shutdown around dependencies: first stop creating new operations, then finish or cancel active operations, then
end the loops they need, and finally join threads and release their owners.
An application-level stopping state can reject new requests while leaving event admission open for close commands and
final replies.
Closing the queue at the start would remove those completion paths too early.

For application parts, :cpp:func:`onStopping() <erbsland::core::ApplicationPart::onStopping>` is the place to begin
asynchronous closure.
It runs on the part's loop and must return so final callbacks can execute.
Call :cpp:func:`completeStopping() <erbsland::core::ApplicationPart::completeStopping>` only when no required
asynchronous completion remains.
After dispatch ends, :cpp:func:`cleanup() <erbsland::core::ApplicationPart::cleanup>` releases resources without
starting another operation that needs a callback.
It must also work after partial initialization or failure.

.. erbsland-in-practice-begin:: Closing a Server Before Its Part Stops

The :doc:`/demo_apps/event/file_server` retains its HTTP server in the part.
Its final handler is installed before the source starts, so both the source's lifetime and the completion path are
established before shutdown can begin:

.. erbsland-demo::
    :source: event/apps/FileServer/ServerPart.hpp
    :show: c:Event source owned, c:Complete part shutdown-c:Bind to loopback(+0,-1), f:onStopping, f:cleanup
    :sha256: 64f74d164cde3ae3692a1f7ec109ab41ea7ba5c5eacaa95d3a8ba5e22724f3ad

.. code-block:: cpp

    el::HttpServerPtr _server;           ///< Event source owned by this part's loop.

    // Complete part shutdown after the source has closed.
    _server->events().onFinal([this]() -> void {
        if (state() == el::ApplicationPartState::Stopping) {
            completeStopping();
        }
    });

    void onStopping() override {
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

``close()`` starts graceful closure and the hook returns.
When the server reports ``onFinal()``, the part calls
:cpp:func:`completeStopping() <erbsland::core::ApplicationPart::completeStopping>`.
An absent or already terminal source permits immediate completion.
The final :cpp:func:`cleanup() <erbsland::core::ApplicationPart::cleanup>` uses ``abort()`` as an unconditional fallback
and releases the retained pointer.
See :doc:`/topics/network/using-http-servers` for this source's close and abort guarantees.

.. erbsland-in-practice-end::

:cpp:func:`Application::quit() <erbsland::core::Application::quit>` enters application-wide shutdown.
Let the part manager coordinate dependency ordering instead of manually quitting each part's loop.
Resources managed directly by your application must likewise finish the work that depends on those loops before the
loops end.
:doc:`/topics/core/application_parts` describes the lifecycle and configuration;
:doc:`/topics/core/service_lifecycle` explains how platform stop requests enter it.

Joining Workers and Handling Shutdown Failure
=============================================

:cpp:func:`EventThread::quit() <erbsland::event::EventThread::quit>` is an asynchronous, idempotent request that also immediately signals the thread's stop token.
:cpp:func:`join() <erbsland::event::EventThread::join>` waits for thread completion without requesting it.
Once required asynchronous operations are complete, the owner can request quit, join from another thread, and release
its owning pointer.
A worker whose remaining queued callbacks need no further deliveries can drain those callbacks during this final quit.

Keep the owner-side handle through the join.
Releasing the final owning thread pointer also requests termination and joins a started worker, so the final release
must happen outside that worker.
A callback capturing the last owner of its own thread can otherwise cause destruction on the very thread that must be
joined.

Retain application-managed workers until the application has stopped and joined them; registration does not replace that
retained ownership.
An :cpp:class:`UnmanagedEventThread <erbsland::event::UnmanagedEventThread>` requires its owner to arrange the whole
sequence.
Event threads are one-shot objects, and :cpp:func:`isRunning() <erbsland::event::EventThread::isRunning>` is a snapshot
rather than a completion barrier.
Use joining for completed execution; see :doc:`processing_work_on_event_threads` and the
:doc:`/reference/event/event_system` lifecycle contract for the remaining startup and joining details.
A join does not transport worker exceptions, so establish the error policy and failure path before starting work.

Deadlines and Cleanup Without Further Callbacks
-----------------------------------------------

Graceful closure may fail or never report success.
Give it a deadline on a loop that remains able to dispatch, and decide which source-specific abort or release path ends
the wait.
When the deadline wins, record a terminal failure once and stop expecting notifications that the abort path suppresses.
Late notifications must not complete the operation a second time.

The part manager normally stops the part graph after a failure and bounds graceful stopping with a default timeout of 30
seconds per part.
A timeout records failure and requests loop termination.
These policies do not forcibly interrupt a callback: a timer on a blocked loop cannot run, and a timeout detected
elsewhere still cannot make unbounded C++ code return.
Lifecycle hooks and worker steps therefore need bounded or cooperative behavior for joins to finish.

If an exception has already ended dispatch, cleanup cannot wait for another callback on that loop.
It must signal cancellation through a mechanism the worker can still observe, release or abort sources according to
their contracts, and join only after removing dependencies on further owner-loop delivery.
For example, a worker waiting for permission to produce its next batch needs a cancellation path that also wakes that
wait.
:doc:`handling_event_errors` explains how operation failure and application failure reach these cleanup paths.
