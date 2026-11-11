..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Demo Applications; ConfigCheck

*******************************************
ConfigCheck: Checking Configuration Batches
*******************************************

``ConfigCheck`` checks ELCL configuration files before deployment and reports all failed files together.
When several files are broken, one run can give you the diagnostics needed to fix them instead of stopping at the first
problem.
For a large collection, ``--max-errors`` limits the number of failed files before the checker stops.

The application uses one managed event worker to process independent parsing requests in command-line order.
Its default ``Store`` policy retains failures for reporting after the worker finishes.
An optional custom handler implements the error limit while leaving the same reporting code in charge of diagnostics.
:doc:`/topics/event/handling_event_errors` explains these two responsibilities.

Checking Files Before Deployment
================================

Build and run the application with one or more configuration paths:

.. code-block:: console

    $ cmake --build cmake-build-debug --target config_check
    $ cmake-build-debug/demo-apps/event/config_check server.elcl worker.elcl
    $ cmake-build-debug/demo-apps/event/config_check --max-errors 2 server.elcl worker.elcl

Each positional argument identifies a top-level configuration file.
The checker uses Core's default parser, including its file-include and access rules.
A successful check means the complete document could be loaded and parsed; validation against the consuming
application's schema remains a separate step.
Parser extensions such as application-specific placeholders or signature verification require that application's
configuration environment.

The default limit is zero, meaning every requested file is checked.
A positive limit stops before the next file after that many files have failed.
Each failed file contributes its first parser error, including errors encountered in its included documents.
The checker writes successful filenames and a summary to standard output, complete diagnostics to standard error, and
returns failure if any file failed.
It reads configurations without changing them.

Each queued callback performs this real operation:

.. erbsland-demo::
    :source: event/apps/ConfigCheck/src/ConfigurationBatch.cpp
    :show: f:ConfigurationBatch::checkFile
    :files: ../../../demos/event/apps/ConfigCheck/example-files/server.elcl ../../../demos/event/apps/ConfigCheck/example-files/worker.elcl
    :exec: event/config_check server.elcl worker.elcl
    :show-cmd-line:
    :sha256: 736dc0cc0b599203cae95faf3504dec565d0c77a8cd624eb1891287b0a340eab

.. code-block:: cpp

    void ConfigurationBatch::checkFile(const el::Path &path) {
        ++_checkedCount;
        // Each file is independent. Parser errors retain the source location and any nested cause.
        (void)el::conf::Parser{}.parseFileOrThrow(path);
        _passedPaths.append(path);
    }

