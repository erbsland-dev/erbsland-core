..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Events; Handling Event Errors and Callback Exceptions
    single: Events; Error Policies
    single: Events; Final Notifications

*********************************************
Handling Event Errors and Callback Exceptions
*********************************************

A callback can fail just like an ordinary function: a file may disappear, a connection may break, or an operation may
receive invalid input.
You do not need to surround every callback with exception handling.
The event loop provides a boundary for operational failures, and ``Application`` can turn them into an orderly shutdown
and a useful diagnostic.
The important decision is where recovery belongs: inside one operation, at the loop that owns it, or at the application
boundary.

This page explains those choices, including what happens when recovery is inappropriate.
It also shows how completion notifications, stored errors, and worker shutdown fit together, so a failed callback does
not leave another component waiting forever.

When an Exception Leaves a Callback
===================================

An invocation runs later, when its target loop dispatches it.
A ``try`` block around ``invoke()`` can catch a failure to submit work; it cannot catch an exception thrown by that work
later.
The same separation applies when you register a timer or an event-source callback.
The callback runs on the dispatching thread's stack, not on the stack that registered it.

When a ``RuntimeError`` or one of its subclasses leaves an invocation or a backend's event-dispatch callback, the loop
applies its error policy or custom error handler.
This includes exceptions such as ``PathError``, ``NetworkError``, and ``ApplicationError``.
These types describe operational failures for which an application can choose a response.
The failed callback has already unwound: statements after the throw did not execute, local destructors ran, and earlier
side effects were not rolled back.
Continuing dispatch means that other callbacks can run; it does not complete or retry the failed operation.

``LogicFailure`` belongs to a different category.
It describes a programming error or a broken contract, rather than an unsuccessful operation.
It and every other exception outside the ``RuntimeError`` hierarchy bypass the loop's policies and handlers.
In particular, ``std::runtime_error`` is not an Erbsland Core ``RuntimeError``.
Allocation failures and thread-resource errors are not converted into recoverable operation errors either.

For a loop driven directly with ``run()``, ``runOnce()``, or ``runUntilIdle()``, these exceptions propagate to the
caller.
On an event thread, an exception escaping the thread entry point terminates the process.
The section on fatal failures below explains the application boundary and why catching everything would undermine it.

Local Recovery Without Catching Every Callback
==============================================

Leave an operational exception to the loop when the surrounding policy expresses the right response.
For example, an application that cannot proceed without its input file can let the file operation throw.
The normal application boundary already knows how to report that error and fail the run.

A local catch is useful when the callback can make a more specific decision.
You might reject one request while keeping a server running, ask the user to correct an incomplete pattern, or translate
an error into a failed result for another component.
Catch the type that describes that expected outcome, then complete the operation or start a bounded recovery attempt.
Logging an error and returning is insufficient when another callback is waiting for a result that will never arrive.

.. erbsland-in-practice-begin:: Keeping an Editable Pattern Local

In :doc:`/demo_apps/event/file_search`, an unfinished glob is ordinary user input.
Compilation catches ``ParseError`` and displays its reason while leaving the input editor active.
A valid pattern proceeds to worker submission:

.. erbsland-demo::
    :source: event/apps/FileSearch/src/FileSearchApp.cpp
    :show: c:Compile the current input-c:An unfinished glob(+0,+3)
    :sha256: 28fbb169243ce3adaad846598c4fd49fd99d1272f7cf01f8abaabf991b8d92a5

.. code-block:: cpp

    // Compile the current input before starting another search.
    auto pattern = el::PathGlobPatternPtr{};
    try {
        pattern = el::PathGlobPattern::compile(_query.text());
    } catch (const el::ParseError &error) {
        // An unfinished glob is ordinary editable input, not an application failure.
        _results.setStatus(el::join("Invalid glob: "_el, error.reason()));
        return;
    }

.. erbsland-demo-end::

The catch has a concrete recovery: the user can continue editing.
It does not catch a programming error, and it does not change the application's policy for unrelated callback failures.

