..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Events; Scheduling Work
    single: Timers; Repetition and Cancellation
    single: Callbacks; Deferred Execution
    single: Debouncing
    single: Timeouts; Inactivity

**************************
Scheduling Work and Timers
**************************

Scheduling lets you decide where work runs and when it becomes ready to run.
You can send a result to another loop, defer a step on your own loop, wait without blocking a thread, or repeat an
action.
This page explains how to pass work through ``invoke``, choose a timer schedule, and keep the required state alive until
execution.

Read :doc:`using_event_apis` for event targets and :doc:`callback_lifetimes` for a broader discussion of ownership.
The scheduling methods described here are available through an application's or event thread's target.

Queuing Work with invoke()
==========================

:cpp:func:`EventTarget::invoke() <erbsland::event::EventTarget::invoke>` submits a callback to a selected event loop.
You can call it from any thread with a valid target: the application's main thread, another event thread, or an ordinary
worker thread that has no event loop of its own.
The callback executes on the thread dispatching the **target loop**, regardless of where you created the lambda or
called ``invoke()``.
For example, ``app.loop()`` selects the application's main loop, while ``worker->loop()`` selects that worker's loop.
The loop must enter dispatch for queued callbacks to execute.

Invocation never calls the callback inline.
When you invoke your current loop from a callback, the new callback waits until the current one returns to dispatch.
This gives you a way to complete a state change before beginning the next step.
When you invoke a loop running on another thread, that thread may begin executing the callback **before your call to
invoke returns**.
Deferred execution therefore does not mean that the caller always finishes first.

.. mermaid::

    sequenceDiagram
        participant W as Calling worker thread
        participant Q as Target loop queue
        participant M as Target loop thread
        W->>Q: invoke(callback with captured input)
        Note over W,M: The threads can now proceed independently
        par Caller continues
            Q-->>W: Submission returns
        and Target dispatches when ready
            Q->>M: Execute callback
        end

``invoke()`` does not create a background thread, and callbacks on one loop execute serially.
A long computation queued on the main loop still blocks its timers, input, and other callbacks while it runs.
Keep callbacks short or move substantial work to a worker as explained in :doc:`processing_work_on_event_threads`.
Other ready work may run between two of your callbacks; an invocation does not promise which unrelated callback runs
next.

Passing Arguments to Invoked Code
=================================

The callback passed to ``invoke()`` takes no arguments.
A lambda supplies those arguments by storing them in its capture list and passing them to the actual operation when the
loop dispatches it.
Capture an independent value when the receiving code needs a snapshot.
An explicit move capture transfers an owning value into the callback, allowing a local result to survive after its
producing function returns.

Keep the lambda focused on this transfer and the call to a named function or member method.
Business logic belongs in that function, where it can be understood and tested independently of the scheduling boundary.
A short operation can fit directly in a lambda, but queuing work does not require putting an entire workflow there.

.. erbsland-in-practice-begin:: Delivering Checksums from a Worker

The :doc:`explicit-invocation checksum packer </demo_apps/event/checksum_packer_invoke>` calculates file digests on an
ordinary worker thread.
Its workflow receives the completed file list on the main loop:

.. erbsland-demo::
    :source: event/apps/ChecksumPackerInvoke/src/Workflow.hpp
    :show: f:start(-3,-1)
    :sha256: f245d8dc9aad3abb4841da5b90cdd3e8c5c2fdd64442a4c98ed69952a162c50e

.. code-block:: cpp

    auto events = el::currentLoop();
    auto workflow = shared_from_this();
    _scanner.scan([events, workflow](FileWithDigestList fileList, std::exception_ptr error) -> void {
        events->invoke([workflow, fileList = std::move(fileList), error = std::move(error)]() mutable -> void {
            workflow->onScanned(std::move(fileList), std::move(error));
        });
    });

.. erbsland-demo-end::

``Workflow::start()`` executes on main and captures ``currentLoop()`` before starting the scan.
The scanner's completion lambda runs on the scanner thread; it uses that saved target to queue the inner lambda on main.
Calling ``currentLoop()`` in the scanner's ordinary thread would not recover the main loop.

The inner lambda captures ``workflow`` by value and moves ``fileList`` and ``error`` into its own storage.
Its ``mutable`` qualifier permits moving those captured values onward to ``onScanned()``; otherwise the lambda's call
operator would expose them as const values.
``onScanned()`` owns the next workflow transition, while the lambda only transfers the result to the correct thread.

