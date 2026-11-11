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

.. erbsland-in-practice-begin:: Cancelling a Survey Between Steps

This snippet surveys one habitat per callback and retains its progress in a stack-owned operation.
A stop request queued after the first step runs before the next continuation.
The continuation observes the token, records cancellation once, and returns without scheduling more work:

.. erbsland-demo::
    :source: event/snippets/EventShutdown/CancellableSurvey.hpp
    :show: c:Loop that dispatches-c:Terminal outcomes
    :sha256: 17c57c5213db11aa9d4395e46cccc60b28b907576e61b61bafc33b9cf3c12647

.. code-block:: cpp

    el::EventLoopPtr _loop;       ///< Loop that dispatches every survey step.
    el::StopToken _stopToken;     ///< Operation cancellation, independent of loop termination.
    std::size_t _processed = 0;   ///< Habitats already surveyed.
    bool _cancelled = false;      ///< Whether cancellation ended the survey.
    std::size_t _completions = 0; ///< Terminal outcomes recorded on the owner loop.

.. erbsland-demo-end::

.. erbsland-demo::
    :source: event/snippets/EventShutdown/CancellableSurvey.cpp
    :show: f:CancellableSurvey::advance, f:CancellableSurvey::finish
    :sha256: f3fd89213dd51833f59f7f8a5728b3ba0902dc06690ab03f97e7d784f2744b82

.. code-block:: cpp

    void CancellableSurvey::advance() {
        if (_completions != 0) {
            return;
        }
        if (_stopToken.isStopRequested()) {
            finish(true);
            return;
        }
        constexpr auto habitats = std::array{"青龙林"_el, "凤凰谷"_el, "麒麟山"_el};
        // One bounded unit of work before returning to dispatch.
        el::io::printLine("Surveyed habitat: "_el, habitats[_processed]);
        ++_processed;
        if (_processed == habitats.size()) {
            finish(false);
            return;
        }
        _loop->invoke([this]() -> void { advance(); });
    }

    void CancellableSurvey::finish(const bool cancelled) {
        _cancelled = cancelled;
        ++_completions;
        if (cancelled) {
            el::io::printLine("Survey cancelled after "_el, _processed, " habitat."_el);
        } else {
            el::io::printLine("Survey complete."_el);
        }
    }

.. erbsland-demo-end::

The runner retains both the operation and the stop source until the loop has dispatched all immediately available work:

.. erbsland-demo::
    :source: event/snippets/EventShutdown/CooperativeCancellation.cpp
    :show: a:loop-c:All borrowed state(+0,+1)
    :exec: event/event_shutdown --demo CooperativeCancellation
    :sha256: 8fcb7354396a5036712d649cecd3abf459175751f719f24d975a2651295d8d96

.. code-block:: cpp

    const auto loop = el::EventLoop::create(el::EventLoopOptions{}.setErrorPolicy(el::EventLoopErrorPolicy::Quit));
    auto cancellation = el::StopSource{};
    auto survey = CancellableSurvey{loop, cancellation.stopToken()};
    auto unrelatedRan = false;

    loop->invoke([&survey]() -> void { survey.advance(); });
    loop->invoke([&cancellation]() -> void {
        cancellation.requestStop();
        cancellation.requestStop(); // Repeating the request does not complete the operation twice.
        el::io::printLine("Cancellation requested; completion is still pending."_el);
    });
    loop->invoke([&unrelatedRan]() -> void {
        unrelatedRan = true;
        el::io::printLine("Unrelated callback still runs."_el);
    });
    // All borrowed state outlives the callbacks, including the survey's continuation.
    const auto dispatched = loop->runUntilIdle();

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mSurveyed habitat: 青龙林
        Cancellation requested; completion is still pending.
        Unrelated callback still runs.
        Survey cancelled after 1 habitat.␛[0m

.. erbsland-demo-end::

The result check verifies one surveyed habitat and one cancelled completion.
An unrelated callback still runs: cancelling the survey does not terminate the loop.
Requesting cancellation twice does not create two completions.
The example uses an open, unlimited queue so its continuation can be submitted; it does not demonstrate cancellation of
an indefinitely blocked callback.

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

.. erbsland-in-practice-begin:: Seeing the Admission Boundary

This snippet queues two observations, requests quit, then attempts a third submission before starting dispatch:

.. erbsland-demo::
    :source: event/snippets/EventShutdown/DrainAcceptedWork.cpp
    :show: a:loop-c:Run the accepted callbacks(+0,+1)
    :exec: event/event_shutdown --demo DrainAcceptedWork
    :sha256: 4834670c1352d574463499a605147ff5b36e52e5da28ce43e7cef02ac10c375c

.. code-block:: cpp

    const auto loop = el::EventLoop::create(el::EventLoopOptions{}.setErrorPolicy(el::EventLoopErrorPolicy::Quit));
    auto observations = el::StringList{};
    loop->invoke([&observations]() -> void { observations.append("Molntäcke"_el); });
    loop->invoke([&observations]() -> void { observations.append("Havsis"_el); });
    loop->quit();
    loop->invoke([&observations]() -> void { observations.append("Snödjup"_el); });
    // Run the accepted callbacks.
    loop->run();

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mRecorded: Molntäcke
        Recorded: Havsis␛[0m

.. erbsland-demo-end::

Only the first two observations appear.
The loop and captured collection remain on the runner's stack until ``run()`` returns, and the demo checks the exact
result afterward.
Calling :cpp:func:`quit() <erbsland::event::EventLoop::quit>` before ``run()`` makes the admission boundary visible
without concurrent producers.

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