.. erbsland-in-practice-end::

Operation Errors and Final Notifications
========================================

An event source's ``onError`` reports failure of the asynchronous operation itself.
It is not a handler for arbitrary exceptions from your callbacks.
For example, a subprocess may report an abnormal outcome through ``onError``; throwing from that notification then
creates a callback failure handled at the loop boundary.

Success and failure often need different responses, but both must finish the operation's bookkeeping.
Many sources therefore provide ``onFinal`` as a common completion notification.
It is a useful place to release retained operation state, remove an item from an active-operation list, or signal that a
worker can finish.
Cancellation can also lead to final notification when the source's contract includes it.
Keeping this shared completion step separate avoids repeating it in every successful and failed outcome handler.

``onFinal`` is a notification with a source-specific contract, not a language-level ``finally`` block.
Read that contract before relying on it after a throwing callback or an explicit close.
Subprocess completion invokes its final handler after an ``onExit`` or ``onError`` handler throws ``RuntimeError``,
provided observation has not been closed.
Host lookup instead finishes its internal operation and skips ``onFinal`` when its result or error handler throws.
Neither promises recovery from a fatal exception.
Use destructors and ownership for indispensable cleanup; use final notifications for the asynchronous lifecycle they
actually describe.

.. erbsland-in-practice-begin:: Completing a Subprocess Worker

:doc:`/demo_apps/event/git_change_summary` installs separate outcome and completion handlers on its worker loop:

.. erbsland-demo::
    :source: event/apps/GitChangeSummary/src/GitChangeSummaryApp.cpp
    :show: f:GitChangeSummaryApp::onStart, f:GitChangeSummaryApp::onError, f:GitChangeSummaryApp::onFinal
    :sha256: b71cca8d4324b3f4617cc3ad871974e50d61e9dc5e3b89d5d764800fb184262a

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

    void GitChangeSummaryApp::onError(const el::PlatformErrorContext &error) {
        throw el::ApplicationError{"Git subprocess failed."_el, error.toString()};
    }

    void GitChangeSummaryApp::onFinal() {
        _worker->quit();
    }

.. erbsland-demo-end::

The outcome callbacks interpret the subprocess result.
``onFinal()`` requests worker termination whether completion was successful or failed.
The worker also uses ``FailApplication``, so a thrown operational error reaches application diagnostics rather than
becoming an unexamined worker error.
Its procedural main can then finish waiting in ``join()``.

.. erbsland-in-practice-end::

See :doc:`/topics/network/resolving-hosts-asynchronously` for lookup completion and
:doc:`shutting_down_event_work` for closing resources before their owner loop stops.

The Application's Default Failure Boundary
==========================================

The default ``Application::main()`` enters the managed main loop through ``runMainEventLoop()``.
This installs a handler that retains the first operational callback failure and stops normal main-loop dispatch.
During shutdown, the application requests termination of its managed event threads and joins them.
The retained failure is rethrown on the application thread and reaches the same ``Application::run()`` boundary as an
exception from a lifecycle method.

For ``RuntimeError``, that boundary performs cleanup, renders a diagnostic including causes, and returns a failure exit
code.
An ``ApplicationError`` supplies its own exit code; other runtime errors use the standard failure code.
Your ``cleanup()`` override runs outside normal event dispatch and must not depend on a new callback arriving there.

This default gives a small application a complete failure path without catch blocks in every callback.
It preserves the diagnostic and prevents an unsuccessful operation from looking like a successful run.
It also provides one place for final cleanup after managed workers finish.
Application code still owns the shutdown protocol for asynchronous resources; stopping a loop cannot interrupt a
callback or finish an operation that requires further notifications.

Do not confuse the main-loop handler with worker defaults.
``createEventThread()`` without options uses ``Store``.
A worker error then remains on that worker and does not automatically fail the application.
Select ``FailApplication`` when the rest of the application depends on that worker's success.

Choosing a Policy for a Loop or Thread
======================================

