..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

*******************************************
ChecksumPackerInvoke: Explicit Return Paths
*******************************************

``ChecksumPackerInvoke`` implements the same scan-then-pack workflow as :doc:`checksum_packer`, but writes out the
return path using :cpp:func:`EventTarget::invoke() <erbsland::event::EventTarget::invoke>` and lambda captures.
It exists to make the execution and ownership steps behind a bound callback visible.
Both applications retain their workflows on main and use ordinary ``std::thread`` workers for scanning and packing;
neither scanner nor packer owns an event loop.

Capturing the Receiving Context
===============================

The application queues ``Workflow::start()`` on its main target after configuration succeeds.
Inside that callback, :cpp:func:`el::currentLoop() <erbsland::event::currentLoop>` identifies the receiving main-loop
target.
``shared_from_this()`` obtains an owning pointer to the workflow.
Both are captured into the completion passed to the scanner:

.. erbsland-demo::
    :source: event/apps/ChecksumPackerInvoke/src/Workflow.hpp
    :show: f:start
    :sha256: 64ea8400200faa45c384922ed5c9b281527f0dd8fb5499d0eb4e538d831afe6d

.. code-block:: cpp

    void start() {
        _logStream->trace("Starting workflow"_el);
        _state.requireAndSet(State::Initialized, State::Scanning);
        auto events = el::currentLoop();
        auto workflow = shared_from_this();
        _scanner.scan([events, workflow](FileWithDigestList fileList, std::exception_ptr error) -> void {
            events->invoke([workflow, fileList = std::move(fileList), error = std::move(error)]() mutable -> void {
                workflow->onScanned(std::move(fileList), std::move(error));
            });
        });
    }

.. erbsland-demo-end::

The outer lambda is called by the ordinary scanner thread.
It does not call ``onScanned()`` directly; it queues an inner lambda on the captured target.
That inner callback owns the moved file list, captured exception, and workflow pointer.
When main dispatches it, it transfers those values into ``onScanned()``.
The target is captured on main because the scanner thread has no current event loop to discover later.

Ownership Across the Handover
=============================

The application retains the workflow for the whole operation.
The worker continuation and queued delivery additionally retain it across the asynchronous handover.
These shared pointers ensure lifetime, not concurrent access safety: the worker only transports values, while the inner
callback performs workflow state changes on main.

The scanner's own list is built on the worker thread.
Completion passes a value into the continuation, which is moved into the queued callback and later moved into packing.
The worker objects stay members of the workflow, so their owner must join them before destruction.
For the underlying callback ownership rules, see :doc:`/topics/event/callback_lifetimes`.

Repeating the Pattern for Packing
=================================

``onScanned()`` joins the scanner before inspecting the result and advancing the state.
A failure is forwarded to the application immediately.
On success, the method captures the current target and workflow again, then gives the packer a completion that
explicitly queues ``onPacked()`` on main.

.. erbsland-demo::
    :source: event/apps/ChecksumPackerInvoke/src/Workflow.hpp
    :show: f:onScanned, f:onPacked
    :sha256: 6aa38e695490d0859c3aa56e41ba8603e382f53a4f13cd2924bbe098dfb92ba8

.. code-block:: cpp

    void onScanned(FileWithDigestList fileList, std::exception_ptr error) {
        _scanner.shutdown();
        if (error != nullptr) {
            _state.requireAndSet(State::Scanning, State::Failed);
            _onDone(std::move(error));
            return;
        }
        _state.requireAndSet(State::Scanning, State::Packing);
        auto events = el::currentLoop();
        auto workflow = shared_from_this();
        _packer.pack(std::move(fileList), [events, workflow](std::exception_ptr error) -> void {
            events->invoke(
                [workflow, error = std::move(error)]() mutable -> void { workflow->onPacked(std::move(error)); });
        });
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

The two nested callbacks have different execution contexts: the outer one runs on the packer thread, the inner one on
main.
Packing errors travel as data until main handles them.
``onPacked()`` joins the packer, records done or failed, and calls the application continuation on main.
The worker can return immediately after posting completion; it does not require a main-loop acknowledgement before join
can finish.

Completion and Failure Remain Application Responsibilities
==========================================================

The application starts one retained workflow per source directory and quits only after all succeed.
It rethrows a delivered error on main for application diagnostics.
Cleanup explicitly cancels and joins the ordinary workers in every workflow, even if event dispatch already ended.
Explicit ``invoke()`` calls do not add automatic worker management or guarantee delivery after the target closes.
The main loop stays open for required completion during the normal workflow.

.. erbsland-demo::
    :source: event/apps/ChecksumPackerInvoke/src/ChecksumPackerApp.hpp
    :show: f:onWorkflowDone, f:cleanup
    :sha256: ceaafdea92ea2dcccdaeb9b46af5dbba677514197b5945f7f59ab2f5c110e153

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
    $ cmake-build-debug/demo-apps/event/checksum_packer_invoke demos/event/apps/FileHasher/example-files --output "$archive_dir"

The run uses two small fixtures and a temporary archive directory, matching the bound-callback application's workload.
The output demonstrates the same completed workflow; the distinction lies in how the source expresses the return path.
:doc:`/topics/event/event_bindings` compares the two forms directly, while :doc:`checksum_packer` explains the shared
configuration and worker stages in more detail.
