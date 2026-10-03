..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Events; Managed Threads
    single: Event Loops; Worker Threads
    single: Events; Result Ownership

.. _event-procedural-workers:

********************************
Processing Work on Event Threads
********************************

An event thread owns an event loop and runs it on a separate thread.
Callbacks on that loop execute one at a time, while callbacks on different threads can run concurrently.
This lets you move independent computation or blocking work away from a loop that needs to stay responsive.
Posting work chooses its execution thread; it does not automatically make the data it uses safe to share.

The usual design gives each thread exclusive access to its mutable state and transfers inputs and results as values.
A receiving loop processes those values in callbacks.
A procedural caller can instead wait for a thread to finish and take over its results after joining it.
These explicit handover points make it possible to coordinate work without adding a mutex around every application
variable.

This page explains how to create managed event threads, send work, choose a result destination, and coordinate
completion and failure.
The file-hasher demo illustrates those APIs in a batch tool whose main function waits while other threads work.
It builds on :doc:`using_event_loops`.

Result Ownership and Synchronization
====================================

Before starting threads, decide which thread may access each mutable object and when that responsibility changes.
A shared pointer solves object lifetime, not concurrent access.
You can avoid an application mutex when only one thread accesses mutable state at a time and synchronization establishes
each handover.

In the file hasher, command-line handling resolves the input directory before ``main()`` begins.
Main then collects the file paths, fixes the expected result count, and reserves result storage before starting any
threads.
While the batch runs, workers calculate hashes in local state, and only the receiver appends results and updates the
completion count.
Main does not inspect that state again until it has joined the receiver.
Starting the threads, posting values, and joining the receiver provide synchronization at the boundaries.
The library coordinates those operations internally; the result list itself needs no additional application mutex.

.. mermaid::

    flowchart TD
        Prepare["Main: prepare result storage"]
        Collect["Receiver: append results"]
        Read["Main: read and sort results"]
        Prepare -->|Start threads; main stops accessing results| Collect
        Collect -->|Receiver exits; join returns| Read

Here, "ownership" means responsibility for access; the result list remains a member of the same application object
throughout all three phases.
The worker and receiver handles also live in that object:

.. erbsland-demo::
    :source: event/FileHasher/FileHasherApp.hpp
    :show: c:Workers retained-c:Read by main only
    :sha256: 9a4c08b9e7ddf4d9a8b3799aa4ef2fb369f44121ff1582ec632a548ac835f16b

.. code-block:: cpp

    std::vector<el::ManagedEventThreadPtr> _workers; ///< Workers retained throughout the batch.
    el::ManagedEventThreadPtr _resultReceiver;       ///< Sole owner of mutable result state while running.
    el::Path _root;                                  ///< The root path to be scanned.
    el::PathList _paths;                             ///< The list of paths to process.
    std::size_t _totalFiles{0};                      ///< Fixed before any thread starts.
    std::size_t _completedFiles{0};                  ///< Updated only on the receiver thread.
    std::vector<HashResult> _results;                ///< Read by main only after the receiver is joined.

.. erbsland-demo-end::

The input list ``_paths`` remains unchanged while the batch runs.
Main copies each path into a worker callback; workers do not traverse or modify the shared list.
The receiver uses the fixed ``_totalFiles`` count to decide when it has collected every result.

``FileHasherApp`` derives from :cpp:class:`Application <erbsland::core::Application>` and stays alive throughout
``app.run()``.
The framework stops and joins its managed threads before application cleanup, so the queued callbacks can safely use
this application's ``this`` pointer.
That lifetime guarantee is separate from the access rules above: capturing ``this`` alone never makes simultaneous
access to its members safe.

Creating and Starting Managed Threads
=====================================

:cpp:func:`createEventThread() <erbsland::core::Application::createEventThread>` creates a thread object with its own
loop and connects it to application shutdown.
Retain the returned ``ManagedEventThreadPtr`` for as long as you need that thread: application management coordinates
shutdown but does not keep the thread object alive for you.
Keep its final owning pointer outside the worker itself so that releasing it can stop and join the thread safely.

:cpp:func:`start() <erbsland::event::EventThread::start>` launches the worker, and ``events()`` provides the target for
posting its callbacks.
A thread object can be started only once; it cannot be restarted after shutdown.
After ``start()`` succeeds, you can submit work without polling ``isRunning()``.
The loop may still be starting, but it can receive queued work.

The demo creates one receiver and up to one worker per reported logical CPU, capped at the number of files.
An empty input returns before any threads are started.
This avoids creating workers that cannot receive a job; the count is a workload choice, not a requirement of the event
API:

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

The receiver must keep running while main waits, because it decides when all results have arrived.
The workers can then send results there independently of the main thread's progress.

Posting Work and Choosing a Result Target
=========================================

``worker->events()->invoke(callback)`` transfers a callback to that worker's queue.
Capture input values or owned objects that will remain valid until dispatch.
Within the callback, perform the work and post its result to the target responsible for receiving it.
An ordinary function call stays on the caller's thread; posting through a target is the step that crosses the thread
boundary.

The result target must belong to a loop that continues dispatching while the work runs.
A responsive main loop can receive results directly and update the application's state in its result callback.
Blocking that loop while waiting for a result would also prevent it from dispatching the callback that delivers the
result.
The working example in :doc:`communicating_between_loops` develops this approach and shows how to handle replies to
requests that have since been replaced.