.. erbsland-demo-run::

    .. code-block:: console

        $ event/config_check server.elcl worker.elcl

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mOK  server.elcl
        OK  worker.elcl
        Checked 2 of 2 files: 2 passed, 0 failed, 0 skipped.␛[0m

.. erbsland-demo-end::

The parser provides the file location and diagnostic context for malformed input and unavailable sources.
An exception leaves the callback before the path enters the successful-files list.
The callback does not need to catch and repackage the parser's error.

Owning a Finite Batch
=====================

The application's procedural ``main()`` creates the batch before its managed worker.
That declaration order keeps the batch alive until the worker, its queued callbacks, and its custom handler have been
released.
The worker uses ``Store`` so failure of one input does not prevent checks of independent inputs:

.. erbsland-demo::
    :source: event/apps/ConfigCheck/src/ConfigCheckApp.cpp
    :show: a:batch(+0,+4)
    :sha256: 4f712054a6f3b4d4a68ed934e7886919006d088443ba034e13884c7e964d4324

.. code-block:: cpp

    auto batch = ConfigurationBatch{_paths, _maximumErrors};
    const auto worker = createEventThread(el::EventLoopOptions{}.setErrorPolicy(el::EventLoopErrorPolicy::Store));
    batch.schedule(worker->workerEventLoop());
    worker->start();
    worker->join();

.. erbsland-demo-end::

``schedule()`` queues one callback per input and a final callback that quits the loop.
All submissions finish before the thread starts, making argument order also the check order.
There are no external producers or asynchronous parser completions to wait for:

.. erbsland-demo::
    :source: event/apps/ConfigCheck/src/ConfigurationBatch.cpp
    :show: f:ConfigurationBatch::schedule
    :sha256: 327c52cdfe28ca67d12ddba867f539aad55ed7acdd2b15049c71c22b831a3916

.. code-block:: cpp

    void ConfigurationBatch::schedule(el::EventLoop &loop) {
        installErrorLimit(loop);
        for (const auto &path : _paths) {
            loop.invoke([this, path]() -> void { checkFile(path); });
        }
        // Store continues after a failed check, so normal completion always reaches this callback.
        loop.invoke([&loop]() -> void { loop.quit(); });
    }

.. erbsland-demo-end::

The batch's counters and successful-path list are accessed only by the worker during dispatch.
Main reads them after ``join()`` returns, so they need no concurrent access or locking.
In normal mode, ``Store`` allows the final quit callback to run even if earlier checks failed.
Joining then establishes completion, but leaves failure reporting to the application.

Reporting Every Failed File
===========================

After joining, main removes errors from the worker in capture order.
``DiagnosticHelper`` preserves source context and nested causes; ``PlainTextRenderer`` produces diagnostics suitable for
both a terminal and redirected build logs:

.. erbsland-demo::
    :source: event/apps/ConfigCheck/src/ConfigCheckApp.cpp
    :show: f:ConfigCheckApp::reportFailures
    :files: ../../../demos/event/apps/ConfigCheck/example-files/server.elcl ../../../demos/event/apps/ConfigCheck/example-files/worker.elcl
    :exec: event/config_check server.elcl {file:none} worker.elcl
    :exec-exit-code: 1
    :show-cmd-line:
    :sha256: 02c9c1237af5a5d410b0cd84f975a383452557adcb9ccc8935be161cdf868709

.. code-block:: cpp

    auto ConfigCheckApp::reportFailures(el::EventLoop &loop) -> el::ItemCount {
        auto failures = el::ItemCount{};
        while (const auto error = loop.takeError()) {
            const auto document = el::DiagnosticHelper::documentFromError(error);
            el::stream::stdErr()->writeLine(el::PlainTextRenderer{document}.build());
            ++failures;
        }
        el::stream::stdErr()->flush();
        return failures;
    }

.. erbsland-demo-run::

    .. code-block:: console

        $ event/config_check server.elcl missing-file.txt worker.elcl

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mError: Reading the Configuration Failed
          The source path is no regular file.
        Error Source:
          Path: missing-file.txt
        Configuration Error Details:
          category: IO
        OK  server.elcl
        OK  worker.elcl
        Checked 3 of 3 files: 2 passed, 1 failed, 0 skipped.␛[0m

.. erbsland-demo-end::

This run includes an unavailable file between two valid configurations.
The later valid file is still checked, the missing file remains visible in the diagnostics, and the command fails.
Main uses the returned failure count in its summary and exit-code decision.
A successful join or a nonempty list of successful files alone would not establish that the batch passed.

This reporting strategy keeps at most one stored parser error per requested top-level file until the finite batch ends.
A continuously running service would need to consume errors during its lifetime instead of accumulating them until
shutdown.

Stopping at a Useful Error Limit
================================

An error limit lets you stop a large check, fix the first few failed configurations, and rerun it.
``Store`` alone would check every file, while ``Quit`` would still drain already queued checks.
The batch therefore installs a custom handler when the requested limit is positive:

.. erbsland-demo::
    :source: event/apps/ConfigCheck/src/ConfigurationBatch.cpp
    :show: f:ConfigurationBatch::installErrorLimit
    :files: ../../../demos/event/apps/ConfigCheck/example-files/server.elcl ../../../demos/event/apps/ConfigCheck/example-files/worker.elcl
    :exec: event/config_check --max-errors 1 server.elcl {file:none} worker.elcl
    :exec-exit-code: 1
    :show-cmd-line:
    :sha256: 3eb65b5b8b7b567f8ddf56b70e153cdc2c754fc34f7a94605c99b92dce568554

.. code-block:: cpp

    void ConfigurationBatch::installErrorLimit(el::EventLoop &loop) {
        if (_maximumErrors.isZero()) {
            return; // Keep the configured Store policy when every file should be checked.
        }
        loop.setErrorHandler([this](std::exception_ptr) noexcept -> el::EventLoopErrorAction {
            ++_failureCount;
            if (_failureCount >= _maximumErrors) {
                return el::EventLoopErrorAction::Stop;
            }
            return el::EventLoopErrorAction::Continue;
        });
    }

.. erbsland-demo-run::

    .. code-block:: console

        $ event/config_check --max-errors 1 server.elcl missing-file.txt worker.elcl

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mError: Reading the Configuration Failed
          The source path is no regular file.
        Error Source:
          Path: missing-file.txt
        Configuration Error Details:
          category: IO
        OK  server.elcl
        Checked 2 of 3 files: 1 passed, 1 failed, 1 skipped.␛[0m

.. erbsland-demo-end::

Here the missing file reaches the limit, so the later worker configuration is reported as skipped.
Below a larger limit, the handler returns ``Continue`` and the next file is checked.
The counter belongs to the worker, just like the parsing callbacks.
The handler only counts and selects an action; it performs no I/O and cannot throw an operational exception.

At the limit, ``Stop`` returns from dispatch before the next queued check and finishes the event thread.
This also skips the final quit callback, but ``join()`` can still return because dispatch has stopped.
The application never restarts the one-shot worker, so its unexecuted callbacks are discarded when it is released.

The loop stores each error before invoking the handler.
Consequently ``reportFailures()`` reports the same diagnostics in both modes, including the failure that reached the
limit.
The handler controls how much work runs; manual consumption supplies the report and exit status.
Programming errors and exceptions outside the ``RuntimeError`` hierarchy retain the event system's fatal behavior.