.. erbsland-in-practice-end::

A reference capture does not extend a local variable's lifetime.
Returning from the submitting function can leave a queued callback with a dangling reference.
Pointers and views captured by value still borrow the data they point to, so copying them does not solve that problem.
Shared mutable data also needs synchronization; thread-safe submission does not make arbitrary captured objects safe to
access concurrently.

:cpp:type:`EventCallback <erbsland::event::EventCallback>` is a ``std::function<void()>``, so its stored callable must be
copyable.
Move capture is useful for copyable owning values too, but directly capturing a ``std::unique_ptr`` by move makes the
lambda unsuitable for this callback type.
Choose an owning value or a deliberate shared-ownership design when the result must outlive its producer.

Calling Members and Keeping Recipients Alive
============================================

A member call inside an invoked lambda runs on the selected loop just like a free-function call.
For a member of the submitting object, ``[this]() -> void { begin(); }`` is a compact forwarding callback.
The crucial question is who keeps that object alive until execution finishes.

.. warning::

    Capturing ``this`` copies a pointer; it does not retain the object.
    The object must outlive every queued or executing callback that dereferences it.
    Queuing through a thread-safe target does not make the object's members thread-safe.

.. erbsland-in-practice-begin:: Starting File Search on the Main Loop

:doc:`FileSearch </demo_apps/event/file_search>` validates its options and then queues startup:

.. erbsland-demo::
    :source: event/apps/FileSearch/src/FileSearchApp.cpp
    :show: f:FileSearchApp::handleCommandLineOptions
    :sha256: fcbe5432f69f40eacda98a1aca2afb5011531375ccd18b3b226ea0d62fcb719a

.. code-block:: cpp

    void FileSearchApp::handleCommandLineOptions(const el::OptionValuesPtr &values) {
        _root = el::Path::fromNativeOrThrow(values->getText(cDirectoryOption)).toAbsoluteOrThrow();
        if (!_root.info().isDirectory()) {
            throw el::OptionError{"Expected a search directory."_el, values->value(cDirectoryOption)};
        }
        if (!terminal()->isInteractive()) {
            throw el::ApplicationError{"File Search requires an interactive terminal."_el};
        }
        loop()->invoke([this]() -> void { begin(); });
    }

.. erbsland-demo-end::

The application object surrounds the entire ``run()`` call in its entry point:

.. erbsland-demo::
    :source: event/apps/FileSearch/src/main.cpp
    :show: f:main
    :sha256: 193611ce8dc1017a2a8fb597ce3c098b86a54752276097e371010f2af1e550ec

.. code-block:: cpp

    auto main(const int argc, char *argv[]) -> int {
        auto app = demo::FileSearchApp{argc, argv};
        return app.run();
    }

.. erbsland-demo-end::

That lifetime covers main-loop dispatch, so the startup callback can call ``begin()`` through ``this``.
``begin()`` contains the setup logic; the invocation merely schedules it in the required context.
This guarantee comes from the application's lifetime, not from the capture itself.

.. erbsland-in-practice-end::

For a separately owned recipient, capturing a ``shared_ptr`` retains it while the callback needs it.
For optional work, capture a ``weak_ptr`` and lock it inside the invoked callback; if the recipient has disappeared,
skip the work.
Keeping the recipient alive does not by itself guarantee that the loop will deliver the callback, and strong captures
can create ownership cycles when the recipient owns the callback's storage.
See :doc:`callback_lifetimes` for choosing and releasing these ownership relationships.

.. erbsland-in-practice-begin:: Retaining the Checksum Workflow

The checksum example above obtains ``workflow`` with ``shared_from_this()`` and captures it in both callbacks.
The queued member call uses ``workflow->onScanned(...)`` rather than a borrowed ``this`` pointer.
It therefore retains the recipient together with the result until delivery or until the callback is discarded.
The application also owns the workflow and joins its workers during shutdown; the walkthrough explains those
dependencies.

.. erbsland-in-practice-end::

Delayed Work and One-Shot Timers
================================

A delay lets the loop do other work while time passes.
:cpp:func:`invokeAfter() <erbsland::event::EventTarget::invokeAfter>` retains a callback and makes it eligible after the
specified delay, without returning a cancellation handle.
The loop still needs time to dispatch it, so the delay is not an exact execution time.
A zero or negative delay queues the callback immediately, like ``invoke()``; it never executes inline.

