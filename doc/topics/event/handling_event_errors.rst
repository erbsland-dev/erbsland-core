..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Events; Handling Event Failures
    single: Events; Callback Exceptions

***********************
Handling Event Failures
***********************

An asynchronous request can fail after the function that started it has returned.
You need a place to observe that failure and a way to finish the task whose result will no longer arrive.
This page explains how source errors, callback exceptions, and loop policies fit together, then shows how managed
workers release a caller that is waiting for results.
It builds on :doc:`using_event_apis`; the first local examples need no worker threads.

API Call Failures, Operation Errors, and Callback Exceptions
============================================================

Consider a lookup started by a queued callback.
An invalid argument can make ``start()`` throw during that call.
A correctly started lookup can later report an operation failure through ``onError()``.
Finally, your own result or error callback can throw while processing a notification.
These failures have different observers:

.. list-table::
    :header-rows: 1
    :widths: 25 35 40

    * - Failure location
      - Observer
      - Typical response
    * - API call or configuration
      - The caller's ``try`` and ``catch``
      - Correct the input or abandon setup.
    * - Asynchronous operation
      - The source's error callback
      - Report a failed result, retry, or finish the task.
    * - Invocation or event-dispatch callback
      - The event loop's error policy or handler
      - Record the exception or end the affected work.

A ``try`` and ``catch`` around ``invoke()`` can observe failures of the submission itself.
It cannot catch an exception thrown later by the submitted callback: that callback runs on a different call stack,
possibly on a different thread.
Likewise, a source's ``onError()`` is not a catch-all handler for exceptions from your callbacks.
An uncaught setup exception inside an invocation also reaches the loop's callback-error boundary.

Operation Errors and Task Completion
====================================

An error notification gives your controller a chance to decide what the failed operation means.
A lookup failure might end one request while the rest of the application continues.
A required startup lookup might instead prevent the application from doing useful work.
In either case, record a terminal outcome or start a bounded recovery attempt; leaving the caller waiting for a success
notification loses the completion path.
:doc:`event_workflows` shows how success and failure callbacks determine whether an operation continues to its next
step or ends.

The lookup demo below chooses application failure.
It retains the lookup in ``lookupData`` and installs its handler before starting the operation on the application's main
loop.
The handler converts the source's structured error context into a
:cpp:class:`NetworkError <erbsland::network::NetworkError>` so that the application's diagnostic boundary can report it:

.. erbsland-demo::
    :source: network/HostLookupFailure/main.cpp
    :show: c:Resolver failures(+4,-2), f:resolveMissingHost
    :sha256: 7b469cbaebd59913f7728194e96bc1b174606b2cce609d46563964f234a8511d

.. code-block:: cpp

    struct LookupData {
        el::HostLookupPtr lookup;
    } lookupData;

    void resolveMissingHost() {
        const auto events = el::application().events();
        lookupData.lookup = events->get<el::Network>().createHostLookup();
        lookupData.lookup->events().onError(
            [](const el::NetworkErrorContext &context) -> void { throw el::NetworkError{context}; });
        lookupData.lookup->start(el::Host::fromStringOrThrow("stelling.invalid"_el));
    }

.. erbsland-demo-end::

The surrounding application queues ``resolveMissingHost()`` through ``app.events()->invoke()`` before ``app.run()``.
This is the same owner-loop setup used in :doc:`using_event_apis`.
The ``.invalid`` name supplies the failure case; resolver timing and diagnostic details depend on the system.
The local callback demos below provide deterministic examples of exception handling without a resolver.

Throwing here deliberately changes an operation error into a callback exception.
For local recovery, handle the context and return normally instead.
For this particular lookup API, a throwing result or error handler skips ``onFinal()``.
Do not put indispensable cleanup solely in that final callback: retain resources through their required lifetime and use
destructors for unconditional cleanup.
See :doc:`/topics/network/resolving-hosts-asynchronously` for the source's completion and cancellation contract.

Reporting Captured Callback Failures
====================================

A directly owned :cpp:class:`EventLoop <erbsland::event::EventLoop>` stores callback exceptions by default and continues
dispatching.
This is useful when a procedural host wants to inspect a finite batch after dispatch, but continuing does not mean the
failed callbacks succeeded.
The example explicitly selects :cpp:enum:`EventLoopErrorPolicy::Store <erbsland::event::EventLoopErrorPolicy>` through
:cpp:class:`EventLoopOptions <erbsland::event::EventLoopOptions>` to make that choice visible.
Options are copied when the loop is created; changing the original options later does not reconfigure it.

Here, two route callbacks throw and a third marks its own completion.
:cpp:func:`runUntilIdle() <erbsland::event::EventLoop::runUntilIdle>` dispatches the immediately available work on the calling thread.
There are no delayed operations or producers in this example, so the three submissions form a finite batch.
In a program with timers or external sources, becoming idle alone would not prove task completion.