``EventLoopOptions::setErrorPolicy()`` selects the response to an uncaught ``RuntimeError`` when no custom handler is
installed.
Pass the options to ``EventLoop::create()``, ``UnmanagedEventThread::create()``, or
``Application::createEventThread()``.
The factory copies the options; changing that options object later does not reconfigure an existing loop.

.. list-table::
    :header-rows: 1
    :widths: 16 44 40

    * - Policy
      - What happens
      - When it fits
    * - ``Ignore``
      - Discard the exception and continue dispatching. No stored diagnostic remains.
      - Rare cases where discarding this failure is intentional and completion does not depend on the failed callback.
    * - ``Store``
      - Append the exception to the loop's error queue and continue dispatching.
      - Independent work whose host regularly consumes and reports failures.
    * - ``Quit``
      - Store the exception, close admission, and drain events already accepted by the loop.
      - End the affected loop when its remaining accepted work can finish without submitting new callbacks there.
    * - ``FailApplication``
      - Store and forward the exception to the owning application, which stops normal main dispatch and requests
        graceful termination of all managed event threads.
      - Required worker work whose failure prevents the application from completing.

Choose the policy according to whether the remaining work can still produce a useful result after a callback fails.
The application must also have a way to learn that the operation failed; continued dispatch alone does not provide one.

``Store`` is the default for standalone loops and both kinds of event thread.
It fits independent operations when an identified owner consumes their errors and decides the overall outcome.
For a finite batch, that owner can report the failures after dispatch finishes, as shown in the next section.
A long-lived loop needs regular consumption because its error queue has no capacity limit.
Without that consumer, failures remain unreported and other components may wait for results that will never arrive.

``Quit`` fits a loop that should finish accepted work and reject further submissions after a failure.
Already queued callbacks still execute, so they must remain safe and useful even when the failed callback did not
finish.
Do not choose it to prevent dependent queued work from running.
Those remaining callbacks also cannot rely on submitting new work to the quitting loop.
Close asynchronous resources while their loops can still deliver the required completion notifications, then quit.

``FailApplication`` fits required worker work whose failure prevents the application from fulfilling its purpose.
It requires an application-managed worker; standalone loop and unmanaged-thread factories reject it with
``ParameterFailure`` because they have no owning application.
The first forwarded application failure is retained; subsequent failures do not replace it.
Shutdown also reaches managed receivers waiting for worker results.

``Ignore`` needs a specific justification for discarding both the exception and its diagnostic.
It is rarely appropriate: the application must be able to proceed without the failed operation, and no component may
rely on a completion signal that the callback failed to send.
A narrow local catch usually expresses an expected, harmless failure more clearly while preserving reporting for other
operational failures.

.. erbsland-in-practice-begin:: Choosing a Policy for FileSearch

:doc:`/demo_apps/event/file_search` depends on its worker to traverse the filesystem and return search results.
It retains that managed worker in its ``_worker`` member through application shutdown.
During startup, it selects ``FailApplication`` before starting the worker:

.. erbsland-demo::
    :source: event/apps/FileSearch/src/FileSearchApp.cpp
    :show: a:_worker(+0,+1)
    :sha256: 99ac61b85dd3a53cdb1a49f68c1bb9b57bb048e739140ecc3f86498cb1d0300b

.. code-block:: cpp

    _worker = createEventThread(el::EventLoopOptions{}.setErrorPolicy(el::EventLoopErrorPolicy::FailApplication));
    _worker->start();

.. erbsland-demo-end::

A search callback that exits through an unexpected ``RuntimeError`` may never send its completion notification.
With ``Store`` alone, the error would remain on the worker while the screen could keep showing "Searching…".
``FailApplication`` instead initiates shutdown and delivers the failure to the application's standard diagnostic
handling.

Expected search problems have more specific outcomes.
The main-loop callback catches invalid glob syntax and lets the user continue editing.
The scanner continues past filesystem traversal errors and marks its results as incomplete.
Neither needs to escape as a callback exception: the policy handles unexpected operational failures that the application
cannot handle within an individual search.

.. erbsland-in-practice-end::

