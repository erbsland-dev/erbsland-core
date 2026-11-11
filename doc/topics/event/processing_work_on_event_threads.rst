..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Events; Worker Threads
    single: Event Threads; Lifecycle
    single: Event Threads; Synchronization

.. _event-procedural-workers:

********************************
Processing Work on Event Threads
********************************

An event thread gives work a dedicated loop on a separate native thread.
You create and retain the thread object, start it, and submit callbacks through its target.
This page follows that lifetime through completion and joining, both when the application manages shutdown and when you
own it yourself.
It assumes the loop, target, and source vocabulary introduced in :doc:`events_in_erbsland_core`.

Main Loop and Worker Loop
=========================

The application's main loop runs on the main thread when the application enters ``runMainEventLoop()``, normally through
its default ``main()``.
An :cpp:class:`EventThread <erbsland::event::EventThread>` owns a separate loop and the native thread that dispatches
it.
Its ``loop()`` exposes the target for posting and service access;
:cpp:func:`workerEventLoop() <erbsland::event::EventThread::workerEventLoop>` exposes the full worker loop when the
owner needs operations such as error inspection.

A dedicated loop is useful when a component needs its own serialized state and event sources, or when blocking work must
not hold up the main loop.
The main and worker loops can run concurrently, but callbacks within each loop still execute one at a time.
An overridden procedural ``main()`` can also use workers without dispatching the application's main loop at all.
In that design, the workers must be able to finish without callbacks on main.

Before creating several workers for independent jobs, consider
:cpp:class:`util::WorkerPool <erbsland::util::WorkerPool>`.
It schedules jobs across reusable worker threads and supports both event-based results and procedural futures.
Dedicated event threads are useful when ownership of a particular loop matters; a pool is often a better fit for a batch
of independent computations.
See :doc:`planned/processing_work_with_a_worker_pool` for job scheduling.

Managed and Unmanaged Threads
=============================

:cpp:func:`Application::createEventThread() <erbsland::core::Application::createEventThread>` creates a managed thread registered with that application.
The framework requests shutdown and joins retained managed threads when the application finishes or fails.
This integrates worker lifetime with the application lifecycle described in :doc:`using_event_loops`.

:cpp:func:`UnmanagedEventThread::create() <erbsland::event::UnmanagedEventThread::create>` creates a standalone thread without application shutdown integration.
Its owner arranges termination, joining, and error delivery, even if it happens to be used inside an ``Application``.
Both factories return an unstarted object, and **both require retained ownership**.
Application registration does not keep a managed thread alive for you.

.. list-table::
    :header-rows: 1

    * - Responsibility
      - Managed thread
      - Unmanaged thread
    * - Factory
      - ``createEventThread()``
      - ``UnmanagedEventThread::create()``
    * - Start and retain
      - Application code
      - Application code
    * - Application shutdown
      - Framework requests quit and joins
      - Owner must arrange it
    * - Operational callback failure
      - ``FailApplication`` can report to the application
      - ``Quit`` can stop dispatch; owner inspects the error

Creation and Startup
====================

Call :cpp:func:`start() <erbsland::event::EventThread::start>` once on the retained thread object.
It starts the native thread, which enters worker-loop dispatch.
After ``start()`` succeeds you can immediately submit work to ``thread->loop()``; there is no need to poll until the
worker reports that it is running.
The queue bridges the interval before dispatch begins.

:cpp:func:`isStarted() <erbsland::event::EventThread::isStarted>` records that native-thread startup succeeded and remains true after the worker has finished.
:cpp:func:`isRunning() <erbsland::event::EventThread::isRunning>` is only a snapshot of dispatch state.
It is not a readiness signal or synchronization barrier: its value may change immediately after you read it.
If your application needs to know that worker initialization has completed, publish an explicit completion notification
through a path that the recipient is actually processing.

.. mermaid::

    flowchart TD
        created["Created, unstarted"] -->|"start() succeeds"| started["Native thread starts"]
        started --> dispatch["Dispatch or wait for work"]
        dispatch -->|"quit(): close admission"| drain["Finish current callback and accepted queue"]
        drain --> ended["Native thread ends"]
        ended -->|"Owner joins"| joined["Joined"]

This is a one-shot lifetime; joining does not make the object restartable.
An idle worker continues waiting until shutdown is requested.
:cpp:func:`quit() <erbsland::event::EventThread::quit>` can also be requested before startup; starting such a thread does not reopen admission.
:cpp:func:`join() <erbsland::event::EventThread::join>` before startup or after an earlier join is a no-op.

Submitting Work Through the Target
==================================

Any producer thread holding a valid :cpp:type:`EventTargetPtr <erbsland::event::EventTargetPtr>` can use its thread-safe
posting and invocation operations.
``worker->loop()->invoke(callback)`` queues the function without calling it inline.
The worker calls it when it dispatches that accepted event, with its loop available through ``el::currentLoop()``.
The producer does not need to run an event loop of its own to submit work.

