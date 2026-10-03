..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Events; Workflows
    single: Callbacks; Operation State
    single: Events; Workflow Completion

***********************************
Building Workflows Across Callbacks
***********************************

A useful operation often needs several asynchronous steps, with each result deciding what happens next.
A callback receives one result; a workflow connects those results to the task the caller wants completed.
You need somewhere to keep progress between callbacks, decide which step may follow, and report the final outcome.

The ``ChecksumPacker`` demo shows this with a practical task: create a ZIP archive of a directory and include a
``checksum.txt`` manifest containing the files' checksums.
Scanning and packing run on worker threads, while callbacks on the main loop coordinate the operation.
This page focuses on operation state and dependencies, building on the bound continuations introduced in
:doc:`event_bindings`.

From Individual Steps to One Operation
======================================

Creating a package has a dependency: packing needs the file list and checksums produced by scanning.
A successful scan makes packing possible, but the caller's task is complete only when the archive has been written.
If either step fails, the workflow must report failure instead of leaving the caller waiting for success.

The demo creates one ``Workflow`` object for each source directory.
That object owns the scanner, packer, current state, and a completion callback supplied by the application.
It survives the individual callbacks, giving all steps a shared context.
Its states describe progress toward one archive:

.. mermaid::

    flowchart TD
        Initialized["Initialized"] -->|Start| Scanning["Scanning"]
        Scanning -->|File list and checksums ready| Packing["Packing"]
        Packing -->|Archive finished| Done["Done"]
        Scanning -->|Scan error| Failed["Failed"]
        Packing -->|Packing error| Failed

``Scanning`` and ``Packing`` mean that the operation is still in progress.
``Done`` and ``Failed`` are its two terminal outcomes.
The application retains its workflows until cleanup, and queued callbacks also capture a shared pointer to their
workflow so it stays alive while they use it.

This state belongs to the whole operation.
The scanner knows how to produce a file list; the workflow knows that a successful file list should lead to packing.
Keeping that decision in the workflow lets each worker concentrate on its own step.
In your own application, the equivalent object might coordinate downloading and importing a document, or loading and
transforming an image.

Advancing the Workflow from a Result
====================================

The first transition happens in ``Workflow::start()``, which the application queues on the main loop.
It sets the state to ``Scanning`` before asking the scanner to begin.
It also supplies the callback that will carry the result back to this workflow:

.. erbsland-demo::
    :source: event/ChecksumPacker/src/Workflow.hpp
    :show: f:start(-2,-1)
    :sha256: 4f4dff8ced409ce699842c6e3f993aefc6e526e9c46df2c017823298a30b3b6e

.. code-block:: cpp

    _state.requireAndSet(State::Initialized, State::Scanning);
    _scanner.scan(el::bindEvent(shared_from_this(), &Workflow::onScanned));

.. erbsland-demo-end::

The binding captures the current main-loop target and retains this workflow.
``scan()`` starts a worker thread and returns; the workflow remains in ``Scanning`` while main dispatches other work.
When scanning finishes, the worker calls its ordinary completion function with the file list and an optional error.
That bound function queues ``onScanned()`` on main, where the result determines the next state:

.. erbsland-demo::
    :source: event/ChecksumPacker/src/Workflow.hpp
    :show: f:onScanned
    :sha256: 3c4a30523405b38f05d5efe6d65f5373196cded92e27fac5aa445efde1467d94

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

A failed scan ends the workflow and reports the error through ``_onDone``.
A successful scan changes the state to ``Packing`` and passes the file list to the packer.
The callback supplied to ``pack()`` arranges for ``onPacked()`` to receive the next result on the main loop.
The method then returns while packing continues.

``requireAndSet()`` expresses which transition is valid: this callback must arrive while the workflow is ``Scanning``.
It makes an unexpected transition a detectable programming error.
You do not need a general workflow engine to connect these steps; named continuation methods and explicit state make the
sequence visible in ordinary application code.

Returning Decisions to the Owner Loop
=====================================

