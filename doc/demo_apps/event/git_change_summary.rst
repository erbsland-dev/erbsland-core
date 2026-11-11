..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Demo Applications; Git Change Summary
    single: Subprocess; Lazy Event Binding

******************
Git Change Summary
******************

This command-line utility turns Git status into a directory-level view of unfinished work.
It counts staged, unstaged, untracked, and conflicted entries under each top-level directory, making it easier to see
which parts of a large repository have changes.
An optional directory argument selects the repository; the default is the current directory.
Git must be installed and available on ``PATH``.

The application demonstrates :doc:`lazy source binding </topics/event/using_event_apis>` in a procedural program.
Main prepares a subprocess without an event loop, then an event thread binds its notifications and starts Git.
Main joins that thread before printing the completed report.
The application's main loop is never dispatched.

Preparing the Process in Procedural Main
========================================

``GitChangeSummaryApp`` derives from ``Application`` and overrides ``main()`` to express a sequential operation.
The normal application lifecycle handles options, help, diagnostics, and managed worker cleanup.
The directory is validated before main starts work.
``findGit()`` searches the platform's executable candidates and selects an executable file; ``Subprocess`` receives that
resolved path and a separate argument list, without a shell.

.. erbsland-demo::
    :source: event/apps/GitChangeSummary/src/GitChangeSummaryApp.cpp
    :show: f:GitChangeSummaryApp::main
    :sha256: 27453e894faa3133e7be1f34bef3d0291fade22a1d0352771c20849fcb0e2e18

.. code-block:: cpp

    auto GitChangeSummaryApp::main() -> el::ExitCode {
        auto processOptions = el::SubprocessOptions{};
        processOptions.setInheritStandardInput(false)
            .setStandardOutputMode(el::SubprocessOutputMode::Capture)
            .setStandardErrorMode(el::SubprocessOutputMode::Capture)
            .setRemoveOutputBom(false);
        const auto arguments = el::StringList{
            "--no-optional-locks"_el,
            "-C"_el,
            _directory.toNativeString(),
            "status"_el,
            "--porcelain=v1"_el,
            "-z"_el,
            "--untracked-files=all"_el};
        // Preparation needs no event loop. The source has no owner yet.
        _process = el::Subprocess::create(findGit(), arguments, processOptions);
        auto loopOptions = el::EventLoopOptions{};
        loopOptions.setErrorPolicy(el::EventLoopErrorPolicy::FailApplication);
        _worker = createEventThread(loopOptions);
        _worker->start();
        _worker->loop()->invoke([this]() -> void { onStart(); });
        _worker->join();
        // Joining completes the handover of worker-owned results to procedural main.
        if (_exitCode.isSuccess()) {
            printReport();
        }
        return _exitCode;
    }

.. erbsland-demo-end::

``Subprocess::create(...)`` only prepares the child.
It does not launch Git and does not bind an event loop.
Standard output and standard error use capture mode, and stdin is closed because this command needs no interactive
input.
BOM removal is disabled to preserve Git's machine-readable bytes.

The managed worker uses ``FailApplication`` so an exception in its setup or output callbacks ends the application
through its diagnostic boundary.
After starting the worker, main queues ``onStart()`` and waits in ``join()``.
Joining does not itself stop the worker; completion or the failure policy must first request shutdown.
See :doc:`/topics/event/processing_work_on_event_threads` for this lifecycle.

Binding and Starting Git on the Worker
======================================

The application retains both the process and its receiving thread:

.. erbsland-demo::
    :source: event/apps/GitChangeSummary/src/GitChangeSummaryApp.hpp
    :show: c:Created without a loop-c:Success is recorded
    :sha256: 23436bf99853d8330701c7f3a58eb2d6542c7ebfde90ef048371a06ba558dbce

.. code-block:: cpp

    el::SubprocessPtr _process;        ///< Created without a loop; retained through worker shutdown.
    el::ManagedEventThreadPtr _worker; ///< Runs the source's owner loop and is joined before reading results.
    GitStatusReport _report;           ///< Written on the worker; read by main only after joining.
    el::ExitCode _exitCode{el::ExitCode::failure()}; ///< Success is recorded only after complete output validation.