Capture input values by value, moving larger owned values into the callback where appropriate.
A reference to a producer's local variable can outlive that variable before the worker uses it.
Borrowing application state requires the owner to survive every use, including worker shutdown and joining.
Keep mutable worker state on the worker and arrange an explicit handover for results.

Thread-safe submission protects the queue, not everything reachable from a callback.
It does not make captured mutable objects safe for concurrent access, and it does not grant cross-thread access to a
source's editor or loop-bound operations.
Queue source creation and handler setup onto their intended owner loop.
The serial execution of worker callbacks then gives those sources and worker-owned state a consistent context.

.. erbsland-in-practice-begin:: A Managed Worker in a Procedural Application

This startup template retains the worker in an application member and queues ``beginWork()`` immediately after startup.
The reader supplies the business operations and connects their successful completion to ``completeWork()``.
The framework supplies the worker loop and application-level failure shutdown.

.. erbsland-demo::
    :source: event/templates/ApplicationStartup/ProceduralWorker/WorkApplication.hpp
    :show: f:main, f:beginWork, f:completeWork, c:Owned by the application
    :sha256: 918d6b1e29e2e6b7667d95f254005d2c8a518554bb3c78a8a17e6e3df538b9f1

.. code-block:: cpp

    auto main() -> el::ExitCode override {
        auto options = el::EventLoopOptions{};
        options.setErrorPolicy(el::EventLoopErrorPolicy::FailApplication);
        _worker = createEventThread(options);
        _worker->start();
        _worker->loop()->invoke([this]() -> void { beginWork(); });
        // completeWork() must quit the worker before this join can return.
        _worker->join();
        // Read worker-owned results only after joining. No main loop was needed.
        return el::ExitCode::success();
    }

    void beginWork() {
        // TODO: Create and retain your worker's business operations here.
        // Connect their final successful completion to completeWork().
        // Do not route required completion through the application's idle main loop.
    }

    void completeWork() { _worker->quit(); }

    el::ManagedEventThreadPtr _worker; ///< Owned by the application through shutdown and joining.

.. erbsland-demo-end::

The join in ``main()`` depends on the completion hook or failure policy ending dispatch.
This template is compiled, not run: until its business operation is implemented, no success callback requests quit.

.. erbsland-in-practice-end::

Completion, Quit, and Join
==========================

Application completion, loop termination, and native-thread joining are three different steps.
First, your operation decides that all required work has finished or has been cancelled.
Then ``quit()`` requests the end of dispatch.
Finally, ``join()`` waits for the native thread to end and synchronizes with its completion.
Joining does not itself request termination.

``quit()`` closes admission to the loop and arranges for accepted queued work to drain before the loop returns.
It does not interrupt the currently executing callback.
New submissions after admission closes are not accepted; an invocation call is not a delivery acknowledgement.
A callback already in progress can continue running, but follow-up work it posts after closure cannot be relied on.

For a long-running callback, capture the worker's ``stopToken()`` before submission and check ``isStopRequested()``
between bounded steps.
The token becomes requested when the worker is quit, including when application shutdown initiates the quit request.
It can be copied safely without retaining the worker or borrowing its loop.
Checking it allows an executing callback to finish early even when a queued cancellation command could not yet run.
The resumable ``EventLoop::stop()`` operation does not request this token.

Draining queued callbacks does not finish every asynchronous operation that might later produce a notification.
If a source still needs dispatch to finish or cancel, keep its loop open until its required completion is handled.
Request quit only when closing that path is safe.
:doc:`shutting_down_event_work` develops cancellation, graceful source closure, and shutdown deadlines.

Synchronizing a Procedural Caller
=================================

A procedural caller can submit a finite operation, join the worker after its completion requests quit, and then read the
results.
While the worker runs, only it accesses the result state; after the join returns, ownership of access passes back to the
caller.
If the caller needs progress before joining, it needs a separate synchronization or event-delivery arrangement.
``isRunning()`` is not a replacement for either.

A main thread blocked in ``join()`` cannot dispatch its main-loop callbacks.
Do not make worker completion depend on a reply to that loop: main would wait for the worker while the worker waits for
main.
Keep the completion path entirely on the worker or on another actively dispatching loop.
For request/reply applications with a responsive main loop, see :doc:`communicating_between_loops`.

The final owning pointer to the thread must remain outside its worker.
An event thread cannot join itself, including implicitly when its final owner is destroyed.
Retain the thread in the calling scope or application object through joining, and release it there.
Although destruction requests quit and joins, explicit completion and joining make result ownership and failure handling
visible.

.. erbsland-in-practice-begin:: FileHasher Joins Its Result Receiver