This is suitable when the callback can remain scheduled for its full lifetime.
A weak recipient can make an obsolete callback harmless if its object disappears.
If the object survives but its request changes, check a request identifier or other operation state as described in
:doc:`communicating_between_loops`.

.. erbsland-in-practice-begin:: Limiting a Monitoring Session

:doc:`FileTimeMonitor </demo_apps/event/file_time_monitor>` schedules application shutdown after a configured duration.
It does not need to change that deadline during the session:

.. erbsland-demo::
    :source: event/apps/FileTimeMonitor/FileTimeMonitor.hpp
    :show: a:timeout(+0,+6)
    :sha256: 31325bc7cda3da391967f4021b57a0b1a73875354c3e25f30071adc6d4a474d8

.. code-block:: cpp

    const auto timeout = el::Seconds{optionValues()->getInteger("timeout"_el)};
    if (timeout.isNegative()) {
        throw el::ApplicationError{"The timeout must be non-negative."_el};
    }
    if (timeout.isPositive()) {
        loop()->invokeAfter(timeout, []() -> void { el::application().quit(); });
    }

.. erbsland-demo-end::

The application subsequently enters its main loop, where the delayed callback can call ``quit()``.
No thread sleeps while the timeout is pending.

.. erbsland-in-practice-end::

When you need to replace or cancel a deadline, retain an
:cpp:class:`EventTimer <erbsland::event::EventTimer>` instead.
:cpp:func:`createTimer() <erbsland::event::EventTarget::createTimer>` stores the callback and returns an **inactive** timer;
creation alone schedules nothing.
Keep its pointer in the component or scope that owns the activity.

:cpp:func:`startOnce() <erbsland::event::EventTimer::startOnce>` schedules one occurrence.
Starting it again replaces the previous schedule, and zero or negative delays schedule immediate dispatch through the
loop.
After a one-shot callback completes, the timer becomes inactive unless it has been restarted.
The handle and callback remain available for reuse.

.. erbsland-in-practice-begin:: Owning a Cancellable Overall Deadline

:doc:`FileActivityMonitor </demo_apps/event/file_activity_monitor>` reports file activity until the file has been quiet
long enough or an optional overall time limit is reached.
It retains the overall timer as a member:

.. erbsland-demo::
    :source: event/apps/FileActivityMonitor/src/FileActivityMonitorApp.hpp
    :show: c:Optional overall limit, c:Starts once and is never extended
    :sha256: a4f51e426ae97a821a75c016535617a9ed0d3837bb5f192cd616f65c33f4769c

.. code-block:: cpp

    el::TimeDelta _timeout{el::Seconds{0}};         ///< Optional overall limit; zero disables it.

    el::EventTimerPtr _deadline;                    ///< Starts once and is never extended by activity.

.. erbsland-demo-end::

Startup creates and starts that timer only when an overall limit was requested:

.. erbsland-demo::
    :source: event/apps/FileActivityMonitor/src/FileActivityMonitorApp.cpp
    :show: f:FileActivityMonitorApp::begin(-7,-3)
    :sha256: bb3739cdcda2b8677238045932c4f1115a7ae74f0dd3deb26405b1cd1ca697bd

.. code-block:: cpp

    if (_timeout.isPositive()) {
        _deadline = loop()->createTimer([this]() -> void { finish("Overall deadline reached."_el); });
        _deadline->startOnce(_timeout);
    }

.. erbsland-demo-end::

The short callback forwards to ``finish()``, which stops the owned timers and quits.
An earlier inactivity timeout uses that same completion method, cancelling the now-unnecessary overall deadline.
The application remains alive throughout dispatch and cleanup, so these member callbacks can capture ``this``.

.. erbsland-in-practice-end::

Repeating Timers and Their Cadence
==================================

Repeating work needs a rule for how the time spent in its callback affects the next occurrence.
:cpp:func:`startFixedDelay() <erbsland::event::EventTimer::startFixedDelay>` waits a full interval **after the callback
finishes** before making the next occurrence eligible.
It suits polling or maintenance where a pause between calls matters more than a regular cadence.

:cpp:func:`startFixedRate() <erbsland::event::EventTimer::startFixedRate>` follows the cadence established when the timer
starts.
The first occurrence is due after one interval, and subsequent deadlines follow that original schedule.
It suits regular observations or display updates where callback duration should not continually shift the schedule.
Missed ticks are skipped rather than accumulated into a burst of catch-up callbacks.

For an interval of 20 ms and a callback that takes 35 ms, an idealized timeline is:

