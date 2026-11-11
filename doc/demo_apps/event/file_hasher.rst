..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

*****************************************
FileHasher: Workers and a Result Receiver
*****************************************

``FileHasher`` illustrates a procedural main that coordinates several managed event loops without running the
application's main loop.
Worker loops hash files, a separate receiver serializes result updates, and main joins that receiver before reading the
results.
The interesting part is the ownership handover, not the SHA-256 calculation.
For the event-thread lifecycle itself, see :doc:`/topics/event/processing_work_on_event_threads`.
For a new application consisting only of independent file jobs, consider :doc:`/reference/util/worker_pool` as well.

Three Execution Roles
=====================

Main resolves the directory, collects regular-file paths while skipping symbolic links, fixes the expected result count,
and reserves result storage before any worker starts.
The workers each receive independent path values and produce ``HashResult`` values.
The receiver is the only thread that appends results or changes the completed count during processing.

.. mermaid::

    sequenceDiagram
        participant M as Procedural main
        participant W as Worker loops
        participant R as Receiver loop
        M->>M: Collect paths and prepare result storage
        M->>W: Queue independent paths
        M->>R: Join receiver (main waits)
        W->>R: Queue owned results
        R->>R: Append each result and count completion
        R->>R: Last result requests quit
        R-->>M: Receiver ends, join returns
        M->>M: Verify count, sort and print

Ownership of the application object stays on main throughout.
Ownership of *access* to result state passes from main's preparation phase to the receiver, then back to main after
join.
The members make these roles visible:

.. erbsland-demo::
    :source: event/apps/FileHasher/FileHasherApp.hpp
    :show: c:Workers retained(+0,+6)
    :sha256: 45f394ca7a68e8a3904fae1435573cd417c2ea99c312dde359b9e92fc3f2f7ef

.. code-block:: cpp

    std::vector<el::ManagedEventThreadPtr> _workers; ///< Workers retained throughout the batch.
    el::ManagedEventThreadPtr _resultReceiver;       ///< Sole owner of mutable result state while running.
    el::Path _root;                                  ///< The root path to be scanned.
    el::PathList _paths;                             ///< The list of paths to process.
    std::size_t _totalFiles{0};                      ///< Fixed before any thread starts.
    std::size_t _completedFiles{0};                  ///< Updated only on the receiver thread.
    std::vector<HashResult> _results;                ///< Read by main only after the receiver is joined.

.. erbsland-demo-end::

Starting and Retaining the Loops
================================

``startLoops()`` creates the receiver first, then at most one worker per logical CPU or input file, whichever is
smaller.
These are managed :cpp:class:`EventThread <erbsland::event::EventThread>` objects, each with its own event loop and
native thread.
The application retains them in ``_resultReceiver`` and ``_workers``; registration alone would not keep them alive.
Every loop uses ``FailApplication`` so an operational failure also has a path to completion.

.. erbsland-demo::
    :source: event/apps/FileHasher/FileHasherApp.hpp
    :show: f:startLoops
    :sha256: 9379aab1376abf242b14f26b6bcb29ce8e25b01f136cd693d422b74c73847bff

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

Moving Paths and Results
========================

``distributePaths()`` assigns paths round-robin to the worker targets.
The outer callback owns its path and computes the hash on that worker.
The inner callback moves the result to the receiver, where ``addResult()`` updates the collection.
No worker directly writes the shared result vector.

.. erbsland-demo::
    :source: event/apps/FileHasher/FileHasherApp.hpp
    :show: f:distributePaths, f:addResult
    :sha256: 9ba5335b344a6aba1015d3583bc136adb28dae7c5c96ce9cacab80baeba98bb7

.. code-block:: cpp

    void distributePaths() {
        for (auto index = el::ItemIndex{}; index.isWithin(_paths.count()); ++index) {
            _workers[index.toSizeT() % _workers.size()]->loop()->invoke([this, path = _paths.get(index)]() -> void {
                auto result = hashFile(path);
                _resultReceiver->loop()->invoke(
                    [this, result = std::move(result)]() mutable -> void { addResult(std::move(result)); });
            });
        }
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

The callbacks borrow ``this`` safely because the application remains alive through managed shutdown and joining.
Only the receiver reads and increments ``_completedFiles`` while it runs.
After the final result, it requests its own quit.
The workers do not need a reply from the blocked main thread to finish their callbacks.

Joining Before Reading
======================

Main waits for the receiver, checks that every expected result arrived, then sorts by path and prints the digests.
An empty directory returns before any threads are created.
After main returns, the application framework stops and joins the still-retained managed workers.
Thus receiver completion and worker cleanup have separate responsibilities.

.. erbsland-demo::
    :source: event/apps/FileHasher/FileHasherApp.hpp
    :show: c:implement Application(-4,+19)
    :sha256: 07dc4f925835a837dfe89aa78e81dddce96fd46f5c12a44f04cbf4ad18e5fc40

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

.. erbsland-demo-end::

From the repository root, this bounded run uses two small repository fixtures and writes no files:

.. code-block:: console

    $ cmake-build-debug/demo-apps/event/file_hasher demos/event/apps/FileHasher/example-files

It prints one SHA-256 digest per file, sorted by path after the receiver joins.
Reading a file may fail after discovery; a managed worker's ``RuntimeError`` then triggers application failure and
requests shutdown of the other managed loops, including the receiver.
Its join can return with fewer results than expected, which main treats as failure rather than printing a partial
success.
The framework reports the captured failure through application diagnostics.
Joining itself does not transport the exception; the managed failure route does that.
See :doc:`/topics/event/handling_event_errors` for that contract.