.. erbsland-demo-end::

The worker dispatches ``onStart()`` with its loop current.
The first ``events()`` call binds the prepared subprocess to that loop for the rest of its lifetime:

.. erbsland-demo::
    :source: event/apps/GitChangeSummary/src/GitChangeSummaryApp.cpp
    :show: f:GitChangeSummaryApp::onStart
    :sha256: 041293546f3efc2aea3cb3c10786e88f11058276937c48663de5a8eee5c47e99

.. code-block:: cpp

    void GitChangeSummaryApp::onStart() {
        // The first events() call binds the source permanently to this worker loop.
        _process->events()
            .onStandardOutput([this](const el::String &chunk) -> void { onStandardOutput(chunk); })
            .onExit([this](const el::ExitCode code) -> void { onExit(code); })
            .onError([this](const el::PlatformErrorContext &error) -> void { onError(error); })
            .onOutputTruncated([this]() -> void { onOutputTruncated(); })
            .onFinal([this]() -> void { onFinal(); });
        _process->start();
    }

.. erbsland-demo-end::

Every connected callback runs on this worker, including output, failure, and completion callbacks.
Registering the handlers does not launch the process; the final ``start()`` call does that after all connections exist.
The callbacks borrow the application through ``this``.
The application's lifetime encloses worker shutdown and joining, so the source need not retain its owner through a
shared-pointer capture.

Processing Git's Output
=======================

Git runs with ``status --porcelain=v1 -z --untracked-files=all``.
The versioned porcelain format supplies stable status fields and repository-relative paths.
NUL separators preserve spaces, tabs, and newlines in filenames without shell quoting.
``--no-optional-locks`` avoids Git's optional index refresh write.
See the `Git status format <https://git-scm.com/docs/git-status#_porcelain_format_version_1>`_ for its record contract.

The stdout callback passes each block to a separate parser:

.. erbsland-demo::
    :source: event/apps/GitChangeSummary/src/GitChangeSummaryApp.cpp
    :show: f:GitChangeSummaryApp::onStandardOutput
    :sha256: e6c39790d524fb4189211b176709aff4c6045af7a8d3c2cc74ab1f4f032441bc

.. code-block:: cpp

    void GitChangeSummaryApp::onStandardOutput(const el::String &chunk) {
        _report.append(chunk);
    }

.. erbsland-demo-end::

An output callback is not a record boundary.
``GitStatusReport`` keeps an incomplete record until the next NUL arrives and separately remembers when a rename or copy
needs its original-path record:

.. erbsland-demo::
    :source: event/apps/GitChangeSummary/src/GitStatusReport.cpp
    :show: f:GitStatusReport::append, f:GitStatusReport::finish
    :sha256: 257caed2026c82368b0c86b4ce55924b30ed7dadfe5f87a069c9a011e8762fbd

.. code-block:: cpp

    void GitStatusReport::append(const el::String &chunk) {
        auto start = el::ByteIndex{};
        while (start.isWithin(chunk.length())) {
            const auto end = chunk.find("\0"_el, start);
            if (end.isNoIndex()) {
                _pending.append(chunk.slice(el::ByteRange{start, el::ByteCount::infinite()}));
                return;
            }
            _pending.append(chunk.slice(el::ByteRange{start, end}));
            consumeRecord(el::String{_pending});
            _pending.clear();
            start = end + el::ByteCount{1};
        }
    }

    void GitStatusReport::finish() const {
        if (!_pending.isEmpty() || _needsOriginalPath) {
            throw el::ApplicationError{"Incomplete Git status output."_el};
        }
    }

.. erbsland-demo-end::

Each ordinary record contributes to its top-level directory.
Files directly in the repository root use ``(root)``; directory labels end with ``/``.
The first status character describes the index and the second describes the working tree, so a file can count in both
the staged and unstaged columns.
Untracked files have their own column, while unmerged entries count only as conflicts.
Renames and copies count in the destination directory; their extra original-path record is consumed without counting
another file.
Submodule entries are counted as reported by Git, without recursively reporting their contents.