.. code-block:: text

    fixed delay: starts 20 -> finishes 55 -> starts 75 -> finishes 110 -> starts 130
    fixed rate:  starts 20 -> finishes 55 -> starts 60 -> finishes 95  -> starts 100
    cadence:     20, 40, 60, 80, 100 ...; the ticks at 40 and 80 are skipped

The callback never overlaps itself on its loop.
Both repeating methods require a strictly positive interval; zero or negative values raise
:cpp:class:`ParameterFailure <erbsland::err::ParameterFailure>`.
OS scheduling and other callbacks can delay either mode, so neither provides a real-time guarantee.
The timeline explains scheduling; it is not a reason to sleep in a callback.

.. erbsland-in-practice-begin:: Polling with a Pause Between Reads

The file-time monitor retains a timer that reloads and displays the selected file's modification time.
It waits one second after each callback completes:

.. erbsland-demo::
    :source: event/apps/FileTimeMonitor/FileTimeMonitor.hpp
    :show: c:Retains the repeating timer(+0,+2), c:Enter the event loop after setup(+3,-2), f:poll
    :sha256: 4439310afa2b402b83c7f3c2d09a9b8c021788bb3c5c3db190f3b31370d25da8

.. code-block:: cpp

    el::EventTimerPtr _pollTimer; ///< Retains the repeating timer.
    el::Path _path;               ///< File being monitored.
    el::PathInfo _pathInfo;       ///< Reloadable filesystem information.

    _pollTimer = loop()->createTimer([this]() -> void { poll(); });
    _pollTimer->startFixedDelay(el::Seconds{1});

    void poll() {
        _pathInfo.reload(el::PathInfoPart::Times);
        el::stdOut()->printLine("Last modification: "_el, _pathInfo.lastModified().toString());
    }

.. erbsland-demo-end::

The retained member keeps the schedule alive while the application dispatches events.
The filesystem metadata read is the work of the callback; there is no artificial sleep to demonstrate elapsed time.

.. erbsland-in-practice-end::

.. erbsland-in-practice-begin:: Observing File Activity on a Regular Schedule

The activity monitor uses a fixed cadence for metadata snapshots:

.. erbsland-demo::
    :source: event/apps/FileActivityMonitor/src/FileActivityMonitorApp.hpp
    :show: c:Reloaded on each poll(+0,+2), c:Positive polling interval, c:Repeats on a fixed cadence(+0,+1)
    :sha256: d2648f29ff65681d0e001942940ac091c364289713a6f2aa672cabf7a1cba9c9

.. code-block:: cpp

    el::PathInfo _fileInfo;                         ///< Reloaded on each poll; never reads file contents.
    el::ByteCount _previousSize;                    ///< Size in the last observed snapshot.
    el::Timestamp _previousModification;            ///< Modification time in the last observed snapshot.

    el::TimeDelta _interval{el::Milliseconds{250}}; ///< Positive polling interval.

    el::EventTimerPtr _pollTimer;                   ///< Repeats on a fixed cadence.
    el::EventTimerPtr _idleTimer;                   ///< Restarts after each observed change.

.. erbsland-demo-end::

.. erbsland-demo::
    :source: event/apps/FileActivityMonitor/src/FileActivityMonitorApp.cpp
    :show: f:FileActivityMonitorApp::begin(-5,-7), f:FileActivityMonitorApp::poll
    :sha256: 685b22164619174996e835e4f6a7e0b4398b362ee32ee409935bd72b0d31947f

.. code-block:: cpp

    _pollTimer = loop()->createTimer([this]() -> void { poll(); });
    _pollTimer->startFixedRate(_interval);

    void FileActivityMonitorApp::poll() {
        _fileInfo.reload(el::PathInfoParts{el::PathInfoPart::Type, el::PathInfoPart::Size, el::PathInfoPart::Times});
        if (!_fileInfo.isRegularFile()) {
            throw el::ApplicationError{"The monitored file is unavailable or no longer a regular file."_el};
        }
        const auto size = _fileInfo.fileSize();
        const auto modification = _fileInfo.lastModified();
        const auto changed = size != _previousSize || modification != _previousModification;
        _previousSize = size;
        _previousModification = modification;
        // The initial snapshot has no activity timer yet.
        if (changed && _idleTimer) {
            onActivity();
        }
    }

.. erbsland-demo-end::

