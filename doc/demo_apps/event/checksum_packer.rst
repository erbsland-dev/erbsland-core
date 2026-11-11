..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

***********************************************
ChecksumPacker: A Workflow with Bound Callbacks
***********************************************

``ChecksumPacker`` demonstrates a task with dependent asynchronous stages: scan a directory and calculate checksums,
then create a ZIP archive with a checksum manifest.
A workflow keeps state transitions on the main loop while ordinary worker threads perform the blocking work.
Bound callbacks bring each result back to the workflow's owner context.
The archive and hashing APIs are supporting details; the focus is composition, result ownership, and completion.
:doc:`/topics/event/event_workflows` explains how persistent state connects events and next-step decisions.

Application Ownership and Startup
=================================

The application validates source directories and output destinations before creating workflows.
It chooses a distinct archive path for each source and rejects destinations inside a source tree.
Each workflow is retained in ``_workflows`` and also captured by its queued startup callback.
The application keeps the default main, so these callbacks run when main-loop dispatch begins.

.. erbsland-demo::
    :source: event/apps/ChecksumPacker/src/ChecksumPackerApp.hpp
    :show: f:startWorkflows
    :sha256: 1f777567cebc2c958a6321ece227b38f3f561978427b3e591879361346fbbca1

.. code-block:: cpp

    void startWorkflows() {
        logStream()->info("Checksum Packer V", info().applicationVersion().toString());
        // process each source directory individually and start a new workflow for it.
        for (const auto &[source, zipFile, logPath] : _preparation.sourceAndZipFiles()) {
            auto logStream = log().createStream(logPath, cVerboseTraceSection);
            // create a new workflow and schedule its start.
            auto workflow = std::make_shared<Workflow>(
                source, zipFile, std::move(logStream), [this](std::exception_ptr error) -> void {
                    this->onWorkflowDone(std::move(error));
                });
            _workflows.append(workflow);
            loop()->invoke([workflow = std::move(workflow)]() -> void { workflow->start(); });
        }
    }

.. erbsland-demo-end::

Different source directories may have active workflows at the same time.
Within one workflow the stages are sequential: packing begins only after successful scanning.
All workflow state transitions and application completion checks execute on main.

Scanning on an Ordinary Thread
==============================

``Workflow::start()`` changes its state from initialized to scanning and passes a bound callback to ``Scanner``.
Because startup runs on main, the binding captures the main target and strongly retains the workflow recipient.
``Scanner::scan()`` starts a regular ``std::thread``; this is not a managed event thread and has no worker event loop.
The worker calls the supplied continuation with owned results and an optional exception.
The binding queues delivery on main instead of calling ``onScanned()`` on that worker.

.. erbsland-demo::
    :source: event/apps/ChecksumPacker/src/Workflow.hpp
    :show: f:start
    :sha256: 096b811f20b6cec6fe17f579d02e7dd36b0b1313e9528f37e077f74fc1e17a68

.. code-block:: cpp

    void start() {
        _logStream->trace("Starting workflow"_el);
        _state.requireAndSet(State::Initialized, State::Scanning);
        _scanner.scan(el::bindEvent(shared_from_this(), &Workflow::onScanned));
    }

.. erbsland-demo-end::

.. erbsland-demo::
    :source: event/apps/ChecksumPacker/src/Scanner.hpp
    :show: f:scan, f:shutdown
    :sha256: 119b2657a3e08861db6cdea900865a4f7d8dacec0b342fd8432f57a6799d7341

.. code-block:: cpp

    void scan(OnDone onDone) {
        _workerThread = std::thread{[onDone = std::move(onDone), this]() -> void { scanInThread(onDone); }};
    }

    void shutdown() {
        _shutdown = true;
        if (_workerThread.joinable()) {
            _workerThread.join();
        }
    }

.. erbsland-demo-end::

The scanner owns its growing list while it walks regular files and computes SHA3-256 digests.
It skips symbolic links and checks an atomic shutdown flag while scanning and reading.
Operational failures are caught and placed in the completion's ``exception_ptr``.
The worker does not modify workflow state directly.

Joining Before the Next Stage
=============================

On main, ``onScanned()`` first shuts down and joins the scanner.
A captured error moves the workflow to failed and is sent to the application.
On success, the file list is moved into the packer and a second bound callback targets ``onPacked()``.
``Packer`` also uses an ordinary ``std::thread``; its capture retains the input list for the duration of packing.

.. erbsland-demo::
    :source: event/apps/ChecksumPacker/src/Workflow.hpp
    :show: f:onScanned, f:onPacked
    :sha256: f6c27eaed5b4eb82b9a571a0056cac884bdf57af30ae8aafeaadb5eaf874a3ab

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

Joining in these handlers is safe with this protocol because the worker has submitted its final result and does not wait
for main to acknowledge it before returning.
The owner joins before it advances or releases the corresponding worker object.
The packer adds the files and a ``checksum.txt`` manifest, finalizes the archive, and reports success or a captured
operational failure.
The detailed ZIP encoding is independent of the event coordination.

Application Completion and Cleanup
==================================

``onPacked()`` joins the packer and sets the terminal workflow state before calling the application continuation.
The application rethrows a delivered error on main.
For success, it checks every retained workflow and requests quit only after all have reached done.
Its cleanup calls each workflow's ``shutdown()``, which requests cancellation and joins both ordinary workers.
These threads are not registered with the framework; the application explicitly owns that obligation.

.. erbsland-demo::
    :source: event/apps/ChecksumPacker/src/ChecksumPackerApp.hpp
    :show: f:onWorkflowDone, f:cleanup
    :sha256: b39971387edb51d70588b6264412af069e9341b8e00ed0381ba0548f647a5c30

.. code-block:: cpp

    void onWorkflowDone(std::exception_ptr error) {
        if (error != nullptr) {
            // rethrow the original exception.
            std::rethrow_exception(std::move(error));
        }
        // if one workflow successfully finished, test if all workflows are done.
        for (const auto &workflow : _workflows) {
            if (!workflow->isDone()) {
                return; // wait for the other workflows to finish.
            }
        }
        logStream()->info("All packages were created successfully."_el);
        quit();
    }

    void cleanup() noexcept override {
        for (const auto &workflow : _workflows) {
            workflow->shutdown();
        }
    }

.. erbsland-demo-end::

Run from the repository root with a temporary output directory:

.. code-block:: console

    $ archive_dir=$(mktemp -d)
    $ cmake-build-debug/demo-apps/event/checksum_packer demos/event/apps/FileHasher/example-files --output "$archive_dir"

This bounded run packages the two small file-hasher fixtures into a disposable output directory.
Cleanup also works when dispatch has already ended: atomic cancellation and joining do not depend on another main-loop
reply.
Error propagation and worker cleanup therefore remain separate from the successful all-done test.

Compare :doc:`checksum_packer_invoke` for the same workflow with explicit target invocations.
:doc:`/topics/event/event_bindings` explains how the bindings replace those captures and nested callbacks.