The file hasher instead overrides ``main()`` with a procedural batch function.
Main waits without dispatching, so posting results to its loop would leave them unprocessed.
A separate receiver loop collects the hashes and decides when the batch is complete.
The main function makes the handover visible: prepare storage, start the loops, submit work, then wait for the receiver
before reading results:

.. literalinclude:: ../../../demos/event/FileHasher/FileHasherApp.hpp
    :language: cpp
    :start-at: [[nodiscard]] auto main()
    :end-before: private:
    :dedent: 4

``collectFiles()`` stores the paths in ``_paths`` and prepares the result storage.
After the receiver finishes, ``printResults()`` sorts and prints the completed hashes.
The setup, file traversal, hashing, and output helpers are implemented in ``FileHasherApp.cpp``.
The application lifecycle behind the setup is explained in :doc:`/topics/core/procedural_applications`.

``distributePaths()`` runs on main and assigns successive paths to workers in round-robin order.
Its two nested callbacks show the complete route from an input path to a received result:

.. erbsland-demo::
    :source: event/FileHasher/FileHasherApp.hpp
    :show: f:distributePaths
    :sha256: af2841267416f8c4e372f1239b9966d8f1b15bd307758e490e17923514657870

.. code-block:: cpp

    void distributePaths() {
        for (auto index = el::ItemIndex{}; index.isWithin(_paths.count()); ++index) {
            _workers[index.toSizeT() % _workers.size()]->events()->invoke([this, path = _paths.get(index)]() -> void {
                auto result = hashFile(path);
                _resultReceiver->events()->invoke(
                    [this, result = std::move(result)]() mutable -> void { addResult(std::move(result)); });
            });
        }
    }

.. erbsland-demo-end::

The first :cpp:func:`invoke() <erbsland::event::Events::invoke>` queues a callback on a worker loop.
Its ``path`` capture is a value, so it remains valid after the loop iteration that submitted the job.
``hashFile(path)`` runs on that worker and returns a ``HashResult`` containing the path and digest.
The worker then moves this result into another callback and queues that callback on the receiver.
Only when the receiver dispatches it does ``addResult()`` run.

Queuing a function changes where it will execute; calling ``addResult()`` directly from the worker would update the
result list on the wrong thread.
Likewise, capturing an object by reference does not make it safe to share.
The demo avoids shared hashing state by calculating each hash locally and passing the completed value to its next owner.

Completion, Quit, and Join
==========================

A result notification, a quit request, and a join have different roles.
Your application defines when the task is complete.
``quit()`` requests graceful termination of a loop; it does not interrupt a callback or wait for the thread.
``join()`` waits for the thread to finish; it does not request termination.
Call it from outside that thread, and ensure that some completion or failure path will stop the loop.

In this batch, the receiver alone changes ``_results`` and ``_completedFiles``.
Each received hash advances the batch by one file, and the last one requests receiver shutdown:

.. erbsland-demo::
    :source: event/FileHasher/FileHasherApp.hpp
    :show: f:addResult
    :sha256: 694e2a1d2be2a2ae0ab56bb23f8bba0a10087356eea190074df43ed73b5b6d25

.. code-block:: cpp

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

The receiver's :cpp:func:`quit() <erbsland::event::EventThread::quit>` request allows its loop to exit after the
callback returns.
Main's :cpp:func:`join() <erbsland::event::EventThread::join>` waits until that thread has finished.
Only then does main inspect the completion count, sort the results, and print them.
The incomplete-count check prevents an interrupted batch from being reported as a successful full run.

This is the final ownership handover: ``join()`` establishes that the receiver has finished and its writes are visible
to main.
The absence of simultaneous access, together with that synchronization, makes reading the results safe.
Merely observing a count or retaining a shared pointer would not establish the same boundary.

The workers remain retained until the application's shutdown sequence stops and joins its managed threads.
The receiver quitting therefore marks completion of result collection, while the application still handles the remaining
thread cleanup.

You can run the demo against a directory from the repository root:

.. code-block:: console

    $ cmake-build-debug/demo-apps/event/file_hasher demos/event/FileHasher

It prints a SHA-256 digest and relative path for each file, sorted by path.
Although workers may finish in any order, sorting after the receiver has joined gives the report a stable order.

Failure and Interrupted Work
============================

A completion design also needs a path for failures that produce no result.
``join()`` waits for termination; it is not an exception-transport mechanism.
Choose an event-loop error policy that lets the waiting caller finish or receive a failure.

All these loops use :cpp:enumerator:`FailApplication <erbsland::event::EventLoopErrorPolicy::FailApplication>`.
If hashing throws instead of producing a result, this policy forwards the error to the application and requests shutdown
of its managed loops, including the receiver.
Without a failure path that releases main's wait, the missing result could leave the batch waiting forever.
:doc:`handling_event_errors` explains failure policies and the application boundary that reports the retained error.

Queue Limits and Cooperative Cancellation
=========================================

This demo submits the complete file list up front.
Limiting the number of worker threads does not limit the number of queued jobs: a large input list still means many
queued callbacks and retained paths.
For larger workloads, limit outstanding jobs and submit another when a result arrives.
:doc:`controlling_event_load` develops bounded requests, callback duration, and queue-overflow choices.

A worker callback still occupies its loop until it returns.
A quit request cannot interrupt a file hash halfway through that callback.
If your jobs need prompt cancellation, divide them into bounded steps and check for cancellation between steps.
:doc:`shutting_down_event_work` explains how cancellation, required replies, and thread joining fit together.
If the work instead becomes a long-lived service with dependencies and asynchronous shutdown, an
:doc:`application part <using_event_loops_in_application_parts>` provides the surrounding lifecycle.