Queue overflow is a separate admission decision; it does not invoke the callback-error handler.
See
:doc:`controlling_event_load` and :ref:`event-loop-options` for that policy family.

Handling Stored Errors Yourself
===============================

Manual consumption is useful when you drive a loop inside another framework, dispatch a finite independent batch, or
join a worker whose failures you want to report individually.
``hasError()`` tells you whether errors are currently stored.
``takeError()`` removes the oldest exception and returns an empty pointer when the queue is empty.
It does not rethrow the exception automatically.

Rethrow each pointer at a boundary that understands the failure, or turn it into a diagnostic with
``DiagnosticHelper::documentFromError()``.
Decide whether the overall operation succeeded separately from whether dispatch returned.
A failed callback can have left a missing result even when later callbacks ran normally.

.. erbsland-in-practice-begin:: Reporting Configuration Errors Together

:doc:`/demo_apps/event/config_check` checks a list of ELCL configuration files before they are deployed.
An invalid file should fail the overall check, but the user still needs to see problems in the other files.
Its default mode therefore selects ``Store`` on a managed worker and queues one parsing callback per file.
Each callback performs a real check without a surrounding catch:

.. erbsland-demo::
    :source: event/apps/ConfigCheck/src/ConfigurationBatch.cpp
    :show: f:ConfigurationBatch::checkFile
    :sha256: e8236074a8773cf171162e31ff2a68a8f739b9e6c1d00d55539d05b0163bc576

.. code-block:: cpp

    void ConfigurationBatch::checkFile(const el::Path &path) {
        ++_checkedCount;
        // Each file is independent. Parser errors retain the source location and any nested cause.
        (void)el::conf::Parser{}.parseFileOrThrow(path);
        _passedPaths.append(path);
    }

.. erbsland-demo-end::

A parser failure leaves that callback and enters the worker's error queue.
Later files are still checked, and a final queued callback calls ``quit()`` to finish the worker.
The application's procedural main joins the worker, then calls this reporting method on the application thread:

.. erbsland-demo::
    :source: event/apps/ConfigCheck/src/ConfigCheckApp.cpp
    :show: f:ConfigCheckApp::reportFailures
    :sha256: 5b81a1adb37f79f8c49a8c5baa9b2922452d87237288e9cd1feb0b24eec8d109

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

.. erbsland-demo-end::

The report preserves each error's source location and cause chain, in capture order.
The returned count contributes to the batch summary and makes the application return a failure exit code when any file
failed.
Successfully parsed files are still reported, so the user can distinguish checked files from files skipped by an error
limit.

This is a finite batch with an explicit completion callback; joining establishes that no more errors can arrive from its
worker.
Reporting after the join is therefore sufficient here.
A long-lived validator would instead need to consume errors regularly while running.

.. erbsland-in-practice-end::

Writing a Custom Error Handler
==============================

A custom ``EventLoopErrorHandler`` is useful when a fixed policy cannot express your response: for example, when a
controller needs to notify a supervisor, record a diagnostic immediately, or decide whether independent work can
continue.
Install it with ``setErrorHandler()`` on a loop, or on a thread's ``workerEventLoop()`` before starting it.

The loop stores the original exception first and then calls the handler on the dispatching thread.
The supplied ``std::exception_ptr`` identifies the current failure.
Return ``Continue`` to keep dispatching, or ``Stop`` to end the current dispatch run without draining the remaining
queue.
``Stop`` does not close admission as ``Quit`` does, and a later explicit run can resume dispatch.
It does not undo side effects or stop another loop waiting for a reply.

The handler replaces the configured policy, even ``Ignore`` or ``FailApplication``.
Installing one therefore transfers responsibility for any required application-wide response to your handler.
Stored errors still need consumption; handling a failure and leaving its pointer in the queue are separate actions.
Clear the handler with ``setErrorHandler({})`` to restore the configured policy.
Clearing it neither restarts dispatch nor removes errors.

.. erbsland-in-practice-begin:: Limiting Failed Configuration Files