``poll()`` refreshes metadata and compares it with the previous size and modification time.
Only a detected change calls ``onActivity()``.
The initial call, before the inactivity timer exists, records the baseline without reporting a change.
Polling can miss changes between snapshots; a quiet period means no change was observed, not that a writer is finished.
This demo makes a small synchronous metadata query on the main loop.
For filesystems where that query can block for a long time, move the reads to a worker.

.. erbsland-in-practice-end::

A timer measures callback execution, not the lifetime of an asynchronous operation launched by that callback.
If the callback starts a request and returns immediately, fixed delay begins counting immediately after that return.
The request may still be running when the next occurrence arrives.
When the pause must begin after the operation finishes, start a one-shot timer from its completion handler instead.

Restarting Deadlines: Debounce and Inactivity
=============================================

Debouncing waits for a quiet period before acting on the latest input.
Keep one timer alongside that input and restart it after each relevant change.
Each ``startOnce()`` replaces the previous deadline, so a burst of changes produces one action when the final quiet
period expires.
The callback reads the latest state rather than preserving every intermediate value in a separate delayed invocation.

.. erbsland-in-practice-begin:: Waiting Until a Search Query Settles

File search polls real keyboard input and calls ``queryChanged()`` when an edit changes the query.
The application owns the latest input and the debounce timer:

.. erbsland-demo::
    :source: event/apps/FileSearch/src/FileSearchApp.hpp
    :show: c:Main-loop input state, c:Starts only the latest edited query
    :sha256: 4f07f9ca90311c76fd392658876a660788319cd8906978ede3b796e7e2009e76

.. code-block:: cpp

    QueryEditor _query;                                          ///< Main-loop input state.

    el::EventTimerPtr _debounce;                                 ///< Starts only the latest edited query.

.. erbsland-demo-end::

Startup connects the timer to ``submitSearch()``.
A nonempty edited query restarts it for 150 ms; clearing the query stops it without scheduling another search:

.. erbsland-demo::
    :source: event/apps/FileSearch/src/FileSearchApp.cpp
    :show: a:_debounce, f:FileSearchApp::queryChanged
    :sha256: 0d307e6dcccae90c25bab0dd70ad3e597e0d9e91f7907da7ea17611d33c82320

.. code-block:: cpp

    _debounce = loop()->createTimer([this]() -> void { submitSearch(); });

    void FileSearchApp::queryChanged() {
        _results.reset();
        _generation->store(_results.generation());
        _debounce->stop();
        if (_query.text().isEmpty()) {
            _results.setStatus("Type a glob: *.cpp at root, **/*.cpp recursively."_el);
            return;
        }
        _results.setStatus("Waiting for input…"_el);
        _debounce->startOnce(el::Milliseconds{150});
    }

.. erbsland-demo-end::

``submitSearch()`` compiles the current query and sends the filesystem scan to a worker.
The timer controls when that work starts; it does not perform the scan on the main loop.
The generation update also makes older search results obsolete, independently of replacing the pending timer.

.. erbsland-in-practice-end::

An inactivity timeout uses the same restart pattern with a different action: it ends a wait when no accepted activity
has occurred for a chosen interval.
Start it when the activity begins, then restart it after each relevant update.
An overall operation deadline instead starts once and remains unchanged across updates or retries.
Using both gives an operation a quiet-period limit and a separate total time budget.

.. erbsland-in-practice-begin:: Ending a Monitor After Observed Activity Stops

The activity monitor starts its quiet period after the initial file snapshot:

.. erbsland-demo::
    :source: event/apps/FileActivityMonitor/src/FileActivityMonitorApp.hpp
    :show: c:Quiet period measured, c:Restarts after each observed change
    :sha256: d774bfce9884c1d23e1f6a8dba5e5acfa12dadb251efe24877b9b218a2c954d2

.. code-block:: cpp

    el::TimeDelta _idleTimeout{el::Seconds{5}};     ///< Quiet period measured from the latest observation.

    el::EventTimerPtr _idleTimer;                   ///< Restarts after each observed change.

.. erbsland-demo-end::

.. erbsland-demo::
    :source: event/apps/FileActivityMonitor/src/FileActivityMonitorApp.cpp
    :show: f:FileActivityMonitorApp::begin(-1,-9), f:FileActivityMonitorApp::onActivity
    :sha256: 94c6cdccd1d9c7b7572cb4594573cf690042cb05893f1a01f0b2fdcbee6e8e0a