.. erbsland-demo::
    :source: event/EventFailures/CapturedErrors.cpp
    :show: f:capturedErrors
    :exec: event/event_failures --demo CapturedErrors
    :sha256: 8d68dc6b3f08bd4a419ecb553addc8a4283e12ced4c084f73baa7eca05853d60

.. code-block:: cpp

    void capturedErrors() {
        auto options = el::EventLoopOptions{};
        options.setErrorPolicy(el::EventLoopErrorPolicy::Store);
        const auto loop = el::EventLoop::create(options);
        auto completed = false;
        loop->invoke([]() -> void { throw el::RuntimeError{"Route data unavailable: 地球 → 火星"_el}; });
        loop->invoke([]() -> void { throw el::RuntimeError{"Route data unavailable: 地球 → 金星"_el}; });
        loop->invoke([&completed]() -> void { completed = true; });
        const auto processed = loop->runUntilIdle();

        // Consume failures in capture order at the reporting boundary.
        auto reported = 0;
        while (const auto error = loop->takeError()) {
            try {
                std::rethrow_exception(error);
            } catch (const el::RuntimeError &failure) {
                el::io::printLine(failure.reason());
                ++reported;
            }
        }
        if (processed != 3 || !completed || reported != 2 || loop->hasError()) {
            throw el::LogicFailure{"Expected two reported errors and a completed final callback."_el};
        }
        el::io::printLine("Final callback ran; both route failures were reported."_el);
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mRoute data unavailable: 地球 → 火星
        Route data unavailable: 地球 → 金星
        Final callback ran; both route failures were reported.␛[0m

.. erbsland-demo-end::

The local ``completed`` variable stays alive throughout dispatch.
Its value proves only that the third callback ran; the two earlier failures still need reporting.
:cpp:func:`hasError() <erbsland::event::EventLoop::hasError>` tests whether captured failures remain.
:cpp:func:`takeError() <erbsland::event::EventLoop::takeError>` removes the oldest exception and returns an empty pointer when the captured queue is empty.
The loop above consumes failures in capture order and rethrows each at a reporting boundary that understands the
expected :cpp:class:`RuntimeError <erbsland::err::RuntimeError>`.
Unexpected exception types propagate to the demo application's outer boundary.

In your own host, choose where that boundary belongs: after a finite dispatch batch, after joining a worker, or in an
explicit controller failure path.
If failures can continue arriving, arrange regular consumption; the stored-error queue has no capacity limit.
For direct dispatch and unmanaged thread ownership, see :doc:`running_event_loops_directly`.

Continuing, Quitting, or Failing the Application
================================================

Choose the default :cpp:func:`errorPolicy() <erbsland::event::EventLoopOptions::errorPolicy>` according to whether the
remaining work is independent of the failed callback.
``Store`` preserves diagnostic evidence while letting other work proceed.
``Ignore`` continues without retaining it, which is appropriate only when losing that evidence is deliberate.
Neither policy supplies a missing result or repairs partially changed application state.

If an exception invalidates the current task, ``Quit`` stores it, closes admission, and drains callbacks already
accepted by the loop.
This is graceful termination, so an already queued callback can still run after the failure.
It must not require a new reply or timer callback to be admitted after that point.
Finish asynchronous resource shutdown before quitting its owner loop, as explained in :doc:`shutting_down_event_work`.

For an application-managed worker, ``FailApplication`` forwards the stored exception to its owning application and
starts application failure handling.
Standalone ``EventLoop`` and :cpp:class:`UnmanagedEventThread <erbsland::event::UnmanagedEventThread>` factories reject
that policy with :cpp:class:`ParameterFailure <erbsland::err::ParameterFailure>` because there is no owning application
to receive the failure.
The same restriction applies to the queue-overflow policy of that name.
The :ref:`event-loop-options` reference lists the exact policy contracts.

Custom Callback Failure Handlers
================================

Install an :cpp:type:`EventLoopErrorHandler <erbsland::event::EventLoopErrorHandler>` with
:cpp:func:`setErrorHandler() <erbsland::event::EventLoop::setErrorHandler>` when the response needs a decision beyond
the fixed policies.
The loop stores the exception before calling the handler on the dispatching thread.
The handler returns :cpp:enum:`EventLoopErrorAction::Continue <erbsland::event::EventLoopErrorAction>` or
:cpp:enum:`EventLoopErrorAction::Stop <erbsland::event::EventLoopErrorAction>`.
``Stop`` ends dispatch without draining the remaining queue; it does not undo the failed callback's side effects.

This variation stops before the second callback and reports the retained exception after dispatch returns:

.. erbsland-demo::
    :source: event/EventFailures/ErrorHandler.cpp
    :show: f:errorHandler
    :exec: event/event_failures --demo ErrorHandler
    :sha256: d914fe9a81427935c2fe671e386fe569955b8ffab168c462eeee97ce75aa0745

.. code-block:: cpp

    void errorHandler() {
        const auto loop = el::EventLoop::create();
        auto handled = false;
        auto nextCallbackRan = false;
        loop->setErrorHandler([&handled](std::exception_ptr error) -> el::EventLoopErrorAction {
            handled = error != nullptr;
            return el::EventLoopErrorAction::Stop;
        });
        loop->invoke([]() -> void { throw el::RuntimeError{"Route calculation failed: 地球 → 火星"_el}; });
        loop->invoke([&nextCallbackRan]() -> void { nextCallbackRan = true; });
        const auto processed = loop->runUntilIdle();

        const auto error = loop->takeError();
        if (processed != 1 || !handled || nextCallbackRan || !error) {
            throw el::LogicFailure{"Expected a captured error and stopped dispatch."_el};
        }
        try {
            std::rethrow_exception(error);
        } catch (const el::RuntimeError &failure) {
            el::io::printLine(failure.reason());
        }
        loop->setErrorHandler({}); // Restore the configured default policy.
        el::io::printLine("Dispatch stopped before the next callback."_el);
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mRoute calculation failed: 地球 → 火星
        Dispatch stopped before the next callback.␛[0m

.. erbsland-demo-end::

The handler captures only local state that remains alive while the loop runs.
It replaces the configured default action, even if that action was ``Ignore`` or ``FailApplication``.
Clearing it with ``setErrorHandler({})`` restores the configured policy; it does not restart the loop or drain pending
work.
The example ends without dispatching the retained second callback.

Keep an error handler small and reliable.
If it throws, the loop captures that additional exception and stops.
The original exception is already stored, so reporting should account for both failures rather than assuming there can
only be one.

Releasing a Caller Waiting for Worker Results
=============================================

The file hasher in :doc:`processing_work_on_event_threads` uses workers to compute hashes and a separate receiver to
collect results.
Its procedural main waits for that receiver with ``join()``.
If a worker throws before posting its result, the receiver's success count can never reach the expected total.
Storing the exception on the worker alone would leave main waiting.

The demo configures both the receiver and workers with ``FailApplication``:

.. erbsland-demo::
    :source: event/FileHasher/FileHasherApp.hpp
    :show: f:startLoops
    :sha256: bb43a71f25ba93791cdd924279cd78d9c234b8453959d774af7e2b0fb96196e9

.. code-block:: cpp

    void startLoops() {
        /// Prepare the result receiver.
        auto options = el::EventLoopOptions{}.setErrorPolicy(el::EventLoopErrorPolicy::FailApplication);
        _resultReceiver = createEventThread(options);
        _resultReceiver->start();
        /// Reserve either enough workers for all CPU cores or for every file if there are less.
        const auto count = std::min<std::size_t>(el::system::info::logicalCpuCount(), _paths.count().toSizeT());
        _workers.reserve(count);
        for (auto index = std::uint32_t{0}; index < count; ++index) {
            auto worker = createEventThread(options);
            _workers.push_back(worker);
            worker->start();
        }
    }

.. erbsland-demo-end::

On failure, the application preserves its first error, stops normal main-loop dispatch, and requests graceful
termination of all its managed threads, including the receiver.
That releases main's join even when main is procedural and never entered its own event loop.
After shutdown processing and worker joins, the application rethrows the retained failure on the application thread.
Core exceptions use its diagnostic and exit-code handling; foreign exceptions run cleanup and escape
:cpp:func:`Application::run() <erbsland::core::Application::run>`.
``join()`` itself transports no exceptions.
See :doc:`/topics/core/procedural_applications` for the surrounding lifecycle.

This policy handles thrown failures, not a source that silently never replies.
A deadline or another explicit cancellation path is still needed for missing results, and no loop policy can interrupt a
callback that never returns.

Exception Boundaries for Extension Authors
==========================================

Loop capture covers invocation callbacks and exceptions from backend
:cpp:func:`handleEvent() <erbsland::event::EventBackend::handleEvent>` dispatch.
It is not a universal wrapper around backend polling, wake-time queries, or driver calls.
An exception escaping a directly driven loop must be handled by its host.
An exception escaping an event thread terminates the process; it cannot later be caught around ``join()``.

A source or backend that calls user callbacks must define its own notification contract, including what happens to later
notifications if a callback throws.
See :doc:`creating_event_sources` and :doc:`writing_event_backends` when extending the event system.
Queue admission is a separate boundary: overflow does not call the callback-error handler.
Its policies and the effect of losing work are covered in :doc:`controlling_event_load`.