:doc:`/demo_apps/event/config_check` also accepts ``--max-errors``.
For a large collection, the user may want to stop after a few failed files, correct those problems, and run the check
again.
A fixed policy cannot express "continue after the first failure, but stop after the second".
When a positive limit is requested, the batch installs this handler before the worker starts:

.. erbsland-demo::
    :source: event/apps/ConfigCheck/src/ConfigurationBatch.cpp
    :show: f:ConfigurationBatch::installErrorLimit
    :sha256: 80909d3c37dead686ecc9a8e166897be5b1aa2b8deec355c431961e0976668c0

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

.. erbsland-demo-end::

The handler counts failed files on the same worker thread that parses them.
It returns ``Continue`` below the limit and ``Stop`` when the limit is reached.
Stopping leaves later queued checks unexecuted, which is precisely what the user requested; ``Quit`` would drain those
checks and defeat the limit.

The batch object remains alive until the worker is joined, so the handler's borrowed ``this`` stays valid.
At the limit, ``Stop`` returns from dispatch and finishes the event thread without needing the final queued quit
callback to run.
The application does not restart that one-shot worker; its remaining callbacks are discarded when the worker is
released.

This handler decides how much work to perform; the reporting method from the preceding section decides how to present
the outcome.
The loop has already stored each exception, and the handler leaves those errors available for reporting after the join.
There is no second diagnostic or duplicate catch in the validation callback.
The final summary identifies unchecked files as skipped, and the failed run returns a failure exit code.
With the default limit of zero, no custom handler is installed and ``Store`` checks every file.

.. erbsland-in-practice-end::

Keep the handler small and reliable.
If it throws ``RuntimeError``, that additional exception is stored and dispatch stops; manual reporting may therefore
encounter both errors.
A ``LogicFailure`` or other exception from the handler terminates the process, even for a directly driven loop: this
error-handling boundary cannot propagate exceptions.

Customizing the Application's Main-Loop Response
================================================

For ordinary recovery, a specific local catch or a worker policy is usually sufficient.
Take control of the main loop when your application needs a different reporting destination or a different decision
about continuing dispatch.
A derived application can override ``main()``, access the protected ``mainEventLoop()``, install its own handler, and
call that loop's ``run()`` directly.

The distinction from ``runMainEventLoop()`` matters: that convenience method installs the application's default handler
each time it enters dispatch.
A handler installed beforehand will be replaced.
Direct dispatch leaves your handler in place, while the surrounding ``Application::run()`` lifecycle still performs
managed-worker shutdown and cleanup.
Your override must explicitly return the appropriate ``ExitCode``; it must not assume that a stopped loop succeeded or
that a code previously passed to ``quit()`` automatically becomes its return value.

.. erbsland-in-practice-begin:: Owning Reporting and Exit Status

This compile-only starting point stops dispatch on a runtime failure and consumes all stored errors.
An application supplies ``startWork()`` and ``reportFailure()``: the former connects real work and completion, and the
latter delivers a complete diagnostic document to the chosen destination.

.. erbsland-demo::
    :source: event/templates/EventErrorHandling/CustomErrorApplication.hpp
    :sha256: 6dff9f8ff51ebaf8f910fd55979a4e9f8a611de9f6fe59ff0355e0ec5ed3f51a

.. code-block:: cpp

    /// An application starting point with explicit main-loop failure reporting.
    class CustomErrorApplication : public el::Application {
    public:
        using Application::Application;

    protected: // customization
        /// Connect application operations and their completion callbacks before entering dispatch.
        /// Completion must call quit(); the unmodified template intentionally has no business work.
        virtual void startWork() = 0;
        /// Send a diagnostic, including its cause chain, to the application's reporting destination.
        /// @param document The diagnostic for one captured operational failure.
        virtual void reportFailure(const el::TextDocument &document) = 0;

    protected: // implement Application
        auto main() -> el::ExitCode override {
            mainEventLoop().setErrorHandler(
                [](std::exception_ptr) -> el::EventLoopErrorAction { return el::EventLoopErrorAction::Stop; });
            loop()->invoke([this]() -> void { startWork(); });
            // Direct dispatch preserves our handler; runMainEventLoop() installs the default handler.
            mainEventLoop().run();
            auto result = el::ExitCode::success();
            while (const auto error = mainEventLoop().takeError()) {
                reportFailure(el::DiagnosticHelper::documentFromError(error));
                result = el::ExitCode::failure();
            }
            mainEventLoop().setErrorHandler({});
            return result;
        }
    };