The report stores counts in an ordered map and prints directories in name order.
Directory labels escape control characters for a readable single-line table.
A clean working tree produces a short message instead of an empty table.

Completion, Failure, and Result Handover
========================================

The exit callback accepts only a successful Git exit with complete, untruncated output.
It validates the parser's final state before marking the result successful.
The final callback then requests worker shutdown:

.. erbsland-demo::
    :source: event/apps/GitChangeSummary/src/GitChangeSummaryApp.cpp
    :show: f:GitChangeSummaryApp::onExit, f:GitChangeSummaryApp::onFinal
    :sha256: c333dc5dfa51aade8973e7fe64fdb79948fdec6bdb403cafdbd09a41c834eb1d

.. code-block:: cpp

    void GitChangeSummaryApp::onExit(const el::ExitCode code) {
        if (code.isFailure()) {
            throw el::ApplicationError{"Git status failed."_el, _process->standardError()};
        }
        if (_process->wasStandardOutputTruncated() || _process->wasStandardErrorTruncated()) {
            onOutputTruncated();
        }
        _report.finish();
        _exitCode = el::ExitCode::success();
    }

    void GitChangeSummaryApp::onFinal() {
        _worker->quit();
    }

.. erbsland-demo-end::

Main reads the report only after ``join()`` returns.
There is no concurrent access to the parser or counters, so they need no mutex.
Required completion stays on the worker rather than depending on the application's undispatched main loop.

Git errors, such as selecting a directory outside a repository, include its captured stderr in the diagnostic.
Native process failures and output truncation raise application errors as well.
The managed failure policy stops and joins event threads even if a callback cannot reach normal completion.
The application prints no successful partial report on these paths.
After managed threads have finished, ``cleanup()`` releases the subprocess and thread pointers.

Running the Utility
===================

Build the ``git_change_summary`` target, then run it against the current repository or pass a directory explicitly.
The following output is an example run against the checkout used to build these docs; counts depend on its current
changes:

.. erbsland-demo::
    :source: event/apps/GitChangeSummary/src/main.cpp
    :show: f:main
    :exec: event/git_change_summary
    :exec-2: event/git_change_summary --help
    :show-cmd-line:
    :sha256: 398536cd5945db393075babe4406b669a7a534aa0f9c5a676640adac0a725ef5

.. code-block:: cpp

    auto main(const int argc, char *argv[]) -> int {
        auto app = demo::GitChangeSummaryApp{argc, argv};
        return app.run();
    }

.. erbsland-demo-run::

    .. code-block:: console

        $ event/git_change_summary

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mStaged  Unstaged  Untracked  Conflicted  Directory
             0         1         13           0  demos/
             0         8          1           0  doc/␛[0m

.. erbsland-demo-run::

    .. code-block:: console

        $ event/git_change_summary --help

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0m
        ␛[1mUsage:
          ␛[92;49m␛[22mgit_change_summary␛[39;49m ␛[90;49m[␛[96;49moptions␛[90;49m]␛[39;49m ␛[90;49m<␛[95;49mdirectory␛[90;49m>

        ␛[39;49m␛[1mOptions:
          ␛[22m    ␛[90;49m<␛[95;49mdirectory␛[90;49m>␛[39;49m         Summarize repository changes by directory.
          ␛[93;49m-h␛[39;49m, ␛[96;49m--help␛[39;49m[=␛[90;49m<␛[92;49mkeyword␛[90;49m>␛[39;49m]  Display this help.
              ␛[96;49m--version␛[39;49m           Display version information.␛[0m

.. erbsland-demo-end::

The parser tests exercise every possible split between two callbacks, single-byte chunks, rename pairs, all conflict
forms, unusual names, and incomplete output.
The integration test creates disposable Git repositories to check normal reports and failure shutdown without changing
the selected working repository.