The scan call has two distinct boundaries: starting a worker and returning its result to the main loop.
``Scanner::scan()`` itself runs on main; it starts a thread that executes ``scanInThread()``.
When the work finishes, ``scanInThread()`` calls the completion function passed to ``scan()``, named ``onDone`` inside
the scanner.
That function is the event binding supplied by the workflow.
Calling it on the worker queues its continuation on main.

.. mermaid::

    sequenceDiagram
        participant M as Main loop / Workflow
        participant S as Scanner (called on main)
        participant W as Scan worker
        M->>M: start(): Initialized to Scanning
        M->>S: scan(onDone)
        S->>W: Start thread: scanInThread(onDone)
        S-->>M: scan() returns
        Note over M: start() returns to dispatch
        Note over W: Build file list and checksums
        W->>W: onDone(fileList, error)
        W-->>M: Bound continuation: queue onScanned()
        Note over M: Later, dispatch the queued callback
        M->>M: onScanned(fileList, error)
        Note over M: Advance to Packing or finish as Failed

The scanner's completion callback reports that one step has finished.
The queued ``onScanned()`` continuation decides what that means for the whole operation.
An ordinary callback has no automatic main-loop affinity.
The binding establishes that return boundary when the workflow registers it.

Packing uses the same pattern: ``onScanned()`` starts the packer, the pack worker reports its result, and a queued
callback enters ``onPacked()`` on main.
All workflow state changes therefore happen on one loop, even though the individual steps run elsewhere.
:doc:`communicating_between_loops` develops the request and reply mechanics in more detail.

Ownership Across the Steps
==========================

The demo needs no application mutex around its workflow state or file list because different threads do not modify those
objects at the same time.
The main loop alone changes ``_state``.
The scan worker builds its own list, supplies a result value to its callback, and that value moves into the queued
continuation.
After receiving it, the main loop passes it onward to the packing worker.

.. list-table::
    :header-rows: 1
    :widths: 45 55

    * - Data
      - Where it is used
    * - Workflow progress and next-step decisions
      - Main-loop callbacks only.
    * - File list being assembled
      - Scan worker only.
    * - Result waiting for dispatch
      - Owned by the queued callback.
    * - File list used to create the archive
      - Pack worker, after the handoff from main.

The strong member binding keeps the workflow alive; it does not make its members thread-safe.
The ownership and execution rules are what make access safe.
The worker shutdown signal is explicitly atomic, and the event queue handles synchronization for delivering callbacks.
Before advancing past a worker's result, the workflow joins that worker through ``shutdown()``.
Application cleanup also stops and joins workers before releasing the workflows.
See :doc:`shutting_down_event_work` for the broader shutdown design.

Defining Completion for the Caller
==================================

Each step has a result, but only the workflow knows whether that result completes the caller's task.
A successful scan continues to packing; a successful pack completes the archive.
Either worker's reported error ends this workflow with failure.
The common completion callback, ``_onDone``, carries ``nullptr`` for success or an exception pointer for failure.

The final continuation records the outcome before notifying the application:

.. erbsland-demo::
    :source: event/ChecksumPacker/src/Workflow.hpp
    :show: f:onPacked
    :sha256: 060392aebce7cee11bbecef3d09999d36cfaf56f1ebea4e42af9b8379a98923e

.. code-block:: cpp

    void onPacked(std::exception_ptr error) {
        _packer.shutdown();
        if (error != nullptr) {
            _state.requireAndSet(State::Packing, State::Failed);
        } else {
            _state.requireAndSet(State::Packing, State::Done);
        }
        _onDone(std::move(error));
    }

.. erbsland-demo-end::

The caller therefore observes a workflow that has already reached its terminal state.
Both failure paths and the success path provide an explicit end to the operation.
When you compose your own steps, give each result this same decision: continue with a valid next step, or finish with an
outcome the caller can handle.

The application may have several independent directory workflows in progress.
It quits successfully when all have reached ``Done``; a reported error is rethrown on main so the application's error
handling can report failure and begin cleanup.
That application-wide policy is separate from the sequence needed to create one archive.
:doc:`handling_event_errors` explains error reporting beyond the individual workflow.