.. code-block:: cpp

    // Establish the initial snapshot before starting the quiet period.
    poll();
    _idleTimer = loop()->createTimer([this]() -> void { finish("Inactivity timeout reached."_el); });
    _idleTimer->startOnce(_idleTimeout);

    void FileActivityMonitorApp::onActivity() {
        // Only an observed change restarts the quiet period; the overall deadline stays unchanged.
        _idleTimer->startOnce(_idleTimeout);
        el::stdOut()->printLine("Changed: "_el, _previousSize, " bytes; modified "_el, _previousModification.toString());
        el::stdOut()->flush();
    }

.. erbsland-demo-end::

Unchanged polls leave the inactivity deadline alone.
A size or modification-time change extends the quiet period, while ``_deadline`` continues toward the original overall
limit.
The walkthrough includes bounded runs and explains how to observe changes made by another process.

.. erbsland-in-practice-end::

Cancellation, Ownership, and Failures
=====================================

:cpp:func:`stop() <erbsland::event::EventTimer::stop>` cancels future occurrences without destroying the timer or its
callback.
You can later start the same timer again.
Releasing a pointer cancels through destruction only when it was the last owner.
The scheduler can retain a timer temporarily during dispatch, so explicitly stopping it expresses cancellation more
clearly than resetting a handle when an occurrence may already be queued.

.. erbsland-in-practice-begin:: Stopping All Monitor Timers Together

Either of the activity monitor's one-shot callbacks finishes the session through one method.
Cleanup also stops the timers, including when a file-access failure ends dispatch before normal completion:

.. erbsland-demo::
    :source: event/apps/FileActivityMonitor/src/FileActivityMonitorApp.cpp
    :show: f:FileActivityMonitorApp::finish, f:FileActivityMonitorApp::stopTimers, f:FileActivityMonitorApp::cleanup
    :sha256: 5668c68d27dfbbc03672b03d8670b77ad9191e13f510c5cbfb026e4900bfb2bf

.. code-block:: cpp

    void FileActivityMonitorApp::finish(const el::String &reason) {
        stopTimers();
        el::stdOut()->printLine(reason);
        el::stdOut()->flush();
        quit();
    }

    void FileActivityMonitorApp::stopTimers() noexcept {
        for (const auto &timer : {_pollTimer, _idleTimer, _deadline}) {
            if (timer) {
                timer->stop();
            }
        }
    }

    void FileActivityMonitorApp::cleanup() noexcept {
        stopTimers();
        _pollTimer.reset();
        _idleTimer.reset();
        _deadline.reset();
    }

.. erbsland-demo-end::

The timer callbacks borrow the application through ``this`` and do not retain their own timer handles.
The application releases those handles during cleanup while it still exists.

.. erbsland-in-practice-end::

Timer controls and accessors are thread-safe, so another thread can request a stop or restart.
That safety does not extend to data captured by the callback.
Keep mutable callback state on its target loop, and send updates to that loop with ``invoke()`` when necessary.
Stopping or restarting does not wait for a callback already being dispatched and cannot interrupt its execution.
That callback can finish with old input after the control call returns; guard publication with operation state or
identity if an obsolete result must be ignored.

:cpp:func:`isActive() <erbsland::event::EventTimer::isActive>` includes an active schedule or an occurrence queued for
execution.
:cpp:func:`mode() <erbsland::event::EventTimer::mode>` reports the
:cpp:enum:`EventTimerMode <erbsland::event::EventTimerMode>`, and
:cpp:func:`interval() <erbsland::event::EventTimer::interval>` reports the configured delay or interval.
Stopping makes the mode inactive but leaves the last configured interval available.
These accessors provide snapshots, not synchronization barriers or proof that a callback has finished.

A blocked loop delays timers, input notifications, and cancellation commands sent through that loop alike.
Scheduling also remains subject to the loop's admission and queue policies, including when delayed work becomes due.
Returning from ``invoke()`` is not an acknowledgement that the callback will execute.
Neither retaining a target nor scheduling a deadline guarantees delivery during shutdown.
See :doc:`controlling_event_load` and :doc:`shutting_down_event_work` for those boundaries.

A callback exception is handled by the loop's configured error policy, not by a ``try`` block around the earlier
submission call.
It is not successful completion of the operation.
Applications use their surrounding failure lifecycle; when running a loop directly with a policy that stores errors,
inspect :cpp:func:`takeError() <erbsland::event::EventLoop::takeError>` after dispatch.
See :doc:`handling_event_errors` and the :ref:`event-loop-options` reference for details.

For reusable callbacks that queue arguments to a chosen loop, continue with :doc:`event_bindings`.