.. erbsland-demo-end::

The template returns failure after reporting any stored error.
It deliberately contains no business operation to run.
An application that chooses to continue after independent failures can return ``Continue``, but must also arrange
regular error consumption and a real completion path.
Managed worker failures using ``FailApplication`` still reach the outer application boundary; replacing the main-loop
handler does not replace that separate forwarding mechanism.

.. erbsland-in-practice-end::

Joining a Worker After Failure
==============================

``join()`` waits for a thread to finish.
It neither requests termination nor rethrows stored callback errors.
With ``Store`` or ``Ignore``, a worker may keep running after a failure, so joining it without a shutdown path can wait
forever.
With ``Quit``, accepted work drains and the thread can finish; inspect its stored errors after joining.
No policy can interrupt a callback that never returns.
Long-running work can poll its thread's ``stopToken()`` to observe a quit request while it executes, including a request
from application failure handling.
The callback must still reach a cancellation check and return; the token cannot interrupt a blocked filesystem call.

A related problem occurs when a separate receiver expects one result per worker.
A worker that fails before sending its result leaves that receiver short of its expected count.
Storing an exception does not create the missing reply.
Either make failure an explicit result in that protocol or select ``FailApplication`` for required managed work.

.. erbsland-in-practice-begin:: Releasing a Waiting Result Receiver

:doc:`/demo_apps/event/file_hasher` has a procedural main that joins its result receiver. It selects
``FailApplication`` for both the receiver and hashing workers:

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

If hashing fails before sending a result, application failure handling requests termination of the receiver too.
Main's join can finish without receiving every successful hash.
The retained error is rethrown on the application thread during shutdown and receives the usual diagnostic handling.
It is not thrown by ``join()`` itself.

.. erbsland-in-practice-end::

A source that never replies without throwing still needs a deadline or cancellation path.
Also, joining the current thread is a programming error and raises ``LogicFailure``; moving that call inside a catch-all
does not make it valid.

Fatal Failures and Catch-All Handlers
=====================================

When ``LogicFailure`` escapes the application's main dispatch, ``Application::run()`` logs it if logging is configured,
performs cleanup, and rethrows it.
Other exceptions outside ``RuntimeError`` also escape after cleanup.
In a normal program without another outer handler, the uncaught exception terminates the process.
On a worker, an exception escaping its thread entry point terminates there; there is no later opportunity to catch it
around ``join()``.

This distinction preserves evidence of broken assumptions and resource exhaustion.
The program may already be in an invalid state, and continuing would hide the cause or produce incorrect results.
Treat foreign operational exceptions at the specific integration boundary where you understand them, rather than
translating every possible exception into an ordinary runtime error.

An empty ``try { ... } catch (...) {}`` defeats both operational reporting and fatal-failure handling.
It can hide a missing result, leave partially updated state, and make a failed run appear successful.
Even a catch-all that only logs and continues has those problems.
Prefer a narrow catch with a concrete recovery, or let the existing boundary act.

A catch-all used for exception-safe cleanup followed immediately by ``throw;`` has a different purpose: it preserves the
failure while releasing resources.
RAII usually makes even that unnecessary.
Likewise, a boundary that deliberately terminates on an otherwise unpropagatable failure is not silently recovering from
it.

For custom event backends, these guarantees apply to callback invocation and ``handleEvent()`` dispatch, not every
polling or native-driver operation.
Exceptions outside those boundaries remain the host's responsibility; see
:doc:`planned/writing_event_backends` and :doc:`planned/running_event_loops_directly` when integrating a loop into another host.