``FileHasher`` keeps main procedural and sends worker results to a separate receiver loop.
That receiver alone appends results and requests its own quit after the last result.
Main joins the receiver before checking the count or reading the collection:

.. erbsland-demo::
    :source: event/apps/FileHasher/FileHasherApp.hpp
    :show: c:implement Application(-4,+19), f:addResult
    :sha256: 03ed70b35f133729370c1cadfebf2a43a15b223a030cdb37844316ffc5f0b164

.. code-block:: cpp

    [[nodiscard]] auto main() -> el::ExitCode override {
        collectFiles();
        if (_totalFiles == 0) {
            el::io::printLine("No files to hash."_el);
            return el::ExitCode::success();
        }
        startLoops();
        el::io::printLine("Hashing "_el, _totalFiles, " files with "_el, _workers.size(), " worker loops."_el);
        distributePaths();
        _resultReceiver->join();
        if (_completedFiles != _totalFiles) {
            return el::ExitCode::failure();
        }
        printResults();
        return el::ExitCode::success();
    }

    void addResult(HashResult result) {
        _results.push_back(std::move(result));
        ++_completedFiles;
        if (_completedFiles % 10U == 0) {
            el::io::print("."_el);
            el::stdOut()->flush();
        }
        if (_completedFiles == _totalFiles) {
            _resultReceiver->quit();
        }
    }

.. erbsland-demo-end::

The application retains all worker objects through its managed shutdown.
The full :doc:`/demo_apps/event/file_hasher` walkthrough explains the worker/receiver routing, ownership handovers, and
failure path.
For ordinary independent file jobs, also consider a worker pool before adopting this multi-loop design.

.. erbsland-in-practice-end::

Failures Must Also End the Wait
===============================

A missing success callback must not leave the owner joining forever.
For a managed worker whose operational failure should end the application, set
``EventLoopOptions::setErrorPolicy(EventLoopErrorPolicy::FailApplication)`` before creating the thread.
A captured :cpp:class:`RuntimeError <erbsland::err::RuntimeError>` then reaches the application, which requests managed
shutdown and reports the failure.
This also works when procedural main is waiting on a managed receiver rather than dispatching the main loop.

For an unmanaged worker, use ``EventLoopErrorPolicy::Quit`` when a captured operational error should end dispatch.
After joining, inspect ``workerEventLoop().takeError()`` and handle or rethrow the captured exception on the owner
thread.
**Joining does not transport worker exceptions.** The default ``Store`` policy alone does not supply a termination path.
These policies handle Erbsland ``RuntimeError`` exceptions; arbitrary exceptions escaping the native worker entry point
are not converted into join results and can terminate the process.

.. erbsland-in-practice-begin:: Owning an Unmanaged Worker

This compile-only counterpart uses ``Quit`` and checks the captured error after joining.
Rethrowing on the application's main thread lets ``Application::run()`` report an operational error.
The reader supplies the business operation and success completion; the owner explicitly handles the standalone thread's
error and lifetime obligations.

.. erbsland-demo::
    :source: event/templates/EventThreads/UnmanagedWorker/WorkApplication.hpp
    :show: f:main, f:cleanup, f:beginWork, f:completeWork, c:Main-thread owner
    :sha256: 4331f24cd3281ef404d21b2a130560e98b7ab36720ca8921a1bda81b2b453dbf

.. code-block:: cpp

    auto main() -> el::ExitCode override {
        const auto options = el::EventLoopOptions{}.setErrorPolicy(el::EventLoopErrorPolicy::Quit);
        _worker = el::UnmanagedEventThread::create(options);
        _worker->start();
        _worker->loop()->invoke([this]() -> void { beginWork(); });
        // completeWork() or the Quit error policy must end dispatch before join returns.
        _worker->join();
        if (const auto error = _worker->workerEventLoop().takeError()) {
            std::rethrow_exception(error);
        }
        // TODO: Read worker-owned results here, after joining and checking failure.
        return el::ExitCode::success();
    }

    void cleanup() noexcept override {
        // The application does not manage this thread. Its final owner stops and joins it.
        _worker.reset();
    }

    void beginWork() {
        // TODO: Create and retain your business operations here.
        // Connect successful completion to completeWork(), without a main-loop reply.
    }

    void completeWork() { _worker->quit(); }

    el::UnmanagedEventThreadPtr _worker; ///< Main-thread owner retained through join and cleanup.

.. erbsland-demo-end::

Cleanup releases the final pointer on main even if submission or later processing fails.
The thread object's destructor then requests quit and joins if necessary, without needing a callback on main.

.. erbsland-in-practice-end::

See :doc:`handling_event_errors` for error handlers and the broader failure contract, and
:doc:`controlling_event_load` for bounded queues and overload behavior.
